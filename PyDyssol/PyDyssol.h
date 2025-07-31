#ifndef PYDYSSOL_H
#define PYDYSSOL_H

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <pybind11/pybind11.h> // Include pybind11 headers
#include <pybind11/stl.h>      // For std::vector and std::pair bindings
#include <pybind11/stl_bind.h>      // For STL bindings with containers
#include "../SimulatorCore/Flowsheet.h"
#include "../SimulatorCore/Simulator.h"
#include "MaterialsDatabase.h"
#include "../SimulatorCore/ModelsManager.h"
#include "../HDF5Handler/H5Handler.h"
#include "UnitParameters.h"

class PyDyssol
{
private:
    CMaterialsDatabase m_materialsDatabase; // Database of materials
    CModelsManager m_modelsManager;         // Units and solvers manager
    CFlowsheet m_flowsheet;                 // Flowsheet, now dependent on materials and models
    CSimulator m_simulator;                 // Simulator
    std::string m_defaultMaterialsPath;     // Default path for materials database
    std::string m_defaultModelsPath;        // Default path for model units
    bool m_isDatabaseLoaded;                // Flag to track database load state
    bool m_isModelsLoaded;                  // Flag to track model path load state
	bool m_debug{ false };                  // Flag for debug mode
	bool PyDyssol::ValidateFlowsheetState() const; // Validate the current state of the flowsheet
    bool PyDyssol::SetCompoundsFallback(const std::vector<std::string>& _compoundKeys);
    

public:
    PyDyssol(const std::string& materialsPath = "D:/Dyssol/Materials.dmdb",
        const std::string& modelsPath = "C:/Program Files/Dyssol/Units",
        bool debug = false);

    bool LoadMaterialsDatabase(const std::string& path);
    bool AddModelPath(const std::string& path);
    bool OpenFlowsheet(const std::string& filePath);
    void PyDyssol::CloseFlowsheet();
    bool SaveFlowsheet(const std::string& filePath);
    void Simulate(double endTime = -1.0); // Default: -1 means no override
    std::string Initialize();
    void DebugFlowsheet();
    //Flowsheet
    std::vector<std::string> GetAvailableModelNames() const;
    bool SetTopology(const pybind11::dict& config, bool initialize = true);
    pybind11::list GetTopology() const;
    std::string ValidateCalculationSequence() const;
    
    // New methods for unit configuration
    pybind11::dict GetUnitConfig(const std::string& unitName) const;
    void SetUnitConfig(const std::string& unitName, const pybind11::dict& config);
    pybind11::dict GetModelInfo(const std::string& unitName) const;

    // Stream management
    CStream* AddStream(const std::string& streamName);
    const CStream* GetStreams_flowsheet(const std::string& streamName) const;

    //Compounds
    std::vector<std::pair<std::string, std::string>> GetDatabaseCompounds() const;
    std::vector<std::string> PyDyssol::GetCompounds() const;

    bool SetCompounds(const std::vector<std::string>& compoundNames);

    bool AddCompound(const std::string& key);

    pybind11::list GetPhases() const;
    bool SetPhases(const pybind11::list& phases);
    bool AddPhase(const pybind11::object& state);


    // Methods for unit parameters
    std::vector<std::pair<std::string, std::string>> GetUnits(); // Returns [Unit name, Model name] pairs
    std::string GetModelNameForUnit(const std::string& unitKey) const;
    std::map<std::string, std::string> GetUnitsDict();  // { unit_name : model_name }

    // Get all parameters (real and combo) as string pairs
    pybind11::object GetUnitParameter(const std::string& unitName, const std::string& paramName) const;
    pybind11::list PyDyssol::GetUnitParameters(const std::string& unitName) const;
    pybind11::list PyDyssol::GetUnitParameters() const;
    pybind11::list PyDyssol::GetUnitParametersAll(const std::string& unitName) const;
    std::map<std::string, std::tuple<pybind11::object, std::string, std::string>> PyDyssol::GetUnitParametersInfo(const std::string& unitName) const;
    std::map<std::string, std::tuple<pybind11::object, std::string, std::string>> PyDyssol::GetUnitParametersAllInfo(const std::string& unitName) const;

    void SetUnitParameter(const std::string& unitName, const std::string& paramName, const std::variant< bool, double, std::string,
        int64_t, uint64_t, std::vector<double>, std::vector<int64_t>, std::vector<uint64_t>, std::vector<std::pair<double, double>>,
        std::vector<pybind11::dict> > value);

