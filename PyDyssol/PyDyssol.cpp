#include "PyDyssol.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <chrono>
#include <iomanip>    // for std::setprecision
#include <sstream>    // for std::ostringstream
#include <pybind11/pybind11.h> 
#include <pybind11/stl.h>      // For std::vector and std::pair bindings
#include <pybind11/stl_bind.h>      // For STL bindings with containers
#include <SaveLoadManager.h>
#include <algorithm> // For std::find_if
#include "MultidimensionalGrid.h"

namespace fs = std::filesystem;
namespace py = pybind11; // Add namespace alias

PyDyssol::PyDyssol(const std::string& materialsPath, const std::string& modelsPath, bool debug)
    : m_flowsheet(&m_modelsManager, &m_materialsDatabase),
    m_defaultMaterialsPath(materialsPath),
    m_defaultModelsPath(modelsPath),
    m_isDatabaseLoaded(false),
    m_isModelsLoaded(false),
    m_debug(debug)
{
    m_simulator.SetFlowsheet(&m_flowsheet);
    if (m_debug)
        std::cout << "[PyDyssol] Dyssol opened in Debug mode\n";
    if (!LoadMaterialsDatabase(m_defaultMaterialsPath)) {
        throw std::runtime_error("Failed to load default materials database: " + materialsPath);
    }
    if (!AddModelPath(m_defaultModelsPath)) {
        throw std::runtime_error("Failed to add default model path: " + modelsPath);
    }
}

bool PyDyssol::OpenFlowsheet(const std::string& filePath)
{
    std::cout << "[PyDyssol] Opening flowsheet: " << filePath << std::endl;
    if (!fs::exists(filePath)) {
        std::cerr << "[PyDyssol] Flowsheet file does not exist: " << filePath << std::endl;
        return false;
    }

    SSaveLoadData data;
    data.flowsheet = &m_flowsheet;

    CSaveLoadManager loader{ data };
    CH5Handler handler;

    handler.Open(filePath);
    if (!handler.IsValid()) {
        std::cerr << "[PyDyssol] Failed to open HDF5 file: " << filePath << std::endl;
        return false;
    }

    if (!loader.LoadFromFile(filePath)) {
        if (!m_flowsheet.LoadFromFile(handler, "/")) {
            std::cerr << "[PyDyssol] Failed to load flowsheet using both CSaveLoadManager and CFlowsheet::LoadFromFile." << std::endl;
            handler.Close();
            return false;
        }
    }

    handler.Close();
    if (m_debug) {
        std::cout << "[PyDyssol] Flowsheet loaded successfully." << std::endl;

        // Post-load check
        std::cout << "[PyDyssol] Post-load check - Units: " << m_flowsheet.GetAllUnits().size()
            << ", Streams: " << m_flowsheet.GetAllStreams().size() << std::endl;
        DebugFlowsheet();
    }
    Initialize();
    return true;
}

void PyDyssol::CloseFlowsheet()
{
    std::cout << "[PyDyssol] Closing current flowsheet..." << std::endl;

    m_flowsheet.Clear();                      // Clears all flowsheet data
    m_simulator.SetFlowsheet(&m_flowsheet);   // Re-bind simulator to the new (cleared) flowsheet

    // Reconnect shared resources
    m_flowsheet.SetMaterialsDatabase(&m_materialsDatabase);

    std::cout << "[PyDyssol] Flowsheet closed and reset." << std::endl;
}

