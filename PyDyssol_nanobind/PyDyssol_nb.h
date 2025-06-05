#ifndef PYDYSSOL_H
#define PYDYSSOL_H

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <nanobind/nanobind.h> // Include nanobind headers
#include <nanobind/stl/string.h> // For std::string bindings
#include <nanobind/stl/vector.h> // For std::vector bindings
#include <nanobind/stl/pair.h>   // For std::pair bindings
#include <nanobind/stl/map.h>    // For std::map bindings
#include <nanobind/stl/tuple.h>  // For std::tuple bindings
#include <nanobind/stl/list.h>   // For nanobind::list bindings

#include "../SimulatorCore/Flowsheet.h"
#include "../SimulatorCore/Simulator.h"
#include "../MaterialsDatabase/MaterialsDatabase.h"
#include "../SimulatorCore/ModelsManager.h"
#include "../HDF5Handler/H5Handler.h"
#include "../ModelsAPI/UnitParameters.h"

std::string PhaseToString(EPhase phase);
EOverall StringToEOverall(const std::string& name);
std::string FormatDouble(double value);
EPhase ConvertPhaseState(const nanobind::object& state);

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
    void SetHoldupValues(CHoldup* holdup, double time, const nanobind::dict& data, const std::vector<const CGridDimension*>& gridDims);

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
    bool AddCompound(const std::string& key);

    nanobind::list GetPhases() const;
    bool SetPhases(const nanobind::list& phases);
    bool AddPhase(const nanobind::object& state);


    // Methods for unit parameters
    std::vector<std::pair<std::string, std::string>> GetUnits(); // Returns [Unit name, Model name] pairs
    std::string GetModelNameForUnit(const std::string& unitKey) const;
    std::map<std::string, std::string> GetUnitsDict();  // { unit_name : model_name }

    // Get all parameters (real and combo) as string pairs
    nanobind::object GetUnitParameter(const std::string& unitName, const std::string& paramName) const;
    std::map<std::string, std::tuple<nanobind::object, std::string, std::string>> GetUnitParameters(const std::string& unitName) const;
    std::map<std::string, std::tuple<nanobind::object, std::string, std::string>> GetUnitParametersAll(const std::string& unitName) const;

    void SetUnitParameter(const std::string& unitName, const std::string& paramName, const std::variant< bool, double, std::string,
        int64_t, uint64_t, std::vector<double>, std::vector<int64_t>, std::vector<uint64_t>, std::vector<std::pair<double, double>>,
        std::vector<nanobind::dict> > value);

    // Parameters
    std::vector<std::string> GetComboOptions(const std::string& unitName, const std::string& paramName) const;
    std::vector<std::pair<double, double>> GetDependentParameterValues(const std::string& unitName, const std::string& paramName) const;
    std::map<std::string, std::vector<std::pair<double, double>>> GetDependentParameters(const std::string& unitName) const;
    EPhase GetPhaseByName(const std::string& phaseName) const;


    //Holdups
    std::vector<std::string> GetUnitHoldups(const std::string& unitName) const;

    std::map<std::string, double> GetUnitHoldupOverall(const std::string& unitName, const std::string& holdupName, double time) const;
    std::map<std::string, double> GetUnitHoldupComposition(const std::string& unitName, const std::string& holdupName, double time) const;
    nanobind::dict GetUnitHoldupDistribution(const std::string& unitName, const std::string& holdupName, double time) const;
    nanobind::dict GetUnitHoldup(const std::string& unitName, const std::string& holdupName, double time) const;
    //Without holdupName
    std::map<std::string, double> GetUnitHoldupOverall(const std::string& unitName, double time) const;
    std::map<std::string, double> GetUnitHoldupComposition(const std::string& unitName, double time) const;
    nanobind::dict GetUnitHoldupDistribution(const std::string& unitName, double time) const;
    nanobind::dict GetUnitHoldup(const std::string& unitName, double time) const;
	// Without timepoints, named holdups
    nanobind::dict GetUnitHoldupOverall(const std::string& unitName, const std::string& holdupName);
    nanobind::dict GetUnitHoldupComposition(const std::string& unitName, const std::string& holdupName);
    nanobind::dict GetUnitHoldupDistribution(const std::string& unitName, const std::string& holdupName);
    nanobind::dict GetUnitHoldup(const std::string& unitName, const std::string& holdupName);
    //Without timepoints
    nanobind::dict GetUnitHoldupOverall(const std::string& unitName);
    nanobind::dict GetUnitHoldupComposition(const std::string& unitName);
    nanobind::dict GetUnitHoldupDistribution(const std::string& unitName);
    nanobind::dict GetUnitHoldup(const std::string& unitName);
    //Sets
    void SetUnitHoldup(const std::string& unitName, const nanobind::dict& data);
    void SetUnitHoldup(const std::string& unitName, const std::string& holdupName, const nanobind::dict& data);


    // Feeds
    std::vector<std::string> GetUnitFeeds(const std::string& unitName) const;

    std::map<std::string, double> GetUnitFeedOverall(const std::string& unitName, const std::string& feedName, double time) const;
    std::map<std::string, double> GetUnitFeedComposition(const std::string& unitName, const std::string& feedName, double time) const;
    nanobind::dict GetUnitFeedDistribution(const std::string& unitName, const std::string& feedName, double time) const;
    nanobind::dict GetUnitFeed(const std::string& unitName, const std::string& feedName, double time) const;
    // Overloads without feedName, default to first feed
    std::map<std::string, double> GetUnitFeedOverall(const std::string& unitName, double time) const;
    std::map<std::string, double> GetUnitFeedComposition(const std::string& unitName, double time) const;
    nanobind::dict GetUnitFeedDistribution(const std::string& unitName, double time) const;
    nanobind::dict GetUnitFeed(const std::string& unitName, double time) const;
    //Without timepoints
    nanobind::dict GetUnitFeedOverall(const std::string& unitName, const std::string& feedName);
    nanobind::dict GetUnitFeedComposition(const std::string& unitName, const std::string& feedName);
    nanobind::dict GetUnitFeedDistribution(const std::string& unitName, const std::string& feedName);
    nanobind::dict GetUnitFeed(const std::string& unitName, const std::string& feedName);
	//Without timepoints, only first feed
    nanobind::dict GetUnitFeedOverall(const std::string& unitName);
    nanobind::dict GetUnitFeedComposition(const std::string& unitName);
    nanobind::dict GetUnitFeedDistribution(const std::string& unitName);
    nanobind::dict GetUnitFeed(const std::string& unitName);
    //Sets
    void SetUnitFeed(const std::string& unitName, const std::string& feedName, double time, const nanobind::dict& data);
    void SetUnitFeed(const std::string& unitName, const nanobind::dict& data);
    void SetUnitFeed(const std::string& unitName, double time, const nanobind::dict& data);
    void SetUnitFeed(const std::string& unitName, const std::string& feedName, const nanobind::dict& data);

    //Unit Streams
    std::vector<std::string> GetUnitStreams(const std::string& unitName) const;
    nanobind::dict GetUnitStream(const std::string& unitName, const std::string& streamName, double time) const;
    std::map<std::string, double> GetUnitStreamOverall(const std::string& unitName, const std::string& streamName, double time) const;
    std::map<std::string, double> GetUnitStreamComposition(const std::string& unitName, const std::string& streamName, double time) const;
    nanobind::dict GetUnitStreamDistribution(const std::string& unitName, const std::string& streamName, double time) const;
	// Overloads without streamName
    std::map<std::string, double> GetUnitStreamOverall(const std::string& unitName, double time) const;
    std::map<std::string, double> GetUnitStreamComposition(const std::string& unitName, double time) const;
    nanobind::dict GetUnitStreamDistribution(const std::string& unitName, double time) const;
    nanobind::dict GetUnitStream(const std::string& unitName, double time) const;
	// Unit streams with explicit stream name
    nanobind::dict GetUnitStreamOverall(const std::string& unitName, const std::string& streamName);
    nanobind::dict GetUnitStreamComposition(const std::string& unitName, const std::string& streamName);
    nanobind::dict GetUnitStreamDistribution(const std::string& unitName, const std::string& streamName);
    nanobind::dict GetUnitStream(const std::string& unitName, const std::string& streamName);
    //Without timepoints, stream names
    nanobind::dict GetUnitStreamOverall(const std::string& unitName);
    nanobind::dict GetUnitStreamComposition(const std::string& unitName);
    nanobind::dict GetUnitStreamDistribution(const std::string& unitName);
    nanobind::dict GetUnitStream(const std::string& unitName);

    //Streams
    nanobind::dict GetStream(const std::string& streamName, double time) const;
    std::map<std::string, double> GetStreamOverall(const std::string& streamName, double time) const;
    std::map<std::string, double> GetStreamComposition(const std::string& streamName, double time) const;
    nanobind::dict GetStreamDistribution(const std::string& streamName, double time) const;
    std::vector<std::string> GetStreams() const;
    //Without timepoints
    nanobind::dict GetStreamOverall(const std::string& streamName);
    nanobind::dict GetStreamComposition(const std::string& streamName);
    nanobind::dict GetStreamDistribution(const std::string& streamName);
    nanobind::dict GetStream(const std::string& streamName);

    //Options
    nanobind::dict GetOptions() const;
    void SetOptions(const nanobind::dict& options);
    nanobind::dict GetOptionsMethods() const;

    //Debug
    void DebugUnitPorts(const std::string& unitName);
    void DebugStreamData(const std::string& streamName, double time);

};

#endif // PYDYSSOL_H