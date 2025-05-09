#include "PyDyssol.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <chrono>
#include <iomanip>    // for std::setprecision
#include <sstream>    // for std::ostringstream
#include <pybind11/pybind11.h> // Add pybind11 headers
#include <pybind11/stl.h>      // For std::vector and std::pair bindings
#include <pybind11/stl_bind.h>      // For STL bindings with containers
#include "../SimulatorCore/Flowsheet.h"
#include "../SimulatorCore/Simulator.h"
#include "MaterialsDatabase.h"
#include "BaseUnit.h"
#include "../SimulatorCore/ModelsManager.h"
#include "../HDF5Handler/H5Handler.h"
#include "UnitParameters.h"
#include "StringFunctions.h"
#include <windows.h>
#include <SaveLoadManager.h>
#include "UnitPorts.h"
#include "Holdup.h"
#include "DyssolUtilities.h"
#include "DyssolDefines.h"
#include "MultidimensionalGrid.h"

inline std::string FormatDouble(double value)
{
    std::ostringstream stream;
    // Use scientific notation for very small or very large numbers
    if (std::abs(value) > 0 && (std::abs(value) < 1e-6 || std::abs(value) > 1e6)) {
        stream << std::scientific << std::setprecision(4) << value;
    }
    else {
        stream << std::fixed << std::setprecision(4) << value;
        // Remove trailing zeros and decimal point if necessary
        std::string str = stream.str();
        if (str.find('.') != std::string::npos) {
            while (str.back() == '0') str.pop_back();
            if (str.back() == '.') str.pop_back();
        }
        return str;
    }

    std::string str = stream.str();
    // Clean up scientific notation (e.g., "1.0000e-11" -> "1e-11")
    if (str.find('e') != std::string::npos) {
        size_t e_pos = str.find('e');
        std::string mantissa = str.substr(0, e_pos);
        std::string exponent = str.substr(e_pos);

        // Remove trailing zeros in the mantissa
        if (mantissa.find('.') != std::string::npos) {
            while (mantissa.back() == '0') mantissa.pop_back();
            if (mantissa.back() == '.') mantissa.pop_back();
        }

        str = mantissa + exponent;
    }

    return str;
}

inline std::string ConvertWStringToString(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();

    // Determine the required buffer size for the converted string
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    if (size_needed == 0) {
        throw std::runtime_error("Failed to convert wstring to string: WideCharToMultiByte failed");
    }

    // Allocate a buffer for the converted string
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &str[0], size_needed, nullptr, nullptr);
    return str;
}

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

bool PyDyssol::LoadMaterialsDatabase(const std::string& path)
{
    std::cout << "[PyDyssol] Loading materials database: " << path << std::endl;
    fs::path absPath = fs::absolute(path);
    if (!m_materialsDatabase.LoadFromFile(absPath)) {
        std::cerr << "[PyDyssol] Failed to load materials database." << std::endl;
        return false;
    }
    std::cout << "[PyDyssol] Materials database loaded. Compounds: " << m_materialsDatabase.GetCompounds().size() << std::endl;
    for (const auto* compound : m_materialsDatabase.GetCompounds()) {
        std::cout << "[PyDyssol] Compound: " << compound->GetName() << " (Key: " << compound->GetKey() << ")" << std::endl;
    }
    return true;
}