bool PyDyssol::SaveFlowsheet(const std::string& filePath)
{
    std::cout << "[PyDyssol] Saving flowsheet to: " << filePath << std::endl;

    // Ensure the directory exists
    fs::path filePathObj(filePath);
    fs::path parentDir = filePathObj.parent_path();
    if (!parentDir.empty() && !fs::exists(parentDir)) {
        std::cout << "[PyDyssol] Creating directory: " << parentDir << std::endl;
        if (!fs::create_directories(parentDir)) {
            std::cerr << "[PyDyssol] Failed to create directory: " << parentDir << std::endl;
            return false;
        }
    }

    CH5Handler handler;
    try {
        handler.Create(filePath);
        if (!handler.IsValid()) {
            std::cerr << "[PyDyssol] Failed to create HDF5 file: " << filePath << std::endl;
            return false;
        }

        if (!m_flowsheet.SaveToFile(handler, "/")) {
            std::cerr << "[PyDyssol] Failed to save flowsheet using CFlowsheet::SaveToFile." << std::endl;
            handler.Close();
            return false;
        }

        handler.Close();
        std::cout << "[PyDyssol] Flowsheet saved successfully." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[PyDyssol] Exception while saving flowsheet: " << e.what() << std::endl;
        handler.Close();
        return false;
    }

    // Verify the file exists
    if (!fs::exists(filePath)) {
        std::cerr << "[PyDyssol] File was not created on disk: " << filePath << std::endl;
        return false;
    }

    return true;
}

void PyDyssol::DebugFlowsheet()
{
    std::cout << "[PyDyssol] Flowsheet Debug Info:" << std::endl;
    std::cout << "  Units: " << m_flowsheet.GetAllUnits().size() << std::endl;
    for (const auto& unit : m_flowsheet.GetAllUnits()) {
        std::cout << "    Unit: " << unit->GetName()
            << " (Model: " << GetModelNameForUnit(unit->GetKey()) << ")" << std::endl;
    }
    std::cout << "  Streams: " << m_flowsheet.GetAllStreams().size() << std::endl;
    for (const auto& stream : m_flowsheet.GetAllStreams()) {
        std::cout << "    Stream: " << stream->GetName() << std::endl;
    }
    std::cout << "  Compounds: " << m_flowsheet.GetCompounds().size() << std::endl;
    for (const auto& compoundKey : m_flowsheet.GetCompounds()) {
        const CCompound* compound = m_materialsDatabase.GetCompound(compoundKey);
        if (compound) {
            std::cout << "    Compound: " << compound->GetName() << std::endl;
        }
        else {
            std::cout << "    Compound: [Not Found in Database] " << std::endl;
        }
    }
    std::cout << "  Phases: " << m_flowsheet.GetPhases().size() << std::endl;
    for (const auto& phase : m_flowsheet.GetPhases()) {
        std::cout << "    Phase: " << static_cast<int>(phase.state) << " (" << phase.name << ")" << std::endl;
    }
}

std::string PyDyssol::Initialize()
{
    std::string error = m_flowsheet.Initialize();
    if (!error.empty()) {
        std::cerr << "[PyDyssol] Initialization failed: " << error << std::endl;
    }
    else if (m_debug) {
        std::cout << "[PyDyssol] Initializing flowsheet..." << std::endl;
        std::cout << "[PyDyssol] Flowsheet initialized successfully." << std::endl;
    }
    return error;
}

