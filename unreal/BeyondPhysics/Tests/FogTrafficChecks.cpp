#include "../Source/BeyondPhysics/Experiment/FogTrial.h"
#include <iostream>
#include <cmath>
using namespace BPPhysics;
int main(){int n=0;auto check=[&](bool p,const char* label){std::cout<<(p?"PASS ":"FAIL ")<<label<<'\n';n+=!p;};
 check(Fog::StartSpeed*3.6==80 && Fog::StartX>-500,"80km/h start and short approach");
 check(Fog::Vehicles.size()==3,"three permanent vehicles only");
 for(int i=0;i<3;++i){auto v=Fog::Vehicles[i];State contact{v.X-4.95,v.Y,0,0,0,0};check(Fog::CollisionIndex(contact)==i,"each lane obstacle has collision geometry");}
 check(Fog::ContactFraction(State{-6,0,0,0,0,0},State{100,0,0,0,0,0})<.02,"long sweep finds earliest car even with clear endpoint");
 const double ref=Fog::ReferenceGap;check(ref<=35,"short declared visibility reference");
 FogTrial t;State a{-ref-6,0,0,20,0,0},b{-ref-4,0,0,20,0,0};t.Reset(a);t.Observe(a,b,20,20,0,.1);
 check(t.ReferenceVehicle==0 && t.ReferenceTime>=0,"first observed vehicle is recorded");
 State later{6,3.5,0,20,0,0};t.Observe(b,later,20,20,0,.5);check(t.ReferenceVehicle==0 && t.IsRunning(),"target is stable and passing first car does not finish all traffic");
 FogTrial hit;auto v=Fog::Vehicles[2];State near{v.X-5.1,v.Y,0,2,0,0};hit.Reset(near);hit.Observe(near,State{v.X-4.95,v.Y,0,2,0,0},2,2,0,.1);check(hit.HitVehicle==2 && hit.Outcome==FogOutcome::Collision,"third car contact records actual hit id");
 FogTrial switched;State sa{-36,0,0,20,0,0},sb{-34,0,0,20,0,0};switched.Reset(sa);switched.Observe(sa,sb,20,20,0,.1);switched.Observe(sb,State{40.04,3.5,0,20,0,0},20,20,0,1);check(switched.ReferenceVehicle==0 && switched.HitVehicle==1 && switched.FinalGap<-40,"later collision retains first-reference gap consistently with trace");
 FogTrial early;State e{-120,0,0,1,0,0};early.Reset(e);early.Observe(e,State{-118,0,0,0,0,0},1,0,1,1);check(early.Outcome==FogOutcome::Stopped && early.ReferenceTime<0,"early precautionary stop has a valid outcome without fake reference time");
 std::cout<<"RESULT failures="<<n<<'\n';return n?1:0;}


