#include "PyDyssol.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

namespace py = pybind11;


PYBIND11_MODULE(PyDyssol, m) {

    // Bind PyDyssol class
    py::class_<PyDyssol>(m, "PyDyssol", "A class to manage Dyssol flowsheet simulations in Python")
        .def(py::init<std::string, std::string, bool>(),
            py::arg("materials_path") = "D:/Dyssol/Materials.dmdb",
            py::arg("models_path") = "C:/Program Files/Dyssol/Units",
            py::arg("debug") = false,
            "Initialize PyDyssol with optional materials/models paths and a debug flag.\n"
            "Args:\n"
            "    materials_path (str): Path to the .dmdb file\n"
            "    models_path (str): Path to model units directory\n"
            "    debug (bool): Enable debug output.")
        .def("load_materials_database", &PyDyssol::LoadMaterialsDatabase,
            py::arg("path"),
            "Load a materials database from a .dmdb file.\n"
            "Args:\n"
            "    path (str): Path to the .dmdb file.\n"
            "Returns:\n"
            "    bool: True if successful, False otherwise.")
        .def("add_model_path", &PyDyssol::AddModelPath,
            py::arg("path"),
            "Add a directory containing model DLLs or shared libraries.\n"
            "Args:\n"
            "    path (str): Path to the directory.\n"
            "Returns:\n"
            "    bool: True if successful, False otherwise.")
        .def("open_flowsheet", &PyDyssol::OpenFlowsheet,
            py::arg("file_path"),
            "Open a flowsheet from a .dflw file.\n"
            "Args:\n"
            "    file_path (str): Path to the .dflw file.\n"
            "Returns:\n"
            "    bool: True if successful, False otherwise.")
        .def("close_flowsheet", &PyDyssol::CloseFlowsheet,
            "Clear the current flowsheet and reset to default state.\n"
            "Useful before loading a new flowsheet or starting from scratch.")
        .def("save_flowsheet", &PyDyssol::SaveFlowsheet,
            py::arg("file_path"),
            "Save the current flowsheet to a .dflw file.\n"
            "Args:\n"
            "    file_path (str): Path to save the .dflw file.\n"
            "Returns:\n"
            "    bool: True if successful, False otherwise.")
        .def("simulate", &PyDyssol::Simulate,
            py::arg("end_time") = -1.0,
            "Run the simulation. Optionally override end time.\n"
            "Args:\n"
            "    end_time (float, optional): End time for simulation (seconds). Default: use flowsheet settings.")
        .def("debug_flowsheet", &PyDyssol::DebugFlowsheet,
            "Print debug information about the current flowsheet, including units, streams, compounds, and phases.")
        //Flowsheet
   
        .def("get_available_models", &PyDyssol::GetAvailableModelNames,
            "Returns a list of available model names.")
        
        .def("get_unit_config", &PyDyssol::GetUnitConfig, py::arg("unit_name"),
            "Get the configuration of a unit as a dictionary with 'unit', 'model', and 'ports'.")
        .def("set_unit_config", &PyDyssol::SetUnitConfig, py::arg("unit_name"), py::arg("config"),
            "Set the configuration of a unit from a dictionary.")
        .def("get_model_info", &PyDyssol::GetModelInfo, py::arg("unit_name"),
            "Returns detailed structural info for the specified unit's model.")
        .def("get_topology", &PyDyssol::GetTopology,
            "Returns a list of dictionaries representing the flowsheet topology, each with 'unit', 'model', and 'ports'.")
        .def("set_topology", &PyDyssol::SetTopology, py::arg("config"), py::arg("initialize") = true,
            R"doc(
            Setup a complete flowsheet from a configuration dictionary.

            This method sets up compounds, phases, grids, unit topology, feeds, holdups, unit parameters, and simulation options
            in a single step.

            Args:
            config (dict): Dictionary containing any of the following keys:
        - 'compounds' (list[str]): Names of compounds to add.
        - 'phases' (list[str]): Names of phases to define.
        - 'grids' (list[dict]): Distribution grids (e.g., Size, Compounds).
        - 'topology' (list[dict]): Units with models, ports, holdups, etc.
        - 'feeds' (list[dict]): Feed stream definitions per unit.
        - 'holdups' (list[dict]): Global holdups to apply after unit creation.
        - 'unit parameters' (list[dict]): Unit parameters with format:
            [{'unit': str, 'parameters': dict[str, value]}]
        - 'options' (dict or single-item list[dict]): Simulation options such as time limits and tolerances.

           initialize (bool, optional): Whether to initialize the flowsheet after setup. Default: True.

            Returns:
                bool: True if setup was successful, False otherwise.
            )doc")

        // Compounds
        .def("get_compounds_mdb", &PyDyssol::GetDatabaseCompounds,
            "Returns list of (key, name) pairs from the loaded materials database.")
        .def("add_compound", &PyDyssol::AddCompound,
            py::arg("key"),
            "Add a compound to the flowsheet by its unique key.")
        .def("get_compounds", &PyDyssol::GetCompounds, "Get list of compound names")
        .def("set_compounds", &PyDyssol::SetCompounds, "Set compounds using compound names from the materials database")

        //Phases
        .def("get_phases", &PyDyssol::GetPhases, "Return current list of phases.")
        .def("set_phases", &PyDyssol::SetPhases,
            py::arg("phases"),
            "Replace current phase list. Each item must be (state).")
        .def("add_phase", &PyDyssol::AddPhase, py::arg("state"),
            "Add a phase by type only (e.g. 'solid').\n.")

        //Units
        .def("get_units", &PyDyssol::GetUnits,
            "Get a list of all units in the flowsheet.\n"
            "Returns:\n"
            "    list[tuple[str, str]]: List of (unit_name, model_name) pairs.")
        .def("get_units_dict", &PyDyssol::GetUnitsDict,
            "Get a dictionary of all units: {unit_name: model_name}")

        //Parameters
        .def("get_unit_parameter", &PyDyssol::GetUnitParameter,
            py::arg("unit_name"), py::arg("param_name"),
            "Get a specific parameter from a unit.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "    param_name (str): Name of the parameter.\n"
            "Returns:\n"
            "    The parameter value (type depends on parameter).")

       .def("get_unit_parameters",
            py::overload_cast<const std::string&>(&PyDyssol::GetUnitParameters, py::const_),
            py::arg("unit_name"),
            "Get active parameters of a single unit; returns a list with one dict {unit, parameters}.")
               // active parameters of all units (zero-arg overload)
       .def("get_unit_parameters",
            py::overload_cast<>(&PyDyssol::GetUnitParameters, py::const_),
            "Get active parameters of *all* units; returns one dict per unit, each with keys 'unit' and nested 'parameters'.")
        .def("get_unit_parameters_all", &PyDyssol::GetUnitParametersAll,
            py::arg("unit_name"),
            "Get all parameters (including inactive ones) defined in the unit.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "Returns:\n"
            "    dict[str, (value, type, unit)]: All parameters regardless of model selection.")

       .def("get_unit_parameters_info", &PyDyssol::GetUnitParametersInfo,
            py::arg("unit_name"),
            "Get info about all active parameters of a unit (according to current model selection).\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "Returns:\n"
            "    dict[str, (value, type, unit)]: Parameters info with values, types, and units.")
       .def("get_unit_parameters_all_info", &PyDyssol::GetUnitParametersAllInfo,
            py::arg("unit_name"),
            "Get info about all parameters (including inactive ones) defined in the unit.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "Returns:\n"
            "    dict[str, (value, type, unit)]: All parameters info regardless of model selection.")

        .def("set_unit_parameter", &PyDyssol::SetUnitParameter,
            py::arg("unitName"), py::arg("paramName"), py::arg("value"),
            "Set a unit parameter for the specified unit.\n"
            "Args:\n"
            "    unitName (str): Name of the unit.\n"
            "    paramName (str): Name of the parameter.\n"
            "    value: Value to set (can be float, int, str, bool, list, etc.).")

        .def("get_combo_options", &PyDyssol::GetComboOptions,
            py::arg("unit_name"), py::arg("param_name"),
            "Get all available combo box options for a given combo parameter.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "    param_name (str): Name of the parameter.\n"
            "Returns:\n"
            "    list[str]: List of valid options for this combo parameter.")

        .def("get_dependent_parameters", &PyDyssol::GetDependentParameters,
            py::arg("unit_name"),
            "Get all TIME_DEPENDENT and PARAM_DEPENDENT parameters of a unit.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "Returns:\n"
            "    dict[str, list[tuple[float, float]]]: Mapping of parameter names to (x, y) value pairs.")
        .def("get_dependent_parameter_values", &PyDyssol::GetDependentParameterValues,
            py::arg("unit_name"), py::arg("param_name"),
            "Get the (x, y) value pairs of a TIME_DEPENDENT or PARAM_DEPENDENT parameter.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "    param_name (str): Name of the dependent parameter.\n"
            "Returns:\n"
            "    list[tuple[float, float]]: (independent, dependent) value pairs.")

        //Holdups
        .def("get_unit_holdups", &PyDyssol::GetUnitHoldups,
            py::arg("unit_name"),
            "Get a list of holdup names defined in the given unit.")

        .def("get_unit_holdup_overall",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldupOverall, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get overall mass, temperature, and pressure of the default holdup at a specific time.")
        .def("get_unit_holdup_composition",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldupComposition, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get compound-phase composition of the default holdup at a specific time.")
        .def("get_unit_holdup_distribution",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldupDistribution, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Return solid-phase size distributions per compound in the default holdup.")
       .def("get_unit_holdup", py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldup),
            py::arg("unit_name"), py::arg("time"),
            "Get holdups for a unit at a specific time")

        .def("set_unit_holdup",
            py::overload_cast<const std::string&, const py::dict&>(&PyDyssol::SetUnitHoldup),
            py::arg("unit_name"), py::arg("holdup_dict"),
            "Set the default holdup of a unit at time t = 0.0 using a dictionary with fields 'overall', 'composition', and 'distributions'.")

        //Holdups with holdupName
        .def("get_unit_holdup_overall",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldupOverall, py::const_),
            py::arg("unit_name"), py::arg("holdup_name"), py::arg("time"),
            "Get overall mass, temperature, and pressure of a specific named holdup at a given time.")
        .def("get_unit_holdup_composition",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldupComposition, py::const_),
            py::arg("unit_name"), py::arg("holdup_name"), py::arg("time"),
            "Get compound-phase composition of a specific named holdup at a given time.")
        .def("get_unit_holdup_distribution",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldupDistribution, py::const_),
            py::arg("unit_name"), py::arg("holdup_name"), py::arg("time"),
            "Get size distribution of a specific named holdup at a given time.")
       .def("get_unit_holdup", py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldup),
             py::arg("unit_name"), py::arg("holdup_name"), py::arg("time"),
             "Get a specific holdup for a unit at a specific time")
       .def("get_unit_holdup",
             py::overload_cast<>(&PyDyssol::GetUnitHoldup),
             "Get full time-series holdup data (overall, composition, distributions) for _all_ units and holdups.")

        .def("set_unit_holdup",
            py::overload_cast<const std::string&, const std::string&, const py::dict&>(&PyDyssol::SetUnitHoldup),
            py::arg("unit_name"), py::arg("holdup_name"), py::arg("holdup_dict"),
            "Set a specific named holdup of a unit at t = 0.0 using a dictionary.")

        //Holdups without timepoint, but with holdupName
        .def("get_unit_holdup_overall",
            py::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitHoldupOverall),
            py::arg("unit_name"), py::arg("holdup_name"),
            "Get overall properties (mass, temp, pressure) over all timepoints for a named holdup.")
        .def("get_unit_holdup_composition",
            py::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitHoldupComposition),
            py::arg("unit_name"), py::arg("holdup_name"),
            "Get compound-phase composition over time for a named holdup.")
        .def("get_unit_holdup_distribution",
            py::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitHoldupDistribution),
            py::arg("unit_name"), py::arg("holdup_name"),
            "Get distribution data over time for a named holdup.")
        .def("get_unit_holdup",
            py::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitHoldup),
            py::arg("unit_name"), py::arg("holdup_name"),
            "Get full time-series data (overall, composition, distributions) for a named holdup.")

        //Holdups without timepoint, no holdupName
        .def("get_unit_holdup_overall",
            py::overload_cast<const std::string&>(&PyDyssol::GetUnitHoldupOverall),
            py::arg("unit_name"),
            "Get overall holdup data (mass, temperature, pressure) over all timepoints for the default holdup.")
        .def("get_unit_holdup_composition",
            py::overload_cast<const std::string&>(&PyDyssol::GetUnitHoldupComposition),
            py::arg("unit_name"),
            "Get compound-phase composition over all timepoints for the default holdup.")
        .def("get_unit_holdup_distribution",
            py::overload_cast<const std::string&>(&PyDyssol::GetUnitHoldupDistribution),
            py::arg("unit_name"),
            "Get distribution vectors over all timepoints for the default holdup.")
        .def("get_unit_holdup",
            py::overload_cast<const std::string&>(&PyDyssol::GetUnitHoldup),
            py::arg("unit_name"),
            "Get full time-series holdup data (overall, composition, distributions) for the default holdup.")

        .def("set_unit_holdup",
            py::overload_cast<const py::dict&>(&PyDyssol::SetUnitHoldup),
            py::arg("holdup_dict"),
            "Set one holdup by passing a dict with keys 'unit', optional 'holdup', plus 'overall', 'composition', 'distributions' sub-dicts.")
        
        // Feeds
        .def("get_unit_feeds", &PyDyssol::GetUnitFeeds,
            py::arg("unit_name"),
            "Get a list of feed names defined for the given unit.")
        //Feed sets
        .def("set_unit_feed",
            py::overload_cast<const std::string&, const std::string&, double, const py::dict&>(&PyDyssol::SetUnitFeed),
            py::arg("unit_name"), py::arg("feed_name"), py::arg("time"), py::arg("data"),
            "Set feed data for a unit's named feed at a specific time.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "    feed_name (str): Name of the feed stream.\n"
            "    time (float): Time [s] to apply the values.\n"
            "    data (dict): Dictionary with keys 'overall', 'composition', and/or 'distributions'.")
        .def("set_unit_feed",
            py::overload_cast<const std::string&, const py::dict&>(&PyDyssol::SetUnitFeed),
            py::arg("unit_name"), py::arg("data"),
            "Set the first feed of a unit at t=0.0 with a data dictionary.")
        .def("set_unit_feed",
            py::overload_cast<const std::string&, double, const py::dict&>(&PyDyssol::SetUnitFeed),
            py::arg("unit_name"), py::arg("time"), py::arg("data"),
            "Set the first feed of a unit at the given time.")
        .def("set_unit_feed",
            py::overload_cast<const std::string&, const std::string&, const py::dict&>(&PyDyssol::SetUnitFeed),
            py::arg("unit_name"), py::arg("feed_name"), py::arg("data"),
            "Set a named feed of a unit at t=0.0 with a data dictionary.")
        .def("set_unit_feed",
            py::overload_cast<const py::dict&>(&PyDyssol::SetUnitFeed),
            py::arg("feed_dict"),
            "Set one feed by passing a dict with keys 'unit', optional 'feed', plus 'overall', 'composition', 'distributions' sub-dicts.")

        .def("get_unit_feed_overall",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeedOverall, py::const_),
            py::arg("unit_name"), py::arg("feed_name"), py::arg("time"),
            "Get overall properties of a feed in a unit at a specific time.")
        .def("get_unit_feed_composition",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeedComposition, py::const_),
            py::arg("unit_name"), py::arg("feed_name"), py::arg("time"),
            "Get composition of a feed in a unit at a specific time.")
        .def("get_unit_feed_distribution",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeedDistribution, py::const_),
            py::arg("unit_name"), py::arg("feed_name"), py::arg("time"),
            "Get distributions of a feed in a unit at a specific time.")
        .def("get_unit_feed",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeed, py::const_),
            py::arg("unit_name"), py::arg("feed_name"), py::arg("time"),
            "Get all data of a feed in a unit at a specific time.")

        // Feeds without feed name
        .def("get_unit_feed_overall",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeedOverall, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get overall properties of the first feed in a unit at a specific time.")
        .def("get_unit_feed_composition",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeedComposition, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get composition of the first feed in a unit at a specific time.")
        .def("get_unit_feed_distribution",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeedDistribution, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get distributions of the first feed in a unit at a specific time.")
        .def("get_unit_feed",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeed, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get all data of the first feed in a unit at a specific time.")


        // Feeds with explicit feed name and no timepoints
        .def("get_unit_feed_overall",
            py::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitFeedOverall),
            py::arg("unit_name"), py::arg("feed_name"),
            "Get overall properties (massflow, temperature, pressure) for all timepoints of a given feed.\n"
            "Returns a dict with 'timepoints', 'massflow', 'temperature', and 'pressure'.")
        .def("get_unit_feed_composition",
            py::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitFeedComposition),
            py::arg("unit_name"), py::arg("feed_name"),
            "Get composition data for all timepoints of a given feed.\n"
            "Returns a dict with 'timepoints' and compound-phase keys mapping to lists of mass values.")
        .def("get_unit_feed_distribution",
            py::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitFeedDistribution),
            py::arg("unit_name"), py::arg("feed_name"),
            "Get distribution vectors for all timepoints of a given feed.\n"
            "Returns a dict with 'timepoints' and distribution names as keys.")
        .def("get_unit_feed",
            py::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitFeed),
            py::arg("unit_name"), py::arg("feed_name"),
            "Get all time-series data (overall, composition, distributions) for a named feed.\n"
            "Returns a dict with keys 'overall', 'composition', and 'distributions'.")


        //Feeds without timepoints
        .def("get_unit_feed_overall",
            py::overload_cast<const std::string&>(&PyDyssol::GetUnitFeedOverall),
            py::arg("unit_name"),
            "Get overall feed data (massflow, temperature, pressure) for all timepoints of the first feed in a unit.")
        .def("get_unit_feed_composition",
            py::overload_cast<const std::string&>(&PyDyssol::GetUnitFeedComposition),
            py::arg("unit_name"),
            "Get composition (mass per compound-phase) over all timepoints of the first feed in a unit.")
        .def("get_unit_feed_distribution",
            py::overload_cast<const std::string&>(&PyDyssol::GetUnitFeedDistribution),
            py::arg("unit_name"),
            "Get distributions (e.g. particle sizes) over all timepoints of the first feed in a unit.")
        .def("get_unit_feed",
            py::overload_cast<const std::string&>(&PyDyssol::GetUnitFeed),
            py::arg("unit_name"),
            "Get full time-series feed data (overall, composition, distributions) for the first feed in a unit.")
        //No arguments
        .def("get_unit_feed", py::overload_cast<>(&PyDyssol::GetUnitFeed),
            "Get all feeds of all units, in the same structure as config['feeds']")


        // Unit Streams
        .def("get_unit_streams", &PyDyssol::GetUnitStreams,
            py::arg("unit_name"),
            "Get names of all internal (work) streams of a unit.")
        .def("get_unit_stream_overall",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStreamOverall, py::const_),
            py::arg("unit_name"), py::arg("stream_name"), py::arg("time"),
            "Get overall properties of a stream inside a unit at a specific time.")
        .def("get_unit_stream_composition",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStreamComposition, py::const_),
            py::arg("unit_name"), py::arg("stream_name"), py::arg("time"),
            "Get compound-phase composition of a stream inside a unit at a specific time.")
        .def("get_unit_stream_distribution",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStreamDistribution, py::const_),
            py::arg("unit_name"), py::arg("stream_name"), py::arg("time"),
            "Get solid-phase distributions of a stream inside a unit at a specific time.")
        .def("get_unit_stream",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStream, py::const_),
            py::arg("unit_name"), py::arg("stream_name"), py::arg("time"),
            "Get all data (overall, composition, distributions) of a stream inside a unit at a specific time.")

        //no arguments
       .def("get_unit_stream",
            py::overload_cast<>(&PyDyssol::GetUnitStream, py::const_),
            "Get data (overall, composition, distributions) for every unit stream in every unit of the flowsheet.")

        // Unit Streams without Stream Name
        .def("get_unit_stream_overall",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStreamOverall, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get overall properties of the first stream inside a unit at a specific time.")
        .def("get_unit_stream_composition",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStreamComposition, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get compound-phase composition of the first stream inside a unit at a specific time.")
        .def("get_unit_stream_distribution",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStreamDistribution, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get solid-phase distributions of the first stream inside a unit at a specific time.")
        .def("get_unit_stream",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStream, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get all data of the first stream inside a unit at a specific time.")

            //Unit Streams with explicit stream name and no timepoints
       .def("get_unit_stream_overall",
            py::overload_cast<const std::string&, const std::string&>(
            &PyDyssol::GetUnitStreamOverall, py::const_),
            py::arg("unit_name"), py::arg("stream_name"),
            "Get time-series overall properties (massflow, temperature, pressure) for a named stream inside a unit.")
       .def("get_unit_stream_composition",
            py::overload_cast<const std::string&, const std::string&>(
            &PyDyssol::GetUnitStreamComposition, py::const_),
            py::arg("unit_name"), py::arg("stream_name"),
            "Get time-series compound-phase composition for a named internal stream.")
       .def("get_unit_stream_distribution",
            py::overload_cast<const std::string&, const std::string&>(
            &PyDyssol::GetUnitStreamDistribution, py::const_),
            py::arg("unit_name"), py::arg("stream_name"),
            "Get time-series distributions for a named stream inside a unit.")
       .def("get_unit_stream",
            py::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitStream, py::const_),
            py::arg("unit_name"), py::arg("stream_name"),
            "Get data for a single unit stream.")

            //Unit Streams without timepoints
            .def("get_unit_stream_overall",
                py::overload_cast<const std::string&>(&PyDyssol::GetUnitStreamOverall),
                py::arg("unit_name"),
                "Get overall stream data (massflow, temperature, pressure) over all timepoints for the first stream in a unit.")
            .def("get_unit_stream_composition",
                py::overload_cast<const std::string&>(&PyDyssol::GetUnitStreamComposition),
                py::arg("unit_name"),
                "Get compound-phase composition over all timepoints for the first stream in a unit.")
            .def("get_unit_stream_distribution",
                py::overload_cast<const std::string&>(&PyDyssol::GetUnitStreamDistribution),
                py::arg("unit_name"),
                "Get distribution vectors over all timepoints for the first stream in a unit.")
            .def("get_unit_stream",
                py::overload_cast<const std::string&>(&PyDyssol::GetUnitStream),
                py::arg("unit_name"),
                "Get all stream data (overall, composition, distributions) over all timepoints for the first stream in a unit.")

            // Streams
            .def("get_streams", &PyDyssol::GetStreams, "Return list of all flowsheet-level stream names.")

            .def("get_stream_overall",
                py::overload_cast<const std::string&, double>(&PyDyssol::GetStreamOverall, py::const_),
                py::arg("stream_name"), py::arg("time"),
                "Get overall stream data at a specific time.")
            .def("get_stream_composition",
                py::overload_cast<const std::string&, double>(&PyDyssol::GetStreamComposition, py::const_),
                py::arg("stream_name"), py::arg("time"),
                "Get stream composition at a specific time.")
            .def("get_stream_distribution",
                py::overload_cast<const std::string&, double>(&PyDyssol::GetStreamDistribution, py::const_),
                py::arg("stream_name"), py::arg("time"),
                "Get stream distribution at a specific time.")
            .def("get_stream",
                py::overload_cast<const std::string&, double>(&PyDyssol::GetStream, py::const_),
                py::arg("stream_name"), py::arg("time"),
                "Get all stream data (overall, composition, distributions) at a given time.")


            //Streams without timepoints
            .def("get_stream_overall",
                static_cast<pybind11::dict(PyDyssol::*)(const std::string&) const>(&PyDyssol::GetStreamOverall),
                py::arg("stream_name"),
                "Get overall stream data for all timepoints.")
           .def("get_stream_composition",
                static_cast<pybind11::dict(PyDyssol::*)(const std::string&) const>(&PyDyssol::GetStreamComposition),
                py::arg("stream_name"),
                "Get stream composition for all timepoints.")
           .def("get_stream_distribution",
                static_cast<pybind11::dict(PyDyssol::*)(const std::string&) const>(&PyDyssol::GetStreamDistribution),
                py::arg("stream_name"),
                "Get stream distribution for all timepoints.")

           .def("get_stream",
                static_cast<pybind11::dict(PyDyssol::*)(const std::string&) const>(
                &PyDyssol::GetStream),
                py::arg("stream_name"),
                "Get all stream data (overall, composition, distributions) for all timepoints.")
            .def("get_stream",
                static_cast<pybind11::list(PyDyssol::*)() const>(
                    &PyDyssol::GetStream),
                "Get data for every flowsheet-level stream: returns a list of {overall, composition, distributions} dicts.")

            //Options
            .def("get_options", &PyDyssol::GetOptions, "Get flowsheet simulation options.")
            .def("set_options", &PyDyssol::SetOptions, py::arg("options"), "Set flowsheet simulation options.")
            .def("get_options_methods", &PyDyssol::GetOptionsMethods,
                "Return valid string options for convergenceMethod and extrapolationMethod.")

            //Grids
            .def("get_grids", &PyDyssol::GetGrids,
                "Return all distribution grids as a list of {type, grid} dictionaries.")
            .def("set_grids", &PyDyssol::SetGrids,
                py::arg("grids"),
                "Replace all existing grids with the provided list.")
            .def("add_grid", &PyDyssol::AddGrid,
                py::arg("grid"),
                "Add or replace grids by type.")

            //Debugging
            .def("debug_unit_ports", &PyDyssol::DebugUnitPorts, py::arg("unit_name"))
            .def("debug_stream_data", &PyDyssol::DebugStreamData, py::arg("stream_name"), py::arg("time"));


    // Register exceptions
    py::register_exception<std::runtime_error>(m, "RuntimeError");
    py::register_exception<std::invalid_argument>(m, "ValueError");

    py::enum_<EPhase>(m, "EPhase")
        .value("SOLID", EPhase::SOLID)
        .value("LIQUID", EPhase::LIQUID)
        .value("VAPOR", EPhase::VAPOR)
        .export_values();

    //Pretty print function
    m.def("pretty_print", [](const py::object& data) {
        if (data.is_none()) {
            py::print("None");
            return;
        }

        if (py::isinstance<py::list>(data)) {
            py::list data_list = data.cast<py::list>();
            if (data_list.size() == 0) {
                py::print("[]");
                return;
            }

            // Check for grid data
            bool is_grid_data = true;
            for (const auto& item : data_list) {
                if (!py::isinstance<py::dict>(item) || !item.cast<py::dict>().contains("type") || !item.cast<py::dict>().contains("grid")) {
                    is_grid_data = false;
                    break;
                }
            }
            if (is_grid_data) {
                print_grid_data(data_list);
                return;
            }

            // Check for topology (list of units with model and ports)
            bool is_topology = true;
            for (const auto& item : data_list) {
                if (!py::isinstance<py::dict>(item)) {
                    is_topology = false;
                    break;
                }
                py::dict d = item.cast<py::dict>();
                if (!d.contains("unit") || !d.contains("model") || !d.contains("ports")) {
                    is_topology = false;
                    break;
                }
            }
            if (is_topology) {
                py::print("[");
                for (size_t i = 0; i < py::len(data_list); ++i) {
                    py::dict d = data_list[i].cast<py::dict>();
                    std::string unit = d["unit"].cast<std::string>();
                    std::string model = d["model"].cast<std::string>();
                    py::dict ports = d["ports"].cast<py::dict>();

                    py::print("    {\"unit\": \"" + unit + "\", \"model\": \"" + model + "\", \"ports\": {");
                    size_t count = 0;
                    for (auto p : ports) {
                        std::string port = p.first.cast<std::string>();
                        py::dict port_dict = p.second.cast<py::dict>();
                        std::string stream = port_dict.contains("stream") ? port_dict["stream"].cast<std::string>() : "";
                        std::string comma = (++count < py::len(ports)) ? "," : "";
                        py::print("        \"" + port + "\": {\"stream\": \"" + stream + "\"}" + comma);
                    }
                    std::string comma = (i + 1 < py::len(data_list)) ? "," : "";
                    py::print("    }}" + comma);
                }
                py::print("]");
                return;
            }

            // Fallback for other lists
            py::print(data);
            return;
        }

        if (py::isinstance<py::dict>(data)) {
            py::dict data_dict = data.cast<py::dict>();
            if (data_dict.size() == 0) {
                py::print("{}");
                return;
            }

            // Check for unit parameters
            bool is_unit_params = true;
            for (const auto& item : data_dict) {
                if (!py::isinstance<py::tuple>(item.second) || item.second.cast<py::tuple>().size() != 3 ||
                    !py::isinstance<py::str>(item.second.cast<py::tuple>()[1]) ||
                    !py::isinstance<py::str>(item.second.cast<py::tuple>()[2])) {
                    is_unit_params = false;
                    break;
                }
            }
            if (is_unit_params) {
                print_unit_params(data_dict);
                return;
            }

            // Check for holdup-like structure
            if (data_dict.contains("overall") && data_dict.contains("composition") && data_dict.contains("distributions")) {
                print_holdup_data(data_dict);
                return;
            }

            // Assume simulation options
            print_options(data_dict);
            return;
        }

        // Fallback for all other types
        py::print(data);
        });
}