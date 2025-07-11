#include "PyDyssol.h"
#include <iostream>
#include <stdexcept>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace fs = std::filesystem;
namespace py = pybind11;

//Holdups
std::vector<std::string> PyDyssol::GetUnitHoldups(const std::string& unitName) const {
    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);
    std::vector<std::string> result;
    const auto& holdups = unit->GetModel()->GetStreamsManager().GetHoldups();
    for (const auto* holdup : holdups) {
        result.push_back(holdup->GetName());
    }
    return result;
} 

std::map<std::string, double> PyDyssol::GetUnitHoldupOverall(const std::string& unitName, double time) const {
    std::map<std::string, double> overall;
    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const auto& holdups = unit->GetModel()->GetStreamsManager().GetHoldups();
    if (holdups.empty()) throw std::runtime_error("No holdups found in unit: " + unitName);
    const CHoldup* holdup = holdups.front();

    overall["mass"] = holdup->GetMass(time);
    overall["temperature"] = holdup->GetTemperature(time);
    overall["pressure"] = holdup->GetPressure(time);
    return overall;
}

std::map<std::string, double> PyDyssol::GetUnitHoldupOverall(const std::string& unitName, const std::string& holdupName, double time) const
{
    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);
    const auto* model = unit->GetModel();
    if (!model) throw std::runtime_error("Model not found for unit: " + unitName);

    const auto* holdup = dynamic_cast<const CHoldup*>(model->GetStreamsManager().GetObjectWork(holdupName));
    if (!holdup) throw std::runtime_error("Holdup not found: " + holdupName);

    return {
        {"mass",        holdup->GetMass(time)},
        {"temperature", holdup->GetTemperature(time)},
        {"pressure",    holdup->GetPressure(time)}
    };
}

std::map<std::string, double> PyDyssol::GetUnitHoldupComposition(const std::string& unitName, double time) const {
    std::map<std::string, double> composition;
    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const auto& holdups = unit->GetModel()->GetStreamsManager().GetHoldups();
    if (holdups.empty()) throw std::runtime_error("No holdups found in unit: " + unitName);
    const CHoldup* holdup = holdups.front();

    for (const auto& compoundKey : m_flowsheet.GetCompounds()) {
        const CCompound* compound = m_materialsDatabase.GetCompound(compoundKey);
        std::string compoundName = compound ? compound->GetName() : compoundKey;

        for (const auto& phase : m_flowsheet.GetPhases()) {
            double mass = holdup->GetCompoundMass(time, compoundKey, phase.state);
            if (std::abs(mass) > 1e-12) {
                std::string name = compoundName + " [" + PhaseToString(phase.state) + "]";
                composition[name] = mass;
            }
        }
    }

    return composition;
}

std::map<std::string, double> PyDyssol::GetUnitHoldupComposition(const std::string& unitName, const std::string& holdupName, double time) const
{
    std::map<std::string, double> composition;

    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);
    const auto* model = unit->GetModel();
    if (!model) throw std::runtime_error("Model not found for unit: " + unitName);

    const auto* holdup = dynamic_cast<const CHoldup*>(model->GetStreamsManager().GetObjectWork(holdupName));
    if (!holdup) throw std::runtime_error("Holdup not found: " + holdupName);

    for (const auto& compoundKey : m_flowsheet.GetCompounds()) {
        const CCompound* compound = m_materialsDatabase.GetCompound(compoundKey);
        std::string compoundName = compound ? compound->GetName() : compoundKey;

        for (const auto& phase : m_flowsheet.GetPhases()) {
            double mass = holdup->GetCompoundMass(time, compoundKey, phase.state);
            if (std::abs(mass) > 1e-12)
                composition[compoundName + " [" + PhaseToString(phase.state) + "]"] = mass;
        }
    }

    return composition;
}

pybind11::dict PyDyssol::GetUnitHoldupDistribution(const std::string& unitName, double time) const {
    pybind11::dict result;

    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const auto& holdups = unit->GetModel()->GetStreamsManager().GetHoldups();
    if (holdups.empty()) throw std::runtime_error("No holdups found in unit: " + unitName);
    const CHoldup* holdup = holdups.front();

    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();

    for (const CGridDimension* dim : gridDims) {
        EDistrTypes distrType = dim->DimensionType();
        if (distrType == EDistrTypes::DISTR_COMPOUNDS) continue;
        int index = GetDistributionTypeIndex(distrType);
        if (index < 0) continue;
        std::string name = DISTR_NAMES[index];

        std::vector<double> combined = holdup->GetDistribution(time, distrType);
        if (!combined.empty())
            result[name.c_str()] = pybind11::cast(combined);
    }

    return result;
}

