#include "PyDyssol.h"
#include <iostream>
#include <stdexcept>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace fs = std::filesystem;
namespace py = pybind11;

//Unit Feeds
std::vector<std::string> PyDyssol::GetUnitFeeds(const std::string& unitName) const {
    const CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto& feeds = unit->GetModel()->GetStreamsManager().GetFeeds();
    std::vector<std::string> result;
    result.reserve(feeds.size());
    for (const auto* feed : feeds)
        result.push_back(feed->GetName());

    return result;
}

//Overalls
pybind11::dict PyDyssol::GetUnitFeedOverall(const std::string& unitName) {
    pybind11::dict overall;
    auto feeds = GetUnitFeeds(unitName);
    if (feeds.empty())
        throw std::runtime_error("No feeds found in unit: " + unitName);

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto* feed = unit->GetModel()->GetStreamsManager().GetFeed(feeds.front());
    if (!feed)
        throw std::runtime_error("Feed not found: " + feeds.front());

    std::vector<double> timepoints = feed->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;

    // Collect overall data for all timepoints
    std::map<std::string, std::vector<double>> overallData;
    for (double t : timepoints) {
        timeList.push_back(t);
        overallData["massflow"].push_back(feed->GetMass(t));
        overallData["temperature"].push_back(feed->GetTemperature(t));
        overallData["pressure"].push_back(feed->GetPressure(t));
    }

    overall["timepoints"] = timeList;
    for (const auto& [key, vec] : overallData)
        overall[key.c_str()] = vec;

    return overall;
}

std::map<std::string, double> PyDyssol::GetUnitFeedOverall(const std::string& unitName, double time) const {
    auto feeds = GetUnitFeeds(unitName);
    if (feeds.empty())
        throw std::runtime_error("No feeds found in unit: " + unitName);
    return GetUnitFeedOverall(unitName, feeds.front(), time);
}

std::map<std::string, double> PyDyssol::GetUnitFeedOverall(const std::string& unitName, const std::string& feedName, double time) const {
    std::map<std::string, double> overall;
    const CStream* feed = m_flowsheet.GetUnitByName(unitName)->GetModel()->GetStreamsManager().GetFeed(feedName);
    if (!feed) throw std::runtime_error("Feed not found: " + feedName + " in unit " + unitName);

    overall["massflow"] = feed->GetMass(time);
    overall["temperature"] = feed->GetTemperature(time);
    overall["pressure"] = feed->GetPressure(time);
    return overall;
}

pybind11::dict PyDyssol::GetUnitFeedOverall(const std::string& unitName, const std::string& feedName) {
    pybind11::dict overall;
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);
    const auto* feed = unit->GetModel()->GetStreamsManager().GetFeed(feedName);
    if (!feed)
        throw std::runtime_error("Feed not found: " + feedName);

    std::vector<double> timepoints = feed->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::map<std::string, std::vector<double>> overallData;
    for (double t : timepoints) {
        timeList.push_back(t);
        overallData["massflow"].push_back(feed->GetMass(t));
        overallData["temperature"].push_back(feed->GetTemperature(t));
        overallData["pressure"].push_back(feed->GetPressure(t));
    }

    overall["timepoints"] = timeList;
    for (const auto& [key, vec] : overallData)
        overall[key.c_str()] = vec;

    return overall;
}

//Compositions
pybind11::dict PyDyssol::GetUnitFeedComposition(const std::string& unitName) {
    pybind11::dict composition;
    auto feeds = GetUnitFeeds(unitName);
    if (feeds.empty())
        throw std::runtime_error("No feeds found in unit: " + unitName);

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto* feed = unit->GetModel()->GetStreamsManager().GetFeed(feeds.front());
    if (!feed)
        throw std::runtime_error("Feed not found: " + feeds.front());

    std::vector<double> timepoints = feed->GetAllTimePoints();
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
                double val = feed->GetCompoundMass(t, compoundKey, phase.state);
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

std::map<std::string, double> PyDyssol::GetUnitFeedComposition(const std::string& unitName, double time) const {
    auto feeds = GetUnitFeeds(unitName);
    if (feeds.empty())
        throw std::runtime_error("No feeds found in unit: " + unitName);
    return GetUnitFeedComposition(unitName, feeds.front(), time);
}

pybind11::dict PyDyssol::GetUnitFeedComposition(const std::string& unitName, const std::string& feedName) {
    pybind11::dict composition;
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);
    const auto* feed = unit->GetModel()->GetStreamsManager().GetFeed(feedName);
    if (!feed)
        throw std::runtime_error("Feed not found: " + feedName);

    std::vector<double> timepoints = feed->GetAllTimePoints();
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
                double val = feed->GetCompoundMass(t, compoundKey, phase.state);
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
        if (!allZero) composition[name.c_str()] = values;
    }

    return composition;
}