void PyDyssol::Simulate(double endTime)
{
    std::string error = Initialize();
    if (!error.empty())
       throw std::runtime_error("Flowsheet initialization failed: " + error);

    // Get simulation time range from CParametersHolder
    CParametersHolder* parameters = m_flowsheet.GetParameters();
    double simStartTime = parameters->startSimulationTime; // Access via proxy
    double simEndTime = parameters->endSimulationTime;     // Access via proxy
    if (endTime > 0.0) {
        simEndTime = endTime;
        parameters->StartSimulationTime(simStartTime); // Set start time
        parameters->EndSimulationTime(simEndTime);     // Set end time
        std::cout << "[PyDyssol] Overriding simulation end time to: " << endTime << " seconds." << std::endl;
    }

    // Log initialization for each unit
    for (const auto& unit : m_flowsheet.GetAllUnits()) {
        const std::string& name = unit->GetName();
        std::string model = GetModelNameForUnit(unit->GetKey());
        std::cout << "Initialization of " << name << " (" << model << ")..." << std::endl;
    }

    // Log simulation for each unit with time range
    for (const auto& unit : m_flowsheet.GetAllUnits()) {
        const std::string& name = unit->GetName();
        std::string model = GetModelNameForUnit(unit->GetKey());
        std::cout << "Simulation of " << name << " (" << model << "): [" << simStartTime << ", " << simEndTime << "]..." << std::endl;
    }

    std::cout << "[PyDyssol] Starting simulation..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    m_simulator.Simulate();
    auto end = std::chrono::high_resolution_clock::now();

    // Log finalization for each unit
    for (const auto& unit : m_flowsheet.GetAllUnits()) {
        const std::string& name = unit->GetName();
        std::string model = GetModelNameForUnit(unit->GetKey());
        std::cout << "Finalization of " << name << " (" << model << ")..." << std::endl;
    }

    // Log saving of tear streams
    std::cout << "Saving new initial values of tear streams..." << std::endl;

    double seconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1'000'000.0;
    std::cout << "[PyDyssol] Simulation finished in " << std::fixed << std::setprecision(3) << seconds << " [s]" << std::endl;
}

// Flowsheet
pybind11::list PyDyssol::GetTopology() const
{
    pybind11::list topology;
    for (const CUnitContainer* unit : m_flowsheet.GetAllUnits()) {
        pybind11::dict unitConfig;
        unitConfig["unit"] = unit->GetName();
        if (unit->GetModel()) {
            std::string modelName = unit->GetModel()->GetUnitName();
            unitConfig["model"] = modelName.empty() ? unit->GetModel()->GetUniqueID() : modelName;
        }
        else {
            unitConfig["model"] = "";
        }

        pybind11::dict ports;
        if (unit->GetModel()) {
            for (const CUnitPort* port : unit->GetModel()->GetPortsManager().GetAllPorts()) {
                std::string portName = port->GetName();
                std::string streamName = port->GetStream() ? port->GetStream()->GetName() : "";
                ports[portName.c_str()] = pybind11::dict(pybind11::arg("stream") = streamName);
            }
        }
        unitConfig["ports"] = ports;

        topology.append(unitConfig);
    }
    return topology;
}

