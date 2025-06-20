#include "PyDyssol.h"
#include <iostream>
#include <stdexcept>
#include <map>
#include <string>
#include <vector>
#include <cmath>  
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace fs = std::filesystem;
namespace py = pybind11;

std::string PhaseToString(EPhase phase)
{
    switch (phase)
    {
    case EPhase::SOLID: return "solid";
    case EPhase::LIQUID: return "liquid";
    case EPhase::VAPOR: return "vapor";
    default: return "unknown";
    }
}

EOverall StringToEOverall(const std::string& name)
{
    if (name == "mass")        return EOverall::OVERALL_MASS;
    if (name == "temperature") return EOverall::OVERALL_TEMPERATURE;
    if (name == "pressure")    return EOverall::OVERALL_PRESSURE;
    throw std::runtime_error("Unknown overall property: " + name);
}

std::string FormatDouble(double value)
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

EPhase PyDyssol::GetPhaseByName(const std::string& phaseName) const
{
    std::string name = phaseName;
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);

    if (name == "SOLID") return EPhase::SOLID;
    if (name == "LIQUID") return EPhase::LIQUID;
    if (name == "VAPOR" || name == "GAS") return EPhase::VAPOR;

    throw std::runtime_error("Unknown phase name: " + phaseName);
}

EPhase ConvertPhaseState(const py::object& state)
{
    if (py::isinstance<py::str>(state)) {
        std::string val = state.cast<std::string>();
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);

        if (val == "solid") return EPhase::SOLID;
        if (val == "liquid") return EPhase::LIQUID;
        if (val == "vapor" || val == "gas") return EPhase::VAPOR;

        throw std::invalid_argument("Invalid phase name: '" + val + "'. Expected one of: 'solid', 'liquid', 'vapor' or 'gas'.");
    }

    if (py::isinstance<EPhase>(state)) {
        return py::cast<EPhase>(state);
    }

    throw std::invalid_argument("Invalid phase value. Please enter a valid phase name: 'solid', 'liquid', 'vapor' or 'gas'.");
}

//Grids
// Convert EDistrTypes to std::string using DISTR_NAMES
std::string ToString(EDistrTypes type) {
    for (size_t i = 0; i < std::size(DISTR_TYPES); ++i) {
        if (DISTR_TYPES[i] == type)
            return DISTR_NAMES[i];
    }
    return "Unknown";
}

// Convert std::string to EDistrTypes using DISTR_NAMES
EDistrTypes StringToDistrType(const std::string& name) {
    for (size_t i = 0; i < std::size(DISTR_NAMES); ++i) {
        if (name == DISTR_NAMES[i])
            return DISTR_TYPES[i];
    }
    throw std::invalid_argument("Unknown distribution type: " + name);
}

