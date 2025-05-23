#ifndef PYDYSSOL_PARAMETERS_H
#define PYDYSSOL_PARAMETERS_H

#include <string>
#include <map>
#include <tuple>
#include <vector>
#include <variant>
#include <pybind11/pybind11.h>
#include "PyDyssol.h"

class PyDyssol; // Forward declare for friend relationship if needed

class PyDyssolParameters
{
public:
    // Get all parameters (real and combo) as string pairs
    pybind11::object GetUnitParameter(const std::string& unitName, const std::string& paramName) const;
    std::map<std::string, std::tuple<pybind11::object, std::string, std::string>> GetUnitParameters(const std::string& unitName) const;
    std::map<std::string, std::tuple<pybind11::object, std::string, std::string>> GetUnitParametersAll(const std::string& unitName) const;

    void SetUnitParameter(
        const std::string& unitName,
        const std::string& paramName,
        const std::variant<
        bool,  // Moved to the front
        double,
        std::string,
        int64_t,
        uint64_t,
        std::vector<double>,
        std::vector<int64_t>,
        std::vector<uint64_t>,
        std::vector<std::pair<double, double>>,
        std::vector<pybind11::dict>
        > value);

    // Parameters
    std::vector<std::string> GetComboOptions(const std::string& unitName, const std::string& paramName) const;
    std::vector<std::pair<double, double>> GetDependentParameterValues(const std::string& unitName, const std::string& paramName) const;
    std::map<std::string, std::vector<std::pair<double, double>>> GetDependentParameters(const std::string& unitName) const;
    // Fetch single parameter
    pybind11::object GetUnitParameter(const std::string& unitName, const std::string& paramName) const;

    // Fetch all active or all parameters with their values, types, units
    std::map<std::string, std::tuple<pybind11::object, std::string, std::string>>
        GetUnitParameters(const std::string& unitName) const;

    std::map<std::string, std::tuple<pybind11::object, std::string, std::string>>
        GetUnitParametersAll(const std::string& unitName) const;

    // Set unit parameter with various allowed types
    void SetUnitParameter(const std::string& unitName,
        const std::string& paramName,
        const std::variant<
        bool,
        double,
        std::string,
        int64_t,
        uint64_t,
        std::vector<double>,
        std::vector<int64_t>,
        std::vector<uint64_t>,
        std::vector<std::pair<double, double>>,
        std::vector<pybind11::dict>> &value);

    // Combo options
    std::vector<std::string> GetComboOptions(const std::string& unitName, const std::string& paramName) const;

    // Dependent parameter values
    std::vector<std::pair<double, double>> GetDependentParameterValues(const std::string& unitName,
        const std::string& paramName) const;

    std::map<std::string, std::vector<std::pair<double, double>>> GetDependentParameters(const std::string& unitName) const;
};

#endif // PYDYSSOL_PARAMETERS_H