std::map<std::string, double> PyDyssol::GetUnitFeedComposition(const std::string& unitName, const std::string& feedName, double time) const {
    std::map<std::string, double> composition;
    const CStream* feed = m_flowsheet.GetUnitByName(unitName)->GetModel()->GetStreamsManager().GetFeed(feedName);
    if (!feed) throw std::runtime_error("Feed not found: " + feedName + " in unit " + unitName);

    for (const auto& compoundKey : m_flowsheet.GetCompounds()) {
        const CCompound* compound = m_materialsDatabase.GetCompound(compoundKey);
        std::string compoundName = compound ? compound->GetName() : compoundKey;

        for (const auto& phase : m_flowsheet.GetPhases()) {
            double mass = feed->GetCompoundMass(time, compoundKey, phase.state);
            if (std::abs(mass) > 1e-12) {
                std::string label = compoundName + " [" + PhaseToString(phase.state) + "]";
                composition[label] = mass;
            }
        }
    }

    return composition;
}

//Distributions
pybind11::dict PyDyssol::GetUnitFeedDistribution(const std::string& unitName) {
    pybind11::dict distributions;
    auto feeds = GetUnitFeeds(unitName);
    if (feeds.empty())
        throw std::runtime_error("No feeds found in unit: " + unitName);

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto* feed = unit->GetModel()->GetStreamsManager().GetFeed(feeds.front());
    if (!feed)
        throw std::runtime_error("Feed not found: " + feeds.front());

    std::vector<double> timepoints = feed->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::set<std::string> allDistributions;
    std::map<std::string, std::vector<std::vector<double>>> distributionData;

    for (double t : timepoints) {
        timeList.push_back(t);
        pybind11::dict dists = GetUnitFeedDistribution(unitName, feeds.front(), t);
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

pybind11::dict PyDyssol::GetUnitFeedDistribution(const std::string& unitName, double time) const {
    auto feeds = GetUnitFeeds(unitName);
    if (feeds.empty())
        throw std::runtime_error("No feeds found in unit: " + unitName);
    return GetUnitFeedDistribution(unitName, feeds.front(), time);
}

pybind11::dict PyDyssol::GetUnitFeedDistribution(const std::string& unitName, const std::string& feedName) {
    pybind11::dict distributions;
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);
    const auto* feed = unit->GetModel()->GetStreamsManager().GetFeed(feedName);
    if (!feed)
        throw std::runtime_error("Feed not found: " + feedName);

    std::vector<double> timepoints = feed->GetAllTimePoints();
    double t_end = m_flowsheet.GetParameters()->endSimulationTime;
    if (timepoints.empty() || std::abs(timepoints.back() - t_end) > 1e-6)
        timepoints.push_back(t_end);

    std::vector<double> timeList;
    std::set<std::string> allDistributions;
    std::map<std::string, std::vector<std::vector<double>>> distributionData;

    for (double t : timepoints) {
        timeList.push_back(t);
        pybind11::dict dists = GetUnitFeedDistribution(unitName, feedName, t);
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
        for (const auto& vec : vecOfVecs)
            if (std::any_of(vec.begin(), vec.end(), [](double v) { return std::abs(v) > 1e-15; })) {
                allZero = false;
                break;
            }
        if (!allZero)
            distributions[distName.c_str()] = vecOfVecs;
    }
    return distributions;
}

pybind11::dict PyDyssol::GetUnitFeedDistribution(const std::string& unitName, const std::string& feedName, double time) const {
    pybind11::dict result;
    const CStream* feed = m_flowsheet.GetUnitByName(unitName)->GetModel()->GetStreamsManager().GetFeed(feedName);
    if (!feed) throw std::runtime_error("Feed not found: " + feedName + " in unit " + unitName);

    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();
    for (const CGridDimension* dim : gridDims) {
        EDistrTypes type = dim->DimensionType();
        if (type == EDistrTypes::DISTR_COMPOUNDS) continue;
        int idx = GetDistributionTypeIndex(type);
        if (idx < 0) continue;
        std::vector<double> dist = feed->GetDistribution(time, type);
        if (!dist.empty())
            result[DISTR_NAMES[idx]] = pybind11::cast(dist);
    }

    return result;
}

//General Getters
pybind11::list PyDyssol::GetUnitFeed(const std::string& unitName) {
    pybind11::list result;
    for (const auto& feedName : GetUnitFeeds(unitName)) {
        pybind11::dict entry;
        entry["unit"] = unitName;
        entry["feed"] = feedName;
        entry["overall"] = GetUnitFeedOverall(unitName, feedName);
        entry["composition"] = GetUnitFeedComposition(unitName, feedName);
        entry["distributions"] = GetUnitFeedDistribution(unitName, feedName);
        result.append(entry);
    }
    return result;
}

pybind11::list PyDyssol::GetUnitFeed(const std::string& unitName, double time) const {
    pybind11::list result;
    for (const auto& feedName : GetUnitFeeds(unitName)) {
        pybind11::dict entry;
        entry["unit"] = unitName;
        entry["feed"] = feedName;
        entry["overall"] = GetUnitFeedOverall(unitName, feedName, time);
        entry["composition"] = GetUnitFeedComposition(unitName, feedName, time);
        entry["distributions"] = GetUnitFeedDistribution(unitName, feedName, time);
        result.append(entry);
    }
    return result;
}

pybind11::list PyDyssol::GetUnitFeed(const std::string& unitName, const std::string& feedName) {
    pybind11::list result;
    pybind11::dict entry;
    entry["unit"] = unitName;
    entry["feed"] = feedName;
    entry["overall"] = GetUnitFeedOverall(unitName, feedName);
    entry["composition"] = GetUnitFeedComposition(unitName, feedName);
    entry["distributions"] = GetUnitFeedDistribution(unitName, feedName);
    result.append(entry);
    return result;
}

pybind11::list PyDyssol::GetUnitFeed(const std::string& unitName, const std::string& feedName, double time) const {
    pybind11::list result;
    pybind11::dict entry;
    entry["unit"] = unitName;
    entry["feed"] = feedName;
    entry["overall"] = GetUnitFeedOverall(unitName, feedName, time);
    entry["composition"] = GetUnitFeedComposition(unitName, feedName, time);
    entry["distributions"] = GetUnitFeedDistribution(unitName, feedName, time);
    result.append(entry);
    return result;
}

//Sets
void PyDyssol::SetUnitFeed(const std::string& unitName, const std::string& feedName, double time, const pybind11::dict& data)
{
    CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    CStream* feed = unit->GetModel()->GetStreamsManager().GetFeed(feedName);
    CStream* feedInit = unit->GetModel()->GetStreamsManager().GetFeedInit(feedName);
    if (!feed || !feedInit) throw std::runtime_error("Feed not found: " + feedName + " in unit " + unitName);

    const auto& compounds = m_flowsheet.GetCompounds();
    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();

    // === Composition ===
    if (data.contains("composition")) {
        const auto compDict = data["composition"].cast<pybind11::dict>();

        // Track total mass per phase
        std::map<EPhase, double> phaseMassMap;

        for (const auto& item : compDict) {
            std::string key = item.first.cast<std::string>();
            double value = item.second.cast<double>();

            // Parse "Compound [phase]" or fallback to SOLID
            size_t split = key.find(" [");
            std::string compoundName = key;
            EPhase phase = EPhase::SOLID;

            if (split != std::string::npos && key.back() == ']') {
                compoundName = key.substr(0, split);
                std::string phaseStr = key.substr(split + 2, key.length() - split - 3);
                phase = GetPhaseByName(phaseStr);
            }

            const CCompound* comp = m_materialsDatabase.GetCompound(compoundName);
            if (!comp)
                comp = m_materialsDatabase.GetCompoundByName(compoundName);
            if (!comp)
                throw std::runtime_error("Unknown compound: " + compoundName);

            feed->SetCompoundMass(time, comp->GetKey(), phase, value);
			feedInit->SetCompoundMass(time, comp->GetKey(), phase, value);
            phaseMassMap[phase] += value;
        }

        // Finalize phase masses
        for (const auto& [phase, total] : phaseMassMap) {
            feed->SetPhaseMass(time, phase, total);
            feedInit->SetPhaseMass(time, phase, total);
        }
    }

    // === Overall ===
    if (data.contains("overall")) {
        const auto overallDict = data["overall"].cast<pybind11::dict>();
        for (const auto& item : overallDict) {
            std::string name = item.first.cast<std::string>();
            double value = item.second.cast<double>();
            feed->SetOverallProperty(time, StringToEOverall(name), value);
			feedInit->SetOverallProperty(time, StringToEOverall(name), value);
        }
    }

    // === Grid name - type map ===
    std::map<std::string, EDistrTypes> nameToType;
    for (const CGridDimension* dim : gridDims) {
        int idx = GetDistributionTypeIndex(dim->DimensionType());
        if (idx >= 0)
            nameToType[DISTR_NAMES[idx]] = DISTR_TYPES[idx];
    }

    // === Distributions ===
    if (data.contains("distributions")) {
        const auto distDict = data["distributions"].cast<pybind11::dict>();
        for (const auto& item : distDict) {
            std::string name = item.first.cast<std::string>();
            std::vector<double> values = item.second.cast<std::vector<double>>();
            if (!nameToType.count(name))
                throw std::runtime_error("Unknown distribution type: " + name);
            auto distrType = nameToType[name];
            std::vector<double> norm = Normalized(values);
            std::vector<double> current = feed->GetDistribution(time, distrType);
            if (norm.size() != current.size())
                throw std::runtime_error("Size mismatch in distribution '" + name + "'");
            feed->SetDistribution(time, distrType, norm);
			feedInit->SetDistribution(time, distrType, norm);
        }
    }
}

void PyDyssol::SetUnitFeed(const std::string& unitName, const py::dict& data) {
    const double time = 0.0;
    const auto feeds = GetUnitFeeds(unitName);
    if (feeds.empty())
        throw std::runtime_error("No feeds found in unit: " + unitName);
    SetUnitFeed(unitName, feeds.front(), time, data);
}

void PyDyssol::SetUnitFeed(const std::string& unitName, double time, const py::dict& data)
{
    const auto feeds = GetUnitFeeds(unitName);
    if (feeds.empty())
        throw std::runtime_error("No feeds found in unit: " + unitName);
    SetUnitFeed(unitName, feeds.front(), time, data);
}

void PyDyssol::SetUnitFeed(const std::string& unitName, const std::string& feedName, const py::dict& data)
{
    // 1. Try to find timepoints from any section
    std::vector<double> timepoints;

    if (data.contains("overall") && py::isinstance<py::dict>(data["overall"])) {
        py::dict overall = data["overall"];
        if (overall.contains("timepoints"))
            timepoints = overall["timepoints"].cast<std::vector<double>>();
    }
    if (timepoints.empty() && data.contains("composition")) {
        py::dict comp = data["composition"];
        if (comp.contains("timepoints"))
            timepoints = comp["timepoints"].cast<std::vector<double>>();
    }
    if (timepoints.empty() && data.contains("distributions")) {
        py::dict dist = data["distributions"];
        if (dist.contains("timepoints"))
            timepoints = dist["timepoints"].cast<std::vector<double>>();
    }

    // 2. If no timepoints fallback to t = 0.0
    if (timepoints.empty()) {
        this->SetUnitFeed(unitName, feedName, 0.0, data);
        return;
    }

    // 3. Loop over timepoints and slice each section manually
    for (size_t i = 0; i < timepoints.size(); ++i) {
        double t = timepoints[i];
        py::dict slice;

        // === Slice overall ===
        if (data.contains("overall")) {
            py::dict slicedOverall;
            py::dict overall = data["overall"];
            for (auto item : overall) {
                std::string key = item.first.cast<std::string>();
                if (key == "timepoints") continue;
                std::vector<double> vec = py::cast<std::vector<double>>(item.second);
                if (i < vec.size())
                    slicedOverall[key.c_str()] = vec[i];
            }
            slice["overall"] = slicedOverall;
        }

        // === Slice composition ===
        if (data.contains("composition")) {
            py::dict slicedComp;
            py::dict comp = data["composition"];
            for (auto item : comp) {
                std::string key = item.first.cast<std::string>();
                if (key == "timepoints") continue;
                std::vector<double> vec = py::cast<std::vector<double>>(item.second);
                if (i < vec.size())
                    slicedComp[key.c_str()] = vec[i];
            }
            slice["composition"] = slicedComp;
        }

        // === Slice distributions ===
        if (data.contains("distributions")) {
            py::dict slicedDist;
            py::dict dist = data["distributions"];
            for (auto item : dist) {
                std::string key = item.first.cast<std::string>();
                if (key == "timepoints") continue;
                std::vector<std::vector<double>> mat = py::cast<std::vector<std::vector<double>>>(item.second);
                if (i < mat.size())
                    slicedDist[key.c_str()] = mat[i];
            }
            slice["distributions"] = slicedDist;
        }

        // 4. Apply sliced feed
        this->SetUnitFeed(unitName, feedName, t, slice);
    }
}

void PyDyssol::SetUnitFeed(const py::dict& d)
{
    // 1) Unit
    const std::string unit = d["unit"].cast<std::string>();

    // 2) Feed name: explicit or first one
    std::string feedName;
    if (d.contains("feed"))
        feedName = d["feed"].cast<std::string>();
    else {
        auto feeds = GetUnitFeeds(unit);
        if (feeds.empty())
            throw std::runtime_error("No feeds defined for unit: " + unit);
        feedName = feeds.front();
    }

    // 3) Dispatch to existing overloads (the 3-arg will internally slice timepoints)
    SetUnitFeed(unit, feedName, d);
}
