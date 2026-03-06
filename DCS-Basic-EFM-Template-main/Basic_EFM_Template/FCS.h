// FCS.h
#pragma once

struct FMState;

class FCS {
public:
    void update(double dt, FMState& s);
private:
    double q_int_ = 0.0;
};


// FCS.h - Flight Control System header file