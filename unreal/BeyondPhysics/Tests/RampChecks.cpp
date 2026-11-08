#include "../Source/BeyondPhysics/Physics/VehicleModel.h"
#include "../Source/BeyondPhysics/Physics/RoadBoundary.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace BPPhysics;
void Check(bool ok,const char* name) { if(!ok) throw std::runtime_error(name); std::cout<<"PASS "<<name<<'\n'; }
int main(int argc,char**argv) {
    try {
        const auto& p=Ramp::Nodes();
        Check(std::abs(p.back().Heading+Ramp::Pi/2)<1e-10,"exit tangent is 90 degrees right");
        for(const auto&n:p) {
            auto s=Ramp::At(n.X,n.Y);
            if(!std::isfinite(s.Z)) throw std::runtime_error("nonfinite road surface");
            if(std::abs(s.S-n.S)>.002 || std::abs(s.Lateral)>.001 || std::abs(s.Z-n.Z)>.001)
                throw std::runtime_error("road projection mismatch");
            if(TouchesRoadWall(State{n.X,n.Y,n.Heading,0,0,0},true)) throw std::runtime_error("centerline collides");
        }
        auto n=p[300]; double nx=-std::sin(n.Heading),ny=std::cos(n.Heading);
        Check(TouchesRoadWall(State{n.X+nx*6,n.Y+ny*6,n.Heading,0,0,0},true),"curved outside wall contact");
        Check(TouchesRoadWall(State{n.X-nx*5.54,n.Y-ny*5.54,n.Heading,0,0,0},true),"inside side-midspan contact is detected");
        Check(ReachedRoadEnd(State{-1,0,0,0,0,0},true),"backward departure at ramp start is blocked");
        VehicleModel flat,ramp; ramp.bRamp=true;
        auto f=flat.Evaluate({},{}),g=ramp.Evaluate(State{n.X,n.Y,n.Heading,0,0,0},{});
        Check(f.Ax==0 && f.Ay==0,"original flat gravity unchanged");
        Check(g.Ax>.28 && g.Ax<.31 && g.Ay<-.38 && g.Ay>-.41,"downhill and bank gravity directions");
        auto hold=ramp.Evaluate(State{n.X,n.Y,n.Heading,8,0,0},{0,1,.1});
        Check(std::hypot(hold.Front.Fx,hold.Front.Fy)<=.12*hold.Front.Fz+1e-8,"ramp combined friction limit");
        Check(!ReachedRoadEnd(State{p[450].X,p[450].Y,0,0,0,0},true) &&
              ReachedRoadEnd(State{p.back().X,p.back().Y,0,0,0,0},true),"curved endpoint follows station");
        State a{5,0,0,0,0,0},b=a;
        for(int i=0;i<5000;++i) { if(!ramp.Step(a,{1,0,0})) throw std::runtime_error("nonfinite ramp integration"); ramp.Step(b,{1,0,0}); }
        Check(a.X==b.X && a.Y==b.Y && a.U==b.U,"ramp repeated run deterministic");
        // A test-only virtual driver checks that the full bend is traversable.
        // No heading/position overrides: the same tyre forces integrate every step.
        State driver{5,0,0,0,0,0}; double maxOffset=0; int ticks=0;
        for(;ticks<30000 && !ReachedRoadEnd(driver,true);++ticks) {
            auto road=Ramp::At(driver.X,driver.Y);
            double speed=ramp.Evaluate(driver,{}).Speed;
            double curvature=(Ramp::Heading(road.S+.1)-Ramp::Heading(road.S-.1))/.2;
            double error=std::remainder(road.Heading-driver.Yaw,2*Ramp::Pi);
            double delta=2.7*curvature+error-.1*road.Lateral-.15*(driver.R-speed*curvature);
            Input control{std::clamp((8-speed)*.12,0.,.2),std::clamp((speed-8)*.02,0.,.15),std::clamp(delta/.5,-1.,1.)};
            if(control.Brake>0) control.Throttle=0;
            if(!ramp.Step(driver,control) || TouchesRoadWall(driver,true)) throw std::runtime_error("full ramp driving failed");
            maxOffset=std::max(maxOffset,std::abs(road.Lateral));
        }
        Check(ticks<30000,"test driver completes full curved ramp through tyre forces");
        std::cout<<"METRIC ramp_drive_station="<<Ramp::At(driver.X,driver.Y).S<<" max_lateral_m="<<maxOffset<<" time_s="<<ticks*FixedStep<<'\n';
        if(argc>1) {
            std::ofstream file(argv[1]); file<<"s,x,y,z,heading,grade,bank\n"<<std::setprecision(15);
            for(auto&q:p) file<<q.S<<','<<q.X<<','<<q.Y<<','<<q.Z<<','<<q.Heading<<','<<q.Grade<<','<<q.Bank<<'\n';
        }
        std::cout<<"RESULT failures=0 length_m="<<Ramp::Length<<" nodes="<<p.size()<<'\n';
    } catch(const std::exception&e) { std::cerr<<"FAIL "<<e.what()<<'\n'; return 1; }
}
