#include "stdafx.h"

namespace FM
{
extern double altitude_AGL;
}

namespace
{
struct MassModel
{
    double empty_mass_kg = 6492.0;
    double internal_fuel = 2111.0;
    double fuel_burn_accum = 0.0;

    void add_burn(double burn_kg);
    double get_total_mass() const;
    bool pop_mass_delta(double& dm, double& x, double& y, double& z,
                        double& dIxx, double& dIyy, double& dIzz);
};

MassModel g_mass;
}

double vertical_speed_AGL = 0.0;

static double agl_prev = 0.0;
static bool agl_init = false;

void update_agl_rate(double dt)
{
    if (!agl_init) {
        agl_prev = FM::altitude_AGL;
        vertical_speed_AGL = 0.0;
        agl_init = true;
        return;
    }

    // Vertical speed over ground in m/s.
    vertical_speed_AGL = (FM::altitude_AGL - agl_prev) / dt;
    agl_prev = FM::altitude_AGL;
}

void MassModel::add_burn(double burn_kg)
{
    if (burn_kg <= 0.0) return;
    fuel_burn_accum += burn_kg;

    // Consume internal fuel first.
    internal_fuel -= burn_kg;
    if (internal_fuel < 0.0) internal_fuel = 0.0;
}

double MassModel::get_total_mass() const
{
    return empty_mass_kg + internal_fuel;
}

bool MassModel::pop_mass_delta(double& dm, double& x, double& y, double& z,
                               double& dIxx, double& dIyy, double& dIzz)
{
    if (fuel_burn_accum <= 0.0) return false;

    dm = fuel_burn_accum;
    x = -1.0;
    y = 1.0;
    z = 0.0;
    dIxx = 0.0;
    dIyy = 0.0;
    dIzz = 0.0;

    fuel_burn_accum = 0.0;
    return true;
}
