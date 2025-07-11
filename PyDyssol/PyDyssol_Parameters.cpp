#include "PyDyssol.h"
#include <iostream>
#include <stdexcept>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

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

    std::cout << "[PyDyssol] Active parameters colected for unit " << unitName << ":" << std::endl;

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

    std::cout << "[PyDyssol] All parameters collected for unit " << unitName << ":" << std::endl;

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

    //std::string error = m_flowsheet.Initialize();
   // if (!error.empty()) {
    //    std::cerr << "[PyDyssol] Flowsheet initialization failed after setting " << paramName << ": " << error << std::endl;
   // }
    //else {
     //   std::cout << "[PyDyssol] Called m_flowsheet.Initialize() after setting " << paramName << " for unit " << unitName << std::endl;
    //}
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