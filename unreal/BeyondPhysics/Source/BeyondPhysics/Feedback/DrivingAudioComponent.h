#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HAL/CriticalSection.h"
#include "Sound/SoundWaveProcedural.h"
#include "Feedback/FeedbackPolicy.h"
#include "DrivingAudioComponent.generated.h"

class UAudioComponent;

// Copied once per audio callback; neither the renderer nor its state references the pawn.
struct FDrivingAudioParameters
{
    float Speed=0, Motor=0, Wind=0, Road=0, Slip=0, Brake=0;
    bool bIce=true;
    uint32 Reset=0, Warning=0, Impact=0;
};

/** Original, deterministic virtual EV sound, not a recording or a measured vehicle model. */
UCLASS(Transient, NotBlueprintable)
class BEYONDPHYSICS_API UDrivingProceduralWave : public USoundWaveProcedural
{
    GENERATED_BODY()
public:
    UDrivingProceduralWave(const FObjectInitializer& ObjectInitializer);
    void SetParameters(const FDrivingAudioParameters& Value);
    virtual int32 OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples) override;

private:
    FCriticalSection ParameterLock;
    FDrivingAudioParameters Parameters;
    // Everything below is used exclusively by the audio render callback.
    FDrivingAudioParameters Smoothed;
    uint32 RenderReset=MAX_uint32, RenderWarning=0, RenderImpact=0, NoiseSeed=0x42504556u;
    double MotorPhase=0, WarningPhase=0, ImpactPhase=0;
    float WindNoise=0, RoadNoise=0, RubNoise=0, ImpactNoise=0;
    float WarningAge=10, ImpactAge=10;
};

/** One reusable non-spatial voice; no assets, queued PCM, or per-frame UObject creation. */
UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class BEYONDPHYSICS_API UDrivingAudioComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDrivingAudioComponent();
    void UpdateFeedback(const BPFeedback::Frame& Frame, const BPFeedback::Output& Output, bool Enabled);
    void ResetFeedback();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Voice;
    UPROPERTY(Transient) TObjectPtr<UDrivingProceduralWave> Wave;
    FDrivingAudioParameters Parameters;
    FDelegateHandle DeactivateHandle, BackgroundHandle, ReactivateHandle, ForegroundHandle, AudioFocusHandle;
    int32 LastMap=INDEX_NONE;
    bool bWarningPlayed=false, bImpactPlayed=false, bLastCrash=false, bSuspended=false, bStopping=false;
    bool bHasAudioFocus=false;
    double ImpactEndsAt=0;
    void Silence(bool bImmediate);
    void SuspendAudio();
    void ResumeAudio();
    void AudioFocusChanged(bool bHasFocus);
};
