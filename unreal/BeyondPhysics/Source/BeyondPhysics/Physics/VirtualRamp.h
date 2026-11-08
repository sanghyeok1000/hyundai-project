#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace BPPhysics::Ramp {
// Fictional training alignment informed by Korean road geometry rules.
// NOT a surveyed road or an engineering-approved highway design.
constexpr double Pi=3.14159265358979323846;
constexpr double Radius=150, Transition=60, Approach=160, Exit=180;
constexpr double CurveLength=Radius*Pi/2+Transition;
constexpr double CurveEnd=Approach+CurveLength, Length=CurveEnd+Exit;
constexpr double HalfWidth=6.75, Elevation=20;
struct Node { double S,X,Y,Z,Heading,Grade,Bank; };
inline double Heading(double s) {
    double u=std::clamp(s-Approach,0.,CurveLength);
    if(u<Transition) return -u*u/(2*Radius*Transition);
    if(u<CurveLength-Transition) return -(u-Transition/2)/Radius;
    double tail=CurveLength-u;
    return -Pi/2+tail*tail/(2*Radius*Transition);
}
inline double Grade(double s) {
    if(s<100) return 0;
    if(s<160) return -.03*(s-100)/60;
    if(s<CurveEnd) return -.03;
    if(s<CurveEnd+60) return -.03*(1-(s-CurveEnd)/60);
    return 0;
}
inline double Height(double s) {
    if(s<100) return Elevation;
    if(s<160) return Elevation-.03*(s-100)*(s-100)/120;
    double z=Elevation-.9-.03*(std::min(s,CurveEnd)-160);
    if(s>CurveEnd) { double t=std::min(s-CurveEnd,60.); z-=.03*(t-t*t/120); }
    return z;
}
inline double Bank(double s) {
    double u=s-Approach;
    return .04*std::clamp(std::min(u,CurveLength-u)/Transition,0.,1.);
}
inline const std::vector<Node>& Nodes() {
    static const auto points=[] {
        std::vector<Node> p; double x=0,y=0,previous=0;
        for(int i=0;;++i) {
            double s=std::min(double(i),Length),ds=s-previous;
            // Composite Simpson integration; subinterval <=0.25m.
            for(int j=0;j<4;++j) {
                double a=previous+ds*j/4,b=a+ds/4,m=(a+b)/2;
                x+=(b-a)/6*(std::cos(Heading(a))+4*std::cos(Heading(m))+std::cos(Heading(b)));
                y+=(b-a)/6*(std::sin(Heading(a))+4*std::sin(Heading(m))+std::sin(Heading(b)));
            }
            p.push_back({s,x,y,Height(s),Heading(s),Grade(s),Bank(s)});
            if(s>=Length) break;
            previous=s;
        }
        return p;
    }();
    return points;
}
struct Surface { double S=0,Lateral=0,Z=0,Hx=0,Hy=0,Heading=0; };
inline Surface At(double x,double y) {
    const auto& nodes=Nodes(); double best=std::numeric_limits<double>::max(),s=0;
    double cx=0,cy=0;
    for(size_t i=1;i<nodes.size();++i) {
        const auto&a=nodes[i-1]; const auto&b=nodes[i];
        double dx=b.X-a.X,dy=b.Y-a.Y;
        double t=std::clamp(((x-a.X)*dx+(y-a.Y)*dy)/(dx*dx+dy*dy),0.,1.);
        double qx=a.X+t*dx,qy=a.Y+t*dy,d=(x-qx)*(x-qx)+(y-qy)*(y-qy);
        if(d<best) { best=d; s=a.S+t*(b.S-a.S);cx=qx;cy=qy; }
    }
    double h=Heading(s),c=std::cos(h),n=std::sin(h);
    // Allow end stations to extend, so end boundaries are unambiguous.
    if(s<1e-6 || s>Length-1e-6) s+=(x-cx)*c+(y-cy)*n;
    double lateral=-(x-cx)*n+(y-cy)*c;
    double u=s-Approach,k=0,bprime=0;
    if(u>0 && u<CurveLength) {
        k=-std::min({u/Transition,1.,(CurveLength-u)/Transition})/Radius;
        if(u<Transition) bprime=.04/Transition;
        else if(u>CurveLength-Transition) bprime=-.04/Transition;
    }
    double bank=Bank(s),along=(Grade(s)+bprime*lateral)/(1-k*lateral);
    return {s,lateral,Height(s)+bank*lateral,along*c-bank*n,along*n+bank*c,h};
}
inline bool Touches(double x,double y,double yaw,double halfLength=2.48,double halfWidth=1.22) {
    double c=std::cos(yaw),sn=std::sin(yaw);
    // Include edge midspans: on the inside of a bend the corners alone can
    // clear the wall while the middle of a side has already crossed it.
    // 2mm conservative pad exceeds sagitta for <=1.24m sample spacing at R>=143m.
    for(double a : {-halfLength,-halfLength/2,0.,halfLength/2,halfLength})
        for(double b : {-halfWidth,0.,halfWidth})
            if(std::abs(At(x+c*a-sn*b,y+sn*a+c*b).Lateral)>=HalfWidth-.002) return true;
    return false;
}
}