pybind11::dict PyDyssol::GetUnitHoldupDistribution(const std::string& unitName, const std::string& holdupName, double time) const
{
    pybind11::dict result;

    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);
    const auto* model = unit->GetModel();
    if (!model) throw std::runtime_error("Model not found for unit: " + unitName);

    const auto* holdup = dynamic_cast<const CHoldup*>(model->GetStreamsManager().GetObjectWork(holdupName));
    if (!holdup) throw std::runtime_error("Holdup not found: " + holdupName);

    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();
    for (const CGridDimension* dim : gridDims) {
        EDistrTypes type = dim->DimensionType();
        if (type == EDistrTypes::DISTR_COMPOUNDS) continue;
        int idx = GetDistributionTypeIndex(type);
        if (idx >= 0) {
            std::vector<double> dist = holdup->GetDistribution(time, type);
            if (!dist.empty())
                result[py::str(DISTR_NAMES[idx])] = py::cast(dist);
        }
    }

    return result;
}


pybind11::dict PyDyssol::GetUnitHoldup(const std::string& unitName, double time) const {
    pybind11::dict result;
    pybind11::dict compDict;
    pybind11::dict overallDict;

    auto overall = GetUnitHoldupOverall(unitName, time);
    auto composition = GetUnitHoldupComposition(unitName, time);

    overallDict["mass"] = overall.at("mass");
    overallDict["temperature"] = overall.at("temperature");
    overallDict["pressure"] = overall.at("pressure");

    for (const auto& [key, val] : composition)
        if (std::abs(val) > 1e-12)
            compDict[key.c_str()] = val;

    result["overall"] = overallDict;
    result["composition"] = compDict;
    result["distributions"] = GetUnitHoldupDistribution(unitName, time);

    return result;
}

pybind11::dict PyDyssol::GetUnitHoldup(const std::string& unitName, const std::string& holdupName, double time) const
{
    pybind11::dict result;
    pybind11::dict compDict;
    pybind11::dict overallDict;

    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const auto* model = unit->GetModel();
    if (!model) throw std::runtime_error("Model not found for unit: " + unitName);

    const auto* holdup = dynamic_cast<const CHoldup*>(model->GetStreamsManager().GetObjectWork(holdupName));
    if (!holdup) throw std::runtime_error("Holdup not found: " + holdupName);

    // === Get overall ===
    overallDict["mass"] = holdup->GetMass(time);
    overallDict["temperature"] = holdup->GetTemperature(time);
    overallDict["pressure"] = holdup->GetPressure(time);

    // === Get composition ===
    for (const auto& compoundKey : m_flowsheet.GetCompounds()) {
        const CCompound* compound = m_materialsDatabase.GetCompound(compoundKey);
        std::string compoundName = compound ? compound->GetName() : compoundKey;

        for (const auto& phase : m_flowsheet.GetPhases()) {
            double mass = holdup->GetCompoundMass(time, compoundKey, phase.state);
            if (std::abs(mass) > 1e-12) {
                std::string label = compoundName + " [" + PhaseToString(phase.state) + "]";
                compDict[label.c_str()] = mass;
            }
        }
    }

    // === Get distributions ===
    pybind11::dict distDict;
    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();
    for (const CGridDimension* dim : gridDims) {
        EDistrTypes type = dim->DimensionType();
        if (type == EDistrTypes::DISTR_COMPOUNDS) continue;
        int idx = GetDistributionTypeIndex(type);
        if (idx >= 0) {
            std::vector<double> dist = holdup->GetDistribution(time, type);
            if (!dist.empty())
                distDict[py::str(DISTR_NAMES[idx])] = py::cast(dist);
        }
    }

    result["overall"] = overallDict;
    result["composition"] = compDict;
    result["distributions"] = distDict;

    return result;
}

pybind11::dict PyDyssol::GetUnitHoldupOverall(const std::string& unitName) {
    pybind11::dict overall;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const auto& holdups = unit->GetModel()->GetStreamsManager().GetHoldups();
    if (holdups.empty()) throw std::runtime_error("No holdups found in unit: " + unitName);
    const CHoldup* holdup = holdups.front();

    std::vector<double> timepoints = holdup->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::map<std::string, std::vector<double>> overallData;

    for (double t : timepoints) {
        timeList.push_back(t);
        overallData["mass"].push_back(holdup->GetMass(t));
        overallData["temperature"].push_back(holdup->GetTemperature(t));
        overallData["pressure"].push_back(holdup->GetPressure(t));
    }

    overall["timepoints"] = timeList;
    for (const auto& [key, vec] : overallData)
        overall[key.c_str()] = vec;

    return overall;
}

