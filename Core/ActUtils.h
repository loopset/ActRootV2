#ifndef ActUtils_h
#define ActUtils_h

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
    bool IsEqZero(int val);// naive case
    bool IsEqZero(float val);
    bool IsEqZero(double val);

} // namespace ActRoot
#endif // !ActUtils_h
