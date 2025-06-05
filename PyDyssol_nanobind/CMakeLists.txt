cmake_minimum_required(VERSION 3.1.0)

project(PyDyssol LANGUAGES CXX)

# Ensure C++17 standard is set
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(nanobind REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS CXX HL)

# Include directories
include_directories(${nanobind_INCLUDE_DIRS})
include_directories(${HDF5_INCLUDE_DIRS})

target_include_directories(PyDyssol_nanobind PRIVATE 
    "D:/Dyssol/SimulatorCore"
    "D:/Dyssol/BaseSolvers"
    "D:/Dyssol/HDF5Handler"
    "D:/Dyssol/ModelsAPI"
    "D:/Dyssol/CacheHandler"
    "D:/Dyssol/EquationSolvers"
    "D:/Dyssol/MaterialsDatabase"
    "D:/Dyssol/Modules"
)
# Define the Python module
nanobind_add_module(PyDyssol_nanobind 
    PyDyssol_nb.cpp
    PyDyssolBindings_nb.cpp 
    PyDyssol_Feeds_nb.cpp 
    PyDyssol_Holdups_nb.cpp 
    PyDyssol_Options_nb.cpp 
    PyDyssol_Parameters_nb.cpp 
    PyDyssol_Phases_Comps_nb.cpp 
    PyDyssol_Streams_nb.cpp 
    PyDyssol_UnitStreams_nb.cpp 
    PyDyssol_Utils_nb.cpp
)

# Link necessary libraries
target_link_libraries(PyDyssol_nanobind 
    PRIVATE ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} 
)
add_dependencies(PyDyssol_nanobind libdyssol_static)
target_link_libraries(PyDyssol_nanobind PRIVATE libdyssol_static)

# Install the Python module
install(TARGETS PyDyssol_nanobind DESTINATION ${CMAKE_INSTALL_LIBDIR}/python)
