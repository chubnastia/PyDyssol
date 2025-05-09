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

   /* py::class_<Holdup>(m, "Holdup")
        .def_readwrite("mass", &Holdup::mass)
        .def_readwrite("temperature", &Holdup::temperature)
        .def_readwrite("pressure", &Holdup::pressure)
        .def_readwrite("composition", &Holdup::composition)
        .def("__repr__", [](const Holdup& h) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3);
        oss << "mass:        " << h.mass << " kg\n"
            << "temperature: " << h.temperature << " K\n"
            << "pressure:    " << h.pressure << " Pa\n";

        for (const auto& comp : h.composition) {
            oss << comp.first << ": " << comp.second << " kg\n";
        }

        return oss.str();
            });*/

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
        .def("get_stream_mass_flow", &PyDyssol::GetStreamMassFlow,
            py::arg("stream_key"), py::arg("time"),
            "Get the mass flow of a stream at a given time.\n"
            "Args:\n"
            "    stream_key (str): Key of the stream.\n"
            "    time (float): Time point to query.\n"
            "Returns:\n"
            "    list[tuple[str, float]]: List of (phase_name, mass_flow) pairs.")
        .def("get_stream_composition", &PyDyssol::GetStreamComposition,
            py::arg("stream_key"), py::arg("time"),
            "Get the composition of a stream at a given time.\n"
            "Args:\n"
            "    stream_key (str): Key of the stream.\n"
            "    time (float): Time point to query.\n"
            "Returns:\n"
            "    list[tuple[str, float]]: List of (compound_key, mass_fraction) pairs.")
        .def("get_units", &PyDyssol::GetUnits,
            "Get a list of all units in the flowsheet.\n"
            "Returns:\n"
            "    list[tuple[str, str]]: List of (unit_name, model_name) pairs.")
        .def("get_units_dict", &PyDyssol::GetUnitsDict,
            "Get a dictionary of all units: {unit_name: model_name}")
        // File: PyDyssolBindings.cpp

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
   
        .def("get_unit_holdup_overall", &PyDyssol::GetUnitHoldupOverall)
        .def("get_unit_holdup_composition", &PyDyssol::GetUnitHoldupComposition)
        .def("get_unit_holdup_distribution", &PyDyssol::GetUnitHoldupDistribution,
                py::arg("unit_name"), py::arg("time"),
                "Return solid-phase size distributions per compound in the unit holdup.")
        .def("get_unit_holdup", &PyDyssol::GetUnitHoldup)
        .def("set_unit_holdup", &PyDyssol::SetUnitHoldup,
        py::arg("unit_name"), py::arg("time"), py::arg("holdup_dict"),
        "Set unit holdup using a combined dict with keys: mass, temperature, pressure, and composition.")

        .def("get_unit_feeds", &PyDyssol::GetUnitFeeds,
        py::arg("unit_name"),
        "Returns a list of feed names defined for the given unit.")
        .def("get_unit_feed_overall", &PyDyssol::GetUnitFeedOverall)
        .def("get_unit_feed_composition", &PyDyssol::GetUnitFeedComposition)
        .def("get_unit_feed_distribution", &PyDyssol::GetUnitFeedDistribution)

        .def("get_unit_feed", &PyDyssol::GetUnitFeed,
                py::arg("unit_name"), py::arg("feed_name"), py::arg("time"),
                "Get feed stream content from a specific unit and feed at a given time.")
        .def("set_unit_feed", &PyDyssol::SetUnitFeed,
                py::arg("unit_name"), py::arg("feed_name"), py::arg("time"), py::arg("feed_dict"),
                "Set feed stream content (overall, composition, distributions) for a specific unit and feed at a given time.")


        .def("debug_unit_ports", &PyDyssol::DebugUnitPorts, py::arg("unit_name"))
        .def("debug_stream_data", &PyDyssol::DebugStreamData, py::arg("stream_name"), py::arg("time"));


    // Register exceptions
    py::register_exception<std::runtime_error>(m, "RuntimeError");
    py::register_exception<std::invalid_argument>(m, "ValueError");
}