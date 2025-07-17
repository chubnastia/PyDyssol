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

EPhase GetPhaseByName(const std::string& phaseName)
{
    std::string name = phaseName;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (name == "solid") return EPhase::SOLID;
    if (name == "liquid") return EPhase::LIQUID;
    if (name == "vapor" || name == "gas") return EPhase::VAPOR;

    throw std::runtime_error("Unknown phase name: " + phaseName);
}

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
    fs::path absPath = fs::absolute(path);
    if (!m_materialsDatabase.LoadFromFile(absPath)) {
        std::cerr << "[PyDyssol] Failed to load materials database." << std::endl;
        m_isDatabaseLoaded = false;
        return false;
    }
    if (m_debug) {
        std::cout << "[PyDyssol] Loading materials database: " << path << std::endl;
        std::cout << "[PyDyssol] Materials database loaded. Compounds: " << m_materialsDatabase.GetCompounds().size() << std::endl;
        for (const auto* compound : m_materialsDatabase.GetCompounds()) {
            std::cout << "[PyDyssol] Compound: " << compound->GetName() << " (Key: " << compound->GetKey() << ")" << std::endl;
        }
    }
    m_isDatabaseLoaded = true;
    // Update flowsheet's database reference
    m_flowsheet.SetMaterialsDatabase(&m_materialsDatabase);
    return true;
}