bool PyDyssol::SetTopology(const py::dict& config, bool initialize)
{
    std::vector<std::string> compoundBackup;
    std::vector<SPhaseDescriptor> phaseBackup;

    try {
        compoundBackup = m_flowsheet.GetCompounds();
        phaseBackup = m_flowsheet.GetPhases();
        std::cout << "[PyDyssol] Backed up compounds: " << compoundBackup.size() << ", phases: " << phaseBackup.size() << std::endl;

        if (config.contains("phases")) {
            py::list phaseList = config["phases"];
            if (!this->SetPhases(phaseList))
                throw std::runtime_error("Failed to set phases");
            std::cout << "[PyDyssol] Phases set: " << m_flowsheet.GetPhasesNumber() << std::endl;
        }

        if (config.contains("grids")) {
            std::vector<std::map<std::string, py::object>> grids;
            for (const auto& grid : config["grids"]) {
                grids.push_back({ {"type", grid["type"]}, {"grid", grid["grid"]} });
            }
            this->SetGrids(grids);
            std::cout << "[PyDyssol] Grids set" << std::endl;
        }

        py::list unitConfigs;
        if (config.contains("topology")) {
            unitConfigs = config["topology"];

            std::cout << "[PyDyssol] Flowsheet state before unit setup: "
                << "units=" << m_flowsheet.GetUnitsNumber()
                << ", streams=" << m_flowsheet.GetStreamsNumber()
                << ", compounds=" << m_flowsheet.GetCompoundsNumber()
                << ", phases=" << m_flowsheet.GetPhasesNumber() << std::endl;

            for (CUnitContainer* unit : m_flowsheet.GetAllUnits()) {
                m_flowsheet.DeleteUnit(unit->GetKey());
            }
            for (CStream* stream : m_flowsheet.GetAllStreams()) {
                m_flowsheet.DeleteStream(stream->GetKey());
            }
            m_flowsheet.SetTopologyModified(true);
            std::cout << "[PyDyssol] Cleared units and streams, topology modified" << std::endl;

            for (const auto& unitConfig : unitConfigs) {
                py::dict cfg = py::cast<py::dict>(unitConfig);
                std::string unitName = py::cast<std::string>(cfg["unit"]);
                this->SetUnitConfig(unitName, cfg);
            }
        }

            if (config.contains("compounds")) {
                std::vector<std::string> compounds;
                for (const auto& item : config["compounds"]) {
                    compounds.push_back(py::cast<std::string>(item));
                }
                if (!this->SetCompounds(compounds)) {
                    throw std::runtime_error("Failed to set compounds");
                }
                std::cout << "[PyDyssol] Compounds set: " << m_flowsheet.GetCompoundsNumber() << std::endl;
            }

            std::set<std::string> holdupUnits;
            if (config.contains("holdups")) {
                for (auto _h : config["holdups"]) {
                    py::dict d = py::cast<py::dict>(_h);
                    holdupUnits.insert(d["unit"].cast<std::string>());
                }
            }

            if (config.contains("feeds")) {
                for (auto _f : config["feeds"]) {
                    py::dict   d = py::cast<py::dict>(_f);
                    std::string unit = d["unit"].cast<std::string>();

                    std::string feedName;
                    if (d.contains("feed")) {
                        feedName = d["feed"].cast<std::string>();
                    }
                    else {
                        auto feeds = this->GetUnitFeeds(unit);
                        if (feeds.empty())
                            throw std::runtime_error("No feeds defined for unit: " + unit);
                        feedName = feeds.front();
                    }

                    try {
                        this->SetUnitFeed(unit, feedName, d);

                        if (!holdupUnits.count(unit)) {
                            auto hols = this->GetUnitHoldups(unit);
                            if (!hols.empty()) {
                                this->SetUnitHoldup(unit, hols.front(), d);
                            }
                        }
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[PyDyssol] Warning: failed to set feed for unit '"
                            << unit << "': " << e.what() << "\n";
                    }
                }
            }

            if (config.contains("holdups")) {
                for (auto _h : config["holdups"]) {
                    py::dict   d = py::cast<py::dict>(_h);
                    std::string unit = d["unit"].cast<std::string>();

                    std::string holdupName;
                    if (d.contains("holdup")) {
                        holdupName = d["holdup"].cast<std::string>();
                    }
                    else {
                        auto hs = this->GetUnitHoldups(unit);
                        if (hs.empty())
                            throw std::runtime_error("No holdups defined for unit: " + unit);
                        holdupName = hs.front();
                    }

                    py::dict data;
                    for (auto kv : d) {
                        std::string key = kv.first.cast<std::string>();
                        if (key != "unit" && key != "holdup")
                            data[key.c_str()] = kv.second;
                    }

                    try {
                        this->SetUnitHoldup(unit, holdupName, data);
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[PyDyssol] Warning: failed to set holdup for unit '"
                            << unit << "': " << e.what() << "\n";
                    }
                }
            }

            if (config.contains("unit parameters")) {
                for (const auto& paramBlock : config["unit parameters"]) {
                    const auto dict = py::cast<py::dict>(paramBlock);
                    const std::string unitName = dict["unit"].cast<std::string>();
                    const py::dict parameters = dict["parameters"].cast<py::dict>();

                    for (auto item : parameters) {
                        const std::string paramName = item.first.cast<std::string>();
                        const UnitParameterVariant valueVariant = py::cast<UnitParameterVariant>(item.second);

                        this->SetUnitParameter(unitName, paramName, valueVariant);
                    }
                }
            }

            if (config.contains("options")) {
                try {
                    py::object optionsObj = config["options"];
                    py::dict optionsDict;

                    if (py::isinstance<py::dict>(optionsObj)) {
                        optionsDict = optionsObj.cast<py::dict>();
                    }
                    else if (py::isinstance<py::list>(optionsObj)) {
                        py::list optionList = optionsObj.cast<py::list>();
                        if (optionList.size() == 1 && py::isinstance<py::dict>(optionList[0])) {
                            optionsDict = optionList[0].cast<py::dict>();
                        }
                        else {
                            throw std::runtime_error("If 'options' is a list, it must contain exactly one dict.");
                        }
                    }
                    else {
                        throw std::runtime_error("'options' must be a dict or a single-item list of dict.");
                    }

                    this->SetOptions(optionsDict);
                    std::cout << "[PyDyssol] Simulation options set" << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[PyDyssol] Failed to set options: " << e.what() << std::endl;
                }
            }

            std::cout << "[PyDyssol] Flowsheet state after setting: "
                << "units=" << m_flowsheet.GetUnitsNumber()
                << ", streams=" << m_flowsheet.GetStreamsNumber()
                << ", compounds=" << m_flowsheet.GetCompoundsNumber()
                << ", phases=" << m_flowsheet.GetPhasesNumber() << std::endl;

            return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[PyDyssol] Flowsheet setup failed: " << e.what() << std::endl;
        if (m_flowsheet.GetCompounds().empty() && !compoundBackup.empty()) {
            m_flowsheet.SetCompounds(compoundBackup);
            std::cerr << "[PyDyssol] Recovered compounds after failure." << std::endl;
        }
        if (m_flowsheet.GetPhases().empty() && !phaseBackup.empty()) {
            m_flowsheet.SetPhases(phaseBackup);
            std::cerr << "[PyDyssol] Recovered phases after failure." << std::endl;
        }
        return false;
    }
}

