#include "../Source/BeyondPhysics/Experiment/FogTrial.h"
#include "../Source/BeyondPhysics/Physics/VehicleModel.h"
#include <iostream>
#include <cmath>
using namespace BPPhysics;
int failures=0;
void Check(bool ok,const char* name){std::cout<<(ok?"PASS ":"FAIL ")<<name<<'\n';failures+=!ok;}
int main(){
    Check(Fog::ContactFraction(State{-4.9,2.4401,0,0,0,0},State{-4.78,2.4401,0,0,0,0})==1,"parallel close pass does not invent wall contact");
    Check(Fog::ContactFraction(State{4.93,2.46,0,0,0,0},State{4.99,2.40,0,0,0,0})<.5,"sweep catches corner crossing with clear endpoints");
    FogTrial alongside;State side{-36,3.5,0,10,0,0};alongside.Reset(side);
    State nearSide{-34,3.5,0,9,0,0};alongside.Observe(side,nearSide,10,9,0,.2);
    alongside.Observe(nearSide,State{0,3.5,0,0,0,0},9,0,1,1);
    Check(alongside.Outcome!=FogOutcome::Stopped,"alongside stop is not labelled before vehicle");
    FogTrial prior;State p{-40,0,0,10,0,0},q{-36,0,0,9,0,0},r{-34,0,0,9,0,0};prior.Reset(p);
    prior.Observe(p,q,10,9,1,.4);prior.Observe(q,r,9,9,0,.2);prior.Observe(r,State{-32,0,0,8,0,0},9,8,1,.2);
    Check(prior.PreBraked && prior.InterruptedBraking,"pre-reference brake and release history survives crossing");
    Fog::AutoDrive drive;
    drive.Update(1,0,0);Check(drive.Update(0,0,0).Throttle>0,"initial held brake permits launch after release");
    Check(drive.Update(0,.2,0).Throttle<=.5,"fog launch uses bounded drive demand");
    Check(drive.Update(0,.2,30).Throttle==0,"fog target is drive-only without forced speed or braking");
    drive.Update(1,.2,20);auto release=drive.Update(0,.2,10);
    Check(release.Throttle==0 && release.Brake==0 && release.Steer==.2,"brake cancels cruise until explicit restart");
    drive.Reset();Check(drive.Update(0,0,0).Throttle>0,"restart restores fog auto launch");
    Check(Fog::TouchesLead(State{-4.95,0,0,0,0,0}),"front bumper contacts stationary lead car");
    Check(!Fog::TouchesLead(State{0,3.5,0,0,0,0}),"adjacent lane does not hit lead car");
    Check(Fog::TouchesLead(State{-3.6,3.5,1.5707963267948966,0,0,0}),"rotated footprint uses both vehicle axes");
    Check(std::abs(Fog::ContactFraction(State{-6,0,0,0,0,0},State{-4,0,0,0,0,0})-.52)<1e-6,"substep contact is interpolated before penetration");
    FogTrial trial;State a{-36,0,0,10,0,0},b{-34,0,0,12,0,0};trial.Reset(a);
    trial.Observe(a,b,10,12,0,.2);
    Check(std::abs(trial.ReferenceTime-.104)<1e-6 && std::abs(trial.ReferenceSpeed-11.04)<1e-6,"reference crossing uses bumper gap and fractional time");
    State c{-32,0,0,10,0,0};trial.Observe(b,c,12,10,1,.2);
    Check(std::abs(trial.ReactionTime()-.096)<1e-6 && std::abs(trial.ReactionDistance-.96)<1e-6,"reaction ends at first brake sample start");
    Check(std::abs(trial.BrakingDistance()-2)<1e-8,"distance after brake excludes reaction distance");
    trial.Finish(FogOutcome::Stopped);trial.Finish(FogOutcome::Collision);
    Check(trial.Outcome==FogOutcome::Stopped,"terminal fog result is immutable");
    FogTrial held;held.Reset(a);held.Observe(a,b,10,12,1,.2);
    Check(held.PreBraked && held.ReactionTime()==0,"prior braking is marked rather than negative reaction time");
    FogTrial contact;contact.Reset(State{-5.1,0,0,0,0,0});contact.Observe(State{-5.1,0,0,0,0,0},State{-4.95,0,0,0,0,0},0,0,1,.1);
    Check(contact.Outcome==FogOutcome::Collision,"contact has priority over low-speed stop");
    VehicleModel model;model.Params.Friction=Fog::Friction;State s{0,0,0,Fog::TargetSpeed,0,0};int ticks=0;
    while(model.Evaluate(s,{0,1,0}).Speed>.05 && ticks++<4000)model.Step(s,{0,1,0});
    const double expected=Fog::TargetSpeed*Fog::TargetSpeed/(2*Fog::Friction*model.Params.Gravity);
    Check(std::abs(s.X-expected)<.03,"100km/h dry-asphalt braking matches v squared over 2mu g");
    std::cout<<"METRIC dry_brake_m="<<s.X<<" analytic_m="<<expected<<'\n';
    std::cout<<"RESULT failures="<<failures<<'\n';return failures?1:0;
}

