#include "PyDyssol_nb.h"
#include <iostream>
#include <stdexcept>
#include <nanobind/nanobind.h> // Include nanobind headers
#include <nanobind/stl/string.h> // For std::string bindings
#include <nanobind/stl/vector.h> // For std::vector bindings
#include <nanobind/stl/pair.h>   // For std::pair bindings
#include <nanobind/stl/map.h>    // For std::map bindings
#include <nanobind/stl/tuple.h>  // For std::tuple bindings

namespace nb = nanobind;

//Unit Streams
std::vector<std::string> PyDyssol::GetUnitStreams(const std::string& unitName) const {
    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto& streams = unit->GetModel()->GetStreamsManager().GetStreams();
    std::vector<std::string> result;
    result.reserve(streams.size());
    for (const auto* s : streams)
        if (s) result.push_back(s->GetName());
    return result;
}

std::map<std::string, double> PyDyssol::GetUnitStreamOverall(const std::string& unitName, const std::string& streamName, double time) const {
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const CStream* stream = unit->GetModel()->GetStreamsManager().GetStream(streamName);
    if (!stream) throw std::runtime_error("Stream not found: " + streamName + " in unit " + unitName);

    return {
        {"massflow", stream->GetMassFlow(time)},
        {"temperature", stream->GetTemperature(time)},
        {"pressure", stream->GetPressure(time)}
    };
}

std::map<std::string, double> PyDyssol::GetUnitStreamComposition(const std::string& unitName, const std::string& streamName, double time) const {
    std::map<std::string, double> composition;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const CStream* stream = unit->GetModel()->GetStreamsManager().GetStream(streamName);
    if (!stream) throw std::runtime_error("Stream not found: " + streamName + " in unit " + unitName);

    for (const auto& compoundKey : m_flowsheet.GetCompounds()) {
        const CCompound* compound = m_materialsDatabase.GetCompound(compoundKey);
        std::string name = compound ? compound->GetName() : compoundKey;

        for (const auto& phase : m_flowsheet.GetPhases()) {
            double mass = stream->GetCompoundMass(time, compoundKey, phase.state);
            if (std::abs(mass) > 1e-12)
                composition[name + " [" + PhaseToString(phase.state) + "]"] = mass;
        }
    }

    return composition;
}

nb::dict PyDyssol::GetUnitStreamDistribution(const std::string& unitName, const std::string& streamName, double time) const {
    nb::dict result;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const CStream* stream = unit->GetModel()->GetStreamsManager().GetStream(streamName);
    if (!stream) throw std::runtime_error("Stream not found: " + streamName + " in unit " + unitName);

    for (const CGridDimension* dim : m_flowsheet.GetGrid().GetGridDimensions()) {
        EDistrTypes type = dim->DimensionType();
        int idx = GetDistributionTypeIndex(type);
        if (idx >= 0) {
            std::vector<double> dist = stream->GetDistribution(time, type);
            if (!dist.empty())
                result[nb::str(DISTR_NAMES[idx])] = nb::cast(dist);
        }
    }

    return result;
}

std::map<std::string, double> PyDyssol::GetUnitStreamOverall(const std::string& unitName, double time) const {
    auto streams = GetUnitStreams(unitName);
    if (streams.empty())
        throw std::runtime_error("No streams found in unit: " + unitName);
    return GetUnitStreamOverall(unitName, streams.front(), time);
}

std::map<std::string, double> PyDyssol::GetUnitStreamComposition(const std::string& unitName, double time) const {
    auto streams = GetUnitStreams(unitName);
    if (streams.empty())
        throw std::runtime_error("No streams found in unit: " + unitName);
    return GetUnitStreamComposition(unitName, streams.front(), time);
}

nb::dict PyDyssol::GetUnitStreamDistribution(const std::string& unitName, double time) const {
    auto streams = GetUnitStreams(unitName);
    if (streams.empty())
        throw std::runtime_error("No streams found in unit: " + unitName);
    return GetUnitStreamDistribution(unitName, streams.front(), time);
}

nb::dict PyDyssol::GetUnitStream(const std::string& unitName, double time) const {
    auto streams = GetUnitStreams(unitName);
    if (streams.empty())
        throw std::runtime_error("No streams found in unit: " + unitName);
    return GetUnitStream(unitName, streams.front(), time);
}