    // Parameters
    std::vector<std::string> GetComboOptions(const std::string& unitName, const std::string& paramName) const;
    std::vector<std::pair<double, double>> GetDependentParameterValues(const std::string& unitName, const std::string& paramName) const;
    std::map<std::string, std::vector<std::pair<double, double>>> GetDependentParameters(const std::string& unitName) const;


    //Holdups
    std::vector<std::string> GetUnitHoldups(const std::string& unitName) const;

    std::map<std::string, double> GetUnitHoldupOverall(const std::string& unitName, const std::string& holdupName, double time) const;
    std::map<std::string, double> GetUnitHoldupComposition(const std::string& unitName, const std::string& holdupName, double time) const;
    pybind11::dict GetUnitHoldupDistribution(const std::string& unitName, const std::string& holdupName, double time) const;
    pybind11::list PyDyssol::GetUnitHoldup(const std::string& unitName, const std::string& holdupName, double time);
    //Without holdupName
    std::map<std::string, double> GetUnitHoldupOverall(const std::string& unitName, double time) const;
    std::map<std::string, double> GetUnitHoldupComposition(const std::string& unitName, double time) const;
    pybind11::dict PyDyssol::GetUnitHoldupDistribution(const std::string& unitName, double time) const;
    pybind11::list PyDyssol::GetUnitHoldup(const std::string& unitName, double time);
	// Without timepoints, named holdups
    pybind11::dict GetUnitHoldupOverall(const std::string& unitName, const std::string& holdupName);
    pybind11::dict GetUnitHoldupComposition(const std::string& unitName, const std::string& holdupName);
    pybind11::dict GetUnitHoldupDistribution(const std::string& unitName, const std::string& holdupName);
    pybind11::list PyDyssol::GetUnitHoldup(const std::string& unitName, const std::string& holdupName);
    //Without timepoints
    pybind11::dict GetUnitHoldupOverall(const std::string& unitName);
    pybind11::dict GetUnitHoldupComposition(const std::string& unitName);
    pybind11::dict GetUnitHoldupDistribution(const std::string& unitName);
    pybind11::list PyDyssol::GetUnitHoldup(const std::string& unitName);
    pybind11::list PyDyssol::GetUnitHoldup();
    //Sets
    void SetUnitHoldup(const std::string& unitName, const pybind11::dict& data);
    void SetUnitHoldup(const std::string& unitName, const std::string& holdupName, const pybind11::dict& data);
    void SetUnitHoldup(const pybind11::dict& holdupDict);


    // Feeds
    std::vector<std::string> GetUnitFeeds(const std::string& unitName) const;

    std::map<std::string, double> GetUnitFeedOverall(const std::string& unitName, const std::string& feedName, double time) const;
    std::map<std::string, double> GetUnitFeedComposition(const std::string& unitName, const std::string& feedName, double time) const;
    pybind11::dict GetUnitFeedDistribution(const std::string& unitName, const std::string& feedName, double time) const;
    pybind11::list GetUnitFeed(const std::string& unitName, const std::string& feedName, double time) const;
    // Overloads without feedName, default to first feed
    std::map<std::string, double> GetUnitFeedOverall(const std::string& unitName, double time) const;
    std::map<std::string, double> GetUnitFeedComposition(const std::string& unitName, double time) const;
    pybind11::dict GetUnitFeedDistribution(const std::string& unitName, double time) const;
    pybind11::list GetUnitFeed(const std::string& unitName, double time) const;
    //Without timepoints
    pybind11::dict GetUnitFeedOverall(const std::string& unitName, const std::string& feedName);
    pybind11::dict GetUnitFeedComposition(const std::string& unitName, const std::string& feedName);
    pybind11::dict GetUnitFeedDistribution(const std::string& unitName, const std::string& feedName);
    pybind11::list GetUnitFeed(const std::string& unitName, const std::string& feedName);
	//Without timepoints, only first feed
    pybind11::dict GetUnitFeedOverall(const std::string& unitName);
    pybind11::dict GetUnitFeedComposition(const std::string& unitName);
    pybind11::dict GetUnitFeedDistribution(const std::string& unitName);
    pybind11::list GetUnitFeed(const std::string& unitName);
	//No arguments
    pybind11::list GetUnitFeed();
    //Sets
    void SetUnitFeed(const std::string& unitName, const std::string& feedName, double time, const pybind11::dict& data);
    void SetUnitFeed(const std::string& unitName, const pybind11::dict& data);
    void SetUnitFeed(const std::string& unitName, double time, const pybind11::dict& data);
    void SetUnitFeed(const std::string& unitName, const std::string& feedName, const pybind11::dict& data);
    void SetUnitFeed(const pybind11::dict& feedDict);

