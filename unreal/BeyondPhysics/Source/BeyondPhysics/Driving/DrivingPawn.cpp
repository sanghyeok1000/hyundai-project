#include "DrivingPawn.h"
#include "AutoDriveInput.h"
#include "Physics/RoadBoundary.h"
#include "Physics/SteeringGeometry.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/InputComponent.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "UnrealClient.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/ScopeExit.h"
#include "Feedback/DrivingAudioComponent.h"
#include "AudioMixerBlueprintLibrary.h"
#include <cmath>
#if PLATFORM_ANDROID
// Set by the official Unreal Android JNI bootstrap to this app's private files directory.
extern FString GInternalFilePath;
#endif

namespace {
constexpr double Deg=180.0/3.14159265358979323846;
// Session choices survive map travel; vehicle output consent starts OFF each app launch.
bool LightFeedback=false,ClimateFeedback=false,SoundFeedback=true;
}
ADrivingPawn::ADrivingPawn()
{
    PrimaryActorTick.bCanEverTick=true;
    SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("PhysicsOrigin")); SetRootComponent(SceneRoot);
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SportsCarBody")); Body->SetupAttachment(SceneRoot);
    Glass=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SportsCarGlass")); Glass->SetupAttachment(SceneRoot);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyAsset(TEXT("/Game/Vehicles/SportsCar/SM_SportsCar.SM_SportsCar"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> GlassAsset(TEXT("/Game/Vehicles/SportsCar/SM_SportsCar_Glass.SM_SportsCar_Glass"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> WheelAsset(TEXT("/Game/Vehicles/SportsCar/SM_SportsCar_Wheel.SM_SportsCar_Wheel"));
    if(BodyAsset.Succeeded()) Body->SetStaticMesh(BodyAsset.Object);
    if(GlassAsset.Succeeded()) Glass->SetStaticMesh(GlassAsset.Object);
    for(auto* C : {Body.Get(),Glass.Get()}) { C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetSimulatePhysics(false); }
    for(int32 I=0;I<4;++I)
    {
        auto* W=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Wheel%d"),I));
        W->SetupAttachment(SceneRoot); if(WheelAsset.Succeeded()) W->SetStaticMesh(WheelAsset.Object);
        W->SetCollisionEnabled(ECollisionEnabled::NoCollision); W->SetSimulatePhysics(false);
        Wheels.Add(W);
    }
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("DrivingCamera")); Camera->SetupAttachment(SceneRoot);
    Camera->SetFieldOfView(ChaseFieldOfView);
    DrivingAudio=CreateDefaultSubobject<UDrivingAudioComponent>(TEXT("MapSensoryAudio"));
}
void ADrivingPawn::BeginPlay()
{
    Super::BeginPlay();
    Model.Params.Mass=VehicleMass; Model.Params.YawInertia=YawInertia;
    Model.Params.MaxSteer=MaximumSteerRadians;
    Model.Params.FrontDriveFraction=FrontDriveFraction;
    Model.bRamp=GetWorld()->GetMapName().Contains(TEXT("IceRampDrive"));
    bFogMap=GetWorld()->GetMapName().Contains(TEXT("FogHighwayDrive"));
    if(bFogMap){bIce=false;Model.Params.Friction=BPPhysics::Fog::Friction;bDriverCamera=true;
        Model.bFourContact=true;Model.Params.MaxSteer=BPPhysics::Fog::RoadWheelMax;Model.Params.TrackWidth=1.88;}
    VehicleInput=ReadPleosVehicleInput();
    ResetSimulation(); ApplySurface();
    if(auto* PC=Cast<APlayerController>(GetController())) { PC->SetViewTarget(this); PC->bShowMouseCursor=false; }
}
void ADrivingPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindTouch(IE_Pressed,this,&ADrivingPawn::OnTouchPressed);
}
void ADrivingPawn::OnTouchPressed(ETouchIndex::Type Finger,FVector Position)
{
    if(Finger!=ETouchIndex::Touch1) return;
    auto* PC=Cast<APlayerController>(GetController()); if(!PC) return;
    int32 Width=0,Height=0; PC->GetViewportSize(Width,Height);
    const float Scale=Width/1600.f;
    if(Scale>0) FeedbackClick(Position.X/Scale,Position.Y/Scale);
    if(Scale<=0 || Position.Y<118*Scale || Position.Y>155*Scale) return;
    bPendingRestartTap |= Position.X>=715*Scale && Position.X<=945*Scale;
    bPendingMapTap |= Position.X>=970*Scale && Position.X<=1325*Scale;
}
void ADrivingPawn::ResetSimulation(double Speed)
{
    if(DrivingAudio) DrivingAudio->ResetFeedback();
    if(Experiment.bRecording) SaveExperiment();
    FinishRampTrial(BPPhysics::RampOutcome::Restarted);
    FinishFogTrial(BPPhysics::FogOutcome::Aborted);
    State={}; State.X=bFogMap?BPPhysics::Fog::StartX:(Model.bRamp?5.:BPPhysics::RoadStartX); State.U=bFogMap?BPPhysics::Fog::StartSpeed:Speed; Previous=State;
    Accumulator=0; SimTime=0; StartupStableSeconds=0;HandwheelDegrees=0; for(double& Angle:WheelRoll) Angle=0; Controls={};
    ExperimentMode=0; bReplay=false; bTestOverride=false; bPaused=false; bCrashed=false;
    bRampTrialActive=false;
    Telemetry=Model.Evaluate(State,Controls); UpdatePresentation(1);
    if(Model.bRamp) { RampDrive.Reset(); RampStudy.Begin(State,Model.Params.Friction); }
    if(bFogMap){FogDrive.Reset();FogStudy.Reset(State);bFogResultSaved=false;}
}
void ADrivingPawn::ApplySurface()
{
    Model.Params.Friction=bFogMap?BPPhysics::Fog::Friction:(bIce?IceFriction:DryFriction);
    // Appearance stays asphalt. Grip is a physical profile independent of visuals.
}
void ADrivingPawn::BPSetSurface(int32 Ice)
{
    if(bFogMap){Status=TEXT("FOG EXERCISE / DRY ASPHALT MU 0.80");return;}
    if(bCrashed) return;
    ResetSimulation(); bIce=Ice!=0; ApplySurface();
    if(Model.bRamp) RampStudy.Current.Friction=Model.Params.Friction;
    Status=FString::Printf(TEXT("ASPHALT %s / MU %.2f / RESET"),bIce?TEXT("LOW GRIP"):TEXT("NORMAL GRIP"),Model.Params.Friction);
    UE_LOG(LogTemp,Display,TEXT("BP_SURFACE ice=%d mu=%.3f"),bIce,Model.Params.Friction);
}
void ADrivingPawn::BPReset()
{
    if(VehicleInput.bEnabled && !VehicleInput.bAllowed) { Status=VehicleInput.Reason; return; }
    VehicleGate.Restart(true);
    ResetSimulation(); Status=bFogMap?TEXT("HIGHWAY AUTO 100 / BRAKE CANCELS DRIVE"):(Model.bRamp?TEXT("GENTLE AUTO / BRAKE LOWERS DRIVE TARGET"):
        (VehicleInput.bEnabled?TEXT("PLEOS INPUT / AUTO DRIVE"):TEXT("AUTO DRIVE / HOLD S TO BRAKE")));
}
void ADrivingPawn::BPMap()
{
    if(VehicleInput.bEnabled && (!VehicleInput.bAllowed || VehicleGate.NeedsRestart)) return;
    if(Experiment.bRecording) SaveExperiment();
    FinishRampTrial(BPPhysics::RampOutcome::Restarted);
    FinishFogTrial(BPPhysics::FogOutcome::Aborted);
    UGameplayStatics::OpenLevel(this,bFogMap?FName(TEXT("IceTrackDrive")):(Model.bRamp?FName(TEXT("FogHighwayDrive")):FName(TEXT("IceRampDrive"))));
}
FString ADrivingPawn::GetVehicleInputText() const
{
    return FString::Printf(TEXT("PLEOS  RAW %+.1f deg  /  STEER %+.3f  /  BRAKE %.0f  /  %s"),
        VehicleInput.RawSteering,VehicleInput.Steering,VehicleInput.Brake,*VehicleInput.Reason);
}
void ADrivingPawn::BPCamera() { if(!bFogMap)bDriverCamera=!bDriverCamera; }
void ADrivingPawn::BPCluster() { bCluster=!bCluster; }
void ADrivingPawn::BPBrakeTest()
{
    if(bFogMap)return;
    if(VehicleInput.bEnabled) return; // Synthetic experiments stay on the PC path.
    if(bCrashed) return;
    ResetSimulation(15); ExperimentMode=1; Experiment.Begin(State,Model.Params.Friction);
    if(Model.bRamp) RampStudy.Exclude();
    Status=TEXT("BRAKING EXPERIMENT / 54 KM/H");
}
void ADrivingPawn::BPTurnTest()
{
    if(bFogMap)return;
    if(VehicleInput.bEnabled) return;
    if(bCrashed) return;
    ResetSimulation(10); ExperimentMode=2; Experiment.Begin(State,Model.Params.Friction);
    if(Model.bRamp) RampStudy.Exclude();
    Status=TEXT("STEERING EXPERIMENT / 36 KM/H");
}
void ADrivingPawn::SaveExperiment()
{
    Experiment.Finish();
    Status=FString::Printf(TEXT("SAVED / %.2f M / %.2f S"),Experiment.Distance,Experiment.Elapsed);
}
void ADrivingPawn::FinishRampTrial(BPPhysics::RampOutcome Outcome)
{
    if(!Model.bRamp || !bRampTrialActive || !RampStudy.IsRunning()) return;
    bRampTrialActive=false;
    RampStudy.Finish(Outcome);
    const auto& Attempt=RampStudy.Current;
    if(Attempt.Distance<1) return;
    TArray<FString> Rows;
    Rows.Add(TEXT("x_m,y_m,speed_m_s,entry_speed_m_s,max_offset_m,mu,outcome"));
    for(const auto& Point:Attempt.Trace)
        Rows.Add(FString::Printf(TEXT("%.6f,%.6f,%.6f,%.6f,%.6f,%.3f,%d"),Point.X,Point.Y,Point.Speed,
            Attempt.EntrySpeed,Attempt.MaxOffset,Attempt.Friction,static_cast<int32>(Attempt.Outcome)));
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("RampTrials");
    IFileManager::Get().MakeDirectory(*Folder,true);
    const FString File=Folder/FString::Printf(TEXT("ramp_%s_%s.csv"),
        *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")),*FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
    const bool Saved=FFileHelper::SaveStringArrayToFile(Rows,*File,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("BP_RAMP_RESULT outcome=%d entry=%.6f max_offset=%.6f distance=%.6f mu=%.3f saved=%d file=%s"),
        static_cast<int32>(Attempt.Outcome),Attempt.EntrySpeed,Attempt.MaxOffset,Attempt.Distance,Attempt.Friction,Saved,*File);
}
void ADrivingPawn::FinishFogTrial(BPPhysics::FogOutcome Outcome)
{
    if(!bFogMap || bFogResultSaved)return;
    FogStudy.Finish(Outcome);if(FogStudy.IsRunning())return;
    bFogResultSaved=true;if(FogStudy.Elapsed<.01)return;
    TArray<FString> Rows;Rows.Add(TEXT("time_s,x_m,y_m,speed_m_s,brake,gap_m,outcome"));
    for(const auto& Point:FogStudy.Trace)Rows.Add(FString::Printf(TEXT("%.6f,%.6f,%.6f,%.6f,%.0f,%.6f,%d"),
        Point.Time,Point.X,Point.Y,Point.Speed,Point.Brake,Point.Gap,static_cast<int32>(FogStudy.Outcome)));
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("FogTrials");IFileManager::Get().MakeDirectory(*Folder,true);
    const FString File=Folder/FString::Printf(TEXT("fog_%s_%s.csv"),*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")),*FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
    const bool Saved=FFileHelper::SaveStringArrayToFile(Rows,*File,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    TArray<FString> Summary;
    Summary.Add(TEXT("outcome,reference_time_s,brake_time_s,reference_speed_m_s,brake_speed_m_s,reaction_s,reaction_m,after_brake_m,final_gap_m,prior_braking,braking_at_reference,interrupted_braking,mu,reference_vehicle,hit_vehicle,rack_ratio,first_brake_time_s,first_brake_speed_m_s"));
    Summary.Add(FString::Printf(TEXT("%d,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%d,%d,%d,%.3f,%d,%d,%.1f,%.9f,%.9f"),
        static_cast<int32>(FogStudy.Outcome),FogStudy.ReferenceTime,FogStudy.BrakeTime,FogStudy.ReferenceSpeed,FogStudy.BrakeSpeed,
        FogStudy.ReactionTime(),FogStudy.ReactionDistance,FogStudy.BrakingDistance(),FogStudy.FinalGap,
        FogStudy.PreBraked,FogStudy.BrakingAtReference,FogStudy.InterruptedBraking,Model.Params.Friction,FogStudy.ReferenceVehicle,FogStudy.HitVehicle,BPPhysics::Fog::RackRatio,FogStudy.FirstBrakeTime,FogStudy.FirstBrakeSpeed));
    const FString SummaryFile=FPaths::ChangeExtension(File,TEXT("summary.csv"));
    const bool SummarySaved=FFileHelper::SaveStringArrayToFile(Summary,*SummaryFile,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("BP_FOG_RESULT outcome=%d ref_time=%.9f brake_time=%.9f ref_speed=%.6f reaction=%.6f reaction_m=%.6f after_brake_m=%.6f gap=%.6f prebraked=%d interrupted_brake=%d saved=%d summary_saved=%d file=%s"),
        static_cast<int32>(FogStudy.Outcome),FogStudy.ReferenceTime,FogStudy.BrakeTime,FogStudy.ReferenceSpeed,FogStudy.ReactionTime(),FogStudy.ReactionDistance,
        FogStudy.BrakingDistance(),FogStudy.FinalGap,FogStudy.PreBraked,FogStudy.InterruptedBraking,Saved,SummarySaved,*File);
}
void ADrivingPawn::BPRecord()
{
    if(bReplay || bCrashed) return;
    if(Experiment.bRecording) SaveExperiment();
    else { Experiment.Begin(State,Model.Params.Friction); Status=TEXT("RECORDING INPUTS / E TO SAVE"); }
}
void ADrivingPawn::BPReplay()
{
    if(bFogMap)return;
    if(VehicleInput.bEnabled) return;
    if(bCrashed) return;
    if(Experiment.bRecording) SaveExperiment();
    if(Experiment.Inputs.IsEmpty()) { Status=TEXT("NO RECORDING / B, T OR E FIRST"); return; }
    FinishRampTrial(BPPhysics::RampOutcome::Restarted);
    if(Model.bRamp) RampStudy.Exclude();
    State=Experiment.InitialState; Previous=State; Model.Params.Friction=Experiment.Friction;
    bIce=Model.Params.Friction<0.5; ApplySurface(); // current profiles only; restored value below
    Model.Params.Friction=Experiment.Friction;
    bReplay=true; ReplayIndex=0; Accumulator=0; SimTime=0; ExperimentMode=0; bPaused=false;
    Status=TEXT("REPLAY / EXACT RECORDED TICK INPUTS");
}
void ADrivingPawn::BPTestInput(float Gas,float Brake,float Steer)
{
    if(bFogMap)return;
    if(VehicleInput.bEnabled) return;
    if(bCrashed) return;
    FinishRampTrial(BPPhysics::RampOutcome::Restarted);
    if(Model.bRamp) RampStudy.Exclude();
    bTestOverride=true; TestControls={Gas,Brake,Steer}; bPaused=false;
}
void ADrivingPawn::BPClearTestInput() { bTestOverride=false; }
void ADrivingPawn::BPReportState()
{
    BPFeedbackReport();
    UE_LOG(LogTemp,Display,TEXT("BP_STATE time=%.6f x=%.9f y=%.9f speed=%.9f yaw=%.9f r=%.9f mu=%.3f paused=%d crashed=%d"),
        SimTime,State.X,State.Y,Telemetry.Speed,State.Yaw,State.R,Model.Params.Friction,bPaused,bCrashed);
    UE_LOG(LogTemp,Display,TEXT("BP_INPUT vehicle=%d allowed=%d restart=%d raw=%.3f steer=%.6f brake=%.0f throttle=%.6f reason=%s"),
        VehicleInput.bEnabled,VehicleInput.bAllowed,VehicleGate.NeedsRestart,VehicleInput.RawSteering,
        Controls.Steer,Controls.Brake,Controls.Throttle,*VehicleInput.Reason);
    const auto Surface=BPPhysics::Ramp::At(State.X,State.Y);
    UE_LOG(LogTemp,Display,TEXT("BP_ROAD ramp=%d station=%.6f lateral=%.6f z=%.6f"),Model.bRamp,Surface.S,Surface.Lateral,Surface.Z);
    if(Model.bRamp) UE_LOG(LogTemp,Display,TEXT("BP_RAMP_LESSON target=%.6f entry=%.6f previous_entry=%.6f outcome=%d previous_outcome=%d trace=%d previous_trace=%d max_offset=%.6f"),
        RampDrive.TargetSpeed,RampStudy.Current.EntrySpeed,RampStudy.Previous.EntrySpeed,
        static_cast<int32>(RampStudy.Current.Outcome),static_cast<int32>(RampStudy.Previous.Outcome),
        static_cast<int32>(RampStudy.Current.Trace.size()),static_cast<int32>(RampStudy.Previous.Trace.size()),RampStudy.Current.MaxOffset);
    if(bFogMap) UE_LOG(LogTemp,Display,TEXT("BP_FOG_STATE outcome=%d gap=%.6f ref_time=%.6f ref_speed=%.6f reaction=%.6f reaction_m=%.6f after_brake_m=%.6f cancelled=%d trace=%d"),
        static_cast<int32>(FogStudy.Outcome),BPPhysics::Fog::Gap(State,FogStudy.ReferenceVehicle),FogStudy.ReferenceTime,FogStudy.ReferenceSpeed,
        FogStudy.ReactionTime(),FogStudy.ReactionDistance,FogStudy.BrakingDistance(),FogDrive.Cancelled,static_cast<int32>(FogStudy.Trace.size()));
    if(bFogMap)UE_LOG(LogTemp,Display,TEXT("BP_FOG_STEERING hand_deg=%.6f road_deg=%.6f fl_deg=%.6f fr_deg=%.6f ratio=%.1f reference_id=%d hit_id=%d four_contact=%d"),
        HandwheelDegrees,Telemetry.SteerCenter*Deg,Telemetry.WheelSteer[0]*Deg,Telemetry.WheelSteer[1]*Deg,
        BPPhysics::Fog::RackRatio,FogStudy.ReferenceVehicle,FogStudy.HitVehicle,Model.bFourContact);
}
void ADrivingPawn::BPWallTest()
{
    if(bFogMap)return;
    if(VehicleInput.bEnabled) return;
    if(bCrashed) return;
    ResetSimulation(); State.Y=5.4; State.V=2; Previous=State;
    if(Model.bRamp) RampStudy.Exclude();
    bTestOverride=true; TestControls={};
    Status=TEXT("WALL CONTACT CHECK");
}
bool ADrivingPawn::HasLightFeedback() const {return LightFeedback;}
bool ADrivingPawn::HasClimateFeedback() const {return ClimateFeedback;}
bool ADrivingPawn::HasSoundFeedback() const {return SoundFeedback;}
void ADrivingPawn::BPFeedback(int32 Lights,int32 Climate,int32 Sound)
{
    if(Lights==0||Lights==1)LightFeedback=Lights!=0;
    if(Climate==0||Climate==1)ClimateFeedback=Climate!=0;
    if(Sound==0||Sound==1)SoundFeedback=Sound!=0;
    FeedbackSeconds=1;
    UE_LOG(LogTemp,Display,TEXT("BP_FEEDBACK_OPTIONS lights=%d climate=%d sound=%d"),LightFeedback,ClimateFeedback,SoundFeedback);
}
void ADrivingPawn::BPFeedbackReport()
{
    FeedbackSeconds=1;UpdateSensoryFeedback(0);
    UE_LOG(LogTemp,Display,TEXT("BP_FEEDBACK lights_opt=%d climate_opt=%d sound=%d menu=%d lights=%s climate=%s color=%u brightness=%.3f fan=%d cooling=%.2f motor=%.3f wind=%.3f slip=%.3f warn=%d detail=%s"),
        LightFeedback,ClimateFeedback,SoundFeedback,bFeedbackMenu,*FeedbackStatus.Lights,*FeedbackStatus.Climate,FeedbackOutput.ColorArgb,
        FeedbackOutput.Brightness,FeedbackOutput.Fan,FeedbackOutput.CoolOffset,FeedbackOutput.Motor,FeedbackOutput.Wind,FeedbackOutput.Slip,FeedbackOutput.Warn,*FeedbackStatus.Detail);
}
void ADrivingPawn::FeedbackClick(float X,float Y)
{
    if(X>=1340&&X<=1570&&Y>=118&&Y<=155){PendingFeedbackAction=bFeedbackMenu?5:1;return;}
    if(!bFeedbackMenu||X<350||X>1250)return;
    if(Y>=285&&Y<=329)PendingFeedbackAction=2;
    if(Y>=345&&Y<=389)PendingFeedbackAction=3;
    if(Y>=405&&Y<=449)PendingFeedbackAction=4;
    if(Y>=525&&Y<=570)PendingFeedbackAction=5;
}
void ADrivingPawn::ProcessFeedbackAction(int32 Action)
{
    if(Action==1){bFeedbackMenu=true;bPaused=true;Accumulator=0;Controls={};Status=TEXT("EFFECTS SETTINGS / APPLY TO RESTART");}
    else if(Action==2)BPFeedback(!LightFeedback,-1,-1);
    else if(Action==3)BPFeedback(-1,!ClimateFeedback,-1);
    else if(Action==4)BPFeedback(-1,-1,!SoundFeedback);
    else if(Action==5){bFeedbackMenu=false;BPReset();}
    FeedbackSeconds=1;
}
void ADrivingPawn::UpdateSensoryFeedback(float Dt)
{
    BPFeedback::Frame F;
    F.Map=bFogMap?2:(Model.bRamp?1:0);
    const bool InputAllowed=!VehicleInput.bEnabled||(VehicleInput.bAllowed&&!VehicleGate.NeedsRestart);
    F.Active=InputAllowed&&!bPaused&&!bCrashed&&!bFeedbackMenu&&!bFeedbackEnding&&SimTime>0;
    F.Crash=bCrashed&&InputAllowed&&!bFeedbackEnding&&!bFeedbackMenu;
    F.Brake=Controls.Brake>0;
    F.SpeedMps=Telemetry.Speed;
    F.Usage=FMath::Max(Telemetry.Front.Utilization,Telemetry.Rear.Utilization);
    F.SlipRad=FMath::Max(FMath::Abs(Telemetry.Front.SlipAngle),FMath::Abs(Telemetry.Rear.SlipAngle));
    if(bFogMap){for(int Index=0;Index<3;++Index)if(BPPhysics::Fog::InReferenceView(State,Index)){
        const double Gap=BPPhysics::Fog::Gap(State,Index);
        if(!F.FogVisible||Gap<F.GapM){F.FogVisible=true;F.GapM=Gap;}
    }}
    auto Next=BPFeedback::Evaluate(F);
    // Smooth light transitions only; no flashing, and never modify vehicle dynamics.
    const double Blend=Dt>0?1-std::exp(-FMath::Clamp(static_cast<double>(Dt),0.,.25)/.35):1;
    auto Color=std::uint32_t(0xff000000u);
    for(int Shift:{0,8,16})Color|=static_cast<std::uint32_t>(FMath::RoundToInt(FMath::Lerp(double((FeedbackOutput.ColorArgb>>Shift)&255),double((Next.ColorArgb>>Shift)&255),Blend)))<<Shift;
    Next.ColorArgb=Color;FeedbackOutput=Next;
    if(DrivingAudio)DrivingAudio->UpdateFeedback(F,FeedbackOutput,SoundFeedback&&!bFeedbackEnding);
    FeedbackSeconds+=FMath::IsFinite(Dt)?FMath::Clamp(double(Dt),0.,1.):0;
    if(FeedbackSeconds<.25)return;
    FeedbackSeconds=0;
    const FString Command=FString::Printf(TEXT("{\"schema\":1,\"active\":%s,\"lights\":%s,\"climate\":%s,\"color\":%u,\"brightness\":%.3f,\"fan\":%d,\"coolOffset\":%.2f,\"map\":%d}"),
        F.Active?TEXT("true"):TEXT("false"),LightFeedback&&!bFeedbackEnding?TEXT("true"):TEXT("false"),ClimateFeedback&&!bFeedbackEnding?TEXT("true"):TEXT("false"),
        FeedbackOutput.ColorArgb,FeedbackOutput.Brightness,FeedbackOutput.Fan,FeedbackOutput.CoolOffset,F.Map);
    const auto New=SendPleosFeedback(Command);
    if(New.Lights!=FeedbackStatus.Lights||New.Climate!=FeedbackStatus.Climate||New.Detail!=FeedbackStatus.Detail)
        UE_LOG(LogTemp,Display,TEXT("BP_FEEDBACK_STATUS lights=%s climate=%s detail=%s"),*New.Lights,*New.Climate,*New.Detail);
    FeedbackStatus=New;
}
void ADrivingPawn::EndPlay(const EEndPlayReason::Type Reason)
{
    if(bAudioCapturing)FinishAudioCapture();
    bFeedbackEnding=true;FeedbackSeconds=1;UpdateSensoryFeedback(0);
    Super::EndPlay(Reason);
}
void ADrivingPawn::BPAudioCapture(float Seconds)
{
    if(bAudioCapturing||!GetWorld()||!FMath::IsFinite(Seconds))return;
    Seconds=FMath::Clamp(Seconds,2.f,10.f);
    UAudioMixerBlueprintLibrary::StartRecordingOutput(this,Seconds);
    bAudioCapturing=true;
    GetWorld()->GetTimerManager().SetTimer(AudioCaptureTimer,this,&ADrivingPawn::FinishAudioCapture,Seconds,false);
    UE_LOG(LogTemp,Display,TEXT("BP_AUDIO_CAPTURE started=1 seconds=%.2f"),Seconds);
}
void ADrivingPawn::FinishAudioCapture()
{
    if(!bAudioCapturing)return;
    bAudioCapturing=false;
    if(GetWorld())GetWorld()->GetTimerManager().ClearTimer(AudioCaptureTimer);
    const FString Directory=
#if PLATFORM_ANDROID
        GInternalFilePath/TEXT("AudioProof");
#else
        FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("AudioProof"));
#endif
    IFileManager::Get().MakeDirectory(*Directory,true);
    const FString Name=FString::Printf(TEXT("map_%d_capture"),bFogMap?2:(Model.bRamp?1:0));
    UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,Name,Directory);
    UE_LOG(LogTemp,Display,TEXT("BP_AUDIO_CAPTURE export_requested=1 file=%s/%s.wav"),*Directory,*Name);
}
void ADrivingPawn::Tick(float Dt)
{
    Super::Tick(Dt);
    // Every return path, including SDK block, collision and loading stalls, retires feedback.
    ON_SCOPE_EXIT { UpdateSensoryFeedback(Dt); };
    // Latch press events even when press and release occur between render ticks.
    const bool EventRestartTap=bPendingRestartTap,EventMapTap=bPendingMapTap;
    bPendingRestartTap=bPendingMapTap=false;
    auto* PC=Cast<APlayerController>(GetController()); if(!PC) return;
    if(PC->WasInputKeyJustPressed(EKeys::F))PendingFeedbackAction=bFeedbackMenu?5:1;
    if(bFeedbackMenu){
        if(PC->WasInputKeyJustPressed(EKeys::L))PendingFeedbackAction=2;
        if(PC->WasInputKeyJustPressed(EKeys::H))PendingFeedbackAction=3;
        if(PC->WasInputKeyJustPressed(EKeys::K))PendingFeedbackAction=4;
    }
    float FeedbackX=0,FeedbackY=0;
    if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)&&PC->GetMousePosition(FeedbackX,FeedbackY)){
        int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
        if(Width>0)FeedbackClick(FeedbackX*1600.f/Width,FeedbackY*1600.f/Width);
    }
    const int32 Action=PendingFeedbackAction;PendingFeedbackAction=0;
    if(Action)ProcessFeedbackAction(Action);
    const auto NextInput=ReadPleosVehicleInput();
    if(NextInput.bEnabled && (NextInput.bAllowed!=VehicleInput.bAllowed || NextInput.Reason!=VehicleInput.Reason ||
       NextInput.RawSteering!=VehicleInput.RawSteering || NextInput.Brake!=VehicleInput.Brake))
        UE_LOG(LogTemp,Display,TEXT("PLEOS_INPUT allowed=%d raw=%.3f normalized=%.6f brake=%.0f reason=%s"),
            NextInput.bAllowed,NextInput.RawSteering,NextInput.Steering,NextInput.Brake,*NextInput.Reason);
    VehicleInput=NextInput;
    if(VehicleInput.bEnabled)
    {
        VehicleGate.Update(VehicleInput.bAllowed,VehicleInput.LossEpoch);
        if(!VehicleInput.bAllowed)
        {
            if(bFogMap && SimTime>0)FinishFogTrial(BPPhysics::FogOutcome::Aborted);
            Controls={}; Accumulator=0; StartupStableSeconds=0;
            if(VehicleGate.NeedsRestart) bPaused=true;
            Status=VehicleInput.Reason; UpdatePresentation(1); return;
        }
    }
    if(bFeedbackMenu){Controls={};Accumulator=0;UpdatePresentation(1);return;}
    bool RestartTap=EventRestartTap,MapTap=EventMapTap,TouchDown=false; float TouchX=0,TouchY=0;
    if(VehicleInput.bEnabled)
    {
        PC->GetInputTouchState(ETouchIndex::Touch1,TouchX,TouchY,TouchDown);
        int32 Width=0,Height=0; PC->GetViewportSize(Width,Height);
        const float Scale=Width/1600.f;
        float ClickX=TouchX,ClickY=TouchY;
        const bool Clicked=PC->WasInputKeyJustPressed(EKeys::LeftMouseButton) && PC->GetMousePosition(ClickX,ClickY);
        if((TouchDown && !bTouchWasDown) || Clicked) {
            RestartTap |= Scale>0 && ClickX>=715*Scale && ClickX<=945*Scale && ClickY>=118*Scale && ClickY<=155*Scale;
            MapTap |= Scale>0 && ClickX>=970*Scale && ClickX<=1325*Scale && ClickY>=118*Scale && ClickY<=155*Scale;
        }
        bTouchWasDown=TouchDown;
    }
    if(PC->WasInputKeyJustPressed(EKeys::R) || RestartTap) BPReset();
    if(PC->WasInputKeyJustPressed(EKeys::M) || MapTap) { BPMap(); return; }
    if(PC->WasInputKeyJustPressed(EKeys::V)) BPCamera();
    if(PC->WasInputKeyJustPressed(EKeys::Tab)) BPCluster();
    if(VehicleInput.bEnabled && VehicleGate.NeedsRestart)
    { Controls={}; Accumulator=0; bPaused=true; Status=TEXT("PARK RESTORED / TAP RESTART OR R"); UpdatePresentation(1); return; }
    if(bCrashed) { UpdatePresentation(1); return; }
    if(PC->WasInputKeyJustPressed(EKeys::One)) BPSetSurface(0);
    if(PC->WasInputKeyJustPressed(EKeys::Two)) BPSetSurface(1);
    if(PC->WasInputKeyJustPressed(EKeys::B)) BPBrakeTest();
    if(PC->WasInputKeyJustPressed(EKeys::T)) BPTurnTest();
    if(PC->WasInputKeyJustPressed(EKeys::E)) BPRecord();
    if(PC->WasInputKeyJustPressed(EKeys::Enter)) BPReplay();
    if(PC->WasInputKeyJustPressed(EKeys::SpaceBar)) { bPaused=!bPaused; Accumulator=0; if(bFogMap && SimTime>0)FinishFogTrial(BPPhysics::FogOutcome::Aborted); }
    if(Model.bRamp && !RampStudy.IsRunning() && !bReplay && !bTestOverride && ExperimentMode==0)
    { bPaused=true; Accumulator=0; Controls={}; Status=TEXT("ATTEMPT ENDED / RESTART FOR A NEW COMPARISON"); UpdatePresentation(1); return; }
    if(bFogMap && !FogStudy.IsRunning())
    { bPaused=true;Accumulator=0;Controls={};Status=TEXT("FOG EXERCISE ENDED / RESTART TO REPEAT");UpdatePresentation(1);return; }
    const bool Focus=GetWorld()->GetGameViewport() && GetWorld()->GetGameViewport()->Viewport &&
        GetWorld()->GetGameViewport()->Viewport->HasFocus();
    if(bHadFocus && !Focus && !bTestOverride) { bPaused=true; Controls={}; Accumulator=0; Status=TEXT("FOCUS LOST / SPACE TO RESUME"); if(bFogMap)FinishFogTrial(BPPhysics::FogOutcome::Aborted); }
    bHadFocus=Focus;
    if(bPaused) { UpdatePresentation(1); return; }
    if(SimTime==0 && !bReplay && !bTestOverride && ExperimentMode==0 && StartupStableSeconds<.75)
    {
        // Prepare rendering/streaming before automatic drive begins. No simulation time is skipped.
        StartupStableSeconds=Dt<.1f ? StartupStableSeconds+Dt : 0;
        Accumulator=0; Status=TEXT("PREPARING SCENE / AUTO DRIVE NEXT");
        UpdatePresentation(1); return;
    }
    if(SimTime==0 && !bReplay && !bTestOverride && ExperimentMode==0)
        Status=bFogMap?TEXT("HIGHWAY AUTO 100 / BRAKE CANCELS DRIVE"):(Model.bRamp?TEXT("GENTLE AUTO / BRAKE LOWERS DRIVE TARGET"):
            (VehicleInput.bEnabled?TEXT("PLEOS INPUT / AUTO DRIVE / BRAKE TO STOP"):TEXT("AUTO DRIVE / HOLD S TO BRAKE")));
    // Loading stalls before the first simulated tick are not driving time.
    if(Dt>0.25f && SimTime==0) { Accumulator=0; UpdatePresentation(1); return; }
    if(Dt>0.25f) { bPaused=true; Accumulator=0; Status=TEXT("FRAME STALL / SPACE TO RESUME"); if(bFogMap)FinishFogTrial(BPPhysics::FogOutcome::Aborted); return; }
    Accumulator+=Dt;
    if(Accumulator>BPPhysics::FixedStep*48) { bPaused=true; Status=TEXT("STEP BUDGET / SPACE TO RESUME"); Accumulator=0; if(bFogMap)FinishFogTrial(BPPhysics::FogOutcome::Aborted); return; }
    const double DesiredSteer=VehicleInput.bEnabled?VehicleInput.Steering:
        (PC->IsInputKeyDown(EKeys::A)?1.0:0.0)-(PC->IsInputKeyDown(EKeys::D)?1.0:0.0);
    while(Accumulator+1e-12>=BPPhysics::FixedStep)
    {
        Previous=State;
        const double BeforeSpeed=Telemetry.Speed;
        const bool TrackRamp=Model.bRamp && RampStudy.IsRunning() && !bReplay && !bTestOverride && ExperimentMode==0;
        const bool TrackFog=bFogMap && FogStudy.IsRunning() && !bReplay && !bTestOverride && ExperimentMode==0;
        if(bReplay)
        {
            if(ReplayIndex>=Experiment.Inputs.Num()) { bReplay=false; bPaused=true; Status=TEXT("REPLAY COMPLETE / SPACE OR R"); break; }
            Controls=Experiment.Inputs[ReplayIndex++];
        }
        else if(ExperimentMode==1) Controls={0,1,0};
        else if(ExperimentMode==2) Controls={0,0,SimTime<1?0:0.2};
        else if(bTestOverride) Controls=TestControls;
        else
        {
            double Steer;
            if(bFogMap){
                const double DesiredHand=VehicleInput.bEnabled?VehicleInput.RawSteering:DesiredSteer*BPPhysics::Fog::HandwheelMaxDegrees;
                HandwheelDegrees=VehicleInput.bEnabled?DesiredHand:FMath::FInterpConstantTo(HandwheelDegrees,DesiredHand,BPPhysics::FixedStep,540.);
                Steer=BPPhysics::HandwheelToRoadRadians(HandwheelDegrees,BPPhysics::Fog::RackRatio,Model.Params.MaxSteer)/Model.Params.MaxSteer;
            }else Steer=VehicleInput.bEnabled?DesiredSteer:FMath::FInterpConstantTo(Controls.Steer,DesiredSteer,BPPhysics::FixedStep,2.5);
            const double Brake=VehicleInput.bEnabled?VehicleInput.Brake:(PC->IsInputKeyDown(EKeys::S)?1.0:0.0);
            Controls=bFogMap?FogDrive.Update(Brake,Steer,Telemetry.Speed):(Model.bRamp?RampDrive.Update(Brake,Steer,Telemetry.Speed):BPPhysics::AutoDriveInput(Brake,Steer));
            if(Model.bRamp && RampStudy.IsRunning()) bRampTrialActive=true;
        }
        if(!Model.Step(State,Controls)) { bPaused=true; Status=TEXT("INVALID MODEL / RESET REQUIRED"); if(bFogMap)FinishFogTrial(BPPhysics::FogOutcome::Aborted); break; }
        const double FogContact=bFogMap?BPPhysics::Fog::ContactFraction(Previous,State):1.;
        if(FogContact<0){State=Previous;Telemetry=Model.Evaluate(State,Controls);bPaused=true;Accumulator=0;
            FinishFogTrial(BPPhysics::FogOutcome::Aborted);Status=TEXT("CONTACT RESOLUTION LIMIT / R TO RESTART");break;}
        if(BPPhysics::TouchesRoadWall(State,Model.bRamp) || (bFogMap && (FogContact<1 || BPPhysics::Fog::TouchesLead(State))))
        {
            const double A=bFogMap?FogContact:BPPhysics::WallContactFraction(Previous,State,Model.bRamp);
            State.X=FMath::Lerp(Previous.X,State.X,A);
            State.Y=FMath::Lerp(Previous.Y,State.Y,A);
            State.Yaw=FMath::Lerp(Previous.Yaw,State.Yaw,A);
            State.U=FMath::Lerp(Previous.U,State.U,A);
            State.V=FMath::Lerp(Previous.V,State.V,A);
            State.R=FMath::Lerp(Previous.R,State.R,A);
            SimTime+=BPPhysics::FixedStep*A;
            Telemetry=Model.Evaluate(State,Controls);
            if(TrackRamp) { RampStudy.Observe(Previous,State,BeforeSpeed,Telemetry.Speed); FinishRampTrial(BPPhysics::RampOutcome::Wall); }
            if(TrackFog){FogStudy.Observe(Previous,State,BeforeSpeed,Telemetry.Speed,Controls.Brake,BPPhysics::FixedStep*A);FinishFogTrial(BPPhysics::Fog::TouchesLead(State)?BPPhysics::FogOutcome::Collision:BPPhysics::FogOutcome::Wall);}
            Experiment.Sample(Controls,Previous,State,Telemetry,BPPhysics::FixedStep*A);
            if(Experiment.bRecording) SaveExperiment();
            bCrashed=true; bPaused=true; bReplay=false; bTestOverride=false;
            ExperimentMode=0; Accumulator=0; Controls={}; Previous=State;
            Status=bFogMap?TEXT("CONTACT / R TO RESTART"):TEXT("WALL CONTACT / R TO RESTART");
            UE_LOG(LogTemp,Display,TEXT("BP_WALL_CONTACT time=%.9f y=%.9f yaw=%.9f speed=%.9f"),SimTime,State.Y,State.Yaw,Telemetry.Speed);
            break;
        }
        SimTime+=BPPhysics::FixedStep; Accumulator-=BPPhysics::FixedStep;
        Telemetry=Model.Evaluate(State,Controls);
        if(TrackRamp) RampStudy.Observe(Previous,State,BeforeSpeed,Telemetry.Speed);
        if(TrackFog)FogStudy.Observe(Previous,State,BeforeSpeed,Telemetry.Speed,Controls.Brake,BPPhysics::FixedStep);
        for(int32 Wheel=0;Wheel<4;++Wheel)
        {
            const double X=Wheel<2?Model.Params.FrontDistance:-Model.Params.RearDistance;
            const double Y=(Wheel%2?-.5:.5)*(bFogMap?Model.Params.TrackWidth:1.88);
            const double Steer=GetWheelSteeringRadians(Wheel);
            WheelRoll[Wheel]+=BPPhysics::WheelLongitudinalSpeed(State,X,Y,Steer)*BPPhysics::FixedStep/.39267;
        }
        Experiment.Sample(Controls,Previous,State,Telemetry);
        if(TrackFog && !FogStudy.IsRunning()){
            FinishFogTrial(FogStudy.Outcome);bPaused=true;Accumulator=0;Controls={};Previous=State;
            Status=TEXT("FOG EXERCISE RESULT / RESTART TO REPEAT");break;
        }
        if((ExperimentMode==1 && Telemetry.Speed<0.01) || (ExperimentMode==2 && SimTime>=12))
        { ExperimentMode=0; SaveExperiment(); bPaused=true; break; }
        if(bFogMap?std::abs(State.X)>=BPPhysics::Fog::HalfLength-BPPhysics::CarHalfLength:BPPhysics::ReachedRoadEnd(State,Model.bRamp))
        {
            if(Experiment.bRecording) SaveExperiment();
            const bool RampComplete=Model.bRamp && BPPhysics::Ramp::At(State.X,State.Y).S>=BPPhysics::Ramp::Length-5;
            if(TrackRamp) FinishRampTrial(RampComplete?BPPhysics::RampOutcome::Complete:BPPhysics::RampOutcome::Restarted);
            bPaused=true; Status=RampComplete?TEXT("RAMP COMPLETE / COMPARE ENTRY AND PATH / R"):TEXT("TEST AREA LIMIT / R TO RESET"); break;
        }
    }
    UpdatePresentation(bPaused?1:Accumulator/BPPhysics::FixedStep);
}
void ADrivingPawn::UpdatePresentation(double Alpha)
{
    const double X=FMath::Lerp(Previous.X,State.X,Alpha),Y=FMath::Lerp(Previous.Y,State.Y,Alpha);
    const double Yaw=FMath::Lerp(Previous.Yaw,State.Yaw,Alpha);
    if(Model.bRamp) {
        const auto Road=BPPhysics::Ramp::At(X,Y);
        const double C=std::cos(Yaw),S=std::sin(Yaw);
        const FVector Forward(C,-S,Road.Hx*C+Road.Hy*S),Normal(-Road.Hx,Road.Hy,1);
        SetActorLocationAndRotation(FVector(X*100,-Y*100,Road.Z*100),FRotationMatrix::MakeFromXZ(Forward,Normal).Rotator());
    } else SetActorLocationAndRotation(FVector(X*100,-Y*100,0),FRotator(0,-Yaw*Deg,0));
    const float WheelX[4]={145,145,-145,-145};
    const float WheelY[4]={-94,94,-94,94};
    const FVector BodyScale=bFogMap?FVector(2.7/2.9,1,1):FVector(1);
    for(auto* Part:{Body.Get(),Glass.Get()}){Part->SetRelativeScale3D(BodyScale);Part->SetRelativeLocation(bFogMap?FVector(-15,0,0):FVector::ZeroVector);}
    for(int32 I=0;I<4;++I)
    {
        Wheels[I]->SetRelativeLocation(FVector(bFogMap?(I<2?Model.Params.FrontDistance:-Model.Params.RearDistance)*100:WheelX[I],bFogMap?(I%2?.5:-.5)*Model.Params.TrackWidth*100:WheelY[I],39.4));
        // Keep roll as a local axle rotation after steering; right wheels mirror about Z.
        const FQuat SteerRotation(FVector::UpVector,-GetWheelSteeringRadians(I)+(I%2?PI:0));
        const FQuat RollRotation(FVector::RightVector,-WheelRoll[I]*(I%2?-1:1));
        Wheels[I]->SetWorldRotation(GetActorQuat()*SteerRotation*RollRotation);
        Wheels[I]->SetVisibility(!bDriverCamera);
    }
    Body->SetVisibility(!bDriverCamera); Glass->SetVisibility(!bDriverCamera);
    if(bDriverCamera) { Camera->SetRelativeLocation(DriverOffset); Camera->SetRelativeRotation(FRotator(-2,0,0)); Camera->SetFieldOfView(80); }
    else { Camera->SetRelativeLocation(ChaseOffset); Camera->SetRelativeRotation((FVector(220,0,75)-ChaseOffset).Rotation()); Camera->SetFieldOfView(ChaseFieldOfView); }
}
