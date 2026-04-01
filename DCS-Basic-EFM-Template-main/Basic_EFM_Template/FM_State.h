// FM_State.h - Defines the structure for the flight model state, including control inputs and outputs.
struct ControlsIn {
    double pitch = 0.0;
    double roll  = 0.0;
    double yaw   = 0.0;
    double throttle_l  = 0.0;
    double throttle_r  = 0.0;
};

struct ControlsOut {
    double de_cmd = 0.0, da_cmd = 0.0, dr_cmd = 0.0;  // FCS命令
    double de     = 0.0, da     = 0.0, dr     = 0.0;  // 作動器後實際角
};

struct FMState {
    // state：alpha beta p q r V 等（略）
    // Air struct
    double alpha = 0.0;                 // deg
    double aoa = 0.0;                   // rad
    double beta  = 0.0;                 // rad
    double beta_deg  = 0.0;             // deg
    double pitch_rate = 0.0;            // rad/s, 暫時重複定義
    double p = 0.0, q = 0.0, r = 0.0;   // rad/s
    double V = 0.0;                     // m/s

    // ground struct
    double agl = 0.0;        // m  altitude above ground
    double vz_agl = 0.0;     // m/s 垂直速度（定義：向上為正 或 向下為正要固定）
    double gear_pos = 1.0;   // 0~1
    bool   on_ground = false;

    // // （可選）三輪接觸用
    // double gear_comp[3] = {0,0,0};       // m 壓縮量（>0 表示壓縮）
    // double gear_comp_rate[3] = {0,0,0};  // m/s

    ControlsIn  in;
    ControlsOut out;
};


