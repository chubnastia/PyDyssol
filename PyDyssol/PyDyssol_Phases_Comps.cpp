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

std::vector<std::string> PyDyssol::GetCompounds() const
{
    std::vector<std::string> result;
    for (const auto& key : m_flowsheet.GetCompounds())
    {
        if (const auto* cmp = m_materialsDatabase.GetCompound(key))
            result.push_back(cmp->GetName());
        else
            result.push_back(key);
    }
    return result;
}

// Helper function to validate flowsheet, streams, and phase grid states
bool PyDyssol::ValidateFlowsheetState() const {

    // Check materials database
    if (m_materialsDatabase.GetCompounds().empty()) {
        std::cerr << "[PyDyssol] Error: Materials database is empty" << std::endl;
        return false;
    }

    // Check grid for DISTR_COMPOUNDS
    if (!m_flowsheet.GetGrid().HasDimension(DISTR_COMPOUNDS)) {
        std::cerr << "[PyDyssol] Error: Grid missing DISTR_COMPOUNDS dimension" << std::endl;
        return false;
    }

    // Check phases
    const auto& phases = m_flowsheet.GetPhases();
    size_t phaseCount = phases.size();
    if (phaseCount == 0) {
        std::cerr << "[PyDyssol] Error: No phases defined in flowsheet" << std::endl;
        return false;
    }
    std::cout << "[PyDyssol] Flowsheet has " << phaseCount << " phases" << std::endl;

    // Check streams
    for (const auto* stream : m_flowsheet.GetAllStreams()) {
        if (!stream) {
            std::cerr << "[PyDyssol] Error: Null stream detected" << std::endl;
            return false;
        }
        // Check each phase in the stream
        for (const auto& phaseDescr : phases) {
            const auto* phase = stream->GetPhase(phaseDescr.state);
            if (!phase) {
                std::cerr << "[PyDyssol] Error: Null phase " << phaseDescr.name << " in stream: " << stream->GetName() << std::endl;
                return false;
            }
            if (!phase->MDDistr()) {
                std::cerr << "[PyDyssol] Error: Invalid distribution in phase " << phaseDescr.name << " of stream: " << stream->GetName() << std::endl;
                return false;
            }
        }
    }

    return true;
}

// Helper function to set compounds one-by-one as a fallback
bool PyDyssol::SetCompoundsFallback(const std::vector<std::string>& _compoundKeys) {
    std::cout << "[PyDyssol] Attempting to set compounds one-by-one..." << std::endl;
    for (const auto& key : _compoundKeys) {
        try {
            m_flowsheet.AddCompound(key);
            const auto* compound = m_materialsDatabase.GetCompound(key);
            std::cout << "[PyDyssol] Successfully added compound: " << (compound ? compound->GetName() : key)
                << " (" << key << ")" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[PyDyssol] Error: Failed to add compound " << key << ": " << e.what() << std::endl;
            return false;
        }
        catch (...) {
            std::cerr << "[PyDyssol] Error: Unknown exception while adding compound " << key << std::endl;
            return false;
        }
    }
    return true;
}

bool PyDyssol::SetCompounds(const std::vector<std::string>& compoundNames) {
    std::cout << "[PyDyssol] Setting compounds from names: ";
    for (const auto& name : compoundNames) std::cout << name << " ";
    std::cout << std::endl;

    std::vector<std::string> compoundKeys;
    for (const auto& name : compoundNames) {
        const CCompound* compound = m_materialsDatabase.GetCompoundByName(name);
        if (!compound) {
            std::cerr << "[PyDyssol] Error: Cannot find compound '" << name << "' in the loaded materials database." << std::endl;
            return false;
        }
        compoundKeys.push_back(compound->GetKey());
    }

    // Keep rest of the logic unchanged
    if (!ValidateFlowsheetState()) {
        std::cerr << "[PyDyssol] Error: Invalid flowsheet state before setting compounds" << std::endl;
        return false;
    }

    std::cout << "[PyDyssol] Resetting calculation sequence..." << std::endl;
    m_flowsheet.GetCalculationSequence()->Clear();
    m_flowsheet.SetTopologyModified(true);

    try {
        m_flowsheet.SetCompounds(compoundKeys);
    }
    catch (const std::exception& e) {
        std::cerr << "[PyDyssol] Error: Failed to set compounds in flowsheet: " << e.what() << std::endl;
        if (!SetCompoundsFallback(compoundKeys)) return false;
    }
    catch (...) {
        std::cerr << "[PyDyssol] Error: Unknown exception while setting compounds in flowsheet" << std::endl;
        if (!SetCompoundsFallback(compoundKeys)) return false;
    }

    size_t compoundsSet = m_flowsheet.GetCompoundsNumber();
    std::cout << "[PyDyssol] Flowsheet state after setting compounds: compounds=" << compoundsSet << std::endl;
    if (compoundsSet != compoundKeys.size()) {
        std::cerr << "[PyDyssol] Error: Expected " << compoundKeys.size() << " compounds, got " << compoundsSet << std::endl;
        return false;
    }

    try {
        m_flowsheet.UpdateGrids();
        std::cout << "[PyDyssol] Successfully updated grids" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[PyDyssol] Error: Failed to update grids: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "[PyDyssol] Error: Unknown exception while updating grids" << std::endl;
        return false;
    }

    compoundsSet = m_flowsheet.GetCompoundsNumber();
    if (compoundsSet != compoundKeys.size()) {
        std::cerr << "[PyDyssol] Error: Compounds lost after updating grids: expected " << compoundKeys.size()
            << ", got " << compoundsSet << std::endl;
        return false;
    }

    std::cout << "[PyDyssol] Final compounds in flowsheet:" << std::endl;
    for (const auto& key : m_flowsheet.GetCompounds()) {
        const auto* compound = m_materialsDatabase.GetCompound(key);
        std::cout << "  Compound: " << (compound ? compound->GetName() : key) << " (" << key << ")" << std::endl;
    }

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
        result.append(py::str(p.name));
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
