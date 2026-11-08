#pragma once
#include "../Physics/FogHighway.h"
#include <vector>
namespace BPPhysics {
enum class FogOutcome {Running,Stopped,Collision,Wall,Passed,Aborted,StoppedAlongside};
struct FogTracePoint {double Time,X,Y,Speed,Brake,Gap;};
struct FogTrial {
    FogOutcome Outcome=FogOutcome::Running;
    double Elapsed=0,Travel=0,ReferenceTime=-1,ReferenceSpeed=-1,BrakeTime=-1,BrakeSpeed=-1,ReferenceDistance=0,BrakeStartDistance=0,ReactionDistance=-1,FinalGap=0;
    bool PreBraked=false,InterruptedBraking=false,BrakingAtReference=false;
    int ReferenceVehicle=-1,HitVehicle=-1;
    double FirstBrakeTime=-1,FirstBrakeDistance=0,FirstBrakeSpeed=-1;
    std::vector<FogTracePoint> Trace;
    bool IsRunning()const{return Outcome==FogOutcome::Running;}
    void Reset(const State& state){*this=FogTrial{};Last={0,state.X,state.Y,std::hypot(state.U,state.V),0,Fog::Gap(state)};Trace.push_back(Last);FinalGap=Last.Gap;}
    void Observe(const State& before,const State& after,double beforeSpeed,double speed,double brake,double dt){
        if(!IsRunning())return;
        const double distance=std::hypot(after.X-before.X,after.Y-before.Y);
        bool crossed=false;
        if(brake>0 && FirstBrakeTime<0){FirstBrakeTime=Elapsed;FirstBrakeDistance=Travel;FirstBrakeSpeed=beforeSpeed;}
        if(ReferenceTime<0 && brake>0)PreBraked=true;
        if(PreBraked && brake<=0 && speed>.05)InterruptedBraking=true;
        int candidate=-1;double nearest=1e100;
        if(ReferenceTime<0)for(int i=0;i<3;++i)if(Fog::InReferenceView(after,i) && Fog::Gap(after,i)<nearest){candidate=i;nearest=Fog::Gap(after,i);}
        if(ReferenceTime<0 && candidate>=0){
            ReferenceVehicle=candidate;
            const double oldGap=Fog::Gap(before,candidate),gap=Fog::Gap(after,candidate);
            const double fraction=oldGap>Fog::ReferenceGap && oldGap>gap?std::clamp((oldGap-Fog::ReferenceGap)/(oldGap-gap),0.,1.):0.;
            ReferenceTime=Elapsed+fraction*dt;ReferenceDistance=Travel+fraction*distance;
            ReferenceSpeed=beforeSpeed+fraction*(speed-beforeSpeed);crossed=true;
        }
        if(ReferenceTime>=0 && BrakeTime<0 && brake>0){
            BrakingAtReference=crossed;
            BrakeTime=crossed?ReferenceTime:Elapsed;
            BrakeStartDistance=crossed?ReferenceDistance:Travel;
            BrakeSpeed=crossed?ReferenceSpeed:beforeSpeed;
            ReactionDistance=std::max(0.,BrakeStartDistance-ReferenceDistance);
        }
        if(BrakeTime>=0 && brake<=0 && speed>.05)InterruptedBraking=true;
        Elapsed+=dt;Travel+=distance;FinalGap=Fog::Gap(after,ReferenceVehicle);
        Last={Elapsed,after.X,after.Y,speed,brake,FinalGap};
        if(Trace.empty() || Elapsed-Trace.back().Time>=.1)Append();
        // Contact takes priority over a low-speed stop at the obstacle boundary.
        HitVehicle=Fog::CollisionIndex(after);
        if(HitVehicle>=0)Finish(FogOutcome::Collision);
        else if(TouchesRoadWall(after))Finish(FogOutcome::Wall);
        else if(Travel>.1 && brake>0 && speed<=.05)Finish(FinalGap>0?FogOutcome::Stopped:FogOutcome::StoppedAlongside);
        else if(after.X>Fog::Vehicles.back().X+2*CarHalfLength || std::abs(after.X)>=Fog::HalfLength-CarHalfLength)Finish(FogOutcome::Passed);
    }
    void Finish(FogOutcome value){if(IsRunning()){Outcome=value;if(!Trace.empty() && Last.Time>Trace.back().Time)Append();}}
    double ReactionTime()const{return BrakeTime<0?-1:BrakeTime-ReferenceTime;}
    double BrakingDistance()const{return FirstBrakeTime<0?-1:Travel-FirstBrakeDistance;}
private:
    FogTracePoint Last{};
    void Append(){if(Trace.size()<4096)Trace.push_back(Last);else Trace.back()=Last;}
};
}
