#pragma once
#include <cstdlib>
#include <string>

template<typename T>
int sign(T input) {
    return (input > (T)0) ? 1 : (input < 0 ? -1 : 0);
}

#if defined(__cplusplus) && __cplusplus < 201703L
namespace std {
    template<typename T> T clamp(T v, const T& lo, const T& hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
}
#endif
