#include "Feedback/DrivingAudioComponent.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "Misc/ScopeLock.h"
#if PLATFORM_ANDROID
#include "Android/AndroidPlatformMisc.h"
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#endif

namespace
{
constexpr float SampleRate=48000.0f;
constexpr float SamplePeriod=1.0f/SampleRate;
constexpr double Tau=6.2831853071795864769;

float Unit(double Value)
{
    return FMath::IsFinite(Value)?static_cast<float>(FMath::Clamp(Value,0.0,1.0)):0.0f;
}

float Envelope(float Age, float Duration)
{
    if (Age<0 || Age>=Duration) return 0;
    return FMath::Min(Age/.012f,1.0f)*FMath::Min((Duration-Age)/.045f,1.0f);
}

float Oscillator(double& Phase, double Frequency)
{
    Phase+=Tau*Frequency/SampleRate;
    if (Phase>=Tau) Phase-=Tau;
    return static_cast<float>(FMath::Sin(Phase));
}

#if PLATFORM_ANDROID
bool InitialAndroidAudioFocus()
{
    JNIEnv* Env=FAndroidApplication::GetJavaEnv();
    if (!Env || !FJavaWrapper::GameActivityThis) return false;
    static jmethodID Method=FJavaWrapper::FindMethod(Env,FJavaWrapper::GameActivityClassID,
        "AndroidThunkJava_BPInitializeAudioFocus","()Z",true);
    if (!Method) return false;
    const bool bGranted=Env->CallBooleanMethod(FJavaWrapper::GameActivityThis,Method)==JNI_TRUE;
    if (Env->ExceptionCheck())
    {
        Env->ExceptionClear();
        UE_LOG(LogTemp,Warning,TEXT("Driving audio focus initialization failed; output remains muted."));
        return false;
    }
    return bGranted;
}
#endif
}

UDrivingProceduralWave::UDrivingProceduralWave(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NumChannels=1;
    SetSampleRate(static_cast<uint32>(SampleRate));
    Duration=INDEFINITELY_LOOPING_DURATION;
    bLooping=true;
    bCanProcessAsync=true;
    VirtualizationMode=EVirtualizationMode::Disabled;
    NumSamplesToGeneratePerCallback=1024;
    NumBufferUnderrunSamples=512;
}

void UDrivingProceduralWave::SetParameters(const FDrivingAudioParameters& Value)
{
    FScopeLock Lock(&ParameterLock);
    Parameters=Value;
}

