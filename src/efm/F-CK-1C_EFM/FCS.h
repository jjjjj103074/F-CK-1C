// Flight Control System interface.
#pragma once

struct FMState;

class FCS {
public:
    // EFMREF: CUSTOM_SYSTEM - Legacy/experimental FCS entrypoint; review against FBWController.
    void update(double dt, FMState& s);
private:
    double q_int_ = 0.0;
};
