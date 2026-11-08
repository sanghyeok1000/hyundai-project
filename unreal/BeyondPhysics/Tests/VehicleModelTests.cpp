#include "../Source/BeyondPhysics/Physics/VehicleModel.h"
#include "../Source/BeyondPhysics/Physics/RoadBoundary.h"
#include "../Source/BeyondPhysics/Driving/AutoDriveInput.h"
#include "../Source/BeyondPhysics/Driving/VehicleInputGate.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <limits>
using namespace BPPhysics;
int Failures = 0;
void Check(bool Ok, const char* Name)
{
    std::cout << (Ok ? "PASS " : "FAIL ") << Name << '\n';
    Failures += !Ok;
}
void Near(double Actual, double Expected, double Error, const char* Name)
{
    if (std::abs(Actual - Expected) > Error)
        std::cout << "  actual=" << Actual << " expected=" << Expected << '\n';
    Check(std::isfinite(Actual) && std::abs(Actual - Expected) <= Error, Name);
}
State Run(VehicleModel& M, State S, Input I, int Steps, double Dt = FixedStep)
{
    for (int N = 0; N < Steps; ++N) M.Step(S, I, Dt);
    return S;
}
double Stop(VehicleModel& M, double Dt, double& Time)
{
    State S; S.U = 15;
    Time = 0;
    for (int N = 0; N < 200000 && S.U > 0.01; ++N)
    {
        M.Step(S, {0,1,0}, Dt); Time += Dt;
    }
    return S.X;
}
int main()
{
    std::cout << std::setprecision(12);
    VehicleModel M;
    const auto Auto=AutoDriveInput(0,.3), Braking=AutoDriveInput(1,.3);
    Check(Auto.Throttle==1 && Auto.Brake==0 && Auto.Steer==.3,"auto drive requests acceleration without accelerator input");
    Check(Braking.Throttle==0 && Braking.Brake==1,"brake takes priority over automatic drive");
    const auto Partial=AutoDriveInput(.2,0);
    Check(Partial.Throttle==0 && Partial.Brake==.2,"partial brake also cuts automatic drive");
    const auto AutoLaunch=Run(M,{},AutoDriveInput(0,0),2400);
    const auto AutoStop=Run(M,AutoLaunch,AutoDriveInput(1,0),12000);
    Check(AutoLaunch.U>11 && std::hypot(AutoStop.U,AutoStop.V)<.01,"auto launch followed by held brake comes to rest");
    Check(Run(M,AutoStop,AutoDriveInput(0,0),240).U>1,"releasing brake resumes automatic acceleration");
    Check(!TouchesRoadWall(State{}),"road center does not collide");
    Check(TouchesRoadWall(State{0,5.53,0,0,0,0}),"right footprint touches wall at exact boundary");
    Check(TouchesRoadWall(State{0,-5.53,0,0,0,0}),"left footprint touches wall at exact boundary");
    Check(!TouchesRoadWall(State{0,5.52,0,0,0,0}),"one centimeter clearance does not collide");
    Check(TouchesRoadWall(State{0,4.28,1.5707963267948966,0,0,0}),"rotated car nose hits wall before center reaches it");
    const double Contact=WallContactFraction(State{0,5,0,0,0,0},State{10,7,0,0,0,0});
    Near(Contact,.265,1e-10,"fast crossing locates wall contact before penetration");
    Near(WallContactFraction(State{0,5.53,0,0,0,0},State{0,6,0,0,0,0}),0,1e-12,"already touching wall stops immediately");
    Near(WheelLongitudinalSpeed(State{0,0,0,10,2,.5},1.2,.8,0),9.6,1e-12,"wheel speed uses local contact point velocity");
    Near(WheelLongitudinalSpeed(State{0,0,0,10,2,.5},1.2,-.8,0),10.4,1e-12,"inner and outer wheel speeds differ in a turn");
    State Facing{0,0,0,10*std::cos(.1),10*std::sin(.1),0};
    auto Aligned=M.Evaluate(Facing,{0,0,.2});
    Near(Aligned.Front.SlipAngle,0,1e-12,"wheel aligned with actual velocity has zero front slip");
    State Original{0,0,0,10,1,.15},Rotated=Original; Rotated.Yaw=1.2;
    auto O=Run(M,Original,{.1,0,.15},240);
    auto Q=Run(M,Rotated,{.1,0,.15},240);
    Near(Q.X,std::cos(1.2)*O.X-std::sin(1.2)*O.Y,1e-8,"steering independent of initial world heading X");
    Near(Q.Y,std::sin(1.2)*O.X+std::cos(1.2)*O.Y,1e-8,"steering independent of initial world heading Y");
    auto Launch=M.Evaluate(State{0,0,0,.1,0,0},{1,0,0});
    Near(Launch.Ax,1.1772,1e-8,"low-grip AWD launch stays within mu g");
    const auto Faster=Run(M,{}, {1,0,0},2400);
    Near(Faster.U*3.6,42.3792,1e-6,"low-grip profile reaches analytical speed from rest after ten seconds");
    M.Params.Friction=.18;
    const auto Baseline=Run(M,{}, {1,0,0},2400);
    Near(Faster.U/Baseline.U,2.0/3.0,1e-9,"restored profile accelerates two thirds as fast as mu 0.18");
    Check(!ReachedRoadEnd(State{600,0,0,0,0,0}),"extended road continues past old 495 meter limit");
    Check(!ReachedRoadEnd(State{994,0,0,0,0,0}) && ReachedRoadEnd(State{995,0,0,0,0,0}) &&
          ReachedRoadEnd(State{-995,0,0,0,0,0}),"two kilometer road ends retain five meter margin");
    M.Params.Friction = 0;
    State S; S.U = 12; S.V = 3; S.R = 0.2;
    S = Run(M,S,{1,1,0.5},2400);
    Near(S.X,120,1e-7,"frictionless world X despite body yaw");
    Near(S.Y,30,1e-7,"frictionless world Y despite body yaw");
    Near(S.Yaw,2,1e-10,"frictionless angular momentum");
    for (double Mu : {0.85,0.18,LowGripFriction})
    {
        M.Params.Friction = Mu;
        double Time; const double Distance = Stop(M,FixedStep,Time);
        const double Exact = 225/(2*Mu*9.81);
        std::cout << "METRIC braking_mu=" << Mu << " distance_m=" << Distance
                  << " ideal_m=" << Exact << " time_s=" << Time << '\n';
        Near(Distance,Exact,0.08,"braking distance agrees with ideal within 8 cm");
        Near(Time,15/(Mu*9.81),0.3,"braking time agrees within regularized stop tail");
    }
    M.Params.Friction = 0.12;
    S = {}; S.U = 20; S.V = 4; S.R = 0.3;
    auto T = M.Evaluate(S,{1,1,0.3});
    Check(std::hypot(T.Front.Fx,T.Front.Fy) <= 0.12*T.Front.Fz+1e-8 &&
          std::hypot(T.Rear.Fx,T.Rear.Fy) <= 0.12*T.Rear.Fz+1e-8,"combined forces respect both friction circles");
    Check(T.Front.Fz > 0 && T.Rear.Fz > 0,"axles carry positive normal loads");
    auto Left = Run(M,State{0,0,0,10,0,0},{0.12,0,0.2},1200);
    auto Right = Run(M,State{0,0,0,10,0,0},{0.12,0,-0.2},1200);
    Check(Left.Y > 0 && Left.Yaw > 0,"positive steering turns LEFT");
    Near(Left.Y,-Right.Y,1e-9,"left/right trajectory symmetry");
    Near(Left.Yaw,-Right.Yaw,1e-9,"left/right yaw symmetry");
    S = Run(M,{}, {0,1,0.5},2400);
    Near(std::hypot(S.U,S.V),0,1e-12,"braking at rest cannot launch or reverse car");
    S = Run(M,State{0,0,1,0.01,0.02,0.02},{0,1,0.3},10000);
    Check(std::isfinite(S.X) && std::hypot(S.U,S.V)<0.01,"low speed settles without instability");
    State Back{0,0,0,-5,2,0.1};
    auto BT=M.Evaluate(Back,{0,1,0});
    Check(BT.Fx*Back.U+BT.Fy*Back.V+BT.Moment*Back.R<=0,"braking/slip dissipate energy for reverse body velocity");
    M.Params.Friction = 0.85;
    // Independent linear single-track steady-state: solve force/moment balance.
    // For v_x=5, steer=.01, a=1.2,b=1.5,Cf=80000,Cr=90000:
    // r = u*delta / (L + m*u²/L*(b/Cf-a/Cr)).
    State Turn{0,0,0,5,0,0};
    for(int N=0;N<12000;++N)
    {
        Turn.U=5; // test fixture's speed-controlled steady-turn reference only
        M.Step(Turn,{0,0,0.02});
    }
    Near(Turn.R,0.0180165151388773,0.00005,"unsaturated steady yaw vs independent linear reference");
    double T1,T2,T3;
    const double D1=Stop(M,1.0/120,T1), D2=Stop(M,1.0/240,T2), D3=Stop(M,1.0/480,T3);
    std::cout << "METRIC convergence_m=" << D1 << ',' << D2 << ',' << D3 << '\n';
    Check(std::abs(D2-D3)<0.005 && std::abs(D1-D3)<0.01,"straight braking step convergence");
    auto A=Run(M,State{0,0,0,8,0,0},{0.2,0.1,0.1},2400);
    auto B=Run(M,State{0,0,0,8,0,0},{0.2,0.1,0.1},2400);
    Check(A.X==B.X && A.Y==B.Y && A.R==B.R,"same tick input replays identically in same build");
    const State Valid{2,3,0,5,0,0};
    S=Valid; M.Params.Friction=std::numeric_limits<double>::quiet_NaN();
    Check(!M.Step(S,{}) && S.X==Valid.X && S.U==Valid.U,"invalid coefficient rejects step without changing state");
    M.Params.Friction=0.12; S=Valid;
    Check(!M.Step(S,{0,0,std::numeric_limits<double>::infinity()}) && S.X==Valid.X && S.U==Valid.U,
          "invalid input rejects step without changing state");
    VehicleInputGate Gate;
    Check(!Gate.Update(false),"missing SDK input blocks initial drive");
    Check(Gate.Update(true),"first valid parked input permits automatic start");
    Check(!Gate.Update(false) && Gate.NeedsRestart,"input loss stops and latches restart");
    Check(!Gate.Update(true),"returning P alone cannot resume driving");
    Gate.Restart(false);
    Check(!Gate.Update(true),"restart while blocked cannot clear safety latch");
    Gate.Restart(true);
    Check(Gate.Update(true),"explicit restart with valid parked inputs resumes");
    VehicleInputGate EpochGate;
    EpochGate.Update(true,0);
    Check(!EpochGate.Update(true,1),"producer loss epoch catches blocked-ready transitions between ticks");
    EpochGate.Restart(true);
    Check(EpochGate.Update(true,1),"restart acknowledges the already sampled loss epoch");
    std::cout << "RESULT failures=" << Failures << '\n';
    return Failures ? 1 : 0;
}
