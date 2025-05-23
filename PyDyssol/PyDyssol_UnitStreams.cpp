#include "PyDyssol.h"
#include <iostream>
#include <stdexcept>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace fs = std::filesystem;
namespace py = pybind11;


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

pybind11::dict PyDyssol::GetUnitStreamDistribution(const std::string& unitName, const std::string& streamName, double time) const {
    pybind11::dict result;

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
                result[py::str(DISTR_NAMES[idx])] = py::cast(dist);
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

pybind11::dict PyDyssol::GetUnitStreamDistribution(const std::string& unitName, double time) const {
    auto streams = GetUnitStreams(unitName);
    if (streams.empty())
        throw std::runtime_error("No streams found in unit: " + unitName);
    return GetUnitStreamDistribution(unitName, streams.front(), time);
}

pybind11::dict PyDyssol::GetUnitStream(const std::string& unitName, double time) const {
    auto streams = GetUnitStreams(unitName);
    if (streams.empty())
        throw std::runtime_error("No streams found in unit: " + unitName);
    return GetUnitStream(unitName, streams.front(), time);
}

//Without timepoints
pybind11::dict PyDyssol::GetUnitStream(const std::string& unitName, const std::string& streamName, double time) const
{
    pybind11::dict result;
    result["overall"] = GetUnitStreamOverall(unitName, streamName, time);
    result["composition"] = GetUnitStreamComposition(unitName, streamName, time);
    result["distributions"] = GetUnitStreamDistribution(unitName, streamName, time);
    return result;
}

pybind11::dict PyDyssol::GetUnitStreamOverallAllTimes(const std::string& unitName) {
    pybind11::dict overall;

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

pybind11::dict PyDyssol::GetUnitStreamCompositionAllTimes(const std::string& unitName) {
    pybind11::dict composition;

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

pybind11::dict PyDyssol::GetUnitStreamDistributionAllTimes(const std::string& unitName) {
    pybind11::dict distributions;

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
        pybind11::dict dists = GetUnitStreamDistribution(unitName, t);
        for (auto item : dists) {
            std::string distName = item.first.cast<std::string>();
            std::vector<double> distValues = item.second.cast<std::vector<double>>();
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

pybind11::dict PyDyssol::GetUnitStreamAllTimes(const std::string& unitName) {
    pybind11::dict result;
    result["overall"] = GetUnitStreamOverallAllTimes(unitName);
    result["composition"] = GetUnitStreamCompositionAllTimes(unitName);
    result["distributions"] = GetUnitStreamDistributionAllTimes(unitName);
    return result;
}
