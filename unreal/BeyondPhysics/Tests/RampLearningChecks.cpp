#include "../Source/BeyondPhysics/Driving/RampAutoDrive.h"
#include "../Source/BeyondPhysics/Experiment/RampLesson.h"
#include "../Source/BeyondPhysics/Physics/VehicleModel.h"
#include "../Source/BeyondPhysics/Physics/RoadBoundary.h"
#include <iostream>
#include <cmath>
using namespace BPPhysics;
int failures=0;
void Check(bool ok,const char* name){std::cout<<(ok?"PASS ":"FAIL ")<<name<<'\n';failures+=!ok;}
int main(){
    RampAutoDrive drive;
    auto launch=drive.Update(0,.25,0);
    Check(launch.Throttle>0 && launch.Throttle<.2 && launch.Brake==0 && launch.Steer==.25,"ramp launches gently without altering steering");
    auto brake=drive.Update(1,.25,8);
    Check(brake.Throttle==0 && brake.Brake==1,"ramp brake always cuts drive");
    drive.Update(1,0,6);
    Check(std::abs(drive.TargetSpeed-6)<1e-10,"braking retains the reduced target speed");
    drive.Update(1,0,7);
    Check(std::abs(drive.TargetSpeed-6)<1e-10,"braking downhill cannot raise the retained target");
    auto downhill=drive.Update(0,0,8);
    Check(downhill.Throttle==0 && downhill.Brake==0,"above target coasts without automatic braking or forced speed");
    auto resume=drive.Update(0,0,5);
    Check(resume.Throttle>0 && resume.Throttle<.2 && std::abs(drive.TargetSpeed-6)<1e-10,"release resumes gentle drive towards the reduced target");
    drive.Update(1,0,0);
    Check(drive.Update(0,0,0).Throttle>0,"release after standstill can launch again");
    drive.Reset(); Check(std::abs(drive.TargetSpeed-50/3.6)<1e-10,"restart restores the initial target");
    Check(AutoDriveInput(0,.3).Throttle==1 && AutoDriveInput(1,.3).Throttle==0,"first-map legacy input stays unchanged");
    RampLesson lesson; State a{159,0,0,10,0,0},b{161,0,0,12,0,0};
    lesson.Begin(a,.85); lesson.Observe(a,b,10,12);
    Check(std::abs(lesson.Current.EntrySpeed-11)<.001,"entry speed is interpolated at the bend crossing");
    lesson.Finish(RampOutcome::Wall);
    lesson.Begin(State{5,0,0,0,0,0});
    Check(lesson.Previous.Outcome==RampOutcome::Wall && std::abs(lesson.Previous.EntrySpeed-11)<.001 && lesson.Current.EntrySpeed<0,"restart preserves the previous result and clears current entry");
    Check(lesson.Previous.Friction==.85 && lesson.Current.Friction==.12,"comparison preserves each attempt's own friction profile");
    auto entry=lesson.Previous.EntrySpeed;
    lesson.Observe(State{5,0,0,0,0,0},State{10,0,0,3,0,0},0,3);
    Check(lesson.Current.Trace.size()>1 && lesson.Previous.EntrySpeed==entry,"current trajectory does not overwrite the previous attempt");
    lesson.Exclude();
    const auto points=lesson.Current.Trace.size();
    lesson.Observe(State{10,0,0,3,0,0},State{300,0,0,20,0,0},3,20);
    lesson.Finish(RampOutcome::Complete);
    Check(lesson.Current.Outcome!=RampOutcome::Complete && lesson.Current.Trace.size()==points,"synthetic input cannot become a completed ordinary attempt");
    lesson.Begin(State{5,0,0,0,0,0});
    lesson.Observe(State{5,0,0,0,0,0},State{10,0,0,3,0,0},0,3);
    lesson.Finish(RampOutcome::Complete);lesson.Finish(RampOutcome::Restarted);
    Check(lesson.Current.Outcome==RampOutcome::Complete,"terminal result remains immutable until a new attempt begins");
    std::cout<<"RESULT failures="<<failures<<'\n';return failures?1:0;
}
