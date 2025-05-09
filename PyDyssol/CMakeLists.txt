cmake_minimum_required(VERSION 3.1.0)

project(PyDyssol LANGUAGES CXX)

# Ensure C++17 standard is set
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(pybind11 REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS CXX HL)

# Include directories
include_directories(${pybind11_INCLUDE_DIRS})
include_directories(${HDF5_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/../SimulatorCore)
include_directories(${CMAKE_SOURCE_DIR}/../Utilities)
include_directories(${CMAKE_SOURCE_DIR}/../BaseSolvers)
include_directories(${CMAKE_SOURCE_DIR}/../HDF5Handler)
include_directories(${CMAKE_SOURCE_DIR}/../ModelsAPI)
include_directories(${CMAKE_SOURCE_DIR}/../CacheHandler)
include_directories(${CMAKE_SOURCE_DIR}/../EquationSolvers)
include_directories(${CMAKE_SOURCE_DIR}/../MaterialsDatabase)
include_directories(${CMAKE_SOURCE_DIR}/../Modules)

# Define the Python module
pybind11_add_module(PyDyssol 
    PyDyssol.cpp
)

# Link necessary libraries
target_link_libraries(PyDyssol 
    PRIVATE ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} 
)
add_dependencies(PyDyssol libdyssol_static)
target_link_libraries(PyDyssol PRIVATE libdyssol_static)

# Install the Python module
install(TARGETS PyDyssol DESTINATION ${CMAKE_INSTALL_LIBDIR}/python)
