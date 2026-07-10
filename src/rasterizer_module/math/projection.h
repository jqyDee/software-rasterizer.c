#pragma once

#include <stdbool.h>

#include "../engine/cam.h"
#include "../renderer/renderer.h"

void project(const cam *cam, const renderer *renderer, vec3f cam_pos,
             vec3f *out);
