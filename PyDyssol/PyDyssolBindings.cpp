#include "PyDyssol.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

namespace py = pybind11;

namespace {
    EPhase ConvertPhaseState(const py::object& state) {
        if (py::isinstance<py::str>(state)) {
            std::string state_str = py::cast<std::string>(state);
            if (state_str == "solid") return EPhase::SOLID;
            if (state_str == "liquid") return EPhase::LIQUID;
            if (state_str == "vapor") return EPhase::VAPOR;
            throw std::invalid_argument("Invalid phase state: " + state_str + ". Must be 'solid', 'liquid', or 'vapor'.");
        }
        else if (py::isinstance<py::int_>(state)) {
            int state_int = py::cast<int>(state);
            if (state_int < 0 || state_int > 2) {
                throw std::invalid_argument("Invalid phase state integer: " + std::to_string(state_int) + ". Must be 0 (SOLID), 1 (LIQUID), or 2 (VAPOR).");
            }
            return static_cast<EPhase>(state_int);
        }
        else if (py::isinstance<EPhase>(state)) {  // Check if the object is an EPhase enum
            return py::cast<EPhase>(state);
        }
        throw std::invalid_argument("Phase state must be a string ('solid', 'liquid', 'vapor'), an integer (0, 1, 2), or an EPhase value.");
    }
}