std::string PyDyssol::ValidateCalculationSequence() const
{
    if (m_flowsheet.GetAllUnits().empty()) return "No units defined in flowsheet";
    if (m_flowsheet.GetCompounds().empty()) return "No compounds defined in flowsheet";
    if (m_flowsheet.GetPhases().empty()) return "No phases defined in flowsheet";
    bool hasFeed = false;
    std::string missingFeedDetails;
    for (const auto& unit : m_flowsheet.GetAllUnits()) {
        auto feeds = GetUnitFeeds(unit->GetName());
        if (feeds.empty()) {
            missingFeedDetails += "Unit '" + unit->GetName() + "' has no feeds defined.\n";
            continue;
        }
        for (const auto& feedName : feeds) {
            auto overall = GetUnitFeedOverall(unit->GetName(), feedName, 0.0);
            if (overall.find("massflow") != overall.end() && overall.at("massflow") > 0.0) {
                hasFeed = true;
                break;
            }
            missingFeedDetails += "Feed '" + feedName + "' for unit '" + unit->GetName() + "' has no valid mass flow at time 0.0.\n";
        }
        if (hasFeed) break;
    }
    if (!hasFeed) return "No valid feed data defined for any unit:\n" + missingFeedDetails;
    for (const auto& unit : m_flowsheet.GetAllUnits()) {
        const auto* model = unit->GetModel();
        if (!model) return "Unit '" + unit->GetName() + "' has no model assigned";
        bool hasConnection = false;
        for (const auto* port : model->GetPortsManager().GetAllPorts()) {
            if (port->GetStream()) {
                hasConnection = true;
                break;
            }
        }
        if (!hasConnection) return "Unit '" + unit->GetName() + "' has no stream connections";
    }
    return "";
}

py::dict PyDyssol::GetUnitConfig(const std::string& unitName) const
{
    py::dict config;
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) {
        throw std::runtime_error("Unit not found: " + unitName);
    }

    config["unit"] = unit->GetName();
    config["model"] = GetModelNameForUnit(unit->GetKey());

    py::dict ports;
    const auto* model = unit->GetModel();
    if (model) {
        const auto& portsList = model->GetPortsManager().GetAllPorts();
        for (const auto* port : portsList) {
            if (port) {
                const auto* stream = port->GetStream();
                ports[port->GetName().c_str()] = stream ? stream->GetName() : "";
            }
        }
    }
    config["ports"] = ports;

    return config;
}

