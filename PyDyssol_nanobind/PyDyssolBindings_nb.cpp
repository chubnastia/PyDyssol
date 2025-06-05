#include "PyDyssol_nb.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

using namespace nb::literals;

NB_MODULE(PyDyssol_nanobind, m) {

    // Bind PyDyssol class
    nb::class_<PyDyssol>(m, "PyDyssol", "A class to manage Dyssol flowsheet simulations in Python")
        .def(nb::init<std::string, std::string>(),
            nb::arg("materials_path") = "D:/Dyssol/Materials.dmdb",
            nb::arg("models_path") = "C:/Program Files/Dyssol/Units",
            "Initialize a new PyDyssol instance with optional paths for materials database and model units.\n"
            "Args:\n"
            "    materials_path (str): Path to the materials database (.dmdb file). Default: 'D:/Dyssol/Materials.dmdb'.\n"
            "    models_path (str): Path to the directory containing model units. Default: 'C:/Program Files/Dyssol/Units'.")
        .def("load_materials_database", &PyDyssol::LoadMaterialsDatabase,
            nb::arg("path"),
            "Load a materials database from a .dmdb file.\n"
            "Args:\n"
            "    path (str): Path to the .dmdb file.\n"
            "Returns:\n"
            "    bool: True if successful, False otherwise.")
        .def("add_model_path", &PyDyssol::AddModelPath,
            nb::arg("path"),
            "Add a directory containing model DLLs or shared libraries.\n"
            "Args:\n"
            "    path (str): Path to the directory.\n"
            "Returns:\n"
            "    bool: True if successful, False otherwise.")
        .def("open_flowsheet", &PyDyssol::OpenFlowsheet,
            nb::arg("file_path"),
            "Open a flowsheet from a .dflw file.\n"
            "Args:\n"
            "    file_path (str): Path to the .dflw file.\n"
            "Returns:\n"
            "    bool: True if successful, False otherwise.")
        .def("save_flowsheet", &PyDyssol::SaveFlowsheet,
            nb::arg("file_path"),
            "Save the current flowsheet to a .dflw file.\n"
            "Args:\n"
            "    file_path (str): Path to save the .dflw file.\n"
            "Returns:\n"
            "    bool: True if successful, False otherwise.")
        .def("simulate", &PyDyssol::Simulate,
            nb::arg("end_time") = -1.0,
            "Run the simulation. Optionally override end time.\n"
            "Args:\n"
            "    end_time (float, optional): End time for simulation (seconds). Default: use flowsheet settings.")
        .def("initialize", &PyDyssol::Initialize,
            "Initialize the flowsheet for simulation.\n"
            "Returns:\n"            "    str: Empty string if successful, error message if failed.")
        .def("debug_flowsheet", &PyDyssol::DebugFlowsheet,
            "Print debug information about the current flowsheet, including units, streams, compounds, and phases.")

        // Compounds
        .def("get_compounds_mdb", &PyDyssol::GetDatabaseCompounds,
            "Returns list of (key, name) pairs from the loaded materials database.")
        .def("add_compound", &PyDyssol::AddCompound,
            nb::arg("key"),
            "Add a compound to the flowsheet by its unique key.")
        .def("get_compounds", &PyDyssol::GetCompounds,
            "Get list of (key, name) pairs of compounds defined in the flowsheet.")
        .def("set_compounds", &PyDyssol::SetCompounds,
            nb::arg("compounds"),
            "Set compounds in the flowsheet by list of keys or names.")

        //Phases
        .def("get_phases", &PyDyssol::GetPhases,
            "Return current list of phases as (name, state) pairs.")
        .def("set_phases", &PyDyssol::SetPhases,
            nb::arg("phases"),
            "Replace current phase list. Each item must be (state).")
        .def("add_phase", &PyDyssol::AddPhase, nb::arg("state"),
            "Add a phase by type only (e.g. 'solid').\n"
            "The name will be auto-generated like 'SOLID_1'.")

        //Units
        .def("get_units", &PyDyssol::GetUnits,
            "Get a list of all units in the flowsheet.\n"
            "Returns:\n"
            "    list[tuple[str, str]]: List of (unit_name, model_name) pairs.")
        .def("get_units_dict", &PyDyssol::GetUnitsDict,
            "Get a dictionary of all units: {unit_name: model_name}")

        //Parameters
        .def("get_unit_parameter", &PyDyssol::GetUnitParameter,
            nb::arg("unit_name"), nb::arg("param_name"),
            "Get a specific parameter from a unit.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "    param_name (str): Name of the parameter.\n"
            "Returns:\n"
            "    The parameter value (type depends on parameter).")

        .def("get_unit_parameters", &PyDyssol::GetUnitParameters,
            nb::arg("unit_name"),
            "Get all active parameters of a unit (according to current model selection).\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "Returns:\n"
            "    dict[str, (value, type, unit)]: Parameters with values, types, and units.")
        .def("get_unit_parameters_all", &PyDyssol::GetUnitParametersAll,
            nb::arg("unit_name"),
            "Get all parameters (including inactive ones) defined in the unit.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "Returns:\n"
            "    dict[str, (value, type, unit)]: All parameters regardless of model selection.")

        .def("set_unit_parameter", &PyDyssol::SetUnitParameter,
            nb::arg("unitName"), nb::arg("paramName"), nb::arg("value"),
            "Set a unit parameter for the specified unit.\n"
            "Args:\n"
            "    unitName (str): Name of the unit.\n"
            "    paramName (str): Name of the parameter.\n"
            "    value: Value to set (can be float, int, str, bool, list, etc.).")

        .def("get_combo_options", &PyDyssol::GetComboOptions,
            nb::arg("unit_name"), nb::arg("param_name"),
            "Get all available combo box options for a given combo parameter.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "    param_name (str): Name of the parameter.\n"
            "Returns:\n"
            "    list[str]: List of valid options for this combo parameter.")

        .def("get_dependent_parameters", &PyDyssol::GetDependentParameters,
            nb::arg("unit_name"),
            "Get all TIME_DEPENDENT and PARAM_DEPENDENT parameters of a unit.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "Returns:\n"
            "    dict[str, list[tuple[float, float]]]: Mapping of parameter names to (x, y) value pairs.")
        .def("get_dependent_parameter_values", &PyDyssol::GetDependentParameterValues,
            nb::arg("unit_name"), nb::arg("param_name"),
            "Get the (x, y) value pairs of a TIME_DEPENDENT or PARAM_DEPENDENT parameter.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "    param_name (str): Name of the dependent parameter.\n"
            "Returns:\n"
            "    list[tuple[float, float]]: (independent, dependent) value pairs.")

        //Holdups
        .def("get_unit_holdups", &PyDyssol::GetUnitHoldups,
            nb::arg("unit_name"),
            "Get a list of holdup names defined in the given unit.")

        .def("get_unit_holdup_overall",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldupOverall, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Get overall mass, temperature, and pressure of the default holdup at a specific time.")
        .def("get_unit_holdup_composition",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldupComposition, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Get compound-phase composition of the default holdup at a specific time.")
        .def("get_unit_holdup_distribution",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldupDistribution, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Return solid-phase size distributions per compound in the default holdup.")
        .def("get_unit_holdup",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldup, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Get all data (overall, composition, distributions) of the default holdup at a specific time.")

        .def("set_unit_holdup",
            nb::overload_cast<const std::string&, const nb::dict&>(&PyDyssol::SetUnitHoldup),
            nb::arg("unit_name"), nb::arg("holdup_dict"),
            "Set the default holdup of a unit at time t = 0.0 using a dictionary with fields 'overall', 'composition', and 'distributions'.")

        //Holdups with holdupName
        .def("get_unit_holdup_overall",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldupOverall, nb::const_),
            nb::arg("unit_name"), nb::arg("holdup_name"), nb::arg("time"),
            "Get overall mass, temperature, and pressure of a specific named holdup at a given time.")
        .def("get_unit_holdup_composition",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldupComposition, nb::const_),
            nb::arg("unit_name"), nb::arg("holdup_name"), nb::arg("time"),
            "Get compound-phase composition of a specific named holdup at a given time.")
        .def("get_unit_holdup_distribution",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldupDistribution, nb::const_),
            nb::arg("unit_name"), nb::arg("holdup_name"), nb::arg("time"),
            "Get size distribution of a specific named holdup at a given time.")
        .def("get_unit_holdup",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldup, nb::const_),
            nb::arg("unit_name"), nb::arg("holdup_name"), nb::arg("time"),
            "Get all data (overall, composition, distributions) from a specific named holdup at a given time.")

        .def("set_unit_holdup",
            nb::overload_cast<const std::string&, const std::string&, const nb::dict&>(&PyDyssol::SetUnitHoldup),
            nb::arg("unit_name"), nb::arg("holdup_name"), nb::arg("holdup_dict"),
            "Set a specific named holdup of a unit at t = 0.0 using a dictionary.")

        //Holdups without timepoint, but with holdupName
        .def("get_unit_holdup_overall",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitHoldupOverall),
            nb::arg("unit_name"), nb::arg("holdup_name"),
            "Get overall properties (mass, temp, pressure) over all timepoints for a named holdup.")
        .def("get_unit_holdup_composition",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitHoldupComposition),
            nb::arg("unit_name"), nb::arg("holdup_name"),
            "Get compound-phase composition over time for a named holdup.")
        .def("get_unit_holdup_distribution",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitHoldupDistribution),
            nb::arg("unit_name"), nb::arg("holdup_name"),
            "Get distribution data over time for a named holdup.")
        .def("get_unit_holdup",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitHoldup),
            nb::arg("unit_name"), nb::arg("holdup_name"),
            "Get full time-series data (overall, composition, distributions) for a named holdup.")

        //Holdups without timepoint, no holdupName
        .def("get_unit_holdup_overall",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitHoldupOverall),
            nb::arg("unit_name"),
            "Get overall holdup data (mass, temperature, pressure) over all timepoints for the default holdup.")
        .def("get_unit_holdup_composition",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitHoldupComposition),
            nb::arg("unit_name"),
            "Get compound-phase composition over all timepoints for the default holdup.")
        .def("get_unit_holdup_distribution",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitHoldupDistribution),
            nb::arg("unit_name"),
            "Get distribution vectors over all timepoints for the default holdup.")
        .def("get_unit_holdup",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitHoldup),
            nb::arg("unit_name"),
            "Get full time-series holdup data (overall, composition, distributions) for the default holdup.")

        // Feeds
        .def("get_unit_feeds", &PyDyssol::GetUnitFeeds,
            nb::arg("unit_name"),
            "Get a list of feed names defined for the given unit.")

        .def("set_unit_feed",
            nb::overload_cast<const std::string&, const std::string&, double, const nb::dict&>(&PyDyssol::SetUnitFeed),
            nb::arg("unit_name"), nb::arg("feed_name"), nb::arg("time"), nb::arg("data"),
            "Set feed data for a unit's named feed at a specific time.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "    feed_name (str): Name of the feed stream.\n"
            "    time (float): Time [s] to apply the values.\n"
            "    data (dict): Dictionary with keys 'overall', 'composition', and/or 'distributions'.")
        .def("set_unit_feed",
            nb::overload_cast<const std::string&, const nb::dict&>(&PyDyssol::SetUnitFeed),
            nb::arg("unit_name"), nb::arg("data"),
            "Set the first feed of a unit at t=0.0 with a data dictionary.")
        .def("set_unit_feed",
            nb::overload_cast<const std::string&, double, const nb::dict&>(&PyDyssol::SetUnitFeed),
            nb::arg("unit_name"), nb::arg("time"), nb::arg("data"),
            "Set the first feed of a unit at the given time.")
        .def("set_unit_feed",
            nb::overload_cast<const std::string&, const std::string&, const nb::dict&>(&PyDyssol::SetUnitFeed),
            nb::arg("unit_name"), nb::arg("feed_name"), nb::arg("data"),
            "Set a named feed of a unit at t=0.0 with a data dictionary.")


        .def("get_unit_feed_overall",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeedOverall, nb::const_),
            nb::arg("unit_name"), nb::arg("feed_name"), nb::arg("time"),
            "Get overall properties of a feed in a unit at a specific time.")
        .def("get_unit_feed_composition",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeedComposition, nb::const_),
            nb::arg("unit_name"), nb::arg("feed_name"), nb::arg("time"),
            "Get composition of a feed in a unit at a specific time.")
        .def("get_unit_feed_distribution",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeedDistribution, nb::const_),
            nb::arg("unit_name"), nb::arg("feed_name"), nb::arg("time"),
            "Get distributions of a feed in a unit at a specific time.")
        .def("get_unit_feed",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeed, nb::const_),
            nb::arg("unit_name"), nb::arg("feed_name"), nb::arg("time"),
            "Get all data of a feed in a unit at a specific time.")

        // Feeds without feed name
        .def("get_unit_feed_overall",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeedOverall, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Get overall properties of the first feed in a unit at a specific time.")
        .def("get_unit_feed_composition",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeedComposition, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Get composition of the first feed in a unit at a specific time.")
        .def("get_unit_feed_distribution",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeedDistribution, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Get distributions of the first feed in a unit at a specific time.")
        .def("get_unit_feed",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeed, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Get all data of the first feed in a unit at a specific time.")


        // Feeds with explicit feed name and no timepoints
        .def("get_unit_feed_overall",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitFeedOverall),
            nb::arg("unit_name"), nb::arg("feed_name"),
            "Get overall properties (massflow, temperature, pressure) for all timepoints of a given feed.\n"
            "Returns a dict with 'timepoints', 'massflow', 'temperature', and 'pressure'.")
        .def("get_unit_feed_composition",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitFeedComposition),
            nb::arg("unit_name"), nb::arg("feed_name"),
            "Get composition data for all timepoints of a given feed.\n"
            "Returns a dict with 'timepoints' and compound-phase keys mapping to lists of mass values.")
        .def("get_unit_feed_distribution",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitFeedDistribution),
            nb::arg("unit_name"), nb::arg("feed_name"),
            "Get distribution vectors for all timepoints of a given feed.\n"
            "Returns a dict with 'timepoints' and distribution names as keys.")
        .def("get_unit_feed",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitFeed),
            nb::arg("unit_name"), nb::arg("feed_name"),
            "Get all time-series data (overall, composition, distributions) for a named feed.\n"
            "Returns a dict with keys 'overall', 'composition', and 'distributions'.")


        //Feeds without timepoints
        .def("get_unit_feed_overall",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitFeedOverall),
            nb::arg("unit_name"),
            "Get overall feed data (massflow, temperature, pressure) for all timepoints of the first feed in a unit.")
        .def("get_unit_feed_composition",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitFeedComposition),
            nb::arg("unit_name"),
            "Get composition (mass per compound-phase) over all timepoints of the first feed in a unit.")
        .def("get_unit_feed_distribution",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitFeedDistribution),
            nb::arg("unit_name"),
            "Get distributions (e.g. particle sizes) over all timepoints of the first feed in a unit.")
        .def("get_unit_feed",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitFeed),
            nb::arg("unit_name"),
            "Get full time-series feed data (overall, composition, distributions) for the first feed in a unit.")


        // Unit Streams
        .def("get_unit_streams", &PyDyssol::GetUnitStreams,
            nb::arg("unit_name"),
            "Get names of all internal (work) streams of a unit.")
        .def("get_unit_stream_overall",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStreamOverall, nb::const_),
            nb::arg("unit_name"), nb::arg("stream_name"), nb::arg("time"),
            "Get overall properties of a stream inside a unit at a specific time.")
        .def("get_unit_stream_composition",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStreamComposition, nb::const_),
            nb::arg("unit_name"), nb::arg("stream_name"), nb::arg("time"),
            "Get compound-phase composition of a stream inside a unit at a specific time.")
        .def("get_unit_stream_distribution",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStreamDistribution, nb::const_),
            nb::arg("unit_name"), nb::arg("stream_name"), nb::arg("time"),
            "Get solid-phase distributions of a stream inside a unit at a specific time.")
        .def("get_unit_stream",
            nb::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStream, nb::const_),
            nb::arg("unit_name"), nb::arg("stream_name"), nb::arg("time"),
            "Get all data (overall, composition, distributions) of a stream inside a unit at a specific time.")

        // Unit Streams without Stream Name

        .def("get_unit_stream_overall",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStreamOverall, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Get overall properties of the first stream inside a unit at a specific time.")
        .def("get_unit_stream_composition",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStreamComposition, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Get compound-phase composition of the first stream inside a unit at a specific time.")
        .def("get_unit_stream_distribution",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStreamDistribution, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Get solid-phase distributions of the first stream inside a unit at a specific time.")
        .def("get_unit_stream",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStream, nb::const_),
            nb::arg("unit_name"), nb::arg("time"),
            "Get all data of the first stream inside a unit at a specific time.")

        //Unit Streams with explicit stream name and no timepoints
        .def("get_unit_stream_overall",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitStreamOverall),
            nb::arg("unit_name"), nb::arg("stream_name"),
            "Get time-series overall properties (massflow, temperature, pressure) for a named stream inside a unit.")
        .def("get_unit_stream_composition",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitStreamComposition),
            nb::arg("unit_name"), nb::arg("stream_name"),
            "Get time-series compound-phase composition for a named internal stream.")
        .def("get_unit_stream_distribution",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitStreamDistribution),
            nb::arg("unit_name"), nb::arg("stream_name"),
            "Get time-series distributions for a named stream inside a unit.")
        .def("get_unit_stream",
            nb::overload_cast<const std::string&, const std::string&>(&PyDyssol::GetUnitStream),
            nb::arg("unit_name"), nb::arg("stream_name"),
            "Get full time-series data (overall, composition, distributions) for a named stream inside a unit.")

        //Unit Streams without timepoints
        .def("get_unit_stream_overall",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitStreamOverall),
            nb::arg("unit_name"),
            "Get overall stream data (massflow, temperature, pressure) over all timepoints for the first stream in a unit.")
        .def("get_unit_stream_composition",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitStreamComposition),
            nb::arg("unit_name"),
            "Get compound-phase composition over all timepoints for the first stream in a unit.")
        .def("get_unit_stream_distribution",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitStreamDistribution),
            nb::arg("unit_name"),
            "Get distribution vectors over all timepoints for the first stream in a unit.")
        .def("get_unit_stream",
            nb::overload_cast<const std::string&>(&PyDyssol::GetUnitStream),
            nb::arg("unit_name"),
            "Get all stream data (overall, composition, distributions) over all timepoints for the first stream in a unit.")

        // Streams
        .def("get_streams", &PyDyssol::GetStreams, "Return list of all flowsheet-level stream names.")

        .def("get_stream_overall",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetStreamOverall, nb::const_),
            nb::arg("stream_name"), nb::arg("time"),
            "Get overall stream data at a specific time.")
        .def("get_stream_composition",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetStreamComposition, nb::const_),
            nb::arg("stream_name"), nb::arg("time"),
            "Get stream composition at a specific time.")
        .def("get_stream_distribution",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetStreamDistribution, nb::const_),
            nb::arg("stream_name"), nb::arg("time"),
            "Get stream distribution at a specific time.")
        .def("get_stream",
            nb::overload_cast<const std::string&, double>(&PyDyssol::GetStream, nb::const_),
            nb::arg("stream_name"), nb::arg("time"),
            "Get all stream data (overall, composition, distributions) at a given time.")


        //Streams without timepoints
        .def("get_stream_overall",
            nb::overload_cast<const std::string&>(&PyDyssol::GetStreamOverall),
            nb::arg("stream_name"),
            "Get overall stream data for all timepoints.")
        .def("get_stream_composition",
            nb::overload_cast<const std::string&>(&PyDyssol::GetStreamComposition),
            nb::arg("stream_name"),
            "Get stream composition for all timepoints.")
        .def("get_stream_distribution",
            nb::overload_cast<const std::string&>(&PyDyssol::GetStreamDistribution),
            nb::arg("stream_name"),
            "Get stream distribution for all timepoints.")

        .def("get_stream",
            nb::overload_cast<const std::string&>(&PyDyssol::GetStream),
            nb::arg("stream_name"),
            "Get all stream data (overall, composition, distributions) for all timepoints.")

        //Options
        .def("get_options", &PyDyssol::GetOptions, "Get flowsheet simulation options.")
        .def("set_options", &PyDyssol::SetOptions, nb::arg("options"), "Set flowsheet simulation options.")
        .def("get_options_methods", &PyDyssol::GetOptionsMethods,
            "Return valid string options for convergenceMethod and extrapolationMethod.")

        //Debugging
        .def("debug_unit_ports", &PyDyssol::DebugUnitPorts, nb::arg("unit_name"))
        .def("debug_stream_data", &PyDyssol::DebugStreamData, nb::arg("stream_name"), nb::arg("time"));

    nb::enum_<EPhase>(m, "EPhase")
        .value("SOLID", EPhase::SOLID)
        .value("LIQUID", EPhase::LIQUID)
        .value("VAPOR", EPhase::VAPOR)
        .export_values();
    /*
    m.def("pretty_print", [](const nb::dict& data) {
        const std::map<std::string, std::string> units = {
            {"mass", "kg"}, {"massflow", "kg/s"}, {"temperature", "K"}, {"pressure", "Pa"}
        };

        // Detect if dict is unit parameters format: values are tuples of (val, type, unit)
        bool is_unit_params = true;
        for (auto [key, value] : data) {
            if (!nb::isinstance<nb::tuple>(value) || nb::len(value) != 3) {
                is_unit_params = false;
                break;
            }
            auto tup = nb::cast<nb::tuple>(value);
            if (!nb::isinstance<nb::str>(tup[1]) || !nb::isinstance<nb::str>(tup[2])) {
                is_unit_params = false;
                break;
            }
        }

        if (is_unit_params) {
            nb::print("=== Unit Parameters ===");
            for (auto [key, value] : data) {
                auto key_str = nb::cast<std::string>(key);
                auto tup = nb::cast<nb::tuple>(value);
                auto val = tup[0];
                auto type = nb::cast<std::string>(tup[1]);
                auto unit = nb::cast<std::string>(tup[2]);
                if (unit.empty()) unit = "-";

                std::string repr;
                if (nb::isinstance<nb::float_>(val)) {
                    repr = std::to_string(nb::cast<double>(val));
                }
                else if (nb::isinstance<nb::int_>(val)) {
                    repr = std::to_string(nb::cast<int>(val));
                }
                else if (nb::isinstance<nb::bool_>(val)) {
                    repr = nb::cast<bool>(val) ? "True" : "False";
                }
                else if (nb::isinstance<nb::str>(val)) {
                    repr = nb::cast<std::string>(val);
                }
                else if (nb::isinstance<nb::list>(val) || nb::isinstance<nb::tuple>(val)) {
                    auto seq = nb::cast<nb::sequence>(val);
                    size_t len = nb::len(seq);
                    if (len <= 5) {
                        repr = "[";
                        for (size_t i = 0; i < len; ++i) {
                            if (i > 0) repr += ", ";
                            repr += nb::cast<std::string>(nb::str(nb::cast<nb::object>(seq[i])));
                        }
                        repr += "]";
                    }
                    else {
                        repr = "<list, length=" + std::to_string(len) + ">";
                    }
                }
                else {
                    repr = nb::cast<std::string>(nb::str(val));
                }

                nb::print(nb::str("{:<25} : {:<30} [{:<15}] ({})").format(key_str, repr, type, unit));
            }
            return;
        }

        // Case 1: Holdup-like structure
        if (data.contains("overall") && data.contains("composition") && data.contains("distributions")) {
            nb::print("=== Overall ===");
            for (auto [key, value] : nb::cast<nb::dict>(data["overall"])) {
                auto key_str = nb::cast<std::string>(key);
                auto val = nb::cast<double>(value);
                auto unit = units.count(key_str) ? units.at(key_str) : "";
                nb::print(nb::str("{:<25}: {:.4f} {}").format(key_str, val, unit));
            }

            std::string comp_unit = "kg";
            if (nb::cast<nb::dict>(data["overall"]).contains("massflow"))
                comp_unit = "kg/s";

            nb::print("\n=== Composition ===");
            for (auto [key, value] : nb::cast<nb::dict>(data["composition"])) {
                auto key_str = nb::cast<std::string>(key);
                auto val = nb::cast<double>(value);
                nb::print(nb::str("{:<25}: {:.4f} {}").format(key_str, val, comp_unit));
            }

            nb::print("\n=== Distributions ===");
            for (auto [key, value] : nb::cast<nb::dict>(data["distributions"])) {
                auto name = nb::cast<std::string>(key);
                auto dist = nb::cast<std::vector<double>>(value);
                nb::print(nb::str("\n{}:").format(name));
                for (double x : dist)
                    nb::print(nb::str("{:.4e}").format(x));
            }
            return;
        }

        // Case 2: Flat dictionary (e.g., from get_options())
        nb::print("=== Simulation Options ===");
        for (auto [key, value] : data) {
            auto key_str = nb::cast<std::string>(key);
            if (nb::isinstance<nb::bool_>(value)) {
                nb::print(nb::str("{:<25}: {}").format(key_str, nb::cast<bool>(value) ? "True" : "False"));
            }
            else if (nb::isinstance<nb::int_>(value)) {
                nb::print(nb::str("{:<25}: {}").format(key_str, nb::cast<int>(value)));
            }
            else if (nb::isinstance<nb::float_>(value)) {
                nb::print(nb::str("{:<25}: {}").format(key_str, nb::cast<double>(value)));
            }
            else if (nb::isinstance<nb::str>(value)) {
                nb::print(nb::str("{:<25}: {}").format(key_str, nb::cast<std::string>(value)));
            }
            else {
                nb::print(nb::str("{:<25}: [unhandled type]").format(key_str));
            }
        }
        });*/
}