int32 UDrivingProceduralWave::OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples)
{
    // USoundWaveProcedural requests <=1024 mono samples. Reject unexpected oversized
    // requests rather than growing a FIFO; its built-in fallback supplies silence.
    if (NumSamples<=0 || NumSamples>4096) return 0;
    FDrivingAudioParameters Target;
    {
        FScopeLock Lock(&ParameterLock);
        Target=Parameters;
    }
    if (RenderReset!=Target.Reset)
    {
        RenderReset=Target.Reset;
        RenderWarning=RenderImpact=0;
        Smoothed=FDrivingAudioParameters{};
        MotorPhase=WarningPhase=ImpactPhase=0;
        WindNoise=RoadNoise=RubNoise=ImpactNoise=0;
        WarningAge=ImpactAge=10;
        NoiseSeed=0x42504556u;
    }
    if (RenderWarning!=Target.Warning)
    {
        RenderWarning=Target.Warning;
        WarningAge=0;
        WarningPhase=0;
    }
    if (RenderImpact!=Target.Impact)
    {
        RenderImpact=Target.Impact;
        ImpactAge=0;
        ImpactPhase=0;
    }
    OutAudio.SetNumUninitialized(NumSamples*sizeof(int16), EAllowShrinking::No);
    int16* PCM=reinterpret_cast<int16*>(OutAudio.GetData());
    // 45ms control smoothing keeps changes in policy/render frame rate inaudible.
    const float Slew=1.0f-FMath::Exp(-SamplePeriod/.045f);
    for (int32 Index=0; Index<NumSamples; ++Index)
    {
        Smoothed.Speed+=(Target.Speed-Smoothed.Speed)*Slew;
        Smoothed.Motor+=(Target.Motor-Smoothed.Motor)*Slew;
        Smoothed.Wind+=(Target.Wind-Smoothed.Wind)*Slew;
        Smoothed.Road+=(Target.Road-Smoothed.Road)*Slew;
        Smoothed.Slip+=(Target.Slip-Smoothed.Slip)*Slew;
        Smoothed.Brake+=(Target.Brake-Smoothed.Brake)*Slew;
        NoiseSeed^=NoiseSeed<<13;
        NoiseSeed^=NoiseSeed>>17;
        NoiseSeed^=NoiseSeed<<5;
        const float Noise=static_cast<float>(NoiseSeed>>8)*(2.0f/16777215.0f)-1.0f;
        WindNoise+=(Noise-WindNoise)*.018f;
        RoadNoise+=(Noise-RoadNoise)*.13f;
        RubNoise+=(Noise-RubNoise)*(Target.bIce?.045f:.24f);
        ImpactNoise+=(Noise-ImpactNoise)*.06f;
        const float Fundamental=Oscillator(MotorPhase,74.0+Smoothed.Speed*5.2);
        const float Motor=.12f*Smoothed.Motor*(.75f*Fundamental+.16f*FMath::Sin(2*MotorPhase)+.09f*FMath::Sin(3*MotorPhase));
        const float Wind=.58f*Smoothed.Wind*WindNoise;
        const float Road=(Target.bIce?.25f:.37f)*Smoothed.Road*RoadNoise;
        const float Slip=(Target.bIce?.36f:.24f)*Smoothed.Slip*RubNoise;
        // A broad brake hush is illustrative; no ABS pulses or engine RPM simulation.
        const float Brake=.18f*Smoothed.Brake*(Noise-RoadNoise);
        float Warning=0;
        if (WarningAge<.43f)
        {
            const bool bSecond=WarningAge>=.235f;
            const float Age=bSecond?WarningAge-.235f:WarningAge;
            Warning=.14f*Envelope(Age,.18f)*Oscillator(WarningPhase,bSecond?880:660);
            WarningAge+=SamplePeriod;
        }
        float Impact=0;
        if (ImpactAge<.48f)
        {
            const float Thump=Oscillator(ImpactPhase,58.0+70.0*FMath::Exp(-ImpactAge*22));
            Impact=.66f*Envelope(ImpactAge,.48f)*FMath::Exp(-ImpactAge*10)*(.65f*Thump+.8f*ImpactNoise);
            ImpactAge+=SamplePeriod;
        }
        const float Mixed=Motor+Wind+Road+Slip+Brake+Warning+Impact;
        // Smooth bounded saturation and 18% digital headroom even at every maximum.
        const float Safe=.82f*Mixed/(1.0f+FMath::Abs(Mixed));
        PCM[Index]=static_cast<int16>(FMath::Clamp(Safe,-.82f,.82f)*32767.0f);
    }
    return NumSamples;
}

UDrivingAudioComponent::UDrivingAudioComponent()
{
    PrimaryComponentTick.bCanEverTick=false;
#if !PLATFORM_ANDROID
    bHasAudioFocus=true; // Android requires an explicit OS grant before any output.
#endif
}

void UDrivingAudioComponent::BeginPlay()
{
    Super::BeginPlay();
    Wave=NewObject<UDrivingProceduralWave>(this);
    Voice=NewObject<UAudioComponent>(GetOwner());
    Voice->bAutoActivate=false;
    Voice->bAutoDestroy=false;
    Voice->bStopWhenOwnerDestroyed=true;
    Voice->bAllowSpatialization=false;
    Voice->bIsUISound=false; // Obey engine pause and the normal mixer.
    Voice->SetSound(Wave);
    Voice->RegisterComponent();
    DeactivateHandle=FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this,&UDrivingAudioComponent::SuspendAudio);
    BackgroundHandle=FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this,&UDrivingAudioComponent::SuspendAudio);
    ReactivateHandle=FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this,&UDrivingAudioComponent::ResumeAudio);
    ForegroundHandle=FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this,&UDrivingAudioComponent::ResumeAudio);
#if PLATFORM_ANDROID
    bHasAudioFocus=false;
    AudioFocusHandle=FAndroidMisc::RegisterAudioFocusChangedEventListener(
        FOnAudioFocusChangedDelegate::CreateUObject(this,&UDrivingAudioComponent::AudioFocusChanged));
    // Registration installs the OS callback before this initial result is read.
    AudioFocusChanged(InitialAndroidAudioFocus());
#endif
}

