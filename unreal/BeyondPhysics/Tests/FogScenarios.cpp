#include "../Source/BeyondPhysics/Experiment/FogTrial.h"
#include "../Source/BeyondPhysics/Physics/VehicleModel.h"
#include <iostream>
#include <fstream>
#include <iomanip>
using namespace BPPhysics;
struct Result {FogOutcome Outcome;double Reaction,ReactionM,BrakeM,Gap,Speed,Time;};
Result Run(int mode,const char* path){
 VehicleModel model;model.Params.Friction=Fog::Friction;model.Params.MaxSteer=Fog::RoadWheelMax;model.bFourContact=true;Fog::AutoDrive drive;FogTrial trial;
 State state{Fog::StartX,0,0,Fog::StartSpeed,0,0};trial.Reset(state);
 for(int tick=0;tick<12000 && trial.IsRunning();++tick){
  const State before=state;const double speed=model.Evaluate(before,{}).Speed;
  const bool after=trial.ReferenceTime>=0 && trial.Elapsed>=trial.ReferenceTime+.5;
  const double brake=(mode==0 && Fog::Gap(before)<=100) || (mode==1 && after)?1.:0.;
  const double steer=mode==3 && after?1.:0.; // aggressive instantaneous full rack, no trajectory override
  const auto input=drive.Update(brake,steer,speed);if(!model.Step(state,input)){trial.Finish(FogOutcome::Aborted);break;}
  double fraction=Fog::ContactFraction(before,state);if(fraction<0){trial.Finish(FogOutcome::Aborted);break;}
  state=Fog::Interpolate(before,state,fraction);trial.Observe(before,state,speed,model.Evaluate(state,input).Speed,brake,FixedStep*fraction);
 }
 std::ofstream csv(path);csv<<"time,x,y,speed,brake,gap\n"<<std::setprecision(12);
 for(auto& p:trial.Trace)csv<<p.Time<<','<<p.X<<','<<p.Y<<','<<p.Speed<<','<<p.Brake<<','<<p.Gap<<'\n';
 return {trial.Outcome,trial.ReactionTime(),trial.ReactionDistance,trial.BrakingDistance(),trial.FinalGap,model.Evaluate(state,{}).Speed,trial.Elapsed};
}
int main(){
 auto early=Run(0,"../../../docs/research/fog-feedback-early.csv"),late=Run(1,"../../../docs/research/fog-feedback-late.csv"),missed=Run(2,"../../../docs/research/fog-feedback-missed.csv"),dodge=Run(3,"../../../docs/research/fog-feedback-dodge.csv");
 for(auto& r:{early,late,missed,dodge})std::cout<<"METRIC outcome="<<static_cast<int>(r.Outcome)<<" reaction_s="<<r.Reaction<<" reaction_m="<<r.ReactionM<<" after_brake_m="<<r.BrakeM<<" gap="<<r.Gap<<" speed="<<r.Speed<<" time_s="<<r.Time<<'\n';
 bool ok=early.Outcome==FogOutcome::Stopped && early.Gap>45 && early.Reaction<0 && std::abs(early.BrakeM-49.1593)<.05 && late.Outcome==FogOutcome::Collision && missed.Outcome==FogOutcome::Collision && missed.Time<12 && (dodge.Outcome==FogOutcome::Collision || dodge.Outcome==FogOutcome::Wall);
 std::cout<<(ok?"PASS":"FAIL")<<" short80to100 approach: early precaution stops; late30m braking and full-rack dodge cannot evade collision\n";return ok?0:1;
}
