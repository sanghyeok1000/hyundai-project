#pragma once
#include "CoreMinimal.h"

struct FPleosVehicleInput
{
    bool bEnabled=false;
    bool bAllowed=false;
    uint64 LossEpoch=0;
    double RawSteering=0, Steering=0, Brake=0;
    FString Reason=TEXT("KEYBOARD");
};

FPleosVehicleInput ReadPleosVehicleInput();

struct FPleosFeedbackStatus {
    FString Lights=TEXT("SIMULATED"),Climate=TEXT("SIMULATED"),Detail=TEXT("PC PREVIEW / NO VEHICLE WRITE");
};
FPleosFeedbackStatus SendPleosFeedback(const FString& Command);