    //Unit Streams
    std::vector<std::string> GetUnitStreams(const std::string& unitName) const;
    pybind11::dict GetUnitStream(const std::string& unitName, const std::string& streamName, double time) const;
    std::map<std::string, double> GetUnitStreamOverall(const std::string& unitName, const std::string& streamName, double time) const;
    std::map<std::string, double> GetUnitStreamComposition(const std::string& unitName, const std::string& streamName, double time) const;
    pybind11::dict GetUnitStreamDistribution(const std::string& unitName, const std::string& streamName, double time) const;
	// Overloads without streamName
    std::map<std::string, double> GetUnitStreamOverall(const std::string& unitName, double time) const;
    std::map<std::string, double> GetUnitStreamComposition(const std::string& unitName, double time) const;
    pybind11::dict GetUnitStreamDistribution(const std::string& unitName, double time) const;
    pybind11::dict GetUnitStream(const std::string& unitName, double time) const;
	// Unit streams with explicit stream name
    pybind11::dict GetUnitStreamOverall(const std::string& unitName, const std::string& streamName) const;
    pybind11::dict GetUnitStreamComposition(const std::string& unitName, const std::string& streamName) const;
    pybind11::dict GetUnitStreamDistribution(const std::string& unitName, const std::string& streamName) const;
    pybind11::dict GetUnitStream(const std::string& unitName, const std::string& streamName) const;
    //Without timepoints, stream names
    pybind11::dict GetUnitStreamOverall(const std::string& unitName);
    pybind11::dict GetUnitStreamComposition(const std::string& unitName);
    pybind11::dict GetUnitStreamDistribution(const std::string& unitName);
    pybind11::dict GetUnitStream(const std::string& unitName);
    //No arguments
    pybind11::list GetUnitStream() const;

    //Streams
    pybind11::dict GetStream(const std::string& streamName, double time) const;
    std::map<std::string, double> GetStreamOverall(const std::string& streamName, double time) const;
    std::map<std::string, double> GetStreamComposition(const std::string& streamName, double time) const;
    pybind11::dict GetStreamDistribution(const std::string& streamName, double time) const;
    std::vector<std::string> GetStreams() const;
    //Without timepoints
    pybind11::dict GetStreamOverall(const std::string& streamName) const;
    pybind11::dict GetStreamComposition(const std::string & streamName) const;
    pybind11::dict GetStreamDistribution(const std::string & streamName) const;
    pybind11::dict GetStream(const std::string & streamName) const;
    //No arguments
    pybind11::list GetStream() const;

    //Options
    pybind11::dict GetOptions() const;
    void SetOptions(const pybind11::dict& options);
    pybind11::dict GetOptionsMethods() const;

    //Grids
    std::vector<pybind11::dict> GetGrids() const;
    void SetGrids(const std::vector<std::map<std::string, pybind11::object>>& grids);
    void AddGrid(const std::map<std::string, pybind11::object>& gridData);
    bool IsGridValid(const std::map<std::string, pybind11::object>& gridData) const;

    //Debug
    void PyDyssol::DebugUnitPorts(const std::string& unitName);
    void PyDyssol::DebugStreamData(const std::string& streamName, double time);

};

std::string PhaseToString(EPhase phase);
EOverall StringToEOverall(const std::string& name);
std::string FormatDouble(double value);
EPhase ConvertPhaseState(const pybind11::object& state);
EDistrTypes StringToDistrType(const std::string& name);
std::string ToString(EDistrTypes type);
std::string GetAllowedDistrNames();
bool IsValidDistributionName(const std::string& name);
EPhase GetPhaseByName(const std::string& phaseName);
void print_grid_data(const pybind11::list& data_list);
void print_unit_params(const pybind11::dict& data_dict);
void print_holdup_data(const pybind11::dict& data_dict);
void print_options(const pybind11::dict& data_dict);
using UnitParameterVariant = std::variant<
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
>;


#endif // PYDYSSOL_H