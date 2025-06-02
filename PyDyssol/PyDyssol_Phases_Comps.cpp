#include "PyDyssol.h"
#include <iostream>
#include <stdexcept>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace fs = std::filesystem;
namespace py = pybind11;

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
py::list PyDyssol::GetPhases() const
{
    py::list result;
    const auto& phases = m_flowsheet.GetPhases();
    for (const auto& p : phases)
        result.append(py::make_tuple(p.name, p.state));
    return result;
}

bool PyDyssol::SetPhases(const py::list& phaseStates)
{
    std::vector<SPhaseDescriptor> descriptors;
    for (const auto& stateObj : phaseStates)
    {
        EPhase state = ConvertPhaseState(py::reinterpret_borrow<py::object>(stateObj));
        std::string name = PhaseToString(state);
        descriptors.push_back({ state, name });
    }
    m_flowsheet.SetPhases(descriptors);
    return true;
}

bool PyDyssol::AddPhase(const py::object& stateObj)
{
    EPhase state = ConvertPhaseState(py::reinterpret_borrow<py::object>(stateObj));
    std::string name = PhaseToString(state);
    m_flowsheet.AddPhase(state, name);
    return true;
}
