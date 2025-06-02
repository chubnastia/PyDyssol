# PyDyssol

**PyDyssol** is a Python-C++ hybrid interface to the [Dyssol](https://github.com/dyssol-project/Dyssol-open) simulation engine for solids processes. It uses [`pybind11`](https://github.com/pybind/pybind11) to expose the C++ simulation backend to Python, enabling users to build, run, and analyze flowsheet simulations programmatically.

---

## Technical Overview

PyDyssol is built on:

- C++ Dyssol core for simulation logic (units, streams, solver)
- `pybind11` bindings to connect C++ functionality to Python
- A Python API that provides full access to:
  - Unit creation and model selection
  - Parameter setting (scalar, combo, compound, MDB, reaction)
  - Stream inspection and manipulation
  - Flowsheet structure (units and connections)
  - Simulation execution: `Initialize`, `RunSimulation`, `Save`, `Load`

---

## Features

- Full simulation control from Python
- Direct access to unit models and their parameters
- Support for complex parameter types (compound-specific values, distributions, reactions)
- Real-time stream access for monitoring or post-processing
- Multi-model and multi-configuration workflows
- Seamless integration with Python tools (NumPy, pandas, matplotlib)
