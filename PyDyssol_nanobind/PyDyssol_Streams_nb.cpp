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

//Streams

std::map<std::string, double> PyDyssol::GetStreamOverall(const std::string& streamName, double time) const {
    const CStream* stream = m_flowsheet.GetStreamByName(streamName);
    if (!stream) throw std::runtime_error("Stream not found: " + streamName);

    return {
        {"massflow", stream->GetMassFlow(time)},
        {"temperature", stream->GetTemperature(time)},
        {"pressure", stream->GetPressure(time)}
    };
}

std::map<std::string, double> PyDyssol::GetStreamComposition(const std::string& streamName, double time) const {
    std::map<std::string, double> composition;
    const CStream* stream = m_flowsheet.GetStreamByName(streamName);
    if (!stream) throw std::runtime_error("Stream not found: " + streamName);

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

nb::dict PyDyssol::GetStreamDistribution(const std::string& streamName, double time) const {
    nb::dict result;
    const CStream* stream = m_flowsheet.GetStreamByName(streamName);
    if (!stream) throw std::runtime_error("Stream not found: " + streamName);

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

nb::dict PyDyssol::GetStream(const std::string& streamName, double time) const {
    nb::dict result;
    result["overall"] = GetStreamOverall(streamName, time);
    result["composition"] = GetStreamComposition(streamName, time);
    result["distributions"] = GetStreamDistribution(streamName, time);
    return result;
}

std::vector<std::string> PyDyssol::GetStreams() const {
    std::vector<std::string> names;
    for (const auto& s : m_flowsheet.GetAllStreams())
        names.push_back(s->GetName());
    return names;
}

//Without timepoints
nb::dict PyDyssol::GetStreamOverall(const std::string& streamName) {
    nb::dict overall;

    const CStream* stream = m_flowsheet.GetStreamByName(streamName);
    if (!stream) throw std::runtime_error("Stream not found: " + streamName);

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

nb::dict PyDyssol::GetStreamComposition(const std::string& streamName) {
    nb::dict composition;

    const CStream* stream = m_flowsheet.GetStreamByName(streamName);
    if (!stream) throw std::runtime_error("Stream not found: " + streamName);

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

nb::dict PyDyssol::GetStreamDistribution(const std::string& streamName) {
    nb::dict distributions;

    const CStream* stream = m_flowsheet.GetStreamByName(streamName);
    if (!stream) throw std::runtime_error("Stream not found: " + streamName);

    std::vector<double> timepoints = stream->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::set<std::string> allDistributions;
    std::map<std::string, std::vector<std::vector<double>>> distributionData;

    for (double t : timepoints) {
        timeList.push_back(t);
        nb::dict dists = GetStreamDistribution(streamName, t);
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

nb::dict PyDyssol::GetStream(const std::string& streamName) {
    nb::dict result;
    result["overall"] = GetStreamOverall(streamName);
    result["composition"] = GetStreamComposition(streamName);
    result["distributions"] = GetStreamDistribution(streamName);
    return result;
}
