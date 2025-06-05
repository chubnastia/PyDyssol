#include "PyDyssol_nb.h"
#include <iostream>
#include <stdexcept>
#include <nanobind/nanobind.h> // Include nanobind headers
#include <nanobind/stl/string.h> // For std::string bindings
#include <nanobind/stl/vector.h> // For std::vector bindings
#include <nanobind/stl/pair.h>   // For std::pair bindings
#include <nanobind/stl/map.h>    // For std::map bindings
#include <nanobind/stl/tuple.h>  // For std::tuple bindings
#include <nanobind/stl/list.h>   // For nanobind::list bindings
#include "../MaterialsDatabase/MaterialsDatabase.h"

namespace nb = nanobind;

//Holdups
std::vector<std::string> PyDyssol::GetUnitHoldups(const std::string& unitName) const
{
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const auto& holdups = unit->GetModel()->GetStreamsManager().GetHoldups();
    std::vector<std::string> result;
    result.reserve(holdups.size());
    for (const auto* holdup : holdups)
        result.push_back(holdup->GetName());

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

nanobind::dict PyDyssol::GetUnitHoldupDistribution(const std::string& unitName, double time) const {
    nanobind::dict result;

    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const auto& holdups = unit->GetModel()->GetStreamsManager().GetHoldups();
    if (holdups.empty()) throw std::runtime_error("No holdups found in unit: " + unitName);
    const CHoldup* holdup = holdups.front();

    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();

    for (const CGridDimension* dim : gridDims) {
        EDistrTypes distrType = dim->DimensionType();
        int index = GetDistributionTypeIndex(distrType);
        if (index < 0) continue;
        std::string name = DISTR_NAMES[index];

        std::vector<double> combined = holdup->GetDistribution(time, distrType);
        if (!combined.empty())
            result[name.c_str()] = nanobind::cast(combined);
    }

    return result;
}

nanobind::dict PyDyssol::GetUnitHoldupDistribution(const std::string& unitName, const std::string& holdupName, double time) const
{
    nanobind::dict result;

    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);
    const auto* model = unit->GetModel();
    if (!model) throw std::runtime_error("Model not found for unit: " + unitName);

    const auto* holdup = dynamic_cast<const CHoldup*>(model->GetStreamsManager().GetObjectWork(holdupName));
    if (!holdup) throw std::runtime_error("Holdup not found: " + holdupName);

    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();
    for (const CGridDimension* dim : gridDims) {
        EDistrTypes type = dim->DimensionType();
        int idx = GetDistributionTypeIndex(type);
        if (idx >= 0) {
            std::vector<double> dist = holdup->GetDistribution(time, type);
            if (!dist.empty())
                result[nb::str(DISTR_NAMES[idx])] = nb::cast(dist);
        }
    }

    return result;
}


