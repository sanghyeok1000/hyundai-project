#pragma once
#include "../Physics/VehicleTypes.h"
#include <algorithm>

namespace BPPhysics
{
// Gameplay input policy, separate from tire dynamics. No real accelerator input.
// Brake has priority; releasing it requests full virtual drive force again.
inline Input AutoDriveInput(double Brake, double Steer)
{
    const double B=std::clamp(Brake,0.0,1.0);
    return {B>0 ? 0.0 : 1.0,B,std::clamp(Steer,-1.0,1.0)};
}
}
