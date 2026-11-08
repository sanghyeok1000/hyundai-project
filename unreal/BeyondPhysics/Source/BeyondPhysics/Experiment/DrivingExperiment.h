#pragma once
#include "CoreMinimal.h"
#include "Physics/VehicleTypes.h"

// Records exactly the normalized inputs consumed by each fixed step.
class FDrivingExperiment
{
public:
    TArray<BPPhysics::Input> Inputs;
    TArray<FString> Rows;
    FString LastFile;
    bool bRecording = false;
    double StartX = 0, StartY = 0, Distance = 0, Elapsed = 0;
    BPPhysics::State InitialState;
    double Friction = BPPhysics::LowGripFriction;
    void Begin(const BPPhysics::State& State, double Mu);
    void Sample(const BPPhysics::Input& Input, const BPPhysics::State& Previous,
                const BPPhysics::State& State, const BPPhysics::Telemetry& Telemetry,
                double StepDuration=BPPhysics::FixedStep);
    FString Finish();
};
