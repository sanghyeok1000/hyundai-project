#include "FeedbackPolicy.h"
namespace BPFeedback {
namespace {
double Unit(double X) {return std::isfinite(X)?std::clamp(X,0.,1.):0.;}
std::uint32_t Mix(std::uint32_t A,std::uint32_t B,double T) {
    std::uint32_t C=0xff000000u;
    for(int Shift:{0,8,16}) C|=static_cast<std::uint32_t>(std::lround(((A>>Shift)&255)*(1-T)+((B>>Shift)&255)*T))<<Shift;
    return C;
}
}
Output Evaluate(const Frame& F) {
    Output O;
    const bool Fog=F.Map==2;
    O.ColorArgb=Fog?0xffa6b9d0u:0xff339cbfu;
    O.Fan=Fog?1:2;O.CoolOffset=Fog?0:1;
    O.Impact=F.Crash;
    if(!F.Active || F.Map<0 || F.Map>2) return O;
    const double Speed=Unit(F.SpeedMps/30.);
    const double Motion=std::isfinite(F.SpeedMps)?Unit((F.SpeedMps-2)/4):0;
    const double Slip=Unit((std::abs(F.SlipRad)-.015)/.18)*Motion;
    const double Risk=std::max(Unit((F.Usage-.7)/.3)*Motion,Slip);
    O.Warn=Fog && F.FogVisible && std::isfinite(F.GapM) && F.GapM>=0 && F.GapM<=30;
    O.ColorArgb=Mix(O.ColorArgb,0xffffa03au,Fog?(O.Warn?1:0):Risk);
    O.Brightness=.25+.1*(Fog?(O.Warn?1:0):Risk);
    O.Motor=(F.Brake?0:.08+.14*Speed)*Motion;
    O.Wind=.28*Speed*Speed;
    O.Road=(Fog?.26:.18)*Speed;
    O.Slip=(Fog?.32:.23)*Slip;
    return O;
}
}
