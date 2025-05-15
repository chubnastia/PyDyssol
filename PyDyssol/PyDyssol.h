#ifndef PYDYSSOL_H
#define PYDYSSOL_H

#include <string>
#include <vector>
#include <map>
#include <pybind11/pybind11.h> // Include pybind11 headers
#include <pybind11/stl.h>      // For std::vector and std::pair bindings
#include <pybind11/stl_bind.h>      // For STL bindings with containers
#include "../SimulatorCore/Flowsheet.h"
#include "../SimulatorCore/Simulator.h"
#include "MaterialsDatabase.h"
#include "../SimulatorCore/ModelsManager.h"
#include "../HDF5Handler/H5Handler.h"
#include "UnitParameters.h"

/*struct Holdup {
    double mass;
    double temperature;
    double pressure;
    std::map<std::string, double> composition;
};*/

class PyDyssol
{
private:
    CMaterialsDatabase m_materialsDatabase; // Database of materials
    CModelsManager m_modelsManager;         // Units and solvers manager
    CFlowsheet m_flowsheet;                 // Flowsheet, now dependent on materials and models
    CSimulator m_simulator;                 // Simulator
    std::string m_defaultMaterialsPath;     // Default path for materials database
    std::string m_defaultModelsPath;        // Default path for model units
    bool m_isInitialized;                   // Flag to track initialization state
    


public:
    PyDyssol(const std::string& materialsPath = "D:/Dyssol/Materials.dmdb",
        const std::string& modelsPath = "C:/Program Files/Dyssol/Units");

    bool LoadMaterialsDatabase(const std::string& path);
    bool AddModelPath(const std::string& path);
    bool OpenFlowsheet(const std::string& filePath);
    bool SaveFlowsheet(const std::string& filePath);
    void Simulate(double endTime = -1.0); // Default: -1 means no override
    std::string Initialize();
    void DebugFlowsheet();
    std::vector<std::pair<std::string, std::string>> GetDatabaseCompounds() const;
    std::vector<std::pair<std::string, std::string>> GetCompounds() const;
    bool SetCompounds(const std::vector<std::string>& compounds);
    bool SetupPhases(const std::vector<std::pair<std::string, int>>& phases);

    // Methods for unit parameters
    std::vector<std::pair<std::string, std::string>> GetUnits(); // Returns [Unit name, Model name] pairs
    std::string GetModelNameForUnit(const std::string& unitKey) const;
    std::map<std::string, std::string> GetUnitsDict();  // { unit_name : model_name }

    // Get all parameters (real and combo) as string pairs
    pybind11::object GetUnitParameter(const std::string& unitName, const std::string& paramName) const;
    std::map<std::string, std::tuple<pybind11::object, std::string, std::string>> GetUnitParameters(const std::string& unitName) const;
    std::map<std::string, std::tuple<pybind11::object, std::string, std::string>> GetUnitParametersAll(const std::string& unitName) const;

    void SetUnitParameter(
        const std::string& unitName,
        const std::string& paramName,
        const std::variant<
        bool,  // Moved to the front
        double,
        std::string,
        int64_t,
        uint64_t,
        std::vector<double>,
        std::vector<int64_t>,
        std::vector<uint64_t>,
        std::vector<std::pair<double, double>>,
        std::vector<pybind11::dict>
        > value);

    // Parameters
    std::vector<std::string> GetComboOptions(const std::string& unitName, const std::string& paramName) const;
    std::vector<std::pair<double, double>> GetDependentParameterValues(const std::string& unitName, const std::string& paramName) const;
    std::map<std::string, std::vector<std::pair<double, double>>> GetDependentParameters(const std::string& unitName) const;
    EPhase GetPhaseByName(const std::string& phaseName) const;

    //Holdups
    std::map<std::string, double> GetUnitHoldupComposition(const std::string& unitName, double time) const;
    std::map<std::string, double> GetUnitHoldupOverall(const std::string& unitName, double time) const;
    void PyDyssol::SetUnitHoldup(const std::string& unitName, double time, const pybind11::dict& data);
    pybind11::dict PyDyssol::GetUnitHoldupDistribution(const std::string& unitName, double time) const;
    pybind11::dict PyDyssol::GetUnitHoldup(const std::string& unitName, double time) const;

	// Feeds
    std::map<std::string, double> GetUnitFeedOverall(const std::string& unitName, const std::string& feedName, double time) const;
    std::map<std::string, double> GetUnitFeedComposition(const std::string& unitName, const std::string& feedName, double time) const;
    pybind11::dict GetUnitFeedDistribution(const std::string& unitName, const std::string& feedName, double time) const;
    void SetUnitFeed(const std::string& unitName, const std::string& feedName, double time, const pybind11::dict& data);
    pybind11::dict GetUnitFeed(const std::string& unitName, const std::string& feedName, double time) const;
    std::vector<std::string> GetUnitFeeds(const std::string& unitName) const;

    std::vector<std::string> GetUnitStreams(const std::string& unitName) const;
    pybind11::dict GetUnitStream(const std::string& unitName, const std::string& streamName, double time) const;
    std::map<std::string, double> GetUnitStreamOverall(const std::string& unitName, const std::string& streamName, double time) const;
    std::map<std::string, double> GetUnitStreamComposition(const std::string& unitName, const std::string& streamName, double time) const;
    pybind11::dict GetUnitStreamDistribution(const std::string& unitName, const std::string& streamName, double time) const;

    pybind11::dict GetStream(const std::string& streamName, double time) const;
    std::map<std::string, double> GetStreamOverall(const std::string& streamName, double time) const;
    std::map<std::string, double> GetStreamComposition(const std::string& streamName, double time) const;
    pybind11::dict GetStreamDistribution(const std::string& streamName, double time) const;
    std::vector<std::string> PyDyssol::GetStreams() const;


    void PyDyssol::DebugUnitPorts(const std::string& unitName);
    void PyDyssol::DebugStreamData(const std::string& streamName, double time);

};

#endif // PYDYSSOL_H