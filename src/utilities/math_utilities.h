#pragma once
#include <algorithm>

template <typename T>
T sign(T input)
{
    if (input > static_cast<T>(0)) { return static_cast<T>(1); }
    else if (input < static_cast<T>(0)) { return static_cast<T>(-1); }
    else { return 0; }
}

#if defined(__cplusplus) && __cplusplus < 201703L
namespace std
{
template <typename T> T clamp(T v, const T& lo, const T& hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}
} // namespace std
#endif

template <typename T>
T saturate(T v) { return std::clamp(v, static_cast<T>(0), static_cast<T>(1)); }