void PyDyssol::SetUnitConfig(const std::string& unitName, const py::dict& config)
{
    CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) {
        unit = m_flowsheet.AddUnit(unitName);
        if (!unit)
            throw std::runtime_error("Failed to add unit: " + unitName);
        std::cout << "[PyDyssol] Added new unit: " << unitName << std::endl;
    }

    if (config.contains("unit")) {
        unit->SetName(py::cast<std::string>(config["unit"]));
    }

    if (config.contains("model")) {
        std::string modelName = py::cast<std::string>(config["model"]);
        std::string modelKey;
        for (const auto& desc : m_modelsManager.GetAvailableUnits()) {
            if (desc.name == modelName) {
                modelKey = desc.uniqueID;
                break;
            }
        }
        if (modelKey.empty())
            throw std::runtime_error("Model not found: " + modelName);
        unit->SetModel(modelKey);
    }

    CBaseUnit* model = unit->GetModel();
    if (!model)
        throw std::runtime_error("Model not initialized for unit: " + unitName);

    if (config.contains("ports")) {
        py::dict portDict = py::cast<py::dict>(config["ports"]);
        for (auto portItem : portDict) {
            std::string portName = portItem.first.cast<std::string>();
            py::dict portConfig = portItem.second.cast<py::dict>();

            std::string typeStr = portConfig.contains("type") ? portConfig["type"].cast<std::string>() : "input";
            std::string streamName = portConfig.contains("stream") ? portConfig["stream"].cast<std::string>() : "";

            EUnitPort portType;
            if (typeStr == "Input" || typeStr == "input")
                portType = EUnitPort::INPUT;
            else if (typeStr == "Output" || typeStr == "output")
                portType = EUnitPort::OUTPUT;
            else
                throw std::runtime_error("Unknown port type: '" + typeStr + "' for port: " + portName);

            CUnitPort* port = model->GetPortsManager().GetPort(portName);
            if (!port) {
                port = model->AddPort(portName, portType);
                if (!port)
                    throw std::runtime_error("Failed to add port: " + portName + " to unit: " + unitName);
                std::cout << "[PyDyssol] Added port: " << portName << " (" << typeStr << ")" << std::endl;
            }

            if (!streamName.empty()) {
                CStream* stream = m_flowsheet.GetStream(streamName);
                if (!stream) {
                    stream = this->AddStream(streamName);
                    if (!stream)
                        throw std::runtime_error("Failed to add stream: " + streamName);
                }
                if (stream->GetName() != streamName) {
                    stream->SetName(streamName);

                }
                port->SetStreamKey(streamName);
                port->SetStream(stream);
            }
        }
    }

    if (config.contains("holdups")) {
        for (const auto& holdupDict : config["holdups"].cast<std::vector<py::dict>>()) {
            const std::string holdupUnitName = holdupDict["unit"].cast<std::string>();
            const std::string holdupName = holdupDict["holdup"].cast<std::string>();
            if (holdupUnitName == unitName) {
                py::dict holdupData;
                for (auto item : holdupDict) {
                    const std::string key = item.first.cast<std::string>();
                    if (key != "unit" && key != "holdup")
                        holdupData[key.c_str()] = item.second;
                }
                this->SetUnitHoldup(unitName, holdupName, holdupData);
            }
        }
    }

    std::cout << "[PyDyssol] Debugging ports for unit: " << unitName << std::endl;
    for (const auto* port : unit->GetModel()->GetPortsManager().GetAllPorts()) {
        std::cout << "  Port: " << port->GetName()
            << ", Stream Key: " << port->GetStreamKey()
            << ", Stream: " << (port->GetStream() ? port->GetStream()->GetName() : "No Stream Connected")
            << std::endl;
    }
}

