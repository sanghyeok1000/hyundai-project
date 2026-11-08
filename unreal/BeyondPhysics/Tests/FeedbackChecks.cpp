#include "../Source/BeyondPhysics/Feedback/FeedbackPolicy.h"
#include <iostream>
#include <limits>
int main(){int errors=0;auto check=[&](bool v,const char* n){std::cout<<(v?"PASS ":"FAIL ")<<n<<'\n';errors+=!v;};
 BPFeedback::Frame f;f.Active=true;f.SpeedMps=25;auto ice=BPFeedback::Evaluate(f);
 f.Map=2;auto fog=BPFeedback::Evaluate(f);
 check(ice.ColorArgb!=fog.ColorArgb && ice.CoolOffset>0 && fog.CoolOffset==0,"map meaning differentiates cold ice from low-visibility fog");
 f.FogVisible=false;f.GapM=10;check(!BPFeedback::Evaluate(f).Warn,"hidden traffic cannot trigger an audible warning");
 f.FogVisible=true;f.GapM=31;check(!BPFeedback::Evaluate(f).Warn,"warning respects declared30m boundary");
 f.GapM=29;check(BPFeedback::Evaluate(f).Warn,"visible close traffic triggers a warning condition");
 f.Active=false;auto off=BPFeedback::Evaluate(f);check(off.Motor==0&&off.Wind==0&&off.Road==0&&off.Slip==0&&!off.Warn,"paused state mutes all continuing channels and warnings");
 f.Crash=true;check(BPFeedback::Evaluate(f).Impact,"crash condition remains available for a single component edge cue");
 f={};f.Active=true;f.SpeedMps=28;f.Map=1;auto grip=BPFeedback::Evaluate(f);f.Usage=1;f.SlipRad=.2;auto slide=BPFeedback::Evaluate(f);
 check(slide.ColorArgb!=grip.ColorArgb&&slide.Slip>grip.Slip,"actual contact usage and slip change light and friction audio");
 f.SpeedMps=0;check(BPFeedback::Evaluate(f).Slip==0,"stationary numerical slip produces no sliding sound");
 f.SpeedMps=1000;f.Usage=1000;f.SlipRad=1000;auto cap=BPFeedback::Evaluate(f);
 check(cap.Fan<=2&&cap.Fan>=1&&cap.CoolOffset<=1&&cap.Brightness<=.4&&cap.Motor<=.4&&cap.Wind<=.4&&cap.Road<=.4&&cap.Slip<=.4,"saturation bounds output and avoids excessive effects");
 f.SpeedMps=std::numeric_limits<double>::quiet_NaN();f.SlipRad=f.Usage=f.SpeedMps;auto bad=BPFeedback::Evaluate(f);
 check(std::isfinite(bad.Motor)&&std::isfinite(bad.Wind)&&std::isfinite(bad.Slip)&&std::isfinite(bad.Brightness),"nonfinite telemetry cannot contaminate device or audio commands");
 std::cout<<"RESULT failures="<<errors<<'\n';return errors?1:0;}
