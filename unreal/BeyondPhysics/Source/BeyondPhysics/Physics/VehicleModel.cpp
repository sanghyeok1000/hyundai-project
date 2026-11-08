#include "VehicleModel.h"
#include "VirtualRamp.h"
#include "SteeringGeometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace BPPhysics {
namespace {
constexpr double kMaxExternalDt = 1.0 / 60.0;
constexpr double kTargetSubstep = 0.0005; // 0.5 ms, convergence still needs testing

inline double Unit(double x) { return std::clamp(x, 0.0, 1.0); }
inline bool Finite(double x) { return std::isfinite(x); }

AxleTelemetry Axle(double longitudinal, double lateral, double load,
                   double corneringStiffness, double driveDemand,
                   double brakeDemand, const Parameters& p)
{
    AxleTelemetry out{};
    out.Fz = std::max(0.0, load);
    const double limit = p.Friction * out.Fz;
    const double refU = std::max(std::abs(longitudinal), p.SlipRegularizationSpeed);
    const double tanAlpha = lateral / refU;
    out.SlipAngle = std::atan(tanAlpha);
    if (limit <= 0.0) return out;

    // Fiala-type pure lateral brush response. This is a simplified steady-state
    // shape, NOT a validated ice-tyre model or dynamic combined-slip model.
    double fyWanted = 0.0;
    if (corneringStiffness > 0.0) {
        const double q = corneringStiffness * tanAlpha / limit;
        if (std::abs(q) >= 3.0) {
            fyWanted = -std::copysign(limit, q);
        } else {
            fyWanted = -limit * (q - q * std::abs(q) / 3.0
                                    + q * q * q / 27.0);
        }
    }

    // Without explicit wheel angular velocities this is only a force-demand
    // approximation. tanh prevents discontinuous brake sign at zero speed.
    const double fxWanted = driveDemand - brakeDemand *
        std::tanh(longitudinal / p.BrakeRegularizationSpeed);

    // No artificial preference for drive/brake demand over lateral demand.
    // Radial combined-force projection respects sqrt(Fx^2 + Fy^2) <= mu Fz.
    const double length = std::hypot(fxWanted, fyWanted);
    const double scale = length > limit ? limit / length : 1.0;
    out.Fx = fxWanted * scale;
    out.Fy = fyWanted * scale;
    out.Utilization = std::hypot(out.Fx, out.Fy) / limit;
    return out;
}

State Derivative(const State& s, const Telemetry& t, const Parameters& p)
{
    const double c = std::cos(s.Yaw), sn = std::sin(s.Yaw);
    return {s.U * c - s.V * sn, s.U * sn + s.V * c, s.R,
            t.Ax + s.R * s.V,
            t.Ay - s.R * s.U,
            t.Moment / p.YawInertia};
}

State Add(const State& a, const State& b, double scale)
{
    return {a.X + scale * b.X, a.Y + scale * b.Y,
            a.Yaw + scale * b.Yaw, a.U + scale * b.U,
            a.V + scale * b.V, a.R + scale * b.R};
}

bool Valid(const State& s, const Input& in, const Parameters& p, double dt)
{
    const double values[] = {
        p.Mass, p.YawInertia, p.FrontDistance, p.RearDistance,
        p.FrontStiffness, p.RearStiffness, p.Gravity, p.Friction,
        p.DriveForce, p.BrakeForce, p.FrontDriveFraction, p.MaxSteer,
        p.SlipRegularizationSpeed, p.BrakeRegularizationSpeed, p.TrackWidth,
        in.Throttle, in.Brake, in.Steer, s.X, s.Y, s.Yaw, s.U, s.V, s.R, dt
    };
    for (double v : values) if (!Finite(v)) return false;
    return dt > 0.0 && dt <= kMaxExternalDt &&
           p.Mass > 0.0 && p.YawInertia > 0.0 &&
           p.FrontDistance > 0.0 && p.RearDistance > 0.0 &&
           p.FrontStiffness >= 0.0 && p.RearStiffness >= 0.0 &&
           p.Gravity >= 0.0 && p.Friction >= 0.0 &&
           p.DriveForce >= 0.0 && p.BrakeForce >= 0.0 &&
           p.FrontDriveFraction >= 0.0 && p.FrontDriveFraction <= 1.0 &&
           p.MaxSteer >= 0.0 && p.TrackWidth>=0 && p.SlipRegularizationSpeed > 0.0 &&
           p.BrakeRegularizationSpeed > 0.0;
}

bool FiniteState(const State& s)
{
    return Finite(s.X) && Finite(s.Y) && Finite(s.Yaw) &&
           Finite(s.U) && Finite(s.V) && Finite(s.R);
}
} // anonymous

double WheelLongitudinalSpeed(const State& state, double x, double y, double steer)
{
    // Rigid-body contact velocity v_contact = v_CG + omega cross position.
    return (state.U-state.R*y)*std::cos(steer)
         + (state.V+state.R*x)*std::sin(steer);
}