CStream* PyDyssol::AddStream(const std::string& streamName)
{
    std::cout << "[PyDyssol] Adding stream: " << streamName << std::endl;
    CStream* stream = m_flowsheet.AddStream(streamName);
    if (!stream) {
        std::cerr << "[PyDyssol] Failed to add stream: " << streamName << std::endl;
        return nullptr;
    }
    return stream;
}

const CStream* PyDyssol::GetStreams_flowsheet(const std::string& streamName) const
{
    std::cout << "[PyDyssol] Getting stream: " << streamName << std::endl;
    const CStream* stream = m_flowsheet.GetStream(streamName);
    if (stream) {
        std::cout << "[PyDyssol] Found stream: " << stream->GetName() << std::endl;
    }
    else {
        std::cout << "[PyDyssol] Stream not found: " << streamName << std::endl;
    }
    return stream;
}
std::vector<std::string> PyDyssol::GetAvailableModelNames() const
{
    std::vector<std::string> names;
    for (const auto& unit : m_modelsManager.GetAvailableUnits()) {
        names.push_back(unit.name);
    }
    return names;
}

// Methods for unit parameters

std::vector<std::pair<std::string, std::string>> PyDyssol::GetUnits()
{
    std::vector<std::pair<std::string, std::string>> units;
    for (const auto& unit : m_flowsheet.GetAllUnits()) {
        std::string modelName = GetModelNameForUnit(unit->GetKey());
        units.emplace_back(unit->GetName(), modelName);
    }
    return units;
}

std::map<std::string, std::string> PyDyssol::GetUnitsDict()
{
    std::map<std::string, std::string> unitMap;
    for (const auto& unit : m_flowsheet.GetAllUnits()) {
        std::string modelName = GetModelNameForUnit(unit->GetKey());
        unitMap[unit->GetName()] = modelName;
    }
    return unitMap;
}

//Grids
std::vector<py::dict> PyDyssol::GetGrids() const {
    std::vector<py::dict> result;

    for (const CGridDimension* dim : m_flowsheet.GetGrid().GetGridDimensions()) {
        py::dict grid;
        if (dim->DimensionType() == EDistrTypes::DISTR_COMPOUNDS)
            continue; // Skip compound distribution grids
        std::string typeStr = ToString(dim->DimensionType());
        grid["type"] = py::str(typeStr);

        if (const auto* num = dynamic_cast<const CGridDimensionNumeric*>(dim)) {
            grid["grid"] = py::cast(num->Grid());
        }
        else if (const auto* sym = dynamic_cast<const CGridDimensionSymbolic*>(dim)) {
            grid["grid"] = py::cast(sym->Grid());
        }
        else {
            throw std::runtime_error("Unknown grid dimension type.");
        }

        result.push_back(grid);
    }

    return result;
}

void PyDyssol::SetGrids(const std::vector<std::map<std::string, py::object>>& grids)
{
    auto& gridMgr = const_cast<CMultidimensionalGrid&>(m_flowsheet.GetGrid());
    std::vector<std::string> errors;

    // 1) Validate all requested grid types
    for (const auto& grid : grids) {
        std::string typeStr = py::cast<std::string>(grid.at("type"));
        if (!IsGridValid(grid)) {
            errors.push_back("Invalid grid type: '" + typeStr + "'. Valid types: " + GetAllowedDistrNames());
        }
    }
    if (!errors.empty()) {
        std::string errorMsg = "Failed to set grids:\n";
        for (const auto& err : errors)
            errorMsg += "  - " + err + "\n";
        throw std::runtime_error(errorMsg);
    }

    // 2) Remove only the existing dimensions _other_ than compounds
    std::vector<EDistrTypes> toRemove;
    for (const auto* dim : gridMgr.GetGridDimensions()) {
        if (dim->DimensionType() != EDistrTypes::DISTR_COMPOUNDS)
            toRemove.push_back(dim->DimensionType());
    }
    for (auto t : toRemove)
        gridMgr.RemoveDimension(t);

    // 3) Add each user-specified grid
    for (const auto& grid : grids) {
        AddGrid(grid);
    }

    // 4) Rebuild internal structures
    m_flowsheet.UpdateGrids();
}

