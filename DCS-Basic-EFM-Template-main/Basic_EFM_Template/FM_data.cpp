#include "stdafx.h"
#define FM_DATA_DEFINE
#include "FM_data.h"

MassModel g_mass;        

double altitude_ASL = 0.0;  // Altitude above sea level
double altitude_AGL = 0.0;  // Altitude above gound/surface level double 
double vertical_speed_AGL = 0.0;

static double agl_prev = 0.0;
static bool agl_init = false;

void update_agl_rate(double dt)
{
    if (!agl_init) {
        agl_prev = altitude_AGL;
        vertical_speed_AGL = 0.0;
        agl_init = true;
        return;
    }

    vertical_speed_AGL = (altitude_AGL - agl_prev) / dt; // m/s，AGL變大=上升
    agl_prev = altitude_AGL;
}

void MassModel::add_burn(double burn_kg)
{
    if (burn_kg <= 0.0) return;
    fuel_burn_accum += burn_kg;

    // 可選：同步扣內油（避免內油不動）
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
    x = -1.0; y = 1.0; z = 0.0;
    dIxx = dIyy = dIzz = 0.0;

    fuel_burn_accum = 0.0;
    return true;
}


