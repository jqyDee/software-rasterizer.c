#include "shaders.h"

#include <stdlib.h>

#include "raylib.h"

void destroy_shaders(renderer *renderer) {
  UnloadShader(renderer->phong_shader);
}

void init_shaders(renderer *renderer) {
  renderer->phong_shader = LoadShader(NULL, "assets/shaders/phong.fs");
  renderer->phong_normal_map_loc =
      GetShaderLocation(renderer->phong_shader, "normalMap");
  renderer->phong_light_dirs_loc =
      GetShaderLocation(renderer->phong_shader, "lightDirs");
  renderer->phong_light_colors_loc =
      GetShaderLocation(renderer->phong_shader, "lightColors");
  renderer->phong_light_intensities_loc =
      GetShaderLocation(renderer->phong_shader, "lightIntensities");
  renderer->phong_light_count_loc =
      GetShaderLocation(renderer->phong_shader, "lightCount");
  renderer->phong_ambient_loc =
      GetShaderLocation(renderer->phong_shader, "ambient");
}
