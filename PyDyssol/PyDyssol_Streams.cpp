#include "PyDyssol.h"
#include <iostream>
#include <stdexcept>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace fs = std::filesystem;
namespace py = pybind11;

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

pybind11::dict PyDyssol::GetStreamDistribution(const std::string& streamName, double time) const {
    pybind11::dict result;
    const CStream* stream = m_flowsheet.GetStreamByName(streamName);
    if (!stream) throw std::runtime_error("Stream not found: " + streamName);

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

pybind11::dict PyDyssol::GetStream(const std::string& streamName, double time) const {
    pybind11::dict result;
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
pybind11::dict PyDyssol::GetStreamOverallAllTimes(const std::string& streamName) {
    pybind11::dict overall;

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

pybind11::dict PyDyssol::GetStreamCompositionAllTimes(const std::string& streamName) {
    pybind11::dict composition;

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

pybind11::dict PyDyssol::GetStreamDistributionAllTimes(const std::string& streamName) {
    pybind11::dict distributions;

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
        pybind11::dict dists = GetStreamDistribution(streamName, t);
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

pybind11::dict PyDyssol::GetStreamAllTimes(const std::string& streamName) {
    pybind11::dict result;
    result["overall"] = GetStreamOverallAllTimes(streamName);
    result["composition"] = GetStreamCompositionAllTimes(streamName);
    result["distributions"] = GetStreamDistributionAllTimes(streamName);
    return result;
}
