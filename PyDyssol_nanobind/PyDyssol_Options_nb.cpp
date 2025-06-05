#include "PyDyssol_nb.h"
#include <iostream>
#include <stdexcept>
#include <nanobind/nanobind.h> // Include nanobind headers
#include <nanobind/stl/string.h> // For std::string bindings
#include <nanobind/stl/vector.h> // For std::vector bindings
#include <nanobind/stl/pair.h>   // For std::pair bindings
#include <nanobind/stl/map.h>    // For std::map bindings
#include <nanobind/stl/tuple.h>  // For std::tuple bindings

namespace nb = nanobind;

inline std::string ToString(EConvergenceMethod method) {
    switch (method) {
    case EConvergenceMethod::DIRECT_SUBSTITUTION: return "DIRECT_SUBSTITUTION";
    case EConvergenceMethod::WEGSTEIN: return "WEGSTEIN";
    case EConvergenceMethod::STEFFENSEN: return "STEFFENSEN";
    default: return "UNKNOWN";
    }
}

inline EConvergenceMethod ToConvergenceMethod(const std::string& name) {
    if (name == "DIRECT_SUBSTITUTION") return EConvergenceMethod::DIRECT_SUBSTITUTION;
    if (name == "WEGSTEIN") return EConvergenceMethod::WEGSTEIN;
    if (name == "STEFFENSEN") return EConvergenceMethod::STEFFENSEN;
    throw std::invalid_argument("Unknown ConvergenceMethod: " + name);
}

inline std::string ToString(EExtrapolationMethod method) {
    switch (method) {
    case EExtrapolationMethod::LINEAR: return "LINEAR";
    case EExtrapolationMethod::SPLINE: return "SPLINE";
    case EExtrapolationMethod::NEAREST: return "NEAREST";
    default: return "UNKNOWN";
    }
}

inline EExtrapolationMethod ToExtrapolationMethod(const std::string& name) {
    if (name == "LINEAR") return EExtrapolationMethod::LINEAR;
    if (name == "SPLINE") return EExtrapolationMethod::SPLINE;
    if (name == "NEAREST") return EExtrapolationMethod::NEAREST;
    throw std::invalid_argument("Unknown ExtrapolationMethod: " + name);
}

//Options
nb::dict PyDyssol::GetOptions() const
{
    nb::dict out;
    const auto* p = m_flowsheet.GetParameters();

    // Tolerances
    out["absTol"] = static_cast<double>(p->absTol);
    out["relTol"] = static_cast<double>(p->relTol);

    // Fractions
    out["minFraction"] = static_cast<double>(p->minFraction);

    // Time
    out["startSimulationTime"] = static_cast<double>(p->startSimulationTime);
    out["endSimulationTime"] = static_cast<double>(p->endSimulationTime);

    // Time windows
    out["initTimeWindow"] = static_cast<double>(p->initTimeWindow);
    out["minTimeWindow"] = static_cast<double>(p->minTimeWindow);
    out["maxTimeWindow"] = static_cast<double>(p->maxTimeWindow);
    out["maxItersNumber"] = static_cast<uint32_t>(p->maxItersNumber);
    out["itersUpperLimit"] = static_cast<uint32_t>(p->itersUpperLimit);
    out["itersLowerLimit"] = static_cast<uint32_t>(p->itersLowerLimit);
    out["iters1stUpperLimit"] = static_cast<uint32_t>(p->iters1stUpperLimit);
    out["magnificationRatio"] = static_cast<double>(p->magnificationRatio);

    // Convergence
    out["convergenceMethod"] = ToString(static_cast<EConvergenceMethod>(p->convergenceMethod));
    out["wegsteinAccelParam"] = static_cast<double>(p->wegsteinAccelParam);
    out["relaxationParam"] = static_cast<double>(p->relaxationParam);

    // Extrapolation
    out["extrapolationMethod"] = ToString(static_cast<EExtrapolationMethod>(p->extrapolationMethod));

    // Compression
    out["saveTimeStep"] = static_cast<double>(p->saveTimeStep);
    out["saveTimeStepFlagHoldups"] = static_cast<bool>(p->saveTimeStepFlagHoldups);

    // Enthalpy
    out["enthalpyMinT"] = static_cast<double>(p->enthalpyMinT);
    out["enthalpyMaxT"] = static_cast<double>(p->enthalpyMaxT);
    out["enthalpyInt"] = static_cast<uint32_t>(p->enthalpyInt);

    return out;
}