void PyDyssol::AddGrid(const std::map<std::string, py::object>& gridData) {
    if (!IsGridValid(gridData))
        return;

    auto& gridMgr = const_cast<CMultidimensionalGrid&>(m_flowsheet.GetGrid());

    std::string typeStr = py::cast<std::string>(gridData.at("type"));
    EDistrTypes gridType = StringToDistrType(typeStr);
    py::list rawGrid = gridData.at("grid");

    bool isSymbolic = py::isinstance<py::str>(rawGrid[0]);

    if (gridMgr.HasDimension(gridType))
        std::cout << "[PyDyssol] Warning: Replacing existing grid of type: " << typeStr << std::endl;

    gridMgr.RemoveDimension(gridType);

    if (isSymbolic) {
        auto classNames = py::cast<std::vector<std::string>>(rawGrid);
        gridMgr.AddSymbolicDimension(gridType, classNames);
    }
    else {
        auto classLimits = py::cast<std::vector<double>>(rawGrid);
        gridMgr.AddNumericDimension(gridType, classLimits);
    }
}

py::dict PyDyssol::GetModelInfo(const std::string& unitName) const {
    py::dict result;

    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);
    const auto* model = unit->GetModel();
    if (!model)
        throw std::runtime_error("Model not set for unit: " + unitName);

    result["unit"] = unit->GetName();
    result["key"] = model->GetUniqueID();
    result["model"] = GetModelNameForUnit(unit->GetKey());
    result["author"] = model->GetAuthorName();

    // Ports
    py::dict ports;
    for (const auto* port : model->GetPortsManager().GetAllPorts()) {
        if (!port) continue;
        std::string typeStr = (port->GetType() == EUnitPort::INPUT) ? "input" :
            (port->GetType() == EUnitPort::OUTPUT) ? "output" : "undefined";
        ports[port->GetName().c_str()] = typeStr;
    }
    result["ports"] = ports;

    // Parameters
    result["parameters"] = GetUnitParametersAll(unitName);

    // Holdups
    result["holdups"] = py::cast(GetUnitHoldups(unitName));

    // Feeds
    result["feeds"] = py::cast(GetUnitFeeds(unitName));

    // Internal streams
    result["streams"] = py::cast(GetUnitStreams(unitName));

    return result;
}

//Debug
void PyDyssol::DebugUnitPorts(const std::string& unitName)
{
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) {
        std::cout << "[Debug] Unit not found: " << unitName << std::endl;
        return;
    }

    const auto& ports = unit->GetModel()->GetPortsManager().GetAllPorts();
    std::cout << "[Debug] Ports for unit: " << unitName << std::endl;
    for (const auto* port : ports) {
        if (port) {
            std::cout << "  Port: " << port->GetName();
            if (port->GetStream())
                std::cout << ", Connected Stream: " << port->GetStream()->GetName();
            else
                std::cout << ", No Stream Connected";
            std::cout << std::endl;
        }
    }
}

void PyDyssol::DebugStreamData(const std::string& streamName, double time)
{
    const auto* stream = m_flowsheet.GetStream(streamName);
    if (!stream) {
        std::cout << "[Debug] Stream not found: " << streamName << std::endl;
        return;
    }
    std::cout << "[Debug] Stream: " << streamName << " at time " << time << std::endl;
    std::cout << "  Mass: " << stream->GetMassFlow(time) << std::endl;
    std::cout << "  Temp: " << stream->GetTemperature(time) << std::endl;
    std::cout << "  Press: " << stream->GetPressure(time) << std::endl;
}