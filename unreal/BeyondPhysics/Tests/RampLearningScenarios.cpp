// Test-only steering driver, identical for both attempts. No path/yaw overrides.
#include "../Source/BeyondPhysics/Driving/RampAutoDrive.h"
#include "../Source/BeyondPhysics/Experiment/RampLesson.h"
#include "../Source/BeyondPhysics/Physics/VehicleModel.h"
#include "../Source/BeyondPhysics/Physics/RoadBoundary.h"
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace BPPhysics;
struct Result {bool Complete; double Entry,MaxOffset,Station,Time;};
Result Run(bool brakeBeforeBend,const char* path){
    VehicleModel model;model.bRamp=true;RampAutoDrive policy;RampLesson lesson;
    State s{5,0,0,0,0,0};lesson.Begin(s); bool complete=false;int tick=0;
    std::ofstream csv(path); csv<<"time,s,x,y,speed,yaw,steer,brake,throttle\n"<<std::setprecision(12);
    for(;tick<60000;++tick){
        const auto road=Ramp::At(s.X,s.Y);const double speed=model.Evaluate(s,{}).Speed;
        const double k=(Ramp::Heading(road.S+.1)-Ramp::Heading(road.S-.1))/.2;
        const double err=std::remainder(road.Heading-s.Yaw,2*Ramp::Pi);
        const double delta=2.7*k+err-.1*road.Lateral-.15*(s.R-speed*k);
        const double brake=brakeBeforeBend && road.S>70 && road.S<160 && speed>6?1.:0.;
        const auto input=policy.Update(brake,std::clamp(delta/.5,-1.,1.),speed);
        const State prev=s;if(!model.Step(s,input))throw std::runtime_error("invalid model");
        const double nextSpeed=model.Evaluate(s,input).Speed;lesson.Observe(prev,s,speed,nextSpeed);
        if(tick%24==0)csv<<tick*FixedStep<<','<<road.S<<','<<s.X<<','<<s.Y<<','<<nextSpeed<<','<<s.Yaw<<','<<input.Steer<<','<<input.Brake<<','<<input.Throttle<<'\n';
        if(TouchesRoadWall(s,true)){lesson.Finish(RampOutcome::Wall);break;}
        if(ReachedRoadEnd(s,true)){complete=true;lesson.Finish(RampOutcome::Complete);break;}
    }
    return {complete,lesson.Current.EntrySpeed,lesson.Current.MaxOffset,Ramp::At(s.X,s.Y).S,tick*FixedStep};
}
int main(){
    const auto fast=Run(false,"../../../docs/research/ramp-learning-fast.csv");
    const auto slow=Run(true,"../../../docs/research/ramp-learning-slow.csv");
    for(const auto& result:{fast,slow})std::cout<<"METRIC complete="<<result.Complete<<" entry_kmh="<<result.Entry*3.6<<" max_offset_m="<<result.MaxOffset<<" station="<<result.Station<<" time="<<result.Time<<'\n';
    const bool ok=slow.Complete && slow.Entry<fast.Entry && !fast.Complete;
    std::cout<<(ok?"PASS":"FAIL")<<" same tyre-force driver: braking before bend completes; faster no-brake attempt hits boundary\n";
    return ok?0:1;
}