PYBIND11_MODULE(PyDyssol, m) {
    // Expose EPhase enum
    py::enum_<EPhase>(m, "EPhase", "Enumeration of phase states")
        .value("SOLID", EPhase::SOLID)
        .value("LIQUID", EPhase::LIQUID)
        .value("VAPOR", EPhase::VAPOR)
        .export_values();

    // Bind PyDyssol class
    py::class_<PyDyssol>(m, "PyDyssol", "A class to manage Dyssol flowsheet simulations in Python")
        .def(py::init<std::string, std::string>(),
            py::arg("materials_path") = "D:/Dyssol/Materials.dmdb",
            py::arg("models_path") = "C:/Program Files/Dyssol/Units",
            "Initialize a new PyDyssol instance with optional paths for materials database and model units.\n"
            "Args:\n"
            "    materials_path (str): Path to the materials database (.dmdb file). Default: 'D:/Dyssol/Materials.dmdb'.\n"
            "    models_path (str): Path to the directory containing model units. Default: 'C:/Program Files/Dyssol/Units'.")
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
        .def("initialize", &PyDyssol::Initialize,
            "Initialize the flowsheet for simulation.\n"
            "Returns:\n"
            "    str: Empty string if successful, error message if failed.")
        .def("debug_flowsheet", &PyDyssol::DebugFlowsheet,
            "Print debug information about the current flowsheet, including units, streams, compounds, and phases.")
        .def("get_database_compounds", &PyDyssol::GetDatabaseCompounds,
            "Returns list of (key, name) pairs from the loaded materials database.")
        .def("get_compounds", &PyDyssol::GetCompounds,
            "Returns list of (key, name) pairs of compounds defined in the flowsheet.")
        .def("set_compounds", &PyDyssol::SetCompounds,
            py::arg("compounds"),
            "Set the compounds for the flowsheet.\n"
            "Args:\n"
            "    compounds (list[str]): List of compound keys or names.\n"
            "Returns:\n"
            "    bool: True if successful, False otherwise.")
        .def("setup_phases",
            [](PyDyssol& self, const py::list& phases) {
                std::vector<std::pair<std::string, int>> phase_descriptors;
                for (const auto& phase : phases) {
                    auto tuple = phase.cast<py::tuple>();
                    if (tuple.size() != 2) {
                        throw std::invalid_argument("Each phase must be a tuple of (name, state).");
                    }
                    std::string name = tuple[0].cast<std::string>();
                    EPhase state = ConvertPhaseState(tuple[1]);
                    phase_descriptors.emplace_back(name, static_cast<int>(state));
                }
                return self.SetupPhases(phase_descriptors);
            },
            py::arg("phases"),
            "Set the phases for the flowsheet.\n"
            "Args:\n"
            "    phases (list[tuple[str, str|int|EPhase]]): List of (name, state) tuples, where state can be a string ('solid', 'liquid', 'vapor'), an integer (0, 1, 2), or an EPhase value.\n"
            "Returns:\n"
            "    bool: True if successful, False otherwise.\n"
            "Example:\n"
            "    pydyssol.setup_phases([('Liquid', 'liquid'), ('Vapor', EPhase.VAPOR)])")
        .def("get_units", &PyDyssol::GetUnits,
            "Get a list of all units in the flowsheet.\n"
            "Returns:\n"
            "    list[tuple[str, str]]: List of (unit_name, model_name) pairs.")
        .def("get_units_dict", &PyDyssol::GetUnitsDict,
            "Get a dictionary of all units: {unit_name: model_name}")

        //Parameters
            .def("get_unit_parameter", &PyDyssol::GetUnitParameter)
            .def("get_unit_parameters", &PyDyssol::GetUnitParameters)
            .def("get_unit_parameters_all", &PyDyssol::GetUnitParametersAll)

            .def("set_unit_parameter", &PyDyssol::SetUnitParameter,
            py::arg("unitName"), py::arg("paramName"), py::arg("value"),
            "Set a unit parameter for the specified unit.\n"
            "Args:\n"
            "    unitName (str): Name of the unit.\n"
            "    paramName (str): Name of the parameter.\n"
            "    value: Value to set (can be float, int, str, bool, list, etc.).")
        .def("get_combo_options", &PyDyssol::GetComboOptions,
            "Returns list of available combo options for given parameter")
        .def("get_dependent_parameter_values", &PyDyssol::GetDependentParameterValues, py::arg("unit_name"), py::arg("param_name"),
            "Get the values of a TIME_DEPENDENT or PARAM_DEPENDENT parameter.\n"
            "Args:\n"
            "    unit_name (str): Name of the unit.\n"
            "    param_name (str): Name of the parameter.\n"
            "Returns:\n"
            "    list[tuple[float, float]]: List of (independent_var, value) pairs.")
        .def("get_dependent_parameters", &PyDyssol::GetDependentParameters)
   
        //Holdups
        .def("get_unit_holdups", &PyDyssol::GetUnitHoldups,
        py::arg("unit_name"),
        "Returns a list of holdup names defined in the given unit.")
        .def("get_unit_holdup_overall",
        py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldupOverall, py::const_),
        py::arg("unit_name"), py::arg("time"))

        .def("get_unit_holdup_composition",
        py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldupComposition, py::const_),
        py::arg("unit_name"), py::arg("time"))

        .def("get_unit_holdup_distribution",
        py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldupDistribution, py::const_),
        py::arg("unit_name"), py::arg("time"),
        "Return solid-phase size distributions per compound in the default holdup.")
        .def("get_unit_holdup",
        py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitHoldup, py::const_),
        py::arg("unit_name"), py::arg("time"),
        "Get the overall, composition, and distributions of the default holdup at a specific time.")
        .def("set_unit_holdup",
                py::overload_cast<const std::string&, const py::dict&>(&PyDyssol::SetUnitHoldup),
                py::arg("unit_name"), py::arg("holdup_dict"),
                "Set the default holdup of a unit at t=0.0")

		//Holdups with holdupName
         .def("get_unit_holdup_overall",
                py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldupOverall, py::const_),
                py::arg("unit_name"), py::arg("holdup_name"), py::arg("time"))

         .def("get_unit_holdup_composition",
                py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldupComposition, py::const_),
                py::arg("unit_name"), py::arg("holdup_name"), py::arg("time"))

          .def("get_unit_holdup_distribution",
                py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldupDistribution, py::const_),
                py::arg("unit_name"), py::arg("holdup_name"), py::arg("time"))

         .def("get_unit_holdup",
                py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitHoldup, py::const_),
                py::arg("unit_name"), py::arg("holdup_name"), py::arg("time"),
                "Get holdup content (overall, composition, distributions) from a specific named holdup at given time.")

        .def("set_unit_holdup",
                py::overload_cast<const std::string&, const std::string&, const py::dict&>(&PyDyssol::SetUnitHoldup),
                py::arg("unit_name"), py::arg("holdup_name"), py::arg("holdup_dict"),
                "Set a specific named holdup of a unit at t=0.0")
        //Holdups without timepoint
            .def("get_unit_holdup_overall",
                &PyDyssol::GetUnitHoldupOverallAllTimes,
                py::arg("unit_name"),
                "Get overall holdup data for all timepoints for the default holdup in a unit.")

            .def("get_unit_holdup_composition",
                &PyDyssol::GetUnitHoldupCompositionAllTimes,
                py::arg("unit_name"),
                "Get composition holdup data for all timepoints for the default holdup in a unit.")

            .def("get_unit_holdup_distribution",
                &PyDyssol::GetUnitHoldupDistributionAllTimes,
                py::arg("unit_name"),
                "Get distributions holdup data for all timepoints for the default holdup in a unit.")

            .def("get_unit_holdup",
                &PyDyssol::GetUnitHoldupAllTimes,
                py::arg("unit_name"),
                "Get all holdup data (overall, composition, distributions) for all timepoints for the default holdup in a unit.")

			// Feeds
        .def("get_unit_feeds", &PyDyssol::GetUnitFeeds,
            py::arg("unit_name"),
            "Get a list of feed names defined for the given unit.")

        .def("get_unit_feed_overall",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeedOverall, py::const_),
            py::arg("unit_name"), py::arg("feed_name"), py::arg("time"),
            "Get overall properties of a feed in a unit at a specific time.")
        .def("get_unit_feed_overall",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeedOverall, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get overall properties of the first feed in a unit at a specific time.")

        .def("get_unit_feed_composition",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeedComposition, py::const_),
            py::arg("unit_name"), py::arg("feed_name"), py::arg("time"),
            "Get composition of a feed in a unit at a specific time.")
        .def("get_unit_feed_composition",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeedComposition, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get composition of the first feed in a unit at a specific time.")

        .def("get_unit_feed_distribution",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeedDistribution, py::const_),
            py::arg("unit_name"), py::arg("feed_name"), py::arg("time"),
            "Get distributions of a feed in a unit at a specific time.")
        .def("get_unit_feed_distribution",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeedDistribution, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get distributions of the first feed in a unit at a specific time.")

        .def("get_unit_feed",
            py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitFeed, py::const_),
            py::arg("unit_name"), py::arg("feed_name"), py::arg("time"),
            "Get all data of a feed in a unit at a specific time.")
        .def("get_unit_feed",
            py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitFeed, py::const_),
            py::arg("unit_name"), py::arg("time"),
            "Get all data of the first feed in a unit at a specific time.")
        //Feeds without timepoints
            .def("get_unit_feed_overall",
                &PyDyssol::GetUnitFeedOverallAllTimes,
                py::arg("unit_name"),
                "Get overall feed data for all timepoints for the first feed in a unit.")

            .def("get_unit_feed_composition",
                &PyDyssol::GetUnitFeedCompositionAllTimes,
                py::arg("unit_name"),
                "Get composition feed data for all timepoints for the first feed in a unit.")

            .def("get_unit_feed_distribution",
                &PyDyssol::GetUnitFeedDistributionAllTimes,
                py::arg("unit_name"),
                "Get distributions feed data for all timepoints for the first feed in a unit.")

            .def("get_unit_feed",
                &PyDyssol::GetUnitFeedAllTimes,
                py::arg("unit_name"),
                "Get all feed data (overall, composition, distributions) for all timepoints for the first feed in a unit.")



         // Unit Streams
        .def("get_unit_streams", &PyDyssol::GetUnitStreams,
                py::arg("unit_name"),
                "Get names of all internal (work) streams of a unit.")
         .def("get_unit_stream_overall",
                py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStreamOverall, py::const_),
                py::arg("unit_name"), py::arg("stream_name"), py::arg("time"),
                "Get overall properties of a stream inside a unit at a specific time.")
         .def("get_unit_stream_overall",
                py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStreamOverall, py::const_),
                py::arg("unit_name"), py::arg("time"),
                "Get overall properties of the first stream inside a unit at a specific time.")

         .def("get_unit_stream_composition",
                py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStreamComposition, py::const_),
                py::arg("unit_name"), py::arg("stream_name"), py::arg("time"),
                "Get compound-phase composition of a stream inside a unit at a specific time.")
         .def("get_unit_stream_composition",
                py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStreamComposition, py::const_),
                py::arg("unit_name"), py::arg("time"),
                "Get compound-phase composition of the first stream inside a unit at a specific time.")

         .def("get_unit_stream_distribution",
                py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStreamDistribution, py::const_),
                py::arg("unit_name"), py::arg("stream_name"), py::arg("time"),
                "Get solid-phase distributions of a stream inside a unit at a specific time.")
         .def("get_unit_stream_distribution",
                py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStreamDistribution, py::const_),
                py::arg("unit_name"), py::arg("time"),
                "Get solid-phase distributions of the first stream inside a unit at a specific time.")

         .def("get_unit_stream",
                py::overload_cast<const std::string&, const std::string&, double>(&PyDyssol::GetUnitStream, py::const_),
                py::arg("unit_name"), py::arg("stream_name"), py::arg("time"),
                "Get all data (overall, composition, distributions) of a stream inside a unit at a specific time.")
         .def("get_unit_stream",
                py::overload_cast<const std::string&, double>(&PyDyssol::GetUnitStream, py::const_),
                py::arg("unit_name"), py::arg("time"),
                "Get all data of the first stream inside a unit at a specific time.")
        //Unit Streams without timepoints
        .def("get_unit_stream_overall",
        &PyDyssol::GetUnitStreamOverallAllTimes,
        py::arg("unit_name"),
        "Get overall stream data for all timepoints for the first stream in a unit.")

        .def("get_unit_stream_composition",
        &PyDyssol::GetUnitStreamCompositionAllTimes,
        py::arg("unit_name"),
        "Get composition stream data for all timepoints for the first stream in a unit.")

        .def("get_unit_stream_distribution",
        &PyDyssol::GetUnitStreamDistributionAllTimes,
        py::arg("unit_name"),
        "Get distributions stream data for all timepoints for the first stream in a unit.")

        .def("get_unit_stream",
        &PyDyssol::GetUnitStreamAllTimes,
        py::arg("unit_name"),
        "Get all stream data (overall, composition, distributions) for all timepoints for the first stream in a unit.")


         // Streams
        .def("get_stream_overall", &PyDyssol::GetStreamOverall,
                py::arg("stream_name"), py::arg("time"),
                "Get overall properties of a flowsheet-level stream.")
        .def("get_stream_composition", &PyDyssol::GetStreamComposition,
                py::arg("stream_name"), py::arg("time"),
                "Get compound-phase composition of a flowsheet-level stream.")
        .def("get_stream_distribution", &PyDyssol::GetStreamDistribution,
                py::arg("stream_name"), py::arg("time"),
                "Get size distributions of a flowsheet-level stream.")
        .def("get_stream", &PyDyssol::GetStream,
                py::arg("stream_name"), py::arg("time"),
                "Get all stream data (overall, composition, distributions) at a given time.")
        .def("get_streams", &PyDyssol::GetStreams, "Return list of all flowsheet-level stream names.")
        //Streams without timepoints
        .def("get_stream_overall",
        &PyDyssol::GetStreamOverallAllTimes,
        py::arg("stream_name"),
        "Get overall stream data for all timepoints.")

        .def("get_stream_composition",
        &PyDyssol::GetStreamCompositionAllTimes,
        py::arg("stream_name"),
        "Get composition stream data for all timepoints.")

        .def("get_stream_distribution",
        &PyDyssol::GetStreamDistributionAllTimes,
        py::arg("stream_name"),
        "Get distributions stream data for all timepoints.")

        .def("get_stream",
        &PyDyssol::GetStreamAllTimes,
        py::arg("stream_name"),
        "Get all stream data (overall, composition, distributions) for all timepoints.")

        //Options
        .def("get_options", &PyDyssol::GetOptions, "Get flowsheet simulation options.")
        .def("set_options", &PyDyssol::SetOptions, py::arg("options"), "Set flowsheet simulation options.")
        .def("get_options_methods", &PyDyssol::GetOptionsMethods,
                "Return valid string options for convergenceMethod and extrapolationMethod.")


        .def("debug_unit_ports", &PyDyssol::DebugUnitPorts, py::arg("unit_name"))
        .def("debug_stream_data", &PyDyssol::DebugStreamData, py::arg("stream_name"), py::arg("time"));

        
    // Register exceptions
    py::register_exception<std::runtime_error>(m, "RuntimeError");
    py::register_exception<std::invalid_argument>(m, "ValueError");


	//Pretty print function
    m.def("pretty_print", [](const py::dict& data) {
        const std::map<std::string, std::string> units = {
            {"mass", "kg"}, {"massflow", "kg/s"}, {"temperature", "K"}, {"pressure", "Pa"}
        };

        // Detect if dict is unit parameters format: values are tuples of (val, type, unit)
        bool is_unit_params = true;
        for (auto item : data) {
            if (!py::isinstance<py::tuple>(item.second)) {
                is_unit_params = false;
                break;
            }
            py::tuple tup = item.second.cast<py::tuple>();
            if (tup.size() != 3 || !py::isinstance<py::str>(tup[1]) || !py::isinstance<py::str>(tup[2])) {
                is_unit_params = false;
                break;
            }
        }

        if (is_unit_params) {
            py::print("=== Unit Parameters ===");
            for (auto item : data) {
                std::string key = item.first.cast<std::string>();
                py::tuple tup = item.second.cast<py::tuple>();

                py::object val = tup[0];
                std::string type = tup[1].cast<std::string>();
                std::string unit = tup[2].cast<std::string>();
                if (unit.empty()) unit = "-";

                // Prepare value representation string
                std::string repr;
                if (py::isinstance<py::float_>(val)) {
                    double d = val.cast<double>();
                    repr = std::to_string(d);
                }
                else if (py::isinstance<py::int_>(val)) {   // new: handle integers
                    int i = val.cast<int>();
                    repr = std::to_string(i);
                }
                else if (py::isinstance<py::bool_>(val)) {  // new: handle bools
                    bool b = val.cast<bool>();
                    repr = b ? "True" : "False";
                }
                else if (py::isinstance<py::str>(val)) {
                    repr = val.cast<std::string>();
                }
                else if (py::isinstance<py::list>(val) || py::isinstance<py::tuple>(val)) {
                    py::sequence seq = val.cast<py::sequence>();
                    size_t len = seq.size();
                    if (len <= 5) {
                        repr = "[";
                        for (size_t i = 0; i < len; ++i) {
                            if (i > 0) repr += ", ";
                            repr += py::str(seq[i]).cast<std::string>();
                        }
                        repr += "]";
                    }
                    else {
                        repr = "<list, length=" + std::to_string(len) + ">";
                    }
                }
                else {
                    // fallback to Python string representation for unknown types
                    repr = py::str(val).cast<std::string>();
                }

                py::print(py::str("{:<25} : {:<30} [{:<15}] ({})").format(key, repr, type, unit));
            }
            return;
        }

        // Case 1: Holdup-like structure
        if (data.contains("overall") && data.contains("composition") && data.contains("distributions")) {
            py::print("=== Overall ===");
            for (const auto& item : data["overall"].cast<py::dict>()) {
                std::string key = item.first.cast<std::string>();
                double val = item.second.cast<double>();
                std::string unit = units.count(key) ? units.at(key) : "";
                py::print(py::str("{:<25}: {:.4f} {}").format(key, val, unit));
            }

            std::string comp_unit = "kg";
            if (data["overall"].cast<py::dict>().contains("massflow"))
                comp_unit = "kg/s";

            py::print("\n=== Composition ===");
            for (const auto& item : data["composition"].cast<py::dict>()) {
                std::string key = item.first.cast<std::string>();
                double val = item.second.cast<double>();
                py::print(py::str("{:<25}: {:.4f} {}").format(key, val, comp_unit));
            }

            py::print("\n=== Distributions ===");
            for (const auto& item : data["distributions"].cast<py::dict>()) {
                std::string name = item.first.cast<std::string>();
                std::vector<double> dist = item.second.cast<std::vector<double>>();
                py::print("\n" + name + ":");
                for (double x : dist)
                    py::print(py::str("{:.4e}").format(x));
            }
            return;
        }

        // Case 2: Flat dictionary (e.g., from get_options())
        py::print("=== Simulation Options ===");
        for (const auto& item : data) {
            std::string key = item.first.cast<std::string>();

            if (py::isinstance<py::bool_>(item.second)) {
                bool val = item.second.cast<bool>();
                py::print(py::str("{:<25}: {}").format(key, val ? "True" : "False"));
            }
            else if (py::isinstance<py::int_>(item.second)) {
                int val = item.second.cast<int>();
                py::print(py::str("{:<25}: {}").format(key, val));
            }
            else if (py::isinstance<py::float_>(item.second)) {
                double val = item.second.cast<double>();
                py::print(py::str("{:<25}: {}").format(key, val));
            }
            else if (py::isinstance<py::str>(item.second)) {
                std::string val = item.second.cast<std::string>();
                py::print(py::str("{:<25}: {}").format(key, val));
            }
            else {
                py::print(py::str("{:<25}: [unhandled type]").format(key));
            }
        }
        });
}