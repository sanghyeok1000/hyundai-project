#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace BPFeedback {
struct Frame {
    int Map=0; //0 straight ice,1 ramp,2 fog
    bool Active=false,Crash=false,Brake=false,FogVisible=false;
    double SpeedMps=0,Usage=0,SlipRad=0,GapM=-1;
};
struct Output {
    std::uint32_t ColorArgb=0xff5599aau;
    double Brightness=.25;
    int Fan=1;
    double CoolOffset=0,Motor=0,Wind=0,Road=0,Slip=0;
    bool Warn=false,Impact=false;
};
Output Evaluate(const Frame& F);
}
