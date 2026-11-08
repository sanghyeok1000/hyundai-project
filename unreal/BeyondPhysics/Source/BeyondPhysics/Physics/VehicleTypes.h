#pragma once
#include <cstdint>

// SI throughout. Body x forward, y LEFT, positive yaw/steer LEFT.
namespace BPPhysics
{
constexpr double LowGripFriction = 0.12; // Virtual low-grip profile; not calibrated to measured ice/tire data.
struct Parameters
{
    double Mass = 1500.0;
    double YawInertia = 2500.0;
    double FrontDistance = 1.2;
    double RearDistance = 1.5;
    double FrontStiffness = 80000.0;
    double RearStiffness = 90000.0;
    double Gravity = 9.81;
    double Friction = LowGripFriction;
    double DriveForce = 6000.0;
    double BrakeForce = 18000.0;
    double FrontDriveFraction = 5.0 / 9.0; // Virtual AWD, proportional to static axle loads.
    double MaxSteer = 0.5;
    double TrackWidth = 1.88; // representative geometry matched to rendered wheel centers [m]
    double SlipRegularizationSpeed = 0.5;
    double BrakeRegularizationSpeed = 0.1;
};
struct Input { double Throttle = 0, Brake = 0, Steer = 0; };
struct State { double X = 0, Y = 0, Yaw = 0, U = 0, V = 0, R = 0; };
struct AxleTelemetry
{
    double SlipAngle = 0, Fx = 0, Fy = 0, Fz = 0, Utilization = 0;
};
struct Telemetry
{
    AxleTelemetry Front, Rear;
    AxleTelemetry Wheels[4]{}; // FL, FR, RL, RR; wheel-local forces
    double WheelSteer[4]{};
    double SteerCenter=0;
    double Fx = 0, Fy = 0, Moment = 0, Ax = 0, Ay = 0, Speed = 0;
};
constexpr double FixedStep = 1.0 / 240.0;
}
