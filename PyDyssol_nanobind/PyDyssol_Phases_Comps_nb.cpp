#include "PyDyssol_nb.h"
#include <iostream>
#include <stdexcept>
#include <nanobind/nanobind.h> // Include nanobind headers
#include <nanobind/stl/string.h> // For std::string bindings
#include <nanobind/stl/vector.h> // For std::vector bindings
#include <nanobind/stl/pair.h>   // For std::pair bindings
#include <nanobind/stl/map.h>    // For std::map bindings
#include <nanobind/stl/tuple.h>  // For std::tuple bindings

namespace nb = nanobind;

//Compounds
std::vector<std::pair<std::string, std::string>> PyDyssol::GetDatabaseCompounds() const {
    std::vector<std::pair<std::string, std::string>> out;
    for (const auto* c : m_materialsDatabase.GetCompounds())
        out.emplace_back(c->GetName(), c->GetKey());
    return out;
}

std::vector<std::pair<std::string, std::string>> PyDyssol::GetCompounds() const
{
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& key : m_flowsheet.GetCompounds())
    {
        const auto* compound = m_materialsDatabase.GetCompound(key);
        if (compound)
            result.emplace_back(compound->GetName(), compound->GetKey());
        else
            result.emplace_back(key, "[unknown]");
    }
    return result;
}

bool PyDyssol::SetCompounds(const std::vector<std::string>& compounds)
{
    std::vector<std::string> validKeys;
    for (const auto& comp : compounds)
    {
        const CCompound* c = m_materialsDatabase.GetCompound(comp);
        if (!c)
            c = m_materialsDatabase.GetCompoundByName(comp);
        if (!c)
        {
            std::cerr << "[PyDyssol] Compound not found: " << comp << std::endl;
            return false;
        }
        validKeys.push_back(c->GetKey());
    }
    m_flowsheet.SetCompounds(validKeys);
    return true;
}

bool PyDyssol::AddCompound(const std::string& compoundKeyOrName)
{
    const CCompound* comp = m_materialsDatabase.GetCompound(compoundKeyOrName);
    if (!comp)
        comp = m_materialsDatabase.GetCompoundByName(compoundKeyOrName);

    if (!comp) {
        std::cerr << "[PyDyssol] Compound not found: " << compoundKeyOrName << std::endl;
        return false;
    }

    m_flowsheet.AddCompound(comp->GetKey());
    return true;
}

//Phases
nb::list PyDyssol::GetPhases() const
{
    nb::list result;
    const auto& phases = m_flowsheet.GetPhases();
    for (const auto& p : phases)
        result.append(nb::make_tuple(p.name, p.state));
    return result;
}

bool PyDyssol::SetPhases(const nb::list& phaseStates)
{
    std::vector<SPhaseDescriptor> descriptors;
    for (const auto& stateObj : phaseStates)
    {
        EPhase state = ConvertPhaseState(nb::cast<nb::object>(stateObj));
        std::string name = PhaseToString(state);
        descriptors.push_back({ state, name });
    }
    m_flowsheet.SetPhases(descriptors);
    return true;
}

bool PyDyssol::AddPhase(const nb::object& stateObj)
{
    EPhase state = ConvertPhaseState(nb::cast<nb::object>(stateObj));
    std::string name = PhaseToString(state);
    m_flowsheet.AddPhase(state, name);
    return true;
}