nb::dict PyDyssol::GetUnitStream(const std::string& unitName, const std::string& streamName, double time) const
{
    nb::dict result;
    result["overall"] = GetUnitStreamOverall(unitName, streamName, time);
    result["composition"] = GetUnitStreamComposition(unitName, streamName, time);
    result["distributions"] = GetUnitStreamDistribution(unitName, streamName, time);
    return result;
}

//Without timepoints, with stream_name
nb::dict PyDyssol::GetUnitStreamOverall(const std::string& unitName, const std::string& streamName) {
    nb::dict overall;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto* stream = unit->GetModel()->GetStreamsManager().GetStream(streamName);
    if (!stream)
        throw std::runtime_error("Stream not found: " + streamName);

    auto timepoints = stream->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::map<std::string, std::vector<double>> data;
    for (double t : timepoints) {
        timeList.push_back(t);
        data["massflow"].push_back(stream->GetMassFlow(t));
        data["temperature"].push_back(stream->GetTemperature(t));
        data["pressure"].push_back(stream->GetPressure(t));
    }

    overall["timepoints"] = timeList;
    for (const auto& [key, vec] : data)
        overall[key.c_str()] = vec;

    return overall;
}

nb::dict PyDyssol::GetUnitStreamComposition(const std::string& unitName, const std::string& streamName) {
    nb::dict composition;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto* stream = unit->GetModel()->GetStreamsManager().GetStream(streamName);
    if (!stream)
        throw std::runtime_error("Stream not found: " + streamName);

    auto timepoints = stream->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::map<std::string, std::vector<double>> data;
    std::set<std::string> allLabels;

    for (double t : timepoints) {
        timeList.push_back(t);
        for (const auto& compoundKey : m_flowsheet.GetCompounds()) {
            const auto* compound = m_materialsDatabase.GetCompound(compoundKey);
            std::string compoundName = compound ? compound->GetName() : compoundKey;

            for (const auto& phase : m_flowsheet.GetPhases()) {
                std::string label = compoundName + " [" + PhaseToString(phase.state) + "]";
                double mass = stream->GetCompoundMass(t, compoundKey, phase.state);
                data[label].push_back(mass);
                allLabels.insert(label);
            }
        }
    }

    composition["timepoints"] = timeList;
    for (const auto& label : allLabels) {
        const auto& values = data[label];
        bool allZero = std::all_of(values.begin(), values.end(), [](double v) { return std::abs(v) < 1e-15; });
        if (!allZero)
            composition[label.c_str()] = values;
    }

    return composition;
}

nb::dict PyDyssol::GetUnitStreamDistribution(const std::string& unitName, const std::string& streamName) {
    nb::dict distributions;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto* stream = unit->GetModel()->GetStreamsManager().GetStream(streamName);
    if (!stream)
        throw std::runtime_error("Stream not found: " + streamName);

    auto timepoints = stream->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::map<std::string, std::vector<std::vector<double>>> data;
    std::set<std::string> allNames;

    for (double t : timepoints) {
        timeList.push_back(t);
        for (const CGridDimension* dim : m_flowsheet.GetGrid().GetGridDimensions()) {
            EDistrTypes type = dim->DimensionType();
            int idx = GetDistributionTypeIndex(type);
            if (idx < 0) continue;

            std::string name = DISTR_NAMES[idx];
            std::vector<double> dist = stream->GetDistribution(t, type);
            data[name].push_back(dist);
            allNames.insert(name);
        }
    }

    distributions["timepoints"] = timeList;
    for (const auto& name : allNames) {
        const auto& vectors = data[name];

        bool allZero = std::all_of(vectors.begin(), vectors.end(), [](const std::vector<double>& vec) {
            return std::all_of(vec.begin(), vec.end(), [](double v) { return std::abs(v) < 1e-15; });
            });

        if (!allZero)
            distributions[name.c_str()] = vectors;
    }

    return distributions;
}

