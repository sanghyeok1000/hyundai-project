#pragma once
#include "AutoDriveInput.h"
namespace BPPhysics {
// Map02 gameplay policy. Only drive demand changes; no extra force, auto brake,
// velocity clamp or knowledge of road curvature. Downhill speed can rise naturally.
struct RampAutoDrive {
    static constexpr double InitialTarget=50/3.6,RestartTarget=15/3.6,MaxThrottle=.12;
    double TargetSpeed=InitialTarget;
    void Reset(){TargetSpeed=InitialTarget;}
    Input Update(double brake,double steer,double speed){
        const double b=std::clamp(brake,0.,1.);
        if(b>0) TargetSpeed=std::min(TargetSpeed,std::max(RestartTarget,speed));
        const double throttle=b>0?0.:std::clamp((TargetSpeed-speed)*.25,0.,MaxThrottle);
        return {throttle,b,std::clamp(steer,-1.,1.)};
    }
};
}
