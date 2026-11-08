#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace BPPhysics {
// Ideal rack: a declared generic ratio, not a calibrated OEM steering map.
inline double HandwheelToRoadRadians(double degrees,double ratio,double maximum){
    return std::clamp(degrees*(std::acos(-1.)/180.)/ratio,-maximum,maximum);
}
inline std::array<double,2> AckermannAngles(double center,double wheelbase,double track){
    const double numerator=wheelbase*std::sin(center),base=wheelbase*std::cos(center);
    return {std::atan2(numerator,base-track*.5*std::sin(center)),
            std::atan2(numerator,base+track*.5*std::sin(center))};
}
}