void UDrivingAudioComponent::Silence(bool bImmediate)
{
    if (!bStopping)
    {
        // Retire unfinished cues too: a later unmute/resume must not continue one.
        ++Parameters.Reset;
        Parameters.Warning=Parameters.Impact=0;
    }
    Parameters.Motor=Parameters.Wind=Parameters.Road=Parameters.Slip=Parameters.Brake=0;
    if (Wave) Wave->SetParameters(Parameters);
    if (Voice && Voice->IsPlaying())
    {
        if (bImmediate) Voice->Stop();
        else if (!bStopping) Voice->FadeOut(.025f,0);
    }
    bStopping=true;
    ImpactEndsAt=0;
}

void UDrivingAudioComponent::SuspendAudio()
{
    bSuspended=true;
    Silence(true);
}

void UDrivingAudioComponent::ResumeAudio()
{
    bSuspended=false; // Restart only on the next valid feedback frame.
}

void UDrivingAudioComponent::AudioFocusChanged(bool bHasFocus)
{
    check(IsInGameThread()); // Android's native callback dispatches this delegate to the game thread.
    bHasAudioFocus=bHasFocus;
    if (!bHasAudioFocus) Silence(true); // Includes transient loss / can-duck: yield completely.
    // Focus gain alone does not start output; wait for the next valid feedback frame.
}

void UDrivingAudioComponent::ResetFeedback()
{
    check(IsInGameThread());
    const uint32 NextReset=Parameters.Reset+1;
    Parameters=FDrivingAudioParameters{};
    Parameters.Reset=NextReset;
    bWarningPlayed=bImpactPlayed=bLastCrash=false;
    LastMap=INDEX_NONE;
    Silence(true);
    if (Wave) Wave->ResetAudio();
}

void UDrivingAudioComponent::UpdateFeedback(const BPFeedback::Frame& Frame, const BPFeedback::Output& Output, bool Enabled)
{
    check(IsInGameThread());
    if (!Voice || !Wave) return;
    if (LastMap!=Frame.Map)
    {
        ResetFeedback();
        LastMap=Frame.Map;
    }
    const bool bCrashEdge=Frame.Crash && !bLastCrash;
    bLastCrash=Frame.Crash;
    const bool bAudible=Enabled && bHasAudioFocus && !bSuspended && FApp::HasFocus() && GetWorld() && !UGameplayStatics::IsGamePaused(GetWorld());
    const double Now=FPlatformTime::Seconds();
    const bool bWarningEdge=Frame.Active && Output.Warn && !bWarningPlayed;
    // Consume edges even while muted/unfocused so restoring sound cannot deliver stale alerts.
    if (bWarningEdge) bWarningPlayed=true;
    const bool bImpactEdge=bCrashEdge && Output.Impact && !bImpactPlayed;
    if (bImpactEdge) bImpactPlayed=true;
    if (!bAudible)
    {
        Silence(true);
        return;
    }
    if (bWarningEdge) ++Parameters.Warning;
    if (bImpactEdge)
    {
        ++Parameters.Impact;
        ImpactEndsAt=Now+.55;
    }
    if (!Frame.Active && Now>=ImpactEndsAt)
    {
        Silence(false);
        return;
    }
    Parameters.bIce=Frame.Map!=2;
    Parameters.Speed=FMath::IsFinite(Frame.SpeedMps)?static_cast<float>(FMath::Clamp(Frame.SpeedMps,0.0,100.0)):0;
    Parameters.Motor=Frame.Active?Unit(Output.Motor):0;
    Parameters.Wind=Frame.Active?Unit(Output.Wind):0;
    Parameters.Road=Frame.Active?Unit(Output.Road):0;
    Parameters.Slip=Frame.Active?Unit(Output.Slip):0;
    Parameters.Brake=Frame.Active && Frame.Brake?Unit(Parameters.Speed/16.0):0;
    Wave->SetParameters(Parameters);
    if (bStopping || !Voice->IsPlaying())
    {
        Voice->Stop();
        Voice->FadeIn(.025f,.85f);
        bStopping=false;
    }
}

void UDrivingAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if PLATFORM_ANDROID
    FAndroidMisc::UnregisterAudioFocusChangedEventListener(AudioFocusHandle);
#endif
    FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(DeactivateHandle);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
    FCoreDelegates::ApplicationHasReactivatedDelegate.Remove(ReactivateHandle);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
    Silence(true);
    if (Voice) Voice->DestroyComponent();
    Voice=nullptr;
    Wave=nullptr;
    Super::EndPlay(EndPlayReason);
}
