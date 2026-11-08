#include "DrivingExperiment.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include <cmath>

void FDrivingExperiment::Begin(const BPPhysics::State& S, double Mu)
{
    Inputs.Reset(); Rows.Reset(); InitialState=S; Friction=Mu;
    StartX=S.X; StartY=S.Y; Distance=0; Elapsed=0; bRecording=true;
    Rows.Add(TEXT("tick,time_s,throttle,brake,steer_normalized,x_m,y_m,speed_m_s,yaw_rad,yawrate_rad_s,ax_m_s2,ay_m_s2,front_slip_rad,rear_slip_rad,front_usage,rear_usage,mu"));
}
void FDrivingExperiment::Sample(const BPPhysics::Input& I,const BPPhysics::State& Prev,
    const BPPhysics::State& S,const BPPhysics::Telemetry& T,double StepDuration)
{
    if(!bRecording) return;
    Inputs.Add(I); Elapsed+=StepDuration;
    Distance+=std::hypot(S.X-Prev.X,S.Y-Prev.Y);
    Rows.Add(FString::Printf(TEXT("%d,%.9f,%.6f,%.6f,%.6f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.6f"),
        Inputs.Num(),Elapsed,I.Throttle,I.Brake,I.Steer,S.X,S.Y,T.Speed,S.Yaw,S.R,T.Ax,T.Ay,
        T.Front.SlipAngle,T.Rear.SlipAngle,T.Front.Utilization,T.Rear.Utilization,Friction));
}
FString FDrivingExperiment::Finish()
{
    if(!bRecording) return LastFile;
    bRecording=false;
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Experiments");
    IFileManager::Get().MakeDirectory(*Folder,true);
    LastFile=Folder/FString::Printf(TEXT("drive_%s_%s.csv"),
        *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")),*FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
    if(!FFileHelper::SaveStringArrayToFile(Rows,*LastFile,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        LastFile=TEXT("CSV SAVE FAILED");
    UE_LOG(LogTemp,Display,TEXT("BP_EXPERIMENT mu=%.3f distance=%.6fm elapsed=%.6fs ticks=%d file=%s"),
        Friction,Distance,Elapsed,Inputs.Num(),*LastFile);
    return LastFile;
}
