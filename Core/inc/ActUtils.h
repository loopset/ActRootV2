#ifndef ActUtils_h
#define ActUtils_h

#include "Math/GenVector/DisplacementVector3D.h"

#include <string>

namespace ActRoot
{
// Templated functions are not added to
// ROOT's dictionary... opting for an
// overload version
// template <typename T>
// bool IsEqZero(T val, T eps = T {0.0001})
// {
//     return val * val <= eps * eps;
// }

// Prefer the overload case!
// Defining constexpr could be more performant
constexpr float kFloatEps {0.0001f};
constexpr double kDoubleEps {0.0001};
bool IsEqZero(int val); // naive case
bool IsEqZero(float val);
bool IsEqZero(double val);

// Casting functions for XYZPoint and XYZVector
template <typename T, typename U>
inline ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<T>, ROOT::Math::DefaultCoordinateSystemTag>
CastXYZPoint(const ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<U>, ROOT::Math::DefaultCoordinateSystemTag>& p)
{
    return {(T)p.X(), (T)p.Y(), (T)p.Z()};
}

template <typename T, typename U>
inline ROOT::Math::DisplacementVector3D<ROOT::Math::Cartesian3D<T>, ROOT::Math::DefaultCoordinateSystemTag>
CastXYZVector(
    const ROOT::Math::DisplacementVector3D<ROOT::Math::Cartesian3D<U>, ROOT::Math::DefaultCoordinateSystemTag>& v)
{

    return {(T)v.X(), (T)v.Y(), (T)v.Z()};
}


// String utility functions
std::string StripSpaces(std::string line);

std::string ToLower(std::string str);
} // namespace ActRoot
#endif // !ActUtils_h
