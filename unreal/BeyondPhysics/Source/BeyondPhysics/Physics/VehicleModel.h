#pragma once
#include "VehicleTypes.h"

namespace BPPhysics
{
double WheelLongitudinalSpeed(const State& StateValue, double X, double Y, double SteerRadians);
class VehicleModel
{
public:
    Parameters Params;
    bool bRamp = false; // Reduced road-constrained slope/bank extension; flat path unchanged.
    bool bFourContact = false; // explicit extension; retain the bicycle as a comparison baseline.
    Telemetry Evaluate(const State& StateValue, const Input& InputValue) const;
    bool Step(State& StateValue, const Input& InputValue, double Dt = FixedStep) const;
};
}
