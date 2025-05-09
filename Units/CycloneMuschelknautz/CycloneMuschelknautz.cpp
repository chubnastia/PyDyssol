#define DLL_EXPORT

#include "CycloneMuschelknautz.h"
#include <sstream>
#include <algorithm>

extern "C" DECLDIR CBaseUnit* DYSSOL_CREATE_MODEL_FUN()
{
    return new CCycloneMuschelknautz();
}

// Define Impl static members
const std::vector<std::string> CCycloneMuschelknautz::Impl::entry_names{ "Rectangular slot", "Full spiral", "Half spiral", "Axial" };
const std::vector<std::string> CCycloneMuschelknautz::Impl::solver_names{ "Euler", "RK4" };
const std::vector<std::string> CCycloneMuschelknautz::Impl::group_names{ "Basic", "Advanced" };
const std::vector<std::string> CCycloneMuschelknautz::Impl::compound_names{ "Air", "N2" };
const std::vector<std::string> CCycloneMuschelknautz::Impl::mdb_gas_names{ "N2_key", "Air_key" }; // Replace with actual keys
const std::vector<std::string> CCycloneMuschelknautz::Impl::reaction_names{ "None", "Combustion" };

void CCycloneMuschelknautz::CreateStructure()
{
    // Define ports
    port_inlet = AddPort("Inlet", EUnitPort::INPUT);
    port_outlet_s = AddPort("OutletSolid", EUnitPort::OUTPUT);
    port_outlet_g = AddPort("OutletGas", EUnitPort::OUTPUT);

    // Previous code (lines 53-55) for STRING parameter
    up_label = AddStringParameter("label", "Cyclone", "User-defined label");

    // LIST_DOUBLE: Velocity profile
    up_velocity_profile = AddListRealParameter("velocity_profile", 1.0, "m/s", "Inlet velocity profile");
    up_velocity_profile->SetUnits(L"m/s");
    up_velocity_profile->SetDescription("Inlet velocity profile");
    up_velocity_profile->SetValues({ 1.0, 1.5, 2.0 });

    // LIST_INT64: Iteration steps
    up_iter_steps = AddListIntParameter("iter_steps", 1, "", "Iteration steps");
    up_iter_steps->SetUnits(L"");
    up_iter_steps->SetDescription("Iteration steps");
    up_iter_steps->SetValues({ 1, 20, 30 });

    // LIST_UINT64: Cycle identifiers
    up_cycle_ids = AddListUIntParameter("cycle_ids", 1001, "", "Cycle identifiers");
    up_cycle_ids->SetUnits(L"");
    up_cycle_ids->SetDescription("Cycle identifiers");
    up_cycle_ids->SetValues({ 1001, 1002, 1003 });

    // CHECKBOX: Use dust filter
    up_use_filter = AddCheckBoxParameter("use_filter", false, "Use dust filter");

    // TIME_DEPENDENT: Temperature profile
    up_temp_profile = AddTDParameter("temp_profile", 298.15, "K", "Temperature profile over time");
    up_temp_profile->SetUnits(L"K");
    up_temp_profile->SetDescription("Temperature profile over time");
    up_temp_profile->SetValues({ 0.0, 10.0, 20.0 }, { 298.15, 300.0, 310.0 });

    // Add lambda_0 as a CONSTANT_DOUBLE parameter
    up_lambda_0 = AddListRealParameter("lambda_0", 0.005, "", "Wall friction coefficients");
    up_lambda_0->SetDescription("List of wall friction coefficients");
    up_lambda_0->SetValues({ 0.005, 0.01, 0.05 });

    // Existing friction_adj parameter
    up_friction_adj = AddDependentParameter("friction_adj", 0.015, "", "lambda_0", 0.005, "", "Friction adjustment");
    up_friction_adj->SetDescription("Friction adjustment");
    up_friction_adj->SetValues({ 0.005, 0.01, 0.05 }, { 0.015, 0.02, 0.025 });

    // CONSTANT_DOUBLE
    up_d_o = AddConstRealParameter("d_o", 0.5, "m", "Cyclone outer diameter");
    up_d_o->SetUnits(L"m");
    up_d_o->SetDescription("Outer diameter of the cyclone");

    // CONSTANT_INT64
    up_max_iter = AddConstIntParameter("max_iter", 100, "", "Maximum iteration count");
    up_max_iter->SetDescription("Maximum number of iterations");

    // CONSTANT_UINT64
    up_N_b = AddConstUIntParameter("N_b", 10, "", "Number of blades");
    up_N_b->SetDescription("Number of blades for axial entry");

    // COMBO: Entry shape (enum-based)
    up_entry_shape = AddComboParameter("entry_shape",
        static_cast<size_t>(EEntry::SLOT_RECT),
        { static_cast<size_t>(EEntry::SLOT_RECT), static_cast<size_t>(EEntry::SPIRAL_FULL), static_cast<size_t>(EEntry::SPIRAL_HALF), static_cast<size_t>(EEntry::AXIAL) },
        CCycloneMuschelknautz::Impl::entry_names,
        "Shape of the gas inlet");

    // COMPOUND
    up_gas_compound = AddCompoundParameter("gas_compound", "Gas compound from list");
  
    // MDB_COMPOUND
    up_mdb_gas = AddMDBCompoundParameter("mdb_gas", "Material database gas key");

    up_reaction = AddReactionParameter("reaction", "Reaction type used in simulation");

    if (up_gas_compound->GetCompound().empty() && !GetAllCompounds().empty())
        up_gas_compound->SetCompound(GetAllCompounds().front());

    if (up_mdb_gas->GetCompound().empty() && !GetAllCompounds().empty())
        up_mdb_gas->SetCompound(GetAllCompounds().front());

    if (up_reaction->GetReactions().empty())
        up_reaction->SetReactions({});
}

