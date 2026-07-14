#pragma once

#include "raylib.h"
#include "../math/vec.h"

Color hsv_to_rgb(float h, float s, float v);
Color color_scale(Color base, vec3f light_rgb);