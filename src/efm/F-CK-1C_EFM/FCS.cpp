// Flight Control System implementation.
#include "stdafx.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "FCS.h"
#include "FM_State.h"
#include <algorithm>
#include <cmath>

static inline double clamp(double x, double lo, double hi)
{
    return std::max(lo, std::min(x, hi));
}

// Pitch-axis tuning.
static constexpr double PI = 3.14159265358979323846;
static constexpr double DEG2RAD = PI / 180.0;

// Stick to pitch-rate command (rad/s per stick).
static constexpr double K_STICK_Q = 1.6;

// Pitch-rate PI loop.
static constexpr double KP_Q = 0.9;
static constexpr double KI_Q = 0.25;

// Elevator limits.
static constexpr double DE_MAX  = 25.0 * DEG2RAD;  // 25 deg
static constexpr double DE_RATE = 60.0 * DEG2RAD;  // 60 deg/s

// Anti-windup gain.
static constexpr double AW_GAIN = 0.5;

void FCS::update(double dt, FMState& s)
{
    // Inputs
    const double stick = clamp(s.in.pitch, -1.0, 1.0);

    // States: s.q = pitch rate (rad/s), s.alpha = AoA (deg).
    const double q   = s.q;                  // rad/s
    const double aoa = s.alpha * DEG2RAD;    // rad

    // Rate command
    const double q_cmd = K_STICK_Q * stick;
    const double q_err = q_cmd - q;

    // PI
    q_int_ += q_err * dt;
    double de_cmd = KP_Q * q_err + KI_Q * q_int_;

    // AoA soft limiter reference.
    const double aoa_max = 25.0 * DEG2RAD;
    if (std::fabs(aoa) > 0.9 * aoa_max)
    {
        double x = (std::fabs(aoa) - 0.9 * aoa_max) / (0.1 * aoa_max);
        double soften = clamp(1.0 - x, 0.0, 1.0);
        de_cmd *= soften;
    }

    // Saturation
    const double de_sat = clamp(de_cmd, -DE_MAX, +DE_MAX);

    // Anti-windup back-calc
    q_int_ += AW_GAIN * (de_sat - de_cmd) * dt;

    // Actuator rate limit
    const double max_step = DE_RATE * dt;
    double de_next = s.out.de + clamp(de_sat - s.out.de, -max_step, +max_step);
    s.out.de = clamp(de_next, -DE_MAX, +DE_MAX);

    // Export the saturated elevator command for diagnostics.
    s.out.de_cmd = de_sat;
}

