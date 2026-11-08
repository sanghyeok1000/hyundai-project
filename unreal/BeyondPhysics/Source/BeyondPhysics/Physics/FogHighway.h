#pragma once
#include "RoadBoundary.h"
#include <array>
namespace BPPhysics::Fog {
struct Obstacle {double X,Y;};
inline constexpr std::array<Obstacle,3> Vehicles{{{0,0},{45,3.5},{85,-3.5}}};
constexpr double HalfLength=2000,StartX=-260,LeadX=0,StartSpeed=80/3.6;
constexpr double Friction=.8,TargetSpeed=100/3.6,ReferenceGap=30;
constexpr double RackRatio=18,HandwheelMaxDegrees=540,RoadWheelMax=Ramp::Pi/6;
inline int NearestIndex(const State& s){for(int i=0;i<3;++i)if(Vehicles[i].X+2*CarHalfLength>=s.X)return i;return 2;}
inline double Gap(const State& s,int index=-1){return Vehicles[index<0?NearestIndex(s):index].X-CarHalfLength-s.X-(std::abs(std::cos(s.Yaw))*CarHalfLength+std::abs(std::sin(s.Yaw))*CarHalfWidth);}
inline bool TouchesVehicle(const State& state,const Obstacle& vehicle){
    const double c=std::cos(state.Yaw),s=std::sin(state.Yaw);
    const double axes[4][2]={{1,0},{0,1},{c,s},{-s,c}};
    for(const auto& axis:axes){
        const double ego=CarHalfLength*std::abs(axis[0]*c+axis[1]*s)+CarHalfWidth*std::abs(-axis[0]*s+axis[1]*c);
        const double lead=CarHalfLength*std::abs(axis[0])+CarHalfWidth*std::abs(axis[1]);
        if(std::abs((state.X-vehicle.X)*axis[0]+(state.Y-vehicle.Y)*axis[1])>ego+lead+1e-8)return false;
    }
    return true;
}
inline int CollisionIndex(const State& state){for(int i=0;i<3;++i)if(TouchesVehicle(state,Vehicles[i]))return i;return -1;}
inline bool TouchesLead(const State& state){return CollisionIndex(state)>=0;}
inline State Interpolate(const State& a,const State& b,double f){
    return {a.X+(b.X-a.X)*f,a.Y+(b.Y-a.Y)*f,a.Yaw+(b.Yaw-a.Yaw)*f,
        a.U+(b.U-a.U)*f,a.V+(b.V-a.V)*f,a.R+(b.R-a.R)*f};
}
inline bool Contact(const State& s){return TouchesLead(s)||TouchesRoadWall(s);}
// Maximum separating-axis gap is a lower bound on rectangle distance.
// Conservative advancement therefore cannot skip a brief corner intersection.
inline double Clearance(const State& state){
    const double c=std::cos(state.Yaw),s=std::sin(state.Yaw);
    const double axes[4][2]={{1,0},{0,1},{c,s},{-s,c}};double nearest=1e100;
    for(const auto& vehicle:Vehicles){double leadGap=0;
    for(const auto& axis:axes){
        const double extent=CarHalfLength*(std::abs(axis[0]*c+axis[1]*s)+std::abs(axis[0]))+
            CarHalfWidth*(std::abs(-axis[0]*s+axis[1]*c)+std::abs(axis[1]));
        leadGap=std::max(leadGap,std::abs((state.X-vehicle.X)*axis[0]+(state.Y-vehicle.Y)*axis[1])-extent);
    }
    nearest=std::min(nearest,leadGap);}
    const double wallGap=RoadHalfWidth-std::abs(state.Y)-std::abs(s)*CarHalfLength-std::abs(c)*CarHalfWidth;
    return std::min(nearest,wallGap);
}
inline double ContactFraction(const State& a,const State& b){
    if(Contact(a))return 0;
    if(std::abs(b.Yaw-a.Yaw)<1e-12){
        const double c=std::cos(a.Yaw),s=std::sin(a.Yaw),axes[4][2]={{1,0},{0,1},{c,s},{-s,c}};
        double first=1;
        for(const auto& vehicle:Vehicles){double enter=0,leave=1;bool possible=true;
        for(const auto& axis:axes){
            const double extent=CarHalfLength*(std::abs(axis[0]*c+axis[1]*s)+std::abs(axis[0]))+
                CarHalfWidth*(std::abs(-axis[0]*s+axis[1]*c)+std::abs(axis[1]));
            const double p=(a.X-vehicle.X)*axis[0]+(a.Y-vehicle.Y)*axis[1],v=(b.X-a.X)*axis[0]+(b.Y-a.Y)*axis[1];
            if(std::abs(v)<1e-14){if(std::abs(p)>extent)possible=false;continue;}
            const double f0=(-extent-p)/v,f1=(extent-p)/v;
            enter=std::max(enter,std::min(f0,f1));leave=std::min(leave,std::max(f0,f1));
        }
        if(possible && enter<=leave)first=std::min(first,enter);}
        const double limit=RoadHalfWidth-std::abs(s)*CarHalfLength-std::abs(c)*CarHalfWidth;
        const double dy=b.Y-a.Y;
        if(dy>0 && b.Y>=limit)first=std::min(first,(limit-a.Y)/dy);
        if(dy<0 && b.Y<=-limit)first=std::min(first,(-limit-a.Y)/dy);
        return first;
    }
    const double motion=std::hypot(b.X-a.X,b.Y-a.Y)+std::hypot(CarHalfLength,CarHalfWidth)*std::abs(b.Yaw-a.Yaw);
    if(motion<1e-12)return 1;
    double f=0;
    for(int i=0;i<256;++i){
        const double gap=Clearance(Interpolate(a,b,f));
        if(gap<=1e-8)return f;
        const double next=f+gap/motion;
        if(next>1)return 1;
        f=next;
    }
    return -1; // unresolved tangency: caller aborts the trial, never invents contact.
}
inline bool InReferenceView(const State& s,int index=-1){
    if(index<0)index=NearestIndex(s);
    const auto& vehicle=Vehicles[index];
    const double angle=std::remainder(std::atan2(vehicle.Y-s.Y,vehicle.X-s.X)-s.Yaw,2*Ramp::Pi);
    return Gap(s,index)<=ReferenceGap && Gap(s,index)>0 && std::abs(angle)<40*Ramp::Pi/180.;
}
struct AutoDrive {
    bool Cancelled=false;
    void Reset(){Cancelled=false;}
    Input Update(double brake,double steer,double speed){
        const double b=std::clamp(brake,0.,1.);if(b>0 && speed>.1)Cancelled=true;
        return {(Cancelled || b>0)?0.:std::clamp((TargetSpeed-speed)*.5,0.,.5),b,std::clamp(steer,-1.,1.)};
    }
};
}