nb::dict PyDyssol::GetUnitStream(const std::string& unitName, const std::string& streamName) {
    nb::dict result;
    result["overall"] = GetUnitStreamOverall(unitName, streamName);
    result["composition"] = GetUnitStreamComposition(unitName, streamName);
    result["distributions"] = GetUnitStreamDistribution(unitName, streamName);
    return result;
}


//Without timepoints, no stream_name
nb::dict PyDyssol::GetUnitStreamOverall(const std::string& unitName) {
    nb::dict overall;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    auto streams = unit->GetModel()->GetStreamsManager().GetStreams();
    if (streams.empty()) throw std::runtime_error("No streams found in unit: " + unitName);
    const CStream* stream = streams.front();

    std::vector<double> timepoints = stream->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::map<std::string, std::vector<double>> overallData;

    for (double t : timepoints) {
        timeList.push_back(t);
        overallData["massflow"].push_back(stream->GetMassFlow(t));
        overallData["temperature"].push_back(stream->GetTemperature(t));
        overallData["pressure"].push_back(stream->GetPressure(t));
    }

    overall["timepoints"] = timeList;
    for (const auto& [key, vec] : overallData)
        overall[key.c_str()] = vec;

    return overall;
}

nb::dict PyDyssol::GetUnitStreamComposition(const std::string& unitName) {
    nb::dict composition;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    auto streams = unit->GetModel()->GetStreamsManager().GetStreams();
    if (streams.empty()) throw std::runtime_error("No streams found in unit: " + unitName);
    const CStream* stream = streams.front();

    std::vector<double> timepoints = stream->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::map<std::string, std::vector<double>> compositionData;
    std::set<std::string> allCompounds;

    for (double t : timepoints) {
        timeList.push_back(t);
        for (const auto& compoundKey : m_flowsheet.GetCompounds()) {
            const auto* compound = m_materialsDatabase.GetCompound(compoundKey);
            std::string name = compound ? compound->GetName() : compoundKey;

            for (const auto& phase : m_flowsheet.GetPhases()) {
                double val = stream->GetCompoundMass(t, compoundKey, phase.state);
                std::string label = name + " [" + PhaseToString(phase.state) + "]";
                compositionData[label].push_back(val);
                allCompounds.insert(label);
            }
        }
    }

    composition["timepoints"] = timeList;
    for (const auto& name : allCompounds) {
        const auto& values = compositionData[name];
        bool allZero = std::all_of(values.begin(), values.end(), [](double v) { return std::abs(v) < 1e-15; });
        if (!allZero) {
            composition[name.c_str()] = values;
        }
    }

    return composition;
}

nb::dict PyDyssol::GetUnitStreamDistribution(const std::string& unitName) {
    nb::dict distributions;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    auto streams = unit->GetModel()->GetStreamsManager().GetStreams();
    if (streams.empty()) throw std::runtime_error("No streams found in unit: " + unitName);
    const CStream* stream = streams.front();

    std::vector<double> timepoints = stream->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::set<std::string> allDistributions;
    std::map<std::string, std::vector<std::vector<double>>> distributionData;

    for (double t : timepoints) {
        timeList.push_back(t);
        nb::dict dists = GetUnitStreamDistribution(unitName, t);
        for (auto [key, value] : dists) {
            std::string distName = nb::cast<std::string>(key);
            std::vector<double> distValues = nb::cast<std::vector<double>>(value);
            distributionData[distName].push_back(distValues);
            allDistributions.insert(distName);
        }
    }

    distributions["timepoints"] = timeList;

    for (const auto& distName : allDistributions) {
        const auto& vecOfVecs = distributionData[distName];

        bool allZero = true;
        for (const auto& vec : vecOfVecs) {
            for (double v : vec) {
                if (std::abs(v) > 1e-15) {
                    allZero = false;
                    break;
                }
            }
            if (!allZero) break;
        }
        if (!allZero) {
            distributions[distName.c_str()] = vecOfVecs;
        }
    }

    return distributions;
}

nb::dict PyDyssol::GetUnitStream(const std::string& unitName) {
    nb::dict result;
    result["overall"] = GetUnitStreamOverall(unitName);
    result["composition"] = GetUnitStreamComposition(unitName);
    result["distributions"] = GetUnitStreamDistribution(unitName);
    return result;
}