bool PyDyssol::AddModelPath(const std::string& path)
{
    fs::path absPath = fs::absolute(path);
    m_modelsManager.AddDir(absPath);
    auto models = m_modelsManager.GetAvailableUnits();
    if (models.empty()) {
        std::cerr << "[PyDyssol] No models found in path: " << path << std::endl;
        m_isModelsLoaded = false;
        return false;
    }
    if (m_debug) {
        std::cout << "[PyDyssol] Adding model path: " << path << std::endl;
        for (const auto& model : models) {
            std::cout << "[PyDyssol] Found model: " << model.name << " (" << model.uniqueID << ")" << std::endl;
        }
    }
    m_isModelsLoaded = true;
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

std::string GetAllowedDistrNames()
{
    std::ostringstream oss;
    for (size_t i = 0; i < std::size(DISTR_NAMES); ++i)
    {
        if (i > 0) oss << ", ";
        oss << "\"" << DISTR_NAMES[i] << "\"";
    }
    return oss.str();
}

bool IsValidDistributionName(const std::string& name)
{
    for (const auto& validName : DISTR_NAMES)
        if (name == validName)
            return true;
    return false;
}

bool PyDyssol::IsGridValid(const std::map<std::string, py::object>& gridData) const {
    std::string typeStr = py::cast<std::string>(gridData.at("type"));

    if (!IsValidDistributionName(typeStr)) {
        std::cout << "[PyDyssol] Warning: Invalid grid type: '" << typeStr
            << "'. Valid types are: " << GetAllowedDistrNames() << std::endl;
        return false;
    }

    EDistrTypes gridType = StringToDistrType(typeStr);

    //if (gridType == EDistrTypes::DISTR_COMPOUNDS) {
     //   std::cout << "[PyDyssol] Warning: Cannot modify 'Compounds' grid. Use set_compounds()." << std::endl;
    //    return false;
    //}

    py::list rawGrid = gridData.at("grid");
    if (py::len(rawGrid) == 0) {
        std::cout << "[PyDyssol] Warning: Empty grid provided for type: " << typeStr << std::endl;
        return false;
    }

    bool isSymbolic = py::isinstance<py::str>(rawGrid[0]);
    if (isSymbolic) {
        std::vector<std::string> classNames = py::cast<std::vector<std::string>>(rawGrid);
        std::set<std::string> unique(classNames.begin(), classNames.end());
        if (unique.size() != classNames.size()) {
            std::cout << "[PyDyssol] Warning: Symbolic grid entries must be unique for type: " << typeStr << std::endl;
            return false;
        }
    }
    else {
        std::vector<double> classLimits = py::cast<std::vector<double>>(rawGrid);
        for (size_t i = 1; i < classLimits.size(); ++i) {
            if (classLimits[i] <= classLimits[i - 1]) {
                std::cout << "[PyDyssol] Warning: Numeric grid values must be strictly increasing for type: " << typeStr << std::endl;
                return false;
            }
        }
    }

    return true;
}

void print_grid_data(const py::list& data_list) {
    py::print("=== Grids ===");
    for (const auto& grid : data_list) {
        py::dict grid_dict = grid.cast<py::dict>();
        std::string type = grid_dict["type"].cast<std::string>();
        py::list grid_vals = grid_dict["grid"];

        py::print(py::str("Type: {}").format(type));
        std::ostringstream oss;
        oss << "  [";
        for (size_t j = 0; j < py::len(grid_vals); ++j) {
            if (j > 0) oss << ", ";
            py::object val = grid_vals[j];
            if (val.is_none()) {
                oss << "null";
            }
            else if (py::isinstance<py::float_>(val) || py::isinstance<py::int_>(val)) {
                oss << py::str("{:.4e}").format(val.cast<double>());
            }
            else {
                oss << py::str(val).cast<std::string>();
            }
        }
        oss << "]";
        py::print(oss.str());
    }
}

void print_unit_params(const py::dict& data_dict) {
    py::print("=== Unit Parameters ===");
    for (const auto& item : data_dict) {
        std::string key = item.first.cast<std::string>();
        py::tuple tup = item.second.cast<py::tuple>();
        py::object val = tup[0];
        std::string type = tup[1].cast<std::string>();
        std::string unit = tup[2].cast<std::string>().empty() ? "-" : tup[2].cast<std::string>();

        std::string repr;
        if (py::isinstance<py::float_>(val)) {
            repr = std::to_string(val.cast<double>());
        }
        else if (py::isinstance<py::int_>(val)) {
            repr = std::to_string(val.cast<int>());
        }
        else if (py::isinstance<py::bool_>(val)) {
            repr = val.cast<bool>() ? "True" : "False";
        }
        else if (py::isinstance<py::str>(val)) {
            repr = val.cast<std::string>();
        }
        else if (py::isinstance<py::list>(val) || py::isinstance<py::tuple>(val)) {
            py::sequence seq = val.cast<py::sequence>();
            size_t len = seq.size();
            if (len <= 5) {
                std::ostringstream oss;
                oss << "[";
                for (size_t i = 0; i < len; ++i) {
                    if (i > 0) oss << ", ";
                    oss << py::str(seq[i]).cast<std::string>();
                }
                oss << "]";
                repr = oss.str();
            }
            else {
                repr = "<list, length=" + std::to_string(len) + ">";
            }
        }
        else {
            repr = py::str(val).cast<std::string>();
        }

        py::print(py::str("{:<25} : {:<30} [{:<15}] ({})").format(key, repr, type, unit));
    }
}

void print_holdup_data(const py::dict& data_dict) {
    static const std::unordered_map<std::string, std::string> units = {
        {"mass", "kg"}, {"massflow", "kg/s"}, {"temperature", "K"}, {"pressure", "Pa"}
    };

    py::print("=== Overall ===");
    for (const auto& item : data_dict["overall"].cast<py::dict>()) {
        std::string key = item.first.cast<std::string>();
        double val = item.second.cast<double>();
        std::string unit = units.count(key) ? units.at(key) : "";
        py::print(py::str("{:<25}: {:.4f} {}").format(key, val, unit));
    }

    std::string comp_unit = data_dict["overall"].cast<py::dict>().contains("massflow") ? "kg/s" : "kg";
    py::print("\n=== Composition ===");
    for (const auto& item : data_dict["composition"].cast<py::dict>()) {
        std::string key = item.first.cast<std::string>();
        double val = item.second.cast<double>();
        py::print(py::str("{:<25}: {:.4f} {}").format(key, val, comp_unit));
    }

    py::print("\n=== Distributions ===");
    for (const auto& item : data_dict["distributions"].cast<py::dict>()) {
        std::string name = item.first.cast<std::string>();
        std::vector<double> dist = item.second.cast<std::vector<double>>();
        py::print("\n" + name + ":");
        for (double x : dist) {
            py::print(py::str("{:.4e}").format(x));
        }
    }
}

void print_options(const py::dict& data_dict) {
    py::print("=== Simulation Options ===");
    for (const auto& item : data_dict) {
        std::string key = item.first.cast<std::string>();
        if (py::isinstance<py::bool_>(item.second)) {
            py::print(py::str("{:<25}: {}").format(key, item.second.cast<bool>() ? "True" : "False"));
        }
        else if (py::isinstance<py::int_>(item.second)) {
            py::print(py::str("{:<25}: {}").format(key, item.second.cast<int>()));
        }
        else if (py::isinstance<py::float_>(item.second)) {
            py::print(py::str("{:<25}: {}").format(key, item.second.cast<double>()));
        }
        else if (py::isinstance<py::str>(item.second)) {
            py::print(py::str("{:<25}: {}").format(key, item.second.cast<std::string>()));
        }
        else {
            py::print(py::str("{:<25}: [unhandled type]").format(key));
        }
    }
}