nanobind::dict PyDyssol::GetUnitHoldup(const std::string& unitName, double time) const {
    nanobind::dict result;
    nanobind::dict compDict;
    nanobind::dict overallDict;

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

nanobind::dict PyDyssol::GetUnitHoldup(const std::string& unitName, const std::string& holdupName, double time) const
{
    nanobind::dict result;
    nanobind::dict compDict;
    nanobind::dict overallDict;

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
    nanobind::dict distDict;
    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();
    for (const CGridDimension* dim : gridDims) {
        EDistrTypes type = dim->DimensionType();
        int idx = GetDistributionTypeIndex(type);
        if (idx >= 0) {
            std::vector<double> dist = holdup->GetDistribution(time, type);
            if (!dist.empty())
                distDict[nb::str(DISTR_NAMES[idx])] = nb::cast(dist);
        }
    }

    result["overall"] = overallDict;
    result["composition"] = compDict;
    result["distributions"] = distDict;

    return result;
}

nanobind::dict PyDyssol::GetUnitHoldupOverall(const std::string& unitName) {
    nanobind::dict overall;

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

nanobind::dict PyDyssol::GetUnitHoldupComposition(const std::string& unitName) {
    nanobind::dict composition;

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

nanobind::dict PyDyssol::GetUnitHoldupDistribution(const std::string& unitName) {
    nanobind::dict distributions;

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
        nanobind::dict dists = GetUnitHoldupDistribution(unitName, t);
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

nanobind::dict PyDyssol::GetUnitHoldup(const std::string& unitName) {
    nanobind::dict result;
    result["overall"] = GetUnitHoldupOverall(unitName);
    result["composition"] = GetUnitHoldupComposition(unitName);
    result["distributions"] = GetUnitHoldupDistribution(unitName);
    return result;
}

nanobind::dict PyDyssol::GetUnitHoldupOverall(const std::string& unitName, const std::string& holdupName) {
    nanobind::dict overall;

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

nanobind::dict PyDyssol::GetUnitHoldupComposition(const std::string& unitName, const std::string& holdupName) {
    nanobind::dict composition;

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

nanobind::dict PyDyssol::GetUnitHoldupDistribution(const std::string& unitName, const std::string& holdupName) {
    nanobind::dict distributions;

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

nanobind::dict PyDyssol::GetUnitHoldup(const std::string& unitName, const std::string& holdupName) {
    nanobind::dict result;
    result["overall"] = GetUnitHoldupOverall(unitName, holdupName);
    result["composition"] = GetUnitHoldupComposition(unitName, holdupName);
    result["distributions"] = GetUnitHoldupDistribution(unitName, holdupName);
    return result;
}


void PyDyssol::SetHoldupValues(CHoldup* holdup, double time, const nanobind::dict& data, const std::vector<const CGridDimension*>& gridDims)
{
    // === Set composition ===
    if (data.contains("composition")) {
        const auto compDict = nb::cast<nanobind::dict>(data["composition"]);
        std::map<EPhase, double> phaseMassMap;

        for (auto [key, value] : compDict) {
            std::string key_str = nb::cast<std::string>(key);
            double value_d = nb::cast<double>(value);

            // Parse "Compound [phase]" or fallback to SOLID
            size_t split = key_str.find(" [");
            std::string compoundName = key_str;
            EPhase phase = EPhase::SOLID;

            if (split != std::string::npos && key_str.back() == ']') {
                compoundName = key_str.substr(0, split);
                std::string phaseStr = key_str.substr(split + 2, key_str.length() - split - 3);
                phase = GetPhaseByName(phaseStr);
            }

            const CCompound* comp = m_materialsDatabase.GetCompound(compoundName);
            if (!comp)
                comp = m_materialsDatabase.GetCompoundByName(compoundName);
            if (!comp)
                throw std::runtime_error("Unknown compound: " + compoundName);

            holdup->SetCompoundMass(time, comp->GetKey(), phase, value_d);
            phaseMassMap[phase] += value_d;
        }

        // Finalize phase masses
        for (const auto& [phase, total] : phaseMassMap)
            holdup->SetPhaseMass(time, phase, total);
    }

    // === Set overall properties ===
    if (data.contains("overall")) {
        const auto overallDict = nb::cast<nanobind::dict>(data["overall"]);
        for (auto [key, value] : overallDict) {
            std::string name = nb::cast<std::string>(key);
            double value_d = nb::cast<double>(value);
            holdup->SetOverallProperty(time, StringToEOverall(name), value_d);
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
        const auto distDict = nb::cast<nanobind::dict>(data["distributions"]);
        for (auto [key, value] : distDict) {
            std::string distrName = nb::cast<std::string>(key);
            std::vector<double> values = nb::cast<std::vector<double>>(value);
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

void PyDyssol::SetUnitHoldup(const std::string& unitName, const nanobind::dict& data)
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

    SetHoldupValues(holdupsWork.front(), time, data, gridDims);
    SetHoldupValues(holdupsInit.front(), time, data, gridDims);
}

void PyDyssol::SetUnitHoldup(const std::string& unitName, const std::string& holdupName, const nanobind::dict& data)
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
    SetHoldupValues(holdupWork, time, data, gridDims);
    SetHoldupValues(holdupInit, time, data, gridDims);
}
