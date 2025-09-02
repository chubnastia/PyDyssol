/* Copyright (c) 2022, Dyssol Development Team. All rights reserved. This file is part of Dyssol. See LICENSE file for license information. */

/*
 * Base on:
 * Muschelknautz, U. (2019).
 * L3.4 Zyklone zum Abscheiden fester Partikel aus Gasen.
 * In: Stephan, P., Kabelac, S., Kind, M., Mewes, D., Schaber, K., Wetzel, T. (eds)
 * VDI-Wärmeatlas. Springer Reference Technik. Springer Vieweg, Berlin, Heidelberg.
 * https://doi.org/10.1007/978-3-662-52989-8_91
 */

#pragma once

#include "UnitDevelopmentDefines.h"
#include "BaseUnit.h" 
#include "UnitParameters.h" // For unit parameter types
#include <vector>
#include <string>

class CCycloneMuschelknautz : public CSteadyStateUnit
{
    /*
     * Shapes of gas entry.
     */
    enum class EEntry : size_t
    {
        SLOT_RECT, SPIRAL_FULL, SPIRAL_HALF, AXIAL
    };
    /*
     * Shapes of blades for axial gas entry.
     */
    enum class EBlade : size_t
    {
        STRAIGHT, CURVED, CURVED_TWISTED
    };

    // Ports
    CUnitPort* port_inlet{ nullptr };
    CUnitPort* port_outlet_s{ nullptr };
    CUnitPort* port_outlet_g{ nullptr };
    CMaterialStream* inlet{ nullptr };
    CMaterialStream* outlet_s{ nullptr };
    CMaterialStream* outlet_g{ nullptr };

    CHoldup* m_holdup{};

    // Transform matrices
    CTransformMatrix tm_i2s;
    CTransformMatrix tm_i2g;

    // Parameters
    CConstRealUnitParameter* up_d_o;
    CConstIntUnitParameter* up_max_iter;
    CConstUIntUnitParameter* up_N_b;
    CListUnitParameter<double>* up_lambda_0;
    CComboUnitParameter* up_entry_shape;
    CComboUnitParameter* up_group_solver;
    CCompoundUnitParameter* up_gas_compound; 
    CMDBCompoundUnitParameter* up_mdb_gas; 
    CReactionUnitParameter* up_reaction;             
    CStringUnitParameter* up_label;
    CListRealUnitParameter* up_velocity_profile;
    CListIntUnitParameter* up_iter_steps;
    CListUIntUnitParameter* up_cycle_ids;
    CCheckBoxUnitParameter* up_use_filter;
    CDependentUnitParameter* up_temp_profile;
    CDependentUnitParameter* up_friction_adj;

    // Variables
    std::string label;
    std::vector<double> velocity_profile;
    std::vector<int64_t> iter_steps;
    std::vector<uint64_t> cycle_ids;
    bool use_filter{ false };
    double r_o{ 0.0 };
    int64_t max_iter{ 0 };
    uint64_t N_b{ 0 };
    EEntry entry_shape{ EEntry::SLOT_RECT };
    std::string gas_compound;
    std::string mdb_gas;
    std::vector<CChemicalReaction> reaction_list;

    // User-defined cyclone parameters
    double h_tot{ 0.0 };                       // Total height of cyclone [m].
    double h_cyl{ 0.0 };                       // Height of the cylindrical part of cyclone [m].
    double r_f{ 0.0 };                         // Radius of vortex finder [m].
    double h_f{ 0.0 };                         // Height (depth) of vortex finder [m].
    double r_exit{ 0.0 };                      // Radius of particles exit [m].
    double b_e{ 0.0 };                         // Width of gas entry/blade channel [m].
    double h_e{ 0.0 };                         // Height of gas entry [m].
    double epsilon{ 0.0 };                     // Spiral angle in spiral gas entry [deg].
    double d_b{ 0.0 };                         // Thickness of blades in axial gas entry [m].
    double r_core{ 0.0 };                      // Core radius of blades in axial gas entry [m].
    double delta{ 0.0 };                       // Angle of attack of blades in axial gas entry [rad].
    double D{ 0.0 };                           // Coefficient for grid efficiency curve calculation [-].
    double K_main{ 0.0 };                      // Constant for solids loading threshold in main stream [-].
    double eta_adj{ 0.0 };                     // Separation efficiency adjustment factor [-].

    // New user-defined parameters
    std::string material;                      // Material of cyclone construction.
    int wall_type{ 0 };                        // Type of cyclone wall surface.
    double press_drop{ 0.0 };                  // Pressure drop across cyclone [Pa].
    int64_t cycle_count{ 0 };                  // Number of separation cycles [#].
    std::vector<double> friction_list;         // List of friction coefficients.
    std::vector<int64_t> seed_list;            // List of random seeds.
    std::vector<uint64_t> batch_list;          // List of batch identifiers.
    double eff_model{ 0.0 };                   // Efficiency model value.

    // Calculated cyclone parameters
    double r_con_mean{ 0 }; // Mean radius of the conical part [m].
    double r_exit_eff{ 0 }; // Effective radius of the particles exit [m].
    double h_con{ 0 };      // Height of the cone part of cyclone [m].
    double h_con_eff{ 0 };  // Effective height of the cone part of cyclone [m].
    double h_sep{ 0 };      // Height of separation zone [m].
    double a{ 0 };          // Height of blades channel in axial gas entry [m].
    double beta{ 0 };       // Relative width of cyclone gas entry [-].
    double A_cyl{ 0 };      // Lateral area of the cylindrical part [m^2].
    double A_con{ 0 };      // Lateral area of the conical part [m^2].
    double A_top{ 0 };      // Area of upper wall [m^2].
    double A_f{ 0 };        // Lateral area of vortex finder [m^2].
    double A_e1{ 0 };       // Average wall area considered for the first revolution after entry [m^2].
    double A_sp{ 0 };       // Frictional area of the spiral in spiral gas entry [m^2].
    double A_con_2{ 0 };    // Lateral area of the top half of conical part [m^2].

    // Plots
    CPlot* plot_sep_3d{ nullptr };
    CPlot* plot_sep{ nullptr };
    CPlot* plot_main_frac{ nullptr };
    CCurve* curve_sep{ nullptr };
    CCurve* curve_main_frac{ nullptr };

public:
    void CreateBasicInfo() override;
    void CreateStructure() override;
    void Initialize(double _time) override;
    void Simulate(double _time) override;

private:
    // Check input parameters.
    void CheckCycloneParameters();
    // Main calculation function.
    void CalculateSeparationMuschelknautz(double _time);
    // Calculates wall friction coefficient for solids-containing gas [-].
    double WallFrictionCoeff(double mu_in) const;
    // Calculates contraction coefficient [-].
    double ContractionCoefficient(double mu_in) const;
    // Calculates inlet velocity in the middle streamline [m/s].
    double InletVelocityStreamline(double Vflow_in_g) const;
    // Calculates tangential velocity at cyclone radius [m/s].
    double OuterTangVelocity(double Vflow_in_g, double v_e, double alpha, double lambda_s) const;
    // Calculates exponent for solids loading threshold [-].
    double SolidsLoadingExp(double mu_in) const;
    // Calculate separation efficiency in internal vortex for particle of size d.
    double CalculateSeparationEff(double Dval, double d_star, double d) const;

private:
    class Impl
    {
    public:
        static const std::vector<std::string> entry_names;
        static const std::vector<std::string> solver_names;
        static const std::vector<std::string> group_names;
        static const std::vector<std::string> compound_names;
        static const std::vector<std::string> mdb_gas_names;
        static const std::vector<std::string> reaction_names;
    };
};