void PyDyssol::SetOptions(const nb::dict& options)
{
    CParametersHolder* p = m_flowsheet.GetParameters();

    auto get_double = [&](const char* key, auto setter) {
        if (options.contains(key)) {
            double val = nb::cast<double>(options[key]);
            (p->*setter)(val);
        }
        };
    auto get_uint = [&](const char* key, auto setter) {
        if (options.contains(key)) {
            uint32_t val = nb::cast<uint32_t>(options[key]);
            (p->*setter)(val);
        }
        };
    auto get_bool = [&](const char* key, auto setter) {
        if (options.contains(key)) {
            bool val = nb::cast<bool>(options[key]);
            (p->*setter)(val);
        }
        };
    auto get_enum_conv = [&](const char* key) {
        if (options.contains(key)) {
            std::string val = nb::cast<std::string>(options[key]);
            p->ConvergenceMethod(ToConvergenceMethod(val));
        }
        };
    auto get_enum_extr = [&](const char* key) {
        if (options.contains(key)) {
            std::string val = nb::cast<std::string>(options[key]);
            p->ExtrapolationMethod(ToExtrapolationMethod(val));
        }
        };

    // Tolerances
    get_double("absTol", &CParametersHolder::AbsTol);
    get_double("relTol", &CParametersHolder::RelTol);

    // Fractions
    get_double("minFraction", &CParametersHolder::MinFraction);

    // Time
    get_double("startSimulationTime", &CParametersHolder::StartSimulationTime);
    get_double("endSimulationTime", &CParametersHolder::EndSimulationTime);

    // Time windows
    get_double("initTimeWindow", &CParametersHolder::InitTimeWindow);
    get_double("minTimeWindow", &CParametersHolder::MinTimeWindow);
    get_double("maxTimeWindow", &CParametersHolder::MaxTimeWindow);
    get_uint("maxItersNumber", &CParametersHolder::MaxItersNumber);
    get_uint("itersUpperLimit", &CParametersHolder::ItersUpperLimit);
    get_uint("itersLowerLimit", &CParametersHolder::ItersLowerLimit);
    get_uint("iters1stUpperLimit", &CParametersHolder::Iters1stUpperLimit);
    get_double("magnificationRatio", &CParametersHolder::MagnificationRatio);

    // Convergence methods
    get_enum_conv("convergenceMethod");
    get_double("wegsteinAccelParam", &CParametersHolder::WegsteinAccelParam);
    get_double("relaxationParam", &CParametersHolder::RelaxationParam);

    // Extrapolation method
    get_enum_extr("extrapolationMethod");

    // Compression
    get_double("saveTimeStep", &CParametersHolder::SaveTimeStep);
    get_bool("saveTimeStepFlagHoldups", &CParametersHolder::SaveTimeStepFlagHoldups);

    // Enthalpy calculator
    get_double("enthalpyMinT", &CParametersHolder::EnthalpyMinT);
    get_double("enthalpyMaxT", &CParametersHolder::EnthalpyMaxT);
    get_uint("enthalpyInt", &CParametersHolder::EnthalpyInt);
}

nb::dict PyDyssol::GetOptionsMethods() const
{
    nb::dict result;

    nb::list conv;
    for (int i = 0; i <= static_cast<int>(EConvergenceMethod::STEFFENSEN); ++i)
        conv.append(ToString(static_cast<EConvergenceMethod>(i)));
    result["convergenceMethod"] = conv;

    nb::list extr;
    for (int i = 0; i <= static_cast<int>(EExtrapolationMethod::NEAREST); ++i)
        extr.append(ToString(static_cast<EExtrapolationMethod>(i)));
    result["extrapolationMethod"] = extr;

    return result;
}