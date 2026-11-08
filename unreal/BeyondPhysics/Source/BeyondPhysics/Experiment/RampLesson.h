#pragma once
#include "../Physics/VirtualRamp.h"
#include "../Physics/VehicleTypes.h"
#include <vector>
#include <utility>
namespace BPPhysics {
enum class RampOutcome { Running,Wall,Complete,Restarted };
struct RampTracePoint {double X=0,Y=0,Speed=0;};
struct RampAttempt {
    std::vector<RampTracePoint> Trace;
    double EntrySpeed=-1,MaxOffset=0,Distance=0,Friction=LowGripFriction;
    RampOutcome Outcome=RampOutcome::Running;
};
struct RampLesson {
    RampAttempt Current,Previous;
    bool IsRunning() const {return Eligible && Current.Outcome==RampOutcome::Running;}
    void Exclude(){Finish(RampOutcome::Restarted);Eligible=false;}
    void Begin(const State& state,double mu=LowGripFriction){
        if(Current.Distance>=1){
            if(Current.Outcome==RampOutcome::Running) Finish(RampOutcome::Restarted);
            Previous=std::move(Current);
        }
        Eligible=true;Current={};
        Current.Friction=mu;
        Last={state.X,state.Y,std::hypot(state.U,state.V)};
        Current.Trace.push_back(Last);
    }
    void Observe(const State& before,const State& after,double beforeSpeed,double speed){
        if(!IsRunning()) return;
        const auto road=Ramp::At(after.X,after.Y);
        Current.MaxOffset=std::max(Current.MaxOffset,std::abs(road.Lateral));
        Current.Distance+=std::hypot(after.X-before.X,after.Y-before.Y);
        if(Current.EntrySpeed<0 && road.S>=Ramp::Approach){
            const auto prev=Ramp::At(before.X,before.Y);
            if(prev.S<Ramp::Approach){
                const double fraction=std::clamp((Ramp::Approach-prev.S)/(road.S-prev.S),0.,1.);
                Current.EntrySpeed=beforeSpeed+fraction*(speed-beforeSpeed);
            }
        }
        Last={after.X,after.Y,speed};
        if(Current.Trace.empty() || std::hypot(Last.X-Current.Trace.back().X,Last.Y-Current.Trace.back().Y)>=1.) Append();
    }
    void Finish(RampOutcome outcome){
        if(!IsRunning()) return;
        Current.Outcome=outcome;
        if(!Current.Trace.empty() && std::hypot(Last.X-Current.Trace.back().X,Last.Y-Current.Trace.back().Y)>1e-8) Append();
    }
private:
    bool Eligible=true;
    RampTracePoint Last;
    void Append(){
        // Bounded even if a player circles in place for a long time.
        if(Current.Trace.size()<4096) Current.Trace.push_back(Last);
        else Current.Trace.back()=Last;
    }
};
}
