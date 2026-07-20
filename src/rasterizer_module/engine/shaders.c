#include "shaders.h"

#include <stdlib.h>

#include "raylib.h"

#define SKY_TEXTURE_PATH "assets/textures/sky.png"

void destroy_shaders(renderer *renderer) {
  UnloadShader(renderer->phong_shader);
  if (renderer->sky_texture.id != 0) {
    UnloadTexture(renderer->sky_texture);
    renderer->sky_texture = (Texture2D){0};
  }
}

void init_shaders(renderer *renderer) {
  renderer->phong_shader = LoadShader(NULL, "assets/shaders/phong.fs");

  /* equirectangular sky panorama — optional, id stays 0 when missing and
   * the shader falls back to the plain clear color */
  renderer->sky_texture = LoadTexture(SKY_TEXTURE_PATH);
  if (renderer->sky_texture.id != 0)
    SetTextureFilter(renderer->sky_texture, TEXTURE_FILTER_BILINEAR);

  renderer->phong_sky_map_loc =
      GetShaderLocation(renderer->phong_shader, "skyMap");
  renderer->phong_use_sky_loc =
      GetShaderLocation(renderer->phong_shader, "useSky");
  renderer->phong_cam_right_loc =
      GetShaderLocation(renderer->phong_shader, "camRight");
  renderer->phong_cam_up_loc =
      GetShaderLocation(renderer->phong_shader, "camUp");
  renderer->phong_cam_fwd_loc =
      GetShaderLocation(renderer->phong_shader, "camFwd");
  renderer->phong_focal_loc =
      GetShaderLocation(renderer->phong_shader, "focalLen");
  renderer->phong_aspect_loc =
      GetShaderLocation(renderer->phong_shader, "aspectRatio");
  renderer->phong_boost_loc =
      GetShaderLocation(renderer->phong_shader, "boostAmount");
  renderer->phong_time_loc =
      GetShaderLocation(renderer->phong_shader, "time");
  renderer->phong_speed_loc =
      GetShaderLocation(renderer->phong_shader, "speedAmount");
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
  renderer->phong_viewport_rects_loc =
      GetShaderLocation(renderer->phong_shader, "viewportRects");
  renderer->phong_viewport_count_loc =
      GetShaderLocation(renderer->phong_shader, "viewportCount");
  renderer->phong_buffer_size_loc =
      GetShaderLocation(renderer->phong_shader, "bufferSize");
}