Telemetry VehicleModel::Evaluate(const State& s, const Input& in) const
{
    const auto& p = Params;
    const double wheelbase = p.FrontDistance + p.RearDistance;
    const double frontShare = p.RearDistance / wheelbase;
    const double rearShare = p.FrontDistance / wheelbase;
    const double steer = std::clamp(in.Steer, -1.0, 1.0) * p.MaxSteer;
    const double cs = std::cos(steer), sn = std::sin(steer);
    const double vf = s.V + p.FrontDistance * s.R;

    const auto surface=bRamp?Ramp::At(s.X,s.Y):Ramp::Surface{};
    const double normalZ=1/std::sqrt(1+surface.Hx*surface.Hx+surface.Hy*surface.Hy);
    // Quasi-static normal load: no suspension, dynamic load transfer or lift-off.
    const double normalGravity=p.Gravity*normalZ;

    Telemetry out{};
    out.SteerCenter=steer;
    if(bFourContact){
        const auto angles=AckermannAngles(steer,wheelbase,p.TrackWidth);
        for(int i=0;i<4;++i){
            const bool front=i<2;
            const double x=front?p.FrontDistance:-p.RearDistance,y=(i%2?-.5:.5)*p.TrackWidth;
            const double delta=front?angles[i]:0,c=std::cos(delta),sine=std::sin(delta);
            // Every tire uses CURRENT local rigid-body velocity, including yaw.
            const double vx=s.U-s.R*y,vy=s.V+s.R*x;
            const double share=front?frontShare:rearShare;
            const double driveShare=front?p.FrontDriveFraction:1-p.FrontDriveFraction;
            auto wheel=Axle(c*vx+sine*vy,-sine*vx+c*vy,p.Mass*normalGravity*share*.5,
                (front?p.FrontStiffness:p.RearStiffness)*.5,
                Unit(in.Throttle)*p.DriveForce*driveShare*.5,Unit(in.Brake)*p.BrakeForce*share*.5,p);
            out.Wheels[i]=wheel;out.WheelSteer[i]=delta;
            const double bx=c*wheel.Fx-sine*wheel.Fy,by=sine*wheel.Fx+c*wheel.Fy;
            out.Fx+=bx;out.Fy+=by;out.Moment+=x*by-y*bx;
            auto& axle=front?out.Front:out.Rear;
            axle.Fx+=wheel.Fx;axle.Fy+=wheel.Fy;axle.Fz+=wheel.Fz;
            axle.SlipAngle+=wheel.SlipAngle*.5;axle.Utilization=std::max(axle.Utilization,wheel.Utilization);
        }
    }else{
    out.Front = Axle(cs * s.U + sn * vf, -sn * s.U + cs * vf,
                     p.Mass * normalGravity * frontShare,
                     p.FrontStiffness, Unit(in.Throttle)*p.DriveForce*p.FrontDriveFraction,
                     Unit(in.Brake) * p.BrakeForce * frontShare, p);
    out.Rear = Axle(s.U, s.V - p.RearDistance * s.R,
                    p.Mass * normalGravity * rearShare,
                    p.RearStiffness, Unit(in.Throttle)*p.DriveForce*(1.0-p.FrontDriveFraction),
                    Unit(in.Brake) * p.BrakeForce * rearShare, p);

    const double ffx = cs * out.Front.Fx - sn * out.Front.Fy;
    const double ffy = sn * out.Front.Fx + cs * out.Front.Fy;
    out.Fx = ffx + out.Rear.Fx;
    out.Fy = ffy + out.Rear.Fy;
    out.Moment = p.FrontDistance * ffy - p.RearDistance * out.Rear.Fy;
    }
    out.Ax = out.Fx / p.Mass;  // chassis-frame specific force (not dot(U))
    out.Ay = out.Fy / p.Mass;  // chassis-frame specific force (not dot(V))
    out.Speed = std::hypot(s.U, s.V);
    if(bRamp) {
        // Resolve tangent tyre forces and gravity onto world XY. U,V remain
        // horizontal chassis components. Yaw/slip retain the small-slope 3DOF
        // approximation; this is not a six-DOF suspension/roll/pitch solver.
        double c=std::cos(s.Yaw),bodySin=std::sin(s.Yaw);
        double fz=surface.Hx*c+surface.Hy*bodySin;
        double inv=1/std::sqrt(1+fz*fz);
        double fx=c*inv,fy=bodySin*inv; fz*=inv;
        double nx=-surface.Hx*normalZ,ny=-surface.Hy*normalZ;
        double lx=ny*fz-normalZ*fy,ly=normalZ*fx-nx*fz;
        double ax=(out.Fx*fx+out.Fy*lx)/p.Mass+p.Gravity*normalZ*nx;
        double ay=(out.Fx*fy+out.Fy*ly)/p.Mass+p.Gravity*normalZ*ny;
        out.Ax=ax*c+ay*bodySin; out.Ay=-ax*bodySin+ay*c;
        double vx=s.U*c-s.V*bodySin,vy=s.U*bodySin+s.V*c;
        double vz=surface.Hx*vx+surface.Hy*vy;
        out.Speed=std::sqrt(vx*vx+vy*vy+vz*vz);
    }
    return out;
}

bool VehicleModel::Step(State& state, const Input& input, double dt) const
{
    if (!Valid(state, input, Params, dt)) return false;

    // Keep the external fixed tick, subdividing it into equal RK4 steps.
    // Commit only when every substep is finite; a failed step leaves state intact.
    const int count = static_cast<int>(std::ceil(dt / kTargetSubstep));
    const double h = dt / count;
    State next = state;
    for (int i = 0; i < count; ++i) {
        const State k1 = Derivative(next, Evaluate(next, input), Params);
        const State s2 = Add(next, k1, h * 0.5);
        const State k2 = Derivative(s2, Evaluate(s2, input), Params);
        const State s3 = Add(next, k2, h * 0.5);
        const State k3 = Derivative(s3, Evaluate(s3, input), Params);
        const State s4 = Add(next, k3, h);
        const State k4 = Derivative(s4, Evaluate(s4, input), Params);
        next = Add(Add(Add(Add(next, k1, h / 6.0), k2, h / 3.0),
                       k3, h / 3.0), k4, h / 6.0);
        if (!FiniteState(next)) return false;
    }
    state = next;
    return true;
}
} // namespace BPPhysics