pybind11::dict PyDyssol::GetUnitHoldupComposition(const std::string& unitName) {
    pybind11::dict composition;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const auto& holdups = unit->GetModel()->GetStreamsManager().GetHoldups();
    if (holdups.empty()) throw std::runtime_error("No holdups found in unit: " + unitName);
    const CHoldup* holdup = holdups.front();

    std::vector<double> timepoints = holdup->GetAllTimePoints();
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
                double val = holdup->GetCompoundMass(t, compoundKey, phase.state);
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

pybind11::dict PyDyssol::GetUnitHoldupDistribution(const std::string& unitName) {
    pybind11::dict distributions;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const auto& holdups = unit->GetModel()->GetStreamsManager().GetHoldups();
    if (holdups.empty()) throw std::runtime_error("No holdups found in unit: " + unitName);
    const CHoldup* holdup = holdups.front();

    std::vector<double> timepoints = holdup->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::set<std::string> allDistributions;
    std::map<std::string, std::vector<std::vector<double>>> distributionData;

    for (double t : timepoints) {
        timeList.push_back(t);
        pybind11::dict dists = GetUnitHoldupDistribution(unitName, t);
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

pybind11::dict PyDyssol::GetUnitHoldup(const std::string& unitName) {
    pybind11::dict result;
    result["overall"] = GetUnitHoldupOverall(unitName);
    result["composition"] = GetUnitHoldupComposition(unitName);
    result["distributions"] = GetUnitHoldupDistribution(unitName);
    return result;
}

pybind11::dict PyDyssol::GetUnitHoldupOverall(const std::string& unitName, const std::string& holdupName) {
    pybind11::dict overall;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);
    const CHoldup* holdup = dynamic_cast<const CHoldup*>(unit->GetModel()->GetStreamsManager().GetObjectWork(holdupName));
    if (!holdup) throw std::runtime_error("Holdup not found: " + holdupName);

    std::vector<double> timepoints = holdup->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::map<std::string, std::vector<double>> data;
    for (double t : timepoints) {
        timeList.push_back(t);
        data["mass"].push_back(holdup->GetMass(t));
        data["temperature"].push_back(holdup->GetTemperature(t));
        data["pressure"].push_back(holdup->GetPressure(t));
    }

    overall["timepoints"] = timeList;
    for (const auto& [key, vec] : data)
        overall[key.c_str()] = vec;

    return overall;
}

pybind11::dict PyDyssol::GetUnitHoldupComposition(const std::string& unitName, const std::string& holdupName) {
    pybind11::dict composition;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);
    const CHoldup* holdup = dynamic_cast<const CHoldup*>(unit->GetModel()->GetStreamsManager().GetObjectWork(holdupName));
    if (!holdup) throw std::runtime_error("Holdup not found: " + holdupName);

    std::vector<double> timepoints = holdup->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::map<std::string, std::vector<double>> compositionData;
    std::set<std::string> allLabels;

    for (double t : timepoints) {
        timeList.push_back(t);
        for (const auto& compoundKey : m_flowsheet.GetCompounds()) {
            const auto* compound = m_materialsDatabase.GetCompound(compoundKey);
            std::string name = compound ? compound->GetName() : compoundKey;

            for (const auto& phase : m_flowsheet.GetPhases()) {
                double mass = holdup->GetCompoundMass(t, compoundKey, phase.state);
                std::string label = name + " [" + PhaseToString(phase.state) + "]";
                compositionData[label].push_back(mass);
                allLabels.insert(label);
            }
        }
    }

    composition["timepoints"] = timeList;
    for (const auto& label : allLabels) {
        const auto& values = compositionData[label];
        bool allZero = std::all_of(values.begin(), values.end(), [](double v) { return std::abs(v) < 1e-15; });
        if (!allZero)
            composition[label.c_str()] = values;
    }

    return composition;
}

pybind11::dict PyDyssol::GetUnitHoldupDistribution(const std::string& unitName, const std::string& holdupName) {
    pybind11::dict distributions;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);
    const CHoldup* holdup = dynamic_cast<const CHoldup*>(unit->GetModel()->GetStreamsManager().GetObjectWork(holdupName));
    if (!holdup) throw std::runtime_error("Holdup not found: " + holdupName);

    std::vector<double> timepoints = holdup->GetAllTimePoints();
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
            if (type == EDistrTypes::DISTR_COMPOUNDS) continue;
            int idx = GetDistributionTypeIndex(type);
            if (idx < 0) continue;

            std::string name = DISTR_NAMES[idx];
            std::vector<double> dist = holdup->GetDistribution(t, type);
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

pybind11::dict PyDyssol::GetUnitHoldup(const std::string& unitName, const std::string& holdupName) {
    pybind11::dict result;
    result["overall"] = GetUnitHoldupOverall(unitName, holdupName);
    result["composition"] = GetUnitHoldupComposition(unitName, holdupName);
    result["distributions"] = GetUnitHoldupDistribution(unitName, holdupName);
    return result;
}


static void SetHoldupValues(CHoldup* holdup, double time, const py::dict& data, const std::vector<const CGridDimension*>& gridDims, const CMaterialsDatabase& materialsDB)
{
    // === Set composition ===
    if (data.contains("composition")) {
        const auto compDict = data["composition"].cast<py::dict>();
        for (const auto& item : compDict) {
            std::string key = item.first.cast<std::string>();
            double value = item.second.cast<double>();

            size_t split = key.find(" [");
            if (split == std::string::npos || key.back() != ']') {
                throw std::runtime_error("[PyDyssol] Invalid composition key format: '" + key + "'. Expected format: 'Compound [Phase]'");
            }
            std::string compoundName = key.substr(0, split);
            std::string phaseName = key.substr(split + 2, key.length() - split - 3);

            const CCompound* compound = materialsDB.GetCompoundByName(compoundName);
            if (!compound)
                throw std::runtime_error("[PyDyssol] Unknown compound: " + compoundName);

            EPhase phase;
            try {
                phase = GetPhaseByName(phaseName);
            }
            catch (const std::exception& e) {
                throw std::runtime_error("[PyDyssol] Invalid phase in composition key '" + key + "': " + e.what());
            }

            holdup->SetCompoundMass(time, compound->GetKey(), phase, value);
        }
    }

    // === Set overall properties ===
    if (data.contains("overall")) {
        const auto overallDict = data["overall"].cast<py::dict>();
        for (const auto& item : overallDict) {
            std::string name = item.first.cast<std::string>();
            double value = item.second.cast<double>();
            holdup->SetOverallProperty(time, StringToEOverall(name), value);
        }
    }

    // === Create grid name type map ===
    std::map<std::string, EDistrTypes> nameToType;
    for (const CGridDimension* dim : gridDims) {
        int idx = GetDistributionTypeIndex(dim->DimensionType());
        if (idx < 0) continue;
        nameToType[DISTR_NAMES[idx]] = DISTR_TYPES[idx];
    }

    // === Set distributions ===
    if (data.contains("distributions")) {
        const auto distDict = data["distributions"].cast<py::dict>();
        for (const auto& item : distDict) {
            std::string distrName = item.first.cast<std::string>();
            std::vector<double> values = item.second.cast<std::vector<double>>();
            if (!nameToType.count(distrName))
                throw std::runtime_error("Unknown or unsupported distribution: " + distrName);
            EDistrTypes distrType = nameToType[distrName];

            std::vector<double> normalized = Normalized(values);
            std::vector<double> current = holdup->GetDistribution(time, distrType);
            if (current.size() != normalized.size())
                throw std::runtime_error("Size mismatch in distribution '" + distrName + "'");
            holdup->SetDistribution(time, distrType, normalized);
        }
    }
}


void PyDyssol::SetUnitHoldup(const std::string& unitName, const pybind11::dict& data)
{
    const double time = 0.0;

    CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    auto* model = unit->GetModel();
    if (!model) throw std::runtime_error("Model not found for unit: " + unitName);

    auto holdupsWork = model->GetStreamsManager().GetHoldups();
    auto holdupsInit = model->GetStreamsManager().GetHoldupsInit();
    if (holdupsWork.empty() || holdupsInit.empty())
        throw std::runtime_error("No holdups defined in unit: " + unitName);

    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();

    SetHoldupValues(holdupsWork.front(), time, data, gridDims, m_materialsDatabase);
    SetHoldupValues(holdupsInit.front(), time, data, gridDims, m_materialsDatabase);
}

void PyDyssol::SetUnitHoldup(const std::string& unitName, const std::string& holdupName, const pybind11::dict& data)
{
    const double time = 0.0;

    CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    auto* model = unit->GetModel();
    if (!model) throw std::runtime_error("Model not found for unit: " + unitName);

    auto* holdupWork = dynamic_cast<CHoldup*>(model->GetStreamsManager().GetObjectWork(holdupName));
    auto* holdupInit = dynamic_cast<CHoldup*>(model->GetStreamsManager().GetObjectInit(holdupName));
    if (!holdupWork || !holdupInit)
        throw std::runtime_error("Holdup not found: " + holdupName);

    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();
    SetHoldupValues(holdupWork, time, data, gridDims, m_materialsDatabase);
    SetHoldupValues(holdupInit, time, data, gridDims, m_materialsDatabase);
}
