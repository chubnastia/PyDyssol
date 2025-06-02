#include "PyDyssol.h"
#include <iostream>
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
#include <Flowsheet.h>

namespace fs = std::filesystem;
namespace py = pybind11; // Add namespace alias

PyDyssol::PyDyssol(const std::string& materialsPath, const std::string& modelsPath)
    : m_flowsheet(&m_modelsManager, &m_materialsDatabase),
    m_defaultMaterialsPath(materialsPath),
    m_defaultModelsPath(modelsPath),
    m_isInitialized(false)
{
    m_simulator.SetFlowsheet(&m_flowsheet);

    // Automatically load the default materials database and model path
    LoadMaterialsDatabase(m_defaultMaterialsPath);
    AddModelPath(m_defaultModelsPath);
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
    std::cout << "[PyDyssol] Flowsheet loaded successfully." << std::endl;

    // Post-load check
    std::cout << "[PyDyssol] Post-load check - Units: " << m_flowsheet.GetAllUnits().size()
        << ", Streams: " << m_flowsheet.GetAllStreams().size() << std::endl;
    DebugFlowsheet();
	Initialize();
    return true;
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
        handler.Create(filePath); // Explicitly create the file
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
    std::cout << "[PyDyssol] Initializing flowsheet..." << std::endl;
    std::string error = m_flowsheet.Initialize();
    if (!error.empty()) {
        std::cerr << "[PyDyssol] Initialization failed: " << error << std::endl;
    }
    else {
        std::cout << "[PyDyssol] Flowsheet initialized successfully." << std::endl;
        m_isInitialized = true; // Set the flag on successful initialization
    }
    return error;
}

void PyDyssol::Simulate(double endTime)
{
    if (!m_isInitialized) {
        std::string error = Initialize();
        if (!error.empty())
            throw std::runtime_error("Flowsheet initialization failed: " + error);
        m_isInitialized = true;
    }

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