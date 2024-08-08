#include "vec.h"

Vec2 vec2(float x, float y) {
    return (Vec2) {
        .x = x,
        .y = y
    };
}

Vec2 vec2s(float x) {
    return vec2(x, x);
}