from enum import Enum
from typing import List, Tuple, Dict, Union, Any

class EPhase(Enum):
    SOLID = 0
    LIQUID = 1
    VAPOR = 2

class PyDyssol:
    def __init__(self, materials_path: str = "D:/Dyssol/Materials.dmdb", models_path: str = "C:/Program Files/Dyssol/Units") -> None:
        """Initialize a new PyDyssol instance with optional paths for materials database and model units."""
        ...

    def load_materials_database(self, path: str) -> bool:
        """Load a materials database from a .dmdb file.
        Args:
        path (str): Path to the .dmdb file.
        Returns:
        bool: True if successful, False otherwise.
        """
        ...

    def add_model_path(self, path: str) -> bool:
        """Add a directory containing model DLLs or shared libraries.
        Args:
        path (str): Path to the directory.
        Returns:
        bool: True if successful, False otherwise.
        """
        ...

    def open_flowsheet(self, file_path: str) -> bool:
        """Open a flowsheet from a .dflw file.
        Args:
        file_path (str): Path to the .dflw file.
        Returns:
        bool: True if successful, False otherwise.
        """
        ...

    def save_flowsheet(self, file_path: str) -> bool:
        """Save the current flowsheet to a .dflw file.
        Args:
        file_path (str): Path to save the .dflw file.
        Returns:
        bool: True if successful, False otherwise.
        """
        ...

    def simulate(self, end_time: float = -1.0) -> None:
        """Run the simulation. Optionally override end time.
        Args:
        end_time (float, optional): End time for simulation (seconds). Default: use flowsheet settings.
        """
        ...

    def initialize(self) -> str:
        """Initialize the flowsheet for simulation.
        Returns:
        str: Empty string if successful, error message if failed.
        """
        ...

    def debug_flowsheet(self) -> None:
        """Print debug information about the current flowsheet, including units, streams, compounds, and phases."""
        ...

    def get_database_compounds(self) -> List[Tuple[str, str]]:
        """Returns list of (key, name) pairs from the loaded materials database."""
        ...

    def get_compounds(self) -> List[Tuple[str, str]]:
        """Returns list of (key, name) pairs of compounds defined in the flowsheet."""
        ...

    def set_compounds(self, compounds: List[str]) -> bool:
        """Set the compounds for the flowsheet.
        Args:
        compounds (list[str]): List of compound keys or names.
        Returns:
        bool: True if successful, False otherwise.
        """
        ...

    def setup_phases(self, phases: List[Tuple[str, Union[str, int, EPhase]]]) -> bool:
        """Set the phases for the flowsheet.
        Args:
        phases (list[tuple[str, str|int|EPhase]]): List of (name, state) tuples, where state can be a string ('solid', 'liquid', 'vapor'), an integer (0, 1, 2), or an EPhase value.
        Returns:
        bool: True if successful, False otherwise.
        Example:
        pydyssol.setup_phases([('Liquid', 'liquid'), ('Vapor', EPhase.VAPOR)])
        """
        ...

    def get_units(self) -> List[Tuple[str, str]]:
        """Get a list of all units in the flowsheet.
        Returns:
        list[tuple[str, str]]: List of (unit_name, model_name) pairs.
        """
        ...

    def get_units_dict(self) -> Dict[str, str]:
        """Get a dictionary of all units: {unit_name: model_name}"""
        ...

    def get_unit_parameter(self, unit_name: str, param_name: str) -> Any:
        """Get a specific parameter from a unit.
        Args:
        unit_name (str): Name of the unit.
        param_name (str): Name of the parameter.
        Returns:
        The parameter value (type depends on parameter).
        """
        ...

    def get_unit_parameters(self, unit_name: str) -> Dict[str, Tuple[Any, str, str]]:
        """Get all active parameters of a unit (according to current model selection).
        Args:
        unit_name (str): Name of the unit.
        Returns:
        dict[str, (value, type, unit)]: Parameters with values, types, and units.
        """
        ...

    def get_unit_parameters_all(self, unit_name: str) -> Dict[str, Tuple[Any, str, str]]:
        """Get all parameters (including inactive ones) defined in the unit.
        Args:
        unit_name (str): Name of the unit.
        Returns:
        dict[str, (value, type, unit)]: All parameters regardless of model selection.
        """
        ...

    def set_unit_parameter(self, unitName: str, paramName: str, value: Any) -> None:
        """Set a unit parameter for the specified unit.
        Args:
        unitName (str): Name of the unit.
        paramName (str): Name of the parameter.
        value: Value to set (can be float, int, str, bool, list, etc.).
        """
        ...

    def get_combo_options(self, unit_name: str, param_name: str) -> List[str]:
        """Get all available combo box options for a given combo parameter.
        Args:
        unit_name (str): Name of the unit.
        param_name (str): Name of the parameter.
        Returns:
        list[str]: List of valid options for this combo parameter.
        """
        ...

    def get_dependent_parameters(self, unit_name: str) -> Dict[str, List[Tuple[float, float]]]:
        """Get all TIME_DEPENDENT and PARAM_DEPENDENT parameters of a unit.
        Args:
        unit_name (str): Name of the unit.
        Returns:
        dict[str, list[tuple[float, float]]]: Mapping of parameter names to (x, y) value pairs.
        """
        ...

    def get_dependent_parameter_values(self, unit_name: str, param_name: str) -> List[Tuple[float, float]]:
        """Get the (x, y) value pairs of a TIME_DEPENDENT or PARAM_DEPENDENT parameter.
        Args:
        unit_name (str): Name of the unit.
        param_name (str): Name of the dependent parameter.
        Returns:
        list[tuple[float, float]]: (independent, dependent) value pairs.
        """
        ...

    def get_unit_holdups(self, unit_name: str) -> List[str]:
        """Get a list of holdup names defined in the given unit."""
        ...

    def get_unit_holdup_overall(self, unit_name: str, time: float = None, holdup_name: str = None) -> Dict[str, float]:
        """Get overall mass, temperature, and pressure of a holdup at a specific time or for all timepoints.
        Args:
        unit_name (str): Name of the unit.
        time (float, optional): Specific time for the data. If None, returns data for all timepoints (default holdup).
        holdup_name (str, optional): Name of the holdup. If None, uses the default holdup.
        Returns:
        dict[str, float]: Dictionary with overall properties (mass, temperature, pressure).
        """
        ...

    def get_unit_holdup_composition(self, unit_name: str, time: float = None, holdup_name: str = None) -> Dict[str, float]:
        """Get compound-phase composition of a holdup at a specific time or for all timepoints.
        Args:
        unit_name (str): Name of the unit.
        time (float, optional): Specific time for the data. If None, returns data for all timepoints (default holdup).
        holdup_name (str, optional): Name of the holdup. If None, uses the default holdup.
        Returns:
        dict[str, float]: Dictionary with compound-phase composition.
        """
        ...

    def get_unit_holdup_distribution(self, unit_name: str, time: float = None, holdup_name: str = None) -> Dict[str, List[float]]:
        """Get size distribution of a holdup at a specific time or for all timepoints.
        Args:
        unit_name (str): Name of the unit.
        time (float, optional): Specific time for the data. If None, returns data for all timepoints (default holdup).
        holdup_name (str, optional): Name of the holdup. If None, uses the default holdup.
        Returns:
        dict[str, list[float]]: Dictionary with size distributions per compound.
        """
        ...

    def get_unit_holdup(self, unit_name: str, time: float = None, holdup_name: str = None) -> Dict[str, Any]:
        """Get all data (overall, composition, distributions) of a holdup at a specific time or for all timepoints.
        Args:
        unit_name (str): Name of the unit.
        time (float, optional): Specific time for the data. If None, returns data for all timepoints (default holdup).
        holdup_name (str, optional): Name of the holdup. If None, uses the default holdup.
        Returns:
        dict[str, Any]: Dictionary with all holdup data (overall, composition, distributions).
        """
        ...

    def set_unit_holdup(self, unit_name: str, holdup_dict: Dict[str, Any], holdup_name: str = None) -> None:
        """Set a holdup of a unit at t = 0.0 using a dictionary.
        Args:
        unit_name (str): Name of the unit.
        holdup_dict (dict): Dictionary with fields 'overall', 'composition', and 'distributions'.
        holdup_name (str, optional): Name of the holdup. If None, sets the default holdup.
        """
        ...

    def get_unit_feeds(self, unit_name: str) -> List[str]:
        """Get a list of feed names defined for the given unit."""
        ...

    def get_unit_feed_overall(self, unit_name: str, time: float, feed_name: str = None) -> Dict[str, float]:
        """Get overall properties of a feed in a unit at a specific time.
        Args:
        unit_name (str): Name of the unit.
        time (float): Specific time for the data.
        feed_name (str, optional): Name of the feed. If None, uses the first feed.
        Returns:
        dict[str, float]: Dictionary with overall properties (mass, temperature, pressure).
        """
        ...

    def get_unit_feed_composition(self, unit_name: str, time: float, feed_name: str = None) -> Dict[str, float]:
        """Get composition of a feed in a unit at a specific time.
        Args:
        unit_name (str): Name of the unit.
        time (float): Specific time for the data.
        feed_name (str, optional): Name of the feed. If None, uses the first feed.
        Returns:
        dict[str, float]: Dictionary with compound-phase composition.
        """
        ...

    def get_unit_feed_distribution(self, unit_name: str, time: float, feed_name: str = None) -> Dict[str, List[float]]:
        """Get distributions of a feed in a unit at a specific time.
        Args:
        unit_name (str): Name of the unit.
        time (float): Specific time for the data.
        feed_name (str, optional): Name of the feed. If None, uses the first feed.
        Returns:
        dict[str, list[float]]: Dictionary with size distributions per compound.
        """
        ...

    def get_unit_feed(self, unit_name: str, time: float, feed_name: str = None) -> Dict[str, Any]:
        """Get all data of a feed in a unit at a specific time.
        Args:
        unit_name (str): Name of the unit.
        time (float): Specific time for the data.
        feed_name (str, optional): Name of the feed. If None, uses the first feed.
        Returns:
        dict[str, Any]: Dictionary with all feed data (overall, composition, distributions).
        """
        ...

    def get_unit_feed_overall_all_times(self, unit_name: str) -> Dict[str, Any]:
        """Get overall feed data for all timepoints for the first feed in a unit.
        Args:
        unit_name (str): Name of the unit.
        Returns:
        dict[str, Any]: Dictionary with overall feed data for all timepoints.
        """
        ...

    def get_unit_feed_composition_all_times(self, unit_name: str) -> Dict[str, Any]:
        """Get composition feed data for all timepoints for the first feed in a unit.
        Args:
        unit_name (str): Name of the unit.
        Returns:
        dict[str, Any]: Dictionary with composition feed data for all timepoints.
        """
        ...

    def get_unit_feed_distribution_all_times(self, unit_name: str) -> Dict[str, Any]:
        """Get distributions feed data for all timepoints for the first feed in a unit.
        Args:
        unit_name (str): Name of the unit.
        Returns:
        dict[str, Any]: Dictionary with distributions feed data for all timepoints.
        """
        ...

    def get_unit_feed_all_times(self, unit_name: str) -> Dict[str, Any]:
        """Get all feed data (overall, composition, distributions) for all timepoints for the first feed in a unit.
        Args:
        unit_name (str): Name of the unit.
        Returns:
        dict[str, Any]: Dictionary with all feed data for all timepoints.
        """
        ...

    def get_unit_streams(self, unit_name: str) -> List[str]:
        """Get names of all internal (work) streams of a unit."""
        ...

    def get_unit_stream_overall(self, unit_name: str, time: float, stream_name: str = None) -> Dict[str, float]:
        """Get overall properties of a stream inside a unit at a specific time.
        Args:
        unit_name (str): Name of the unit.
        time (float): Specific time for the data.
        stream_name (str, optional): Name of the stream. If None, uses the first stream.
        Returns:
        dict[str, float]: Dictionary with overall properties (mass, temperature, pressure).
        """
        ...

    def get_unit_stream_composition(self, unit_name: str, time: float, stream_name: str = None) -> Dict[str, float]:
        """Get compound-phase composition of a stream inside a unit at a specific time.
        Args:
        unit_name (str): Name of the unit.
        time (float): Specific time for the data.
        stream_name (str, optional): Name of the stream. If None, uses the first stream.
        Returns:
        dict[str, float]: Dictionary with compound-phase composition.
        """
        ...

    def get_unit_stream_distribution(self, unit_name: str, time: float, stream_name: str = None) -> Dict[str, List[float]]:
        """Get solid-phase distributions of a stream inside a unit at a specific time.
        Args:
        unit_name (str): Name of the unit.
        time (float): Specific time for the data.
        stream_name (str, optional): Name of the stream. If None, uses the first stream.
        Returns:
        dict[str, list[float]]: Dictionary with size distributions per compound.
        """
        ...

    def get_unit_stream(self, unit_name: str, time: float, stream_name: str = None) -> Dict[str, Any]:
        """Get all data (overall, composition, distributions) of a stream inside a unit at a specific time.
        Args:
        unit_name (str): Name of the unit.
        time (float): Specific time for the data.
        stream_name (str, optional): Name of the stream. If None, uses the first stream.
        Returns:
        dict[str, Any]: Dictionary with all stream data (overall, composition, distributions).
        """
        ...

    def get_unit_stream_overall_all_times(self, unit_name: str) -> Dict[str, Any]:
        """Get overall stream data for all timepoints for the first stream in a unit.
        Args:
        unit_name (str): Name of the unit.
        Returns:
        dict[str, Any]: Dictionary with overall stream data for all timepoints.
        """
        ...

    def get_unit_stream_composition_all_times(self, unit_name: str) -> Dict[str, Any]:
        """Get composition stream data for all timepoints for the first stream in a unit.
        Args:
        unit_name (str): Name of the unit.
        Returns:
        dict[str, Any]: Dictionary with composition stream data for all timepoints.
        """
        ...

    def get_unit_stream_distribution_all_times(self, unit_name: str) -> Dict[str, Any]:
        """Get distributions stream data for all timepoints for the first stream in a unit.
        Args:
        unit_name (str): Name of the unit.
        Returns:
        dict[str, Any]: Dictionary with distributions stream data for all timepoints.
        """
        ...

    def get_unit_stream_all_times(self, unit_name: str) -> Dict[str, Any]:
        """Get all stream data (overall, composition, distributions) for all timepoints for the first stream in a unit.
        Args:
        unit_name (str): Name of the unit.
        Returns:
        dict[str, Any]: Dictionary with all stream data for all timepoints.
        """
        ...

    def get_stream_overall(self, stream_name: str, time: float) -> Dict[str, float]:
        """Get overall properties of a flowsheet-level stream.
        Args:
        stream_name (str): Name of the stream.
        time (float): Specific time for the data.
        Returns:
        dict[str, float]: Dictionary with overall properties (mass, temperature, pressure).
        """
        ...

    def get_stream_composition(self, stream_name: str, time: float) -> Dict[str, float]:
        """Get compound-phase composition of a flowsheet-level stream.
        Args:
        stream_name (str): Name of the stream.
        time (float): Specific time for the data.
        Returns:
        dict[str, float]: Dictionary with compound-phase composition.
        """
        ...

    def get_stream_distribution(self, stream_name: str, time: float) -> Dict[str, List[float]]:
        """Get size distributions of a flowsheet-level stream.
        Args:
        stream_name (str): Name of the stream.
        time (float): Specific time for the data.
        Returns:
        dict[str, list[float]]: Dictionary with size distributions per compound.
        """
        ...

    def get_stream(self, stream_name: str, time: float) -> Dict[str, Any]:
        """Get all stream data (overall, composition, distributions) at a given time.
        Args:
        stream_name (str): Name of the stream.
        time (float): Specific time for the data.
        Returns:
        dict[str, Any]: Dictionary with all stream data (overall, composition, distributions).
        """
        ...

    def get_streams(self) -> List[str]:
        """Return list of all flowsheet-level stream names."""
        ...

    def get_stream_overall_all_times(self, stream_name: str) -> Dict[str, Any]:
        """Get overall stream data for all timepoints.
        Args:
        stream_name (str): Name of the stream.
        Returns:
        dict[str, Any]: Dictionary with overall stream data for all timepoints.
        """
        ...

    def get_stream_composition_all_times(self, stream_name: str) -> Dict[str, Any]:
        """Get composition stream data for all timepoints.
        Args:
        stream_name (str): Name of the stream.
        Returns:
        dict[str, Any]: Dictionary with composition stream data for all timepoints.
        """
        ...

    def get_stream_distribution_all_times(self, stream_name: str) -> Dict[str, Any]:
        """Get distributions stream data for all timepoints.
        Args:
        stream_name (str): Name of the stream.
        Returns:
        dict[str, Any]: Dictionary with distributions stream data for all timepoints.
        """
        ...

    def get_stream_all_times(self, stream_name: str) -> Dict[str, Any]:
        """Get all stream data (overall, composition, distributions) for all timepoints.
        Args:
        stream_name (str): Name of the stream.
        Returns:
        dict[str, Any]: Dictionary with all stream data for all timepoints.
        """
        ...

    def get_options(self) -> Dict[str, Any]:
        """Get flowsheet simulation options."""
        ...

    def set_options(self, options: Dict[str, Any]) -> None:
        """Set flowsheet simulation options.
        Args:
        options (dict): Dictionary with simulation options.
        """
        ...

    def get_options_methods(self) -> Dict[str, Any]:
        """Return valid string options for convergenceMethod and extrapolationMethod."""
        ...

    def debug_unit_ports(self, unit_name: str) -> None:
        """Debug unit ports for a given unit."""
        ...

    def debug_stream_data(self, stream_name: str, time: float) -> None:
        """Debug stream data at a specific time."""
        ...

    def pretty_print(data: Dict[str, Any]) -> None:
        """Pretty print a dictionary containing unit parameters, holdup, or simulation options.""" 
        ...