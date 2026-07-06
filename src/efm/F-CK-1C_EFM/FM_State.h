// Flight model state shared with the FCS layer.
struct ControlsIn {
    double pitch = 0.0;
    double roll  = 0.0;
    double yaw   = 0.0;
    double throttle_l  = 0.0;
    double throttle_r  = 0.0;
};

struct ControlsOut {
    double de_cmd = 0.0, da_cmd = 0.0, dr_cmd = 0.0;  // FCS commands.
    double de     = 0.0, da     = 0.0, dr     = 0.0;  // Actuator outputs.
};

struct FMState {
    // Air state.
    double alpha = 0.0;                 // deg
    double aoa = 0.0;                   // rad
    double beta  = 0.0;                 // rad
    double beta_deg  = 0.0;             // deg
    double pitch_rate = 0.0;            // rad/s, mirrors q until state ownership is consolidated
    double p = 0.0, q = 0.0, r = 0.0;   // rad/s
    double V = 0.0;                     // m/s

    // Ground state.
    double agl = 0.0;        // m  altitude above ground
    double vz_agl = 0.0;     // m/s vertical speed above ground
    double gear_pos = 1.0;   // 0~1
    bool   on_ground = false;

    ControlsIn  in;
    ControlsOut out;
};


