#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Physics/VehicleModel.h"
#include "Experiment/DrivingExperiment.h"
#include "PleosVehicleInput.h"
#include "VehicleInputGate.h"
#include "RampAutoDrive.h"
#include "Experiment/RampLesson.h"
#include "Experiment/FogTrial.h"
#include "Feedback/FeedbackPolicy.h"
#include "TimerManager.h"
#include "DrivingPawn.generated.h"

class UStaticMeshComponent;
class UCameraComponent;
class UMaterialInstanceDynamic;
class UDrivingAudioComponent;

UCLASS()
class BEYONDPHYSICS_API ADrivingPawn : public APawn
{
    GENERATED_BODY()
public:
    ADrivingPawn();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    UPROPERTY(EditAnywhere,Category="Vehicle Model",meta=(ClampMin="100",ClampMax="5000")) float VehicleMass=1500;
    UPROPERTY(EditAnywhere,Category="Vehicle Model",meta=(ClampMin="100",ClampMax="10000")) float YawInertia=2500;
    UPROPERTY(EditAnywhere,Category="Vehicle Model",meta=(ClampMin="0",ClampMax="0.6")) float MaximumSteerRadians=0.5;
    UPROPERTY(EditAnywhere,Category="Environment",meta=(ClampMin="0",ClampMax="2")) double DryFriction=0.85;
    UPROPERTY(EditAnywhere,Category="Environment",meta=(ClampMin="0",ClampMax="2")) double IceFriction=BPPhysics::LowGripFriction;
    UPROPERTY(EditAnywhere,Category="Camera",meta=(ClampMin="45",ClampMax="110")) float ChaseFieldOfView=66;
    UPROPERTY(EditAnywhere,Category="Camera") FVector ChaseOffset=FVector(-950,-240,430);
    UPROPERTY(EditAnywhere,Category="Camera") FVector DriverOffset=FVector(10,-32,120);
    UPROPERTY(EditAnywhere,Category="Vehicle Model",meta=(ClampMin="0",ClampMax="1")) double FrontDriveFraction=5.0/9.0;
    const BPPhysics::State& GetPhysicsState() const { return State; }
    const BPPhysics::Telemetry& GetTelemetry() const { return Telemetry; }
    const BPPhysics::Input& GetControls() const { return Controls; }
    double GetFriction() const { return Model.Params.Friction; }
    double GetSteeringRadians() const { return Model.bFourContact?Telemetry.SteerCenter:Controls.Steer*Model.Params.MaxSteer; }
    double GetWheelSteeringRadians(int32 Wheel) const {return Model.bFourContact?Telemetry.WheelSteer[Wheel]:(Wheel<2?GetSteeringRadians():0);}
    double GetHandwheelDegrees() const {return HandwheelDegrees;}
    bool IsIce() const { return bIce; }
    bool IsRamp() const { return Model.bRamp; }
    bool IsFog() const { return bFogMap; }
    const BPPhysics::FogTrial& GetFogTrial() const { return FogStudy; }
    bool IsPaused() const { return bPaused; }
    bool IsCrashed() const { return bCrashed; }
    bool IsClusterVisible() const { return bCluster; }
    FString GetStatus() const { return Status; }
    double GetSimTime() const { return SimTime; }
    const FDrivingExperiment& GetExperiment() const { return Experiment; }
    const BPPhysics::RampLesson& GetRampLesson() const { return RampStudy; }
    double GetRampDriveTarget() const { return RampDrive.TargetSpeed; }
    bool HasVehicleInput() const { return VehicleInput.bEnabled; }
    bool IsVehicleInputBlocked() const { return VehicleInput.bEnabled && (!VehicleInput.bAllowed || VehicleGate.NeedsRestart); }
    FString GetVehicleInputText() const;
    bool IsFeedbackMenuOpen() const {return bFeedbackMenu;}
    bool HasLightFeedback() const;
    bool HasClimateFeedback() const;
    bool HasSoundFeedback() const;
    const FPleosFeedbackStatus& GetFeedbackStatus() const {return FeedbackStatus;}
    const BPFeedback::Output& GetFeedbackOutput() const {return FeedbackOutput;}
    UFUNCTION(Exec) void BPFeedback(int32 Lights,int32 Climate,int32 Sound);
    UFUNCTION(Exec) void BPFeedbackReport();
    UFUNCTION(Exec) void BPAudioCapture(float Seconds=4);
    UFUNCTION(Exec) void BPBrakeTest();
    UFUNCTION(Exec) void BPTurnTest();
    UFUNCTION(Exec) void BPSetSurface(int32 Ice);
    UFUNCTION(Exec) void BPCamera();
    UFUNCTION(Exec) void BPCluster();
    UFUNCTION(Exec) void BPReset();
    UFUNCTION(Exec) void BPMap();
    UFUNCTION(Exec) void BPReplay();
    UFUNCTION(Exec) void BPRecord();
    UFUNCTION(Exec) void BPTestInput(float Gas,float Brake,float Steer);
    UFUNCTION(Exec) void BPClearTestInput();
    UFUNCTION(Exec) void BPReportState();
    UFUNCTION(Exec) void BPWallTest();
private:
    UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Glass;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Wheels;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> SurfaceMaterials;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UDrivingAudioComponent> DrivingAudio;
    BPFeedback::Output FeedbackOutput;
    FPleosFeedbackStatus FeedbackStatus;
    double FeedbackSeconds=0;
    bool bFeedbackMenu=false,bFeedbackEnding=false;
    int32 PendingFeedbackAction=0;
    bool bAudioCapturing=false;
    FTimerHandle AudioCaptureTimer;
    void FinishAudioCapture();
    void UpdateSensoryFeedback(float DeltaSeconds);
    void FeedbackClick(float X,float Y);
    void ProcessFeedbackAction(int32 Action);
    BPPhysics::VehicleModel Model;
    BPPhysics::State State, Previous;
    BPPhysics::Telemetry Telemetry;
    BPPhysics::Input Controls, TestControls;
    FDrivingExperiment Experiment;
    BPPhysics::RampAutoDrive RampDrive;
    BPPhysics::RampLesson RampStudy;
    bool bRampTrialActive=false;
    bool bFogMap=false,bFogResultSaved=false;
    BPPhysics::Fog::AutoDrive FogDrive;
    BPPhysics::FogTrial FogStudy;
    FPleosVehicleInput VehicleInput;
    BPPhysics::VehicleInputGate VehicleGate;
    bool bTouchWasDown=false;
    bool bPendingRestartTap=false,bPendingMapTap=false;
    void OnTouchPressed(ETouchIndex::Type Finger,FVector Position);
    double Accumulator=0, SimTime=0, StartupStableSeconds=0;
    double WheelRoll[4]={0,0,0,0};
    double HandwheelDegrees=0;
    int32 ExperimentMode=0, ReplayIndex=0;
    bool bIce=true,bPaused=false,bCluster=false,bDriverCamera=true,bHadFocus=false;
    bool bReplay=false,bTestOverride=false,bCrashed=false;
    FString Status=TEXT("AUTO DRIVE / HOLD S TO BRAKE");
    void ResetSimulation(double Speed=0);
    void ApplySurface();
    void UpdatePresentation(double Alpha);
    void SaveExperiment();
    void FinishRampTrial(BPPhysics::RampOutcome Outcome);
    void FinishFogTrial(BPPhysics::FogOutcome Outcome);
};
