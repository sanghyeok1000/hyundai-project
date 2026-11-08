#include "../Source/BeyondPhysics/Physics/VehicleModel.h"
#include "../Source/BeyondPhysics/Physics/SteeringGeometry.h"
#include <iostream>
#include <cmath>
using namespace BPPhysics;
int failures=0;void Check(bool v,const char* n){std::cout<<(v?"PASS ":"FAIL ")<<n<<'\n';failures+=!v;}
int main(){
 const double pi=std::acos(-1.),L=2.7,t=1.88,d=.15;
 auto angles=AckermannAngles(d,L,t);
 Check(angles[0]>d && angles[1]<d,"inner wheel steers further than outer wheel");
 Check(std::abs(1/std::tan(angles[1])-1/std::tan(angles[0])-t/L)<1e-12,"independent Ackermann cotangent relation");
 auto right=AckermannAngles(-d,L,t);Check(std::abs(right[0]+angles[1])+std::abs(right[1]+angles[0])<1e-12,"left-right rack symmetry");
 Check(std::abs(HandwheelToRoadRadians(90,18,pi/6)-5*pi/180)<1e-12,"90deg handwheel maps to5deg road wheel with18to1 rack");
 VehicleModel m;m.bFourContact=true;m.Params.Friction=.8;m.Params.TrackWidth=t;
 State s{0,0,0,5,0,0};auto force=m.Evaluate(s,{0,0,d/m.Params.MaxSteer});
 double fx=0,fy=0,mz=0;
 for(int i=0;i<4;++i){double x=i<2?1.2:-1.5,y=i%2?-t/2:t/2,c=std::cos(force.WheelSteer[i]),sn=std::sin(force.WheelSteer[i]);auto w=force.Wheels[i];double bx=c*w.Fx-sn*w.Fy,by=sn*w.Fx+c*w.Fy;fx+=bx;fy+=by;mz+=x*by-y*bx;Check(std::hypot(w.Fx,w.Fy)<=.8*w.Fz+1e-8,"each contact respects its own friction circle");}
 Check(std::abs(fx-force.Fx)+std::abs(fy-force.Fy)+std::abs(mz-force.Moment)<1e-8,"four forces and lateral moment arms sum to rigid-body result");
 // Pure rolling circle about the rear axle: independent geometry yields zero slip.
 const double u=5,r=u*std::tan(d)/L;State circle{0,0,0,u,1.5*r,r};auto pure=m.Evaluate(circle,{0,0,d/m.Params.MaxSteer});
 for(auto w:pure.Wheels)Check(std::abs(w.SlipAngle)<1e-12,"all wheel axes match their actual rigid-body contact velocities");
 VehicleModel bicycle=m;bicycle.bFourContact=false;m.Params.TrackWidth=0;State q{0,0,0,17,.7,.18};Input in{.2,.4,.12};auto a=m.Evaluate(q,in),b=bicycle.Evaluate(q,in);
 Check(std::abs(a.Fx-b.Fx)+std::abs(a.Fy-b.Fy)+std::abs(a.Moment-b.Moment)<1e-7,"zero-track four contacts reduce to the reference bicycle model");m.Params.TrackWidth=t;
 auto run=[&](State state,Input input){for(int k=0;k<480;++k)Check(m.Step(state,input),"finite fixed step");return state;};
 // Avoid per-step output for independent low/high-speed and symmetry comparisons.
 State left{0,0,0,20,0,0},rightState=left,rot=left;rot.Yaw=1.1;
 for(int k=0;k<480;++k){m.Step(left,{0,0,.08});m.Step(rightState,{0,0,-.08});m.Step(rot,{0,0,.08});}
 Check(std::abs(left.Y+rightState.Y)<1e-8 && std::abs(left.Yaw+rightState.Yaw)<1e-8,"four-contact trajectories mirror under steering reversal");
 Check(std::abs(rot.X-std::cos(1.1)*left.X+std::sin(1.1)*left.Y)<1e-7 && std::abs(rot.Y-std::sin(1.1)*left.X-std::cos(1.1)*left.Y)<1e-7,"world heading does not change contact dynamics");
 State brake{0,0,0,100/3.6,0,0};for(int k=0;k<4000 && std::hypot(brake.U,brake.V)>.05;++k)m.Step(brake,{0,1,0});
 Check(std::abs(brake.X-(100/3.6)*(100/3.6)/(2*.8*9.81))<.03,"four-contact100km/h straight braking preserves analytical force limit");
 for(double speed:{5.,25.}){
  State steady{0,0,0,speed,0,0};const double small=.005;
  for(int k=0;k<12000;++k){steady.U=speed;m.Step(steady,{0,0,small/m.Params.MaxSteer});}
  const double reference=speed*small/(L+m.Params.Mass*speed*speed/L*(1.5/80000.-1.2/90000.));
  Check(std::abs(steady.R-reference)/reference<.02,"four-contact steady yaw matches independent linear dynamics at low and highway speed");
  std::cout<<"METRIC speed_m_s="<<speed<<" yaw_rate="<<steady.R<<" linear_reference="<<reference<<'\n';
 }
 for(double direction:{-1.,1.}){State moving{0,0,.5,direction*20,2,.3};auto dissipative=m.Evaluate(moving,{0,1,.3});
  Check(dissipative.Fx*moving.U+dissipative.Fy*moving.V+dissipative.Moment*moving.R<=0,"four-contact braking and slip dissipate energy in both directions");}
 std::cout<<"METRIC brake_m="<<brake.X<<" left_y_m="<<left.Y<<" yaw_rad="<<left.Yaw<<'\n';
 std::cout<<"RESULT failures="<<failures<<'\n';return failures?1:0;
}