bool PyDyssol::AddModelPath(const std::string& path)
{
    std::cout << "[PyDyssol] Adding model path: " << path << std::endl;
    fs::path absPath = fs::absolute(path);
    m_modelsManager.AddDir(absPath);
    for (const auto& model : m_modelsManager.GetAvailableUnits()) {
        std::cout << "[PyDyssol] Found model: " << model.name << " (" << model.uniqueID << ")" << std::endl;
    }
    return true;
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

std::string PyDyssol::GetModelNameForUnit(const std::string& unitKey) const
{
    for (const auto& unit : m_flowsheet.GetAllUnits())
    {
        if (unit->GetKey() == unitKey)
        {
            const CBaseUnit* model = unit->GetModel();  // access model
            if (!model) return "Unknown";
            const std::string& modelID = model->GetUniqueID();  // model type ID
            for (const auto& registered : m_modelsManager.GetAvailableUnits())
            {
                if (registered.uniqueID == modelID)
                    return registered.name;
            }
        }
    }
    return "Unknown";
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

std::vector<std::pair<std::string, std::string>> PyDyssol::GetDatabaseCompounds() const {
    std::vector<std::pair<std::string, std::string>> out;
    for (const auto* c : m_materialsDatabase.GetCompounds())
        out.emplace_back(c->GetKey(), c->GetName());
    return out;
}

std::vector<std::pair<std::string, std::string>> PyDyssol::GetCompounds() const {
    std::vector<std::pair<std::string, std::string>> out;
    for (const auto& key : m_flowsheet.GetCompounds()) {
        const CCompound* compound = m_materialsDatabase.GetCompound(key);
        if (compound)
            out.emplace_back(compound->GetKey(), compound->GetName());
    }
    return out;
}

bool PyDyssol::SetCompounds(const std::vector<std::string>& compounds)
{
    std::cout << "[PyDyssol] Setting up compounds..." << std::endl;
    std::vector<std::string> keys;
    for (const auto& compound : compounds) {
        auto* comp = m_materialsDatabase.GetCompound(compound);
        if (!comp) {
            comp = m_materialsDatabase.GetCompoundByName(compound);
        }
        if (!comp) {
            std::cerr << "[PyDyssol] Compound not found: " << compound << std::endl;
            return false;
        }
        keys.push_back(comp->GetKey());
        std::cout << "[PyDyssol] Added compound: " << compound << " (Key: " << comp->GetKey() << ")" << std::endl;
    }
    m_flowsheet.SetCompounds(keys);
    return true;
}

bool PyDyssol::SetupPhases(const std::vector<std::pair<std::string, int>>& phases)
{
    std::cout << "[PyDyssol] Setting up phases..." << std::endl;
    std::vector<SPhaseDescriptor> phaseDescriptors;
    for (const auto& phase : phases) {
        phaseDescriptors.push_back(SPhaseDescriptor{ static_cast<EPhase>(phase.second), phase.first });
        std::cout << "[PyDyssol] Added phase: " << phase.first << " (State: " << phase.second << ")" << std::endl;
    }
    m_flowsheet.SetPhases(phaseDescriptors);
    return true;
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

std::vector<std::pair<std::string, double>> PyDyssol::GetStreamMassFlow(const std::string& streamKey, double time)
{
    std::vector<std::pair<std::string, double>> result;
    CMaterialStream* stream = m_flowsheet.GetStreamByName(streamKey);
    if (!stream) {
        std::cerr << "[PyDyssol] Stream not found: " << streamKey << std::endl;
        return result;
    }

    // Get overall mass flow for each phase
    for (const auto& phase : m_flowsheet.GetPhases()) {
        double massFlow = stream->GetMassFlow(time);
        result.emplace_back(phase.name, massFlow);
        std::cout << "[PyDyssol] Stream " << streamKey << " at time " << time
            << ": Mass flow (" << phase.name << ") = " << massFlow << std::endl;
    }

    return result;
}

std::vector<std::pair<std::string, double>> PyDyssol::GetStreamComposition(const std::string& streamKey, double time)
{
    std::vector<std::pair<std::string, double>> result;
    CMaterialStream* stream = m_flowsheet.GetStreamByName(streamKey);
    if (!stream) {
        std::cerr << "[PyDyssol] Stream not found: " << streamKey << std::endl;
        return result;
    }

    // Get mass fraction for each compound
    for (const auto& compoundKey : m_flowsheet.GetCompounds()) {
        double massFraction = stream->GetCompoundFraction(time, compoundKey);
        const CCompound* compound = m_materialsDatabase.GetCompound(compoundKey);
        std::string compoundName = compound ? compound->GetName() : compoundKey;
        result.emplace_back(compoundName, massFraction);
        std::cout << "[PyDyssol] Stream " << streamKey << " at time " << time
            << ": Mass fraction (" << compoundName << ") = " << massFraction << std::endl;
    }

    return result;
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
pybind11::object GetNativeUnitParameter(const CBaseUnitParameter* param, const CMaterialsDatabase& db)
{
    namespace py = pybind11;

    switch (param->GetType())
    {
    case EUnitParameter::CONSTANT:
    case EUnitParameter::CONSTANT_DOUBLE:
        if (const auto* cast = dynamic_cast<const CConstRealUnitParameter*>(param))
            return py::float_(cast->GetValue());
        break;
    case EUnitParameter::CONSTANT_INT64:
        if (const auto* cast = dynamic_cast<const CConstIntUnitParameter*>(param))
            return py::int_(cast->GetValue());
        break;
    case EUnitParameter::CONSTANT_UINT64:
        if (const auto* cast = dynamic_cast<const CConstUIntUnitParameter*>(param))
            return py::int_(cast->GetValue());
        break;
    case EUnitParameter::CHECKBOX:
        if (const auto* cast = dynamic_cast<const CCheckBoxUnitParameter*>(param))
            return py::bool_(cast->GetValue());
        break;
    case EUnitParameter::STRING:
        if (const auto* cast = dynamic_cast<const CStringUnitParameter*>(param))
            return py::str(cast->GetValue());
        break;
    case EUnitParameter::COMBO:
    case EUnitParameter::SOLVER:
    case EUnitParameter::GROUP:
        if (const auto* cast = dynamic_cast<const CComboUnitParameter*>(param))
            return py::str(cast->GetNameByItem(cast->GetValue()));
        break;
    case EUnitParameter::LIST_DOUBLE:
        if (const auto* cast = dynamic_cast<const CListUnitParameter<double>*>(param))
            return py::cast(cast->GetValues());
        break;
    case EUnitParameter::LIST_INT64:
        if (const auto* cast = dynamic_cast<const CListUnitParameter<int64_t>*>(param))
            return py::cast(cast->GetValues());
        break;
    case EUnitParameter::LIST_UINT64:
        if (const auto* cast = dynamic_cast<const CListUnitParameter<uint64_t>*>(param))
            return py::cast(cast->GetValues());
        break;
    case EUnitParameter::COMPOUND:
    case EUnitParameter::MDB_COMPOUND:
        if (const auto* cast = dynamic_cast<const CCompoundUnitParameter*>(param)) {
            const auto* comp = db.GetCompound(cast->GetCompound());
            return py::str(comp ? comp->GetName() : cast->GetCompound());
        }
        if (const auto* cast = dynamic_cast<const CMDBCompoundUnitParameter*>(param)) {
            const auto* comp = db.GetCompound(cast->GetCompound());
            return py::str(comp ? comp->GetName() : cast->GetCompound());
        }
        break;
    case EUnitParameter::REACTION:
        // Could return a list of dicts later, for now just "[reaction list]"
        return py::str("[Reaction object]");
    case EUnitParameter::TIME_DEPENDENT:
    case EUnitParameter::PARAM_DEPENDENT:
        if (const auto* dep = dynamic_cast<const CDependentUnitParameter*>(param))
            return py::cast(dep->GetParamValuePairs());
        break;
    default:
        break;
    }
    throw std::runtime_error("Unsupported or unknown parameter type");
}

// Helper function to get the type string of a parameter (used for return value only)
std::string GetParameterTypeString(EUnitParameter type) {
    switch (type) {
    case EUnitParameter::CONSTANT:
    case EUnitParameter::CONSTANT_DOUBLE:
        return "CONSTANT_DOUBLE";
    case EUnitParameter::CONSTANT_INT64:
        return "CONSTANT_INT64";
    case EUnitParameter::CONSTANT_UINT64:
        return "CONSTANT_UINT64";
    case EUnitParameter::STRING:
        return "STRING";
    case EUnitParameter::CHECKBOX:
        return "CHECKBOX";
    case EUnitParameter::COMBO:
        return "COMBO";
    case EUnitParameter::SOLVER:
        return "SOLVER";
    case EUnitParameter::GROUP:
        return "GROUP";
    case EUnitParameter::COMPOUND:
        return "COMPOUND";
    case EUnitParameter::MDB_COMPOUND:
        return "MDB_COMPOUND";
    case EUnitParameter::REACTION:
        return "REACTION";
    case EUnitParameter::LIST_DOUBLE:
        return "LIST_DOUBLE";
    case EUnitParameter::LIST_INT64:
        return "LIST_INT64";
    case EUnitParameter::LIST_UINT64:
        return "LIST_UINT64";
    case EUnitParameter::TIME_DEPENDENT:
        return "TIME_DEPENDENT";
    case EUnitParameter::PARAM_DEPENDENT:
        return "PARAM_DEPENDENT";
    case EUnitParameter::UNKNOWN:
    default:
        return "UNKNOWN";
    }
} 

std::map<std::string, std::tuple<pybind11::object, std::string, std::string>> PyDyssol::GetUnitParameters(const std::string& unitName) const
{
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto* model = unit->GetModel();
    if (!model)
        throw std::runtime_error("Model not found for unit: " + unitName);

    const auto& mgr = model->GetUnitParametersManager();
    auto activeParams = mgr.GetActiveParameters();

    std::cout << "[PyDyssol] Active parameters for unit " << unitName << ":" << std::endl;

    std::map<std::string, std::tuple<pybind11::object, std::string, std::string>> result;
    for (const auto* param : activeParams) {
        std::string name = param->GetName();
        std::string unitsStr = ConvertWStringToString(param->GetUnits());
        std::string typeStr = GetParameterTypeString(param->GetType());

        // Get the native value
        pybind11::object value = GetNativeUnitParameter(param, m_materialsDatabase);
        // Store the tuple (value, type, units)
        result[name] = std::make_tuple(value, typeStr, unitsStr);
    }
    return result;
}

std::map<std::string, std::tuple<pybind11::object, std::string, std::string>> PyDyssol::GetUnitParametersAll(const std::string& unitName) const
{
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto* model = unit->GetModel();
    if (!model)
        throw std::runtime_error("Model not found for unit: " + unitName);

    const auto& mgr = model->GetUnitParametersManager();
    auto allParams = mgr.GetParameters();

    std::cout << "[PyDyssol] All parameters for unit " << unitName << ":" << std::endl;

    std::map<std::string, std::tuple<pybind11::object, std::string, std::string>> result;
    for (const auto* param : allParams) {
        std::string name = param->GetName();
        std::string unitsStr = ConvertWStringToString(param->GetUnits());
        std::string typeStr = GetParameterTypeString(param->GetType());

        // Get the native value
        pybind11::object value = GetNativeUnitParameter(param, m_materialsDatabase);

        // Store the tuple (value, type, units)
        result[name] = std::make_tuple(value, typeStr, unitsStr);
    }
    return result;
}

void PyDyssol::SetUnitParameter(const std::string& unitName, const std::string& paramName,
    const std::variant<
    bool,  
    double,
    std::string,
    int64_t,
    uint64_t,
    std::vector<double>,
    std::vector<int64_t>,
    std::vector<uint64_t>,
    std::vector<std::pair<double, double>>,
    std::vector<pybind11::dict>
    > value)
{
    auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    auto* model = unit->GetModel();
    if (!model)
        throw std::runtime_error("Model not found for unit: " + unitName);

    auto* param = model->GetUnitParametersManager().GetParameter(paramName);
    if (!param)
        throw std::runtime_error("Parameter not found: " + paramName);

    // Determine the expected parameter type(s) based on the variant type
    if (std::holds_alternative<bool>(value)) {
        // Expected type: CHECKBOX
        if (param->GetType() != EUnitParameter::CHECKBOX)
            throw std::runtime_error("Parameter " + paramName + " expects type CHECKBOX (6), but got type " + std::to_string(static_cast<int>(param->GetType())));

        auto* checkbox = dynamic_cast<CCheckBoxUnitParameter*>(param);
        bool val = std::get<bool>(value);
        checkbox->SetValue(val);
        std::cout << "[PyDyssol] Set " << paramName << " to " << (val ? "true" : "false") << " for unit " << unitName << std::endl;
    }
    else if (std::holds_alternative<double>(value)) {
        // Expected type: CONSTANT_DOUBLE
        if (param->GetType() != EUnitParameter::CONSTANT_DOUBLE)
            throw std::runtime_error("Parameter " + paramName + " expects type CONSTANT_DOUBLE (2), but got type " + std::to_string(static_cast<int>(param->GetType())));

        auto* real = dynamic_cast<CConstRealUnitParameter*>(param);
        real->SetValue(std::get<double>(value));
        std::cout << "[PyDyssol] Set " << paramName << " to " << std::get<double>(value) << " for unit " << unitName << std::endl;
    }
    else if (std::holds_alternative<int64_t>(value) || std::holds_alternative<uint64_t>(value)) {
        // Expected types: CONSTANT_INT64 or CONSTANT_UINT64
        if (param->GetType() == EUnitParameter::CONSTANT_INT64) {
            auto* intParam = dynamic_cast<CConstIntUnitParameter*>(param);
            int64_t val;
            if (std::holds_alternative<int64_t>(value)) {
                val = std::get<int64_t>(value);
            }
            else {
                uint64_t uval = std::get<uint64_t>(value);
                if (uval > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
                    throw std::runtime_error("Parameter " + paramName + " value out of range for int64_t: " + std::to_string(uval));
                val = static_cast<int64_t>(uval);
            }
            intParam->SetValue(val);
            std::cout << "[PyDyssol] Set " << paramName << " to " << val << " for unit " << unitName << std::endl;
        }
        else if (param->GetType() == EUnitParameter::CONSTANT_UINT64) {
            auto* uintParam = dynamic_cast<CConstUIntUnitParameter*>(param);
            uint64_t val;
            if (std::holds_alternative<uint64_t>(value)) {
                val = std::get<uint64_t>(value);
            }
            else {
                int64_t ival = std::get<int64_t>(value);
                if (ival < 0)
                    throw std::runtime_error("Parameter " + paramName + " expects an unsigned integer, but got a negative value: " + std::to_string(ival));
                val = static_cast<uint64_t>(ival);
            }
            uintParam->SetValue(val);
            std::cout << "[PyDyssol] Set " << paramName << " to " << val << " for unit " << unitName << std::endl;
        }
        else {
            throw std::runtime_error("Parameter " + paramName + " expects type CONSTANT_INT64 (3) or CONSTANT_UINT64 (4), but got type " + std::to_string(static_cast<int>(param->GetType())));
        }
    }
    else if (std::holds_alternative<std::string>(value)) {
        // Expected types: STRING, COMPOUND, MDB_COMPOUND, or COMBO/SOLVER/GROUP
        std::string val = std::get<std::string>(value);
        if (param->GetType() == EUnitParameter::STRING) {
            auto* strParam = dynamic_cast<CStringUnitParameter*>(param);
            strParam->SetValue(val);
            std::cout << "[PyDyssol] Set " << paramName << " to " << val << " for unit " << unitName << std::endl;
        }
        else if (param->GetType() == EUnitParameter::COMPOUND) {
            auto* compound = dynamic_cast<CCompoundUnitParameter*>(param);
            const CCompound* found = m_materialsDatabase.GetCompound(val);
            if (!found)
                found = m_materialsDatabase.GetCompoundByName(val);
            if (!found)
                throw std::runtime_error("Compound '" + val + "' not found in the materials database.");
            compound->SetCompound(found->GetKey());
            std::cout << "[PyDyssol] Set " << paramName << " to compound name " << found->GetName()
                << " (Key: " << found->GetKey() << ") for unit " << unitName << std::endl;
        }
        else if (param->GetType() == EUnitParameter::MDB_COMPOUND) {
            auto* mdbCompound = dynamic_cast<CMDBCompoundUnitParameter*>(param);
            const CCompound* found = m_materialsDatabase.GetCompound(val);
            if (!found)
                found = m_materialsDatabase.GetCompoundByName(val);
            if (!found)
                throw std::runtime_error("Compound '" + val + "' not found in the materials database.");
            mdbCompound->SetCompound(found->GetKey());
            std::cout << "[PyDyssol] Set " << paramName << " to MDB compound name " << found->GetName()
                << " (Key: " << found->GetKey() << ") for unit " << unitName << std::endl;
        }
        else if (param->GetType() == EUnitParameter::COMBO || param->GetType() == EUnitParameter::SOLVER || param->GetType() == EUnitParameter::GROUP) {
            auto* combo = dynamic_cast<CComboUnitParameter*>(param);
            size_t index = combo->GetItemByName(val);
            combo->SetValue(index);
            std::cout << "[PyDyssol] Set " << paramName << " to " << val << " for unit " << unitName << std::endl;
        }
        else if (param->GetType() == EUnitParameter::LIST_INT64) {
            // Parse comma-separated string for LIST_INT64 (e.g., "1,2,3")
            auto* listInt = dynamic_cast<CListUnitParameter<int64_t>*>(param);
            std::vector<int64_t> values;
            std::stringstream ss(val);
            std::string item;
            while (std::getline(ss, item, ',')) {
                item.erase(0, item.find_first_not_of(" \t"));
                item.erase(item.find_last_not_of(" \t") + 1);
                try {
                    size_t pos;
                    int64_t v = std::stoll(item, &pos);
                    if (pos != item.length()) {
                        throw std::runtime_error("Invalid integer in list: " + item);
                    }
                    values.push_back(v);
                }
                catch (const std::exception& e) {
                    throw std::runtime_error("Failed to parse integer from string: " + item + " (" + e.what() + ")");
                }
            }
            if (values.empty()) {
                throw std::runtime_error("Parameter " + paramName + " received an empty list of integers");
            }
            listInt->SetValues(values);
            std::cout << "[PyDyssol] Set " << paramName << " to parsed list of int64_t: " << val << " for unit " << unitName << std::endl;
        }
        else if (param->GetType() == EUnitParameter::LIST_UINT64) {
            // Parse comma-separated string for LIST_UINT64 (e.g., "1,2,3")
            auto* listUInt = dynamic_cast<CListUnitParameter<uint64_t>*>(param);
            std::vector<uint64_t> values;
            std::stringstream ss(val);
            std::string item;
            while (std::getline(ss, item, ',')) {
                item.erase(0, item.find_first_not_of(" \t"));
                item.erase(item.find_last_not_of(" \t") + 1);
                try {
                    size_t pos;
                    auto v = std::stoull(item, &pos);
                    if (pos != item.length()) {
                        throw std::runtime_error("Invalid unsigned integer in list: " + item);
                    }
                    values.push_back(v);
                }
                catch (const std::exception& e) {
                    throw std::runtime_error("Failed to parse unsigned integer from string: " + item + " (" + e.what() + ")");
                }
            }
            if (values.empty()) {
                throw std::runtime_error("Parameter " + paramName + " received an empty list of unsigned integers");
            }
            listUInt->SetValues(values);
            std::cout << "[PyDyssol] Set " << paramName << " to parsed list of uint64_t: " << val << " for unit " << unitName << std::endl;
        }
        else {
            throw std::runtime_error("Parameter " + paramName + " expects type STRING (5), COMPOUND (10), MDB_COMPOUND (11), COMBO/SOLVER/GROUP (7/8/9), LIST_INT64 (14), or LIST_UINT64 (15), but got type " + std::to_string(static_cast<int>(param->GetType())));
        }
    }
    else if (std::holds_alternative<std::vector<double>>(value)) {
        // Expected types: LIST_DOUBLE, LIST_INT64, or LIST_UINT64
        if (param->GetType() == EUnitParameter::LIST_DOUBLE) {
            auto* listDouble = dynamic_cast<CListUnitParameter<double>*>(param);
            listDouble->SetValues(std::get<std::vector<double>>(value));
            std::cout << "[PyDyssol] Set " << paramName << " to list of doubles for unit " << unitName << std::endl;
        }
        else if (param->GetType() == EUnitParameter::LIST_INT64) {
            auto* listInt = dynamic_cast<CListUnitParameter<int64_t>*>(param);
            const auto& doubleValues = std::get<std::vector<double>>(value);
            std::vector<int64_t> intValues;
            intValues.reserve(doubleValues.size());
            for (const auto& val : doubleValues) {
                if (std::floor(val) != val) {
                    throw std::runtime_error("Parameter " + paramName + " expects a list of integers, but got a non-integer value: " + std::to_string(val));
                }
                if (val > static_cast<double>(std::numeric_limits<int64_t>::max()) ||
                    val < static_cast<double>(std::numeric_limits<int64_t>::min())) {
                    throw std::runtime_error("Parameter " + paramName + " value out of range for int64_t: " + std::to_string(val));
                }
                intValues.push_back(static_cast<int64_t>(val));
            }
            if (intValues.empty()) {
                throw std::runtime_error("Parameter " + paramName + " received an empty list of integers");
            }
            listInt->SetValues(intValues);
            std::cout << "[PyDyssol] Converted and set " << paramName << " to list of int64_t for unit " << unitName << std::endl;
        }
        else if (param->GetType() == EUnitParameter::LIST_UINT64) {
            auto* listUInt = dynamic_cast<CListUnitParameter<uint64_t>*>(param);
            const auto& doubleValues = std::get<std::vector<double>>(value);
            std::vector<uint64_t> uintValues;
            uintValues.reserve(doubleValues.size());
            for (const auto& val : doubleValues) {
                if (std::floor(val) != val) {
                    throw std::runtime_error("Parameter " + paramName + " expects a list of unsigned integers, but got a non-integer value: " + std::to_string(val));
                }
                if (val < 0) {
                    throw std::runtime_error("Parameter " + paramName + " expects a list of unsigned integers, but got a negative value: " + std::to_string(val));
                }
                if (val > static_cast<double>(std::numeric_limits<uint64_t>::max())) {
                    throw std::runtime_error("Parameter " + paramName + " value out of range for uint64_t: " + std::to_string(val));
                }
                uintValues.push_back(static_cast<uint64_t>(val));
            }
            if (uintValues.empty()) {
                throw std::runtime_error("Parameter " + paramName + " received an empty list of unsigned integers");
            }
            listUInt->SetValues(uintValues);
            std::cout << "[PyDyssol] Converted and set " << paramName << " to list of uint64_t for unit " << unitName << std::endl;
        }
        else {
            throw std::runtime_error("Parameter " + paramName + " expects type LIST_DOUBLE (13), LIST_INT64 (14), or LIST_UINT64 (15), but got type " + std::to_string(static_cast<int>(param->GetType())));
        }
    }
    else if (std::holds_alternative<std::vector<int64_t>>(value)) {
        // Expected types: LIST_INT64 or LIST_UINT64
        if (param->GetType() == EUnitParameter::LIST_INT64) {
            auto* listInt = dynamic_cast<CListUnitParameter<int64_t>*>(param);
            listInt->SetValues(std::get<std::vector<int64_t>>(value));
            std::cout << "[PyDyssol] Set " << paramName << " to list of int64_t for unit " << unitName << std::endl;
        }
        else if (param->GetType() == EUnitParameter::LIST_UINT64) {
            auto* listUInt = dynamic_cast<CListUnitParameter<uint64_t>*>(param);
            const auto& intValues = std::get<std::vector<int64_t>>(value);
            std::vector<uint64_t> uintValues;
            uintValues.reserve(intValues.size());
            for (const auto& val : intValues) {
                if (val < 0) {
                    throw std::runtime_error("Parameter " + paramName + " expects a list of unsigned integers, but got a negative value: " + std::to_string(val));
                }
                uintValues.push_back(static_cast<uint64_t>(val));
            }
            if (uintValues.empty()) {
                throw std::runtime_error("Parameter " + paramName + " received an empty list of unsigned integers");
            }
            listUInt->SetValues(uintValues);
            std::cout << "[PyDyssol] Converted and set " << paramName << " to list of uint64_t for unit " << unitName << std::endl;
        }
        else {
            throw std::runtime_error("Parameter " + paramName + " expects type LIST_INT64 (14) or LIST_UINT64 (15), but got type " + std::to_string(static_cast<int>(param->GetType())));
        }
    }
    else if (std::holds_alternative<std::vector<uint64_t>>(value)) {
        // Expected type: LIST_UINT64
        if (param->GetType() != EUnitParameter::LIST_UINT64)
            throw std::runtime_error("Parameter " + paramName + " expects type LIST_UINT64 (15), but got type " + std::to_string(static_cast<int>(param->GetType())));

        auto* listUInt = dynamic_cast<CListUnitParameter<uint64_t>*>(param);
        listUInt->SetValues(std::get<std::vector<uint64_t>>(value));
        std::cout << "[PyDyssol] Set " << paramName << " to list of uint64_t for unit " << unitName << std::endl;
    }
    else if (std::holds_alternative<std::vector<std::pair<double, double>>>(value)) {
        // Expected types: TIME_DEPENDENT or PARAM_DEPENDENT
        if (param->GetType() != EUnitParameter::TIME_DEPENDENT && param->GetType() != EUnitParameter::PARAM_DEPENDENT)
            throw std::runtime_error("Parameter " + paramName + " expects type TIME_DEPENDENT (16) or PARAM_DEPENDENT (17), but got type " + std::to_string(static_cast<int>(param->GetType())));

        auto* dep = dynamic_cast<CDependentUnitParameter*>(param);
        const auto& pairs = std::get<std::vector<std::pair<double, double>>>(value);
        std::vector<double> indep, depVals;
        for (const auto& [x, y] : pairs) {
            indep.push_back(x);
            depVals.push_back(y);
        }
        dep->SetValues(indep, depVals);
        std::cout << "[PyDyssol] Set " << paramName << " to " << pairs.size()
            << " dependent value pairs for unit " << unitName << std::endl;
    }
    else if (std::holds_alternative<std::vector<pybind11::dict>>(value)) {
        // Expected type: REACTION
        if (param->GetType() != EUnitParameter::REACTION)
            throw std::runtime_error("Parameter " + paramName + " expects type REACTION (12), but got type " + std::to_string(static_cast<int>(param->GetType())));

        auto* reactionParam = dynamic_cast<CReactionUnitParameter*>(param);
        const std::vector<pybind11::dict>& pyReactions = std::get<std::vector<pybind11::dict>>(value);
        std::vector<CChemicalReaction> reactions;

        for (const pybind11::dict& pyRxn : pyReactions) {
            CChemicalReaction reaction;
            reaction.SetName(pyRxn["name"].cast<std::string>());

            std::string baseSubstanceKey = pyRxn["base"].cast<std::string>();
            size_t baseSubstanceIndex = 0;
            bool found = false;
            for (size_t i = 0; i < m_flowsheet.GetCompounds().size(); ++i) {
                if (m_flowsheet.GetCompounds()[i] == baseSubstanceKey) {
                    baseSubstanceIndex = i;
                    found = true;
                    break;
                }
            }
            if (!found) {
                throw std::runtime_error("Base substance '" + baseSubstanceKey + "' not found in flowsheet compounds.");
            }
            reaction.SetBaseSubstance(baseSubstanceIndex);

            const auto pySubstances = pyRxn["substances"].cast<std::vector<pybind11::dict>>();
            for (const pybind11::dict& pySubstance : pySubstances) {
                CChemicalReaction::SChemicalSubstance substance;
                substance.key = pySubstance["key"].cast<std::string>();
                substance.nu = pySubstance["nu"].cast<double>();
                substance.order = pySubstance["order"].cast<double>();
                substance.phase = GetPhaseByName(pySubstance["phase"].cast<std::string>());
                reaction.AddSubstance(substance);
            }

            reactions.push_back(std::move(reaction));
        }

        reactionParam->SetReactions(reactions);
        std::cout << "[PyDyssol] Reaction set successfully for " << paramName << std::endl;
    }
    else {
        throw std::runtime_error("Unsupported value type for parameter " + paramName);
    }

    std::string error = m_flowsheet.Initialize();
    if (!error.empty()) {
        std::cerr << "[PyDyssol] Flowsheet initialization failed after setting " << paramName << ": " << error << std::endl;
    }
    else {
        std::cout << "[PyDyssol] Called m_flowsheet.Initialize() after setting " << paramName << " for unit " << unitName << std::endl;
    }
}

pybind11::object PyDyssol::GetUnitParameter(const std::string& unitName, const std::string& paramName) const
{
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto* model = unit->GetModel();
    if (!model)
        throw std::runtime_error("Model not found for unit: " + unitName);

    const auto* param = model->GetUnitParametersManager().GetParameter(paramName);
    if (!param)
        throw std::runtime_error("Parameter not found: " + paramName);

    return GetNativeUnitParameter(param, m_materialsDatabase);
}

std::vector<std::string> PyDyssol::GetComboOptions(const std::string& unitName, const std::string& paramName) const
{
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto* model = unit->GetModel();
    if (!model)
        throw std::runtime_error("Model not found for unit: " + unitName);

    const auto* param = model->GetUnitParametersManager().GetParameter(paramName);
    if (!param)
        throw std::runtime_error("Parameter not found: " + paramName);

    if (const auto* combo = dynamic_cast<const CComboUnitParameter*>(param)) {
        return combo->GetNames();
    }

    throw std::runtime_error("Parameter is not a combo: " + paramName);
}

std::vector<std::pair<double, double>> PyDyssol::GetDependentParameterValues(const std::string& unitName, const std::string& paramName) const
{
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    const auto* model = unit->GetModel();
    if (!model)
        throw std::runtime_error("Model not found for unit: " + unitName);

    const auto* param = model->GetUnitParametersManager().GetParameter(paramName);
    if (!param)
        throw std::runtime_error("Parameter not found: " + paramName);

    if (const auto* dep = dynamic_cast<const CDependentUnitParameter*>(param)) {
        if (param->GetType() == EUnitParameter::TIME_DEPENDENT) {
            // For TIME_DEPENDENT, the independent values are time points stored in the parameter
            return dep->GetParamValuePairs();
        }
        else if (param->GetType() == EUnitParameter::PARAM_DEPENDENT) {
            // For PARAM_DEPENDENT, the independent variable is another parameter
            const std::string& indepParamName = dep->GetParamName();
            const auto* indepParam = model->GetUnitParametersManager().GetParameter(indepParamName);
            if (!indepParam)
                throw std::runtime_error("Independent parameter not found: " + indepParamName);

            // Get the independent parameter's values
            std::vector<double> indepValues;
            if (const auto* constParam = dynamic_cast<const CConstRealUnitParameter*>(indepParam)) {
                indepValues = { constParam->GetValue() };
            }
            else if (const auto* listParam = dynamic_cast<const CListRealUnitParameter*>(indepParam)) {
                indepValues = listParam->GetValues();
            }
            else if (const auto* depParam = dynamic_cast<const CDependentUnitParameter*>(indepParam)) {
                indepValues = depParam->GetParams();
            }
            else {
                throw std::runtime_error("Independent parameter type not supported: " + indepParamName);
            }

            // Get the dependent values
            const auto& depValues = dep->GetValues();

            // Ensure the sizes match
            if (indepValues.size() != depValues.size()) {
                throw std::runtime_error("Mismatch between independent and dependent value counts for parameter: " + paramName);
            }

            // Pair the independent and dependent values
            std::vector<std::pair<double, double>> result;
            for (size_t i = 0; i < indepValues.size(); ++i) {
                result.emplace_back(indepValues[i], depValues[i]);
            }
            return result;
        }
        else {
            throw std::runtime_error("Parameter is not TIME_DEPENDENT or PARAM_DEPENDENT: " + paramName);
        }
    }

    throw std::runtime_error("Parameter is not a dependent parameter: " + paramName);
}

std::map<std::string, std::vector<std::pair<double, double>>> PyDyssol::GetDependentParameters(const std::string& unitName) const
{
    std::map<std::string, std::vector<std::pair<double, double>>> result;

    // Find the unit
    const auto* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit)
        throw std::runtime_error("Unit not found: " + unitName);

    // Get the unit's model
    const auto* model = unit->GetModel();
    if (!model)
        throw std::runtime_error("Model not found for unit: " + unitName);

    // Get the unit's parameters manager
    const auto& paramsManager = model->GetUnitParametersManager();
    // Get all parameters (assuming GetParameters returns std::vector<CBaseUnitParameter*>)
    const auto parameters = paramsManager.GetParameters();

    // Iterate over all parameters
    for (const auto* param : parameters) {
        if (!param)
            continue; // Skip null parameters

        // Check if the parameter is TIME_DEPENDENT or PARAM_DEPENDENT
        if (param->GetType() == EUnitParameter::TIME_DEPENDENT || param->GetType() == EUnitParameter::PARAM_DEPENDENT) {
            try {
                // Use the existing GetDependentParameterValues to get the values
                auto values = GetDependentParameterValues(unitName, param->GetName());
                result[param->GetName()] = values;
            }
            catch (const std::exception& e) {
                // Log the error but continue processing other parameters
                std::cerr << "Error fetching values for parameter " << param->GetName() << ": " << e.what() << std::endl;
                continue;
            }
        }
    }

    return result;
}

EPhase PyDyssol::GetPhaseByName(const std::string& phaseName) const
{
    std::string name = phaseName;
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);

    if (name == "SOLID") return EPhase::SOLID;
    if (name == "LIQUID") return EPhase::LIQUID;
    if (name == "VAPOR" || name == "GAS") return EPhase::VAPOR;

    throw std::runtime_error("Unknown phase name: " + phaseName);
}

inline std::string PhaseToString(EPhase phase)
{
    switch (phase)
    {
    case EPhase::SOLID: return "solid";
    case EPhase::LIQUID: return "liquid";
    case EPhase::VAPOR: return "vapor";
    default: return "unknown";
    }
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
        int index = GetDistributionTypeIndex(distrType);
        if (index < 0) continue;
        std::string name = DISTR_NAMES[index];

        std::vector<double> combined = holdup->GetDistribution(time, distrType);
        if (!combined.empty())
            result[name.c_str()] = pybind11::cast(combined);
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

inline EOverall StringToEOverall(const std::string& name)
{
    if (name == "mass")        return EOverall::OVERALL_MASS;
    if (name == "temperature") return EOverall::OVERALL_TEMPERATURE;
    if (name == "pressure")    return EOverall::OVERALL_PRESSURE;
    throw std::runtime_error("Unknown overall property: " + name);
}

void PyDyssol::SetUnitHoldup(const std::string& unitName, double time, const pybind11::dict& data)
{
    CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    const auto& holdups = unit->GetModel()->GetStreamsManager().GetHoldups();
    if (holdups.empty()) throw std::runtime_error("No holdups found in unit: " + unitName);
    CHoldup* holdup = holdups.front();

    const auto& compounds = m_flowsheet.GetCompounds();
    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();

    // === Set composition ===
    if (data.contains("composition")) {
        const auto compDict = data["composition"].cast<pybind11::dict>();
        for (const auto& item : compDict) {
            std::string name = item.first.cast<std::string>();
            double value = item.second.cast<double>();
            holdup->SetCompoundMass(time, name, EPhase::SOLID, value);
        }
    }

    // === Set overall properties ===
    if (data.contains("overall")) {
        const auto overallDict = data["overall"].cast<pybind11::dict>();
        for (const auto& item : overallDict) {
            std::string name = item.first.cast<std::string>();
            double value = item.second.cast<double>();
            EOverall type = StringToEOverall(name);  
            holdup->SetOverallProperty(time, type, value);
        }
    }

    // === Create grid name  type map ===
    std::map<std::string, EDistrTypes> nameToType;
    for (const CGridDimension* dim : gridDims) {
        int idx = GetDistributionTypeIndex(dim->DimensionType());
        if (idx < 0) continue;
        nameToType[DISTR_NAMES[idx]] = DISTR_TYPES[idx];
    }

    // === Set distributions ===
    if (data.contains("distributions")) {
        const auto distDict = data["distributions"].cast<pybind11::dict>();
        for (const auto& item : distDict) {
            std::string distrName = item.first.cast<std::string>();
            std::vector<double> values = item.second.cast<std::vector<double>>();
            if (!nameToType.count(distrName))
                throw std::runtime_error("Unknown or unsupported distribution: " + distrName);
            EDistrTypes distrType = nameToType[distrName];

            std::vector<double> normalized = Normalized(values);
            std::vector<double> current = holdup->GetDistribution(time, distrType);
            if (current.size() != normalized.size())
                throw std::runtime_error("Size mismatch in distribution '" + distrName + "' for unit '" + unitName + "'");
            holdup->SetDistribution(time, distrType, normalized);
        }
    }
}

std::vector<std::string> PyDyssol::GetUnitFeeds(const std::string& unitName) const
{
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

std::map<std::string, double> PyDyssol::GetUnitFeedOverall(const std::string& unitName, const std::string& feedName, double time) const {
    std::map<std::string, double> overall;
    const CStream* feed = m_flowsheet.GetUnitByName(unitName)->GetModel()->GetStreamsManager().GetFeed(feedName);
    if (!feed) throw std::runtime_error("Feed not found: " + feedName + " in unit " + unitName);

    overall["mass"] = feed->GetMass(time);
    overall["temperature"] = feed->GetTemperature(time);
    overall["pressure"] = feed->GetPressure(time);
    return overall;
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

pybind11::dict PyDyssol::GetUnitFeedDistribution(const std::string& unitName, const std::string& feedName, double time) const {
    pybind11::dict result;
    const CStream* feed = m_flowsheet.GetUnitByName(unitName)->GetModel()->GetStreamsManager().GetFeed(feedName);
    if (!feed) throw std::runtime_error("Feed not found: " + feedName + " in unit " + unitName);

    const auto& gridDims = m_flowsheet.GetGrid().GetGridDimensions();
    for (const CGridDimension* dim : gridDims) {
        EDistrTypes type = dim->DimensionType();
        int idx = GetDistributionTypeIndex(type);
        if (idx < 0) continue;
        std::vector<double> dist = feed->GetDistribution(time, type);
        if (!dist.empty())
            result[DISTR_NAMES[idx]] = pybind11::cast(dist);
    }

    return result;
}

void PyDyssol::SetUnitFeed(const std::string& unitName, const std::string& feedName, double time, const pybind11::dict& data)
{
    CUnitContainer* unit = m_flowsheet.GetUnitByName(unitName);
    if (!unit) throw std::runtime_error("Unit not found: " + unitName);

    CStream* feed = unit->GetModel()->GetStreamsManager().GetFeed(feedName);
    if (!feed) throw std::runtime_error("Feed not found: " + feedName + " in unit " + unitName);

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
            phaseMassMap[phase] += value;
        }

        // Finalize phase masses
        for (const auto& [phase, total] : phaseMassMap)
            feed->SetPhaseMass(time, phase, total);
    }

    // === Overall ===
    if (data.contains("overall")) {
        const auto overallDict = data["overall"].cast<pybind11::dict>();
        for (const auto& item : overallDict) {
            std::string name = item.first.cast<std::string>();
            double value = item.second.cast<double>();
            feed->SetOverallProperty(time, StringToEOverall(name), value);
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
        }
    }
}

pybind11::dict PyDyssol::GetUnitFeed(const std::string& unitName, const std::string& feedName, double time) const {
    pybind11::dict result;
    result["overall"] = GetUnitFeedOverall(unitName, feedName, time);
    result["composition"] = GetUnitFeedComposition(unitName, feedName, time);
    result["distributions"] = GetUnitFeedDistribution(unitName, feedName, time);
    return result;
}

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