void CCycloneMuschelknautz::Initialize(double _time)
{

    // Check phases and distributions
    if (!IsPhaseDefined(EPhase::GAS))    RaiseError("Gas phase not defined.");
    if (!IsPhaseDefined(EPhase::SOLID))  RaiseError("Solid phase not defined.");
    if (!IsDistributionDefined(DISTR_SIZE)) RaiseError("Particle size distribution not defined.");

    // Bind streams
    //inlet = port_inlet->GetStream();
    //outlet_s = port_outlet_s->GetStream();
    //outlet_g = port_outlet_g->GetStream();

    // Read parameters
    label = up_label->GetValue();
    velocity_profile = up_velocity_profile->GetValues();
    iter_steps = up_iter_steps->GetValues();
    cycle_ids = up_cycle_ids->GetValues();
    use_filter = up_use_filter->GetValue();
    // Constants
    r_o = up_d_o->GetValue();
    max_iter = up_max_iter->GetValue();
    N_b = up_N_b->GetValue();

    // Combo types
    entry_shape = static_cast<EEntry>(up_entry_shape->GetValue());
    //group_solver = up_group_solver->GetValue();
    gas_compound = up_gas_compound->GetCompound();
    mdb_gas = up_mdb_gas->GetCompound();  // if string keys used
    auto reaction_list = up_reaction->GetReactions();


    // Reapply values if they are empty or incorrect
    if (velocity_profile.empty()) {
        std::cout << "[CycloneMuschelknautz] Reapplying velocity_profile values" << std::endl;
        up_velocity_profile->SetValues({ 1.0, 1.5, 2.0 });
        velocity_profile = up_velocity_profile->GetValues();
    }
    if (iter_steps.empty()) {
        std::cout << "[CycloneMuschelknautz] Reapplying iter_steps values" << std::endl;
        up_iter_steps->SetValues({ 1, 20, 30 });
        iter_steps = up_iter_steps->GetValues();
    }
    if (cycle_ids.empty() || (cycle_ids.size() == 1 && cycle_ids[0] == 0)) {
        std::cout << "[CycloneMuschelknautz] Reapplying cycle_ids values" << std::endl;
        up_cycle_ids->SetValues({ 1001, 1002, 1003 });
        cycle_ids = up_cycle_ids->GetValues();
    }

    // Rest of the Initialize code...
    h_tot = 2.0;
    h_cyl = 1.0;
    r_f = 0.2;
    h_f = 0.2;
    r_exit = 0.1;
    b_e = 0.1;
    h_e = 0.2;
    epsilon = 270.0;
    d_b = 0.005;
    r_core = 0.05;
    delta = 20.0 * MATH_PI / 180.0;
    D = 3.0;
    K_main = 0.025;
    eta_adj = 1.0;

    // Geometric calculations
    r_con_mean = (r_o + r_exit) / 2;
    r_exit_eff = std::max(r_exit, r_f);
    //r_e = r_o;
    h_con = h_tot - h_cyl;
    h_con_eff = h_con;
    h_sep = h_cyl + h_con - h_f;
    a = h_e;
    beta = b_e / r_o;
    A_cyl = 2 * MATH_PI * r_o * h_cyl;
    A_con = MATH_PI * (r_o + r_exit_eff) * sqrt(pow(r_o - r_exit_eff, 2) + pow(h_con_eff, 2));
    A_top = MATH_PI * pow(r_o, 2) - MATH_PI * pow(r_f, 2);
    A_f = 2 * MATH_PI * r_f * h_f;
    //A_tot = A_cyl + A_con + A_f + A_top;
    A_e1 = 2 * MATH_PI * r_o * h_e / 2;
    A_sp = 0;
    A_con_2 = MATH_PI * (r_o + r_con_mean) * sqrt(pow(r_o - r_con_mean, 2) + pow(h_con / 2, 2));
    //A_sed = A_cyl + A_con_2;

    // Set up plots
    plot_sep_3d = AddPlot("Separation", "Diameter [m]", "Separation efficiency [%]", "Time [s]");
    plot_sep = AddPlot("Total separation efficiency", "Time [s]", "Efficiency [%]");
    plot_main_frac = AddPlot("Main stream fraction", "Time [s]", "Fraction [-]");
    curve_sep = plot_sep->AddCurve("Efficiency");
    curve_main_frac = plot_main_frac->AddCurve("Fraction");

    CheckCycloneParameters();
}

void CCycloneMuschelknautz::CreateBasicInfo()
{
    SetUnitName("CycloneMuschelknautz");
    SetUniqueID("4E2C9FB3BFA44B8E829AC393042F2CD4");
}

void CCycloneMuschelknautz::Simulate(double _time)
{
    CalculateSeparationMuschelknautz(_time);
}

void CCycloneMuschelknautz::CheckCycloneParameters()
{
    if (r_o <= 0) RaiseError("Outer radius must be positive.");
    if (h_tot <= 0) RaiseError("Total height must be positive.");
}

void CCycloneMuschelknautz::CalculateSeparationMuschelknautz(double _time)
{
    double mu_in = 0.0; // Placeholder
    double Vflow_in_g = 1.0; // Placeholder
    double alpha = ContractionCoefficient(mu_in);
    double lambda_s = WallFrictionCoeff(mu_in);
    double v_e = InletVelocityStreamline(Vflow_in_g);
    double v_tang = OuterTangVelocity(Vflow_in_g, v_e, alpha, lambda_s);
    double exp = SolidsLoadingExp(mu_in);
    double d_star = 0.0; // Placeholder
    //for (size_t i = 0; i < classes_num; ++i) {
   //     double eff = CalculateSeparationEff(D, d_star, aver_diam[i]);
   //     tm_i2s.SetValue(DISTR_SIZE, i, eff);
   //     tm_i2g.SetValue(DISTR_SIZE, i, 1.0 - eff);
  //  }
}

double CCycloneMuschelknautz::WallFrictionCoeff(double mu_in) const
{
    const auto& values = up_lambda_0->GetValues();
    double lambda_0_value = values.empty() ? 0.005 : values.front(); // Use the first value
    return lambda_0_value * (1 + 2 * sqrt(mu_in));
}

double CCycloneMuschelknautz::ContractionCoefficient(double mu_in) const
{
    return 1.0 / (1.0 + sqrt(mu_in));
}

double CCycloneMuschelknautz::InletVelocityStreamline(double Vflow_in_g) const
{
    return Vflow_in_g / (b_e * h_e);
}

double CCycloneMuschelknautz::OuterTangVelocity(double Vflow_in_g, double v_e, double alpha, double lambda_s) const
{
    return v_e * alpha / (1.0 + lambda_s);
}

double CCycloneMuschelknautz::SolidsLoadingExp(double mu_in) const
{
    return 1.0 + mu_in;
}

double CCycloneMuschelknautz::CalculateSeparationEff(double Dval, double d_star, double d) const
{
    if (d <= d_star) return 0.0;
    return 1.0 - exp(-Dval * pow(d / d_star, 2));
}