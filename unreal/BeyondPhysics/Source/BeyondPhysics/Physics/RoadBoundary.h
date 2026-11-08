#pragma once
#include "VehicleTypes.h"
#include "VirtualRamp.h"
#include <cmath>

namespace BPPhysics
{
// Road/wall geometry in SI, shared by collision and presentation.
constexpr double RoadHalfWidth = 6.75;
constexpr double RoadHalfLength = 1000.0;
constexpr double RoadStartX = -440.0;
constexpr double CarHalfLength = 2.48;
constexpr double CarHalfWidth = 1.22;
inline bool ReachedRoadEnd(const State& S, bool IsRamp=false)
{
    if(IsRamp) { const double Station=Ramp::At(S.X,S.Y).S; return Station<=0 || Station>=Ramp::Length-5; }
    return std::abs(S.X)>=RoadHalfLength-5.0;
}
inline bool TouchesRoadWall(const State& S, bool IsRamp=false)
{
    if(IsRamp) return Ramp::Touches(S.X,S.Y,S.Yaw);
    const double Extent=std::abs(std::sin(S.Yaw))*CarHalfLength+
                        std::abs(std::cos(S.Yaw))*CarHalfWidth;
    return S.Y-Extent<=-RoadHalfWidth || S.Y+Extent>=RoadHalfWidth;
}
// Locate first contact along the accepted step's interpolated pose. This is a
// gameplay boundary, not an impact-force or rebound model.
inline double WallContactFraction(const State& From, const State& To, bool IsRamp=false)
{
    if(TouchesRoadWall(From,IsRamp)) return 0;
    double Low=0,High=1;
    for(int I=0;I<40;++I)
    {
        const double A=(Low+High)*.5;
        State Pose=From;
        Pose.X=From.X+(To.X-From.X)*A;
        Pose.Y=From.Y+(To.Y-From.Y)*A;
        Pose.Yaw=From.Yaw+(To.Yaw-From.Yaw)*A;
        if(TouchesRoadWall(Pose,IsRamp)) High=A; else Low=A;
    }
    return High;
}
}
