#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "types.h"
#include "vec.h"

static inline vec3f get_forward(cam *cam) {
  return (vec3f){cosf(cam->pitch) * sinf(cam->yaw), sinf(cam->pitch),
                 cosf(cam->pitch) * cosf(cam->yaw)};
}

static inline vec3f get_right(cam *cam) {
  return (vec3f){sinf(cam->yaw - M_PI_2), 0, cosf(cam->yaw - M_PI_2)};
}

// 0 is nothing, 1 is reload .so, 2 is reload meshes and camera
int handle_user_input(cam *cam, const float delta_time) {
  static bool is_rotating = false;
  static Vector2 window_center = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
  const float mouse_sensitivity = 0.003f;
  const float move_speed = 5.0f;

  // MOUSE LOOK ONLY IF LEFT BUTTON HELD
  if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    if (!is_rotating) {
      // First frame: center mouse, no rotation yet
      SetMousePosition(window_center.x, window_center.y);
      is_rotating = true;
      return 0;
    }

    Vector2 mouse_pos = GetMousePosition();
    Vector2 delta = {mouse_pos.x - window_center.x,
                     mouse_pos.y - window_center.y};

    cam->yaw += delta.x * mouse_sensitivity;
    cam->pitch -= delta.y * mouse_sensitivity;

    // Clamp pitch [-89°, +89°] in radians
    const float max_pitch = M_PI_2 - 0.01f;
    if (cam->pitch > max_pitch)
      cam->pitch = max_pitch;
    if (cam->pitch < -max_pitch)
      cam->pitch = -max_pitch;

    // Reset mouse back to center to keep delta small next frame
    SetMousePosition(window_center.x, window_center.y);
  } else {
    is_rotating = false;
  }

  // MOVE CAMERA WITH WASDQE RELATIVE TO YAW/PITCH
  vec3f forward = get_forward(cam);
  vec3f right = get_right(cam);

  if (IsKeyDown(KEY_W)) {
    cam->pos = vec_add(cam->pos, vec_scale(forward, move_speed * delta_time));
  }
  if (IsKeyDown(KEY_S)) {
    cam->pos = vec_sub(cam->pos, vec_scale(forward, move_speed * delta_time));
  }
  if (IsKeyDown(KEY_A)) {
    cam->pos = vec_add(cam->pos, vec_scale(right, move_speed * delta_time));
  }
  if (IsKeyDown(KEY_D)) {
    cam->pos = vec_sub(cam->pos, vec_scale(right, move_speed * delta_time));
  }
  if (IsKeyDown(KEY_Q)) {
    cam->pos.y += move_speed * delta_time;
  }
  if (IsKeyDown(KEY_E)) {
    cam->pos.y -= move_speed * delta_time;
  }

  // RELOAD
  if (IsKeyPressed(KEY_F5)) {
    return 1;
  }

  // RELOAD MESHES AND CAMERA
  if (IsKeyPressed(KEY_F6)) {
    return 2;
  }

  return 0;
}

void destroy_world(world *world) {
  if (!world)
    return;

  if (world->renderer->depthbuffer)
    free(world->renderer->depthbuffer);

  if (world->renderer->framebuffer)
    free(world->renderer->framebuffer);

  if (world->renderer)
    free(world->renderer);

  if (world->cam)
    free(world->cam);

  if (world->meshes)
    free(world->meshes);
}

bool init_cam(cam *cam) {
  if (!cam)
    return false;

  cam->pos = (vec3f){0, 0, 5};

  cam->pitch = 0.0f;
  cam->yaw = M_PI;

  cam->fov = 60.0f;

  float fov_rad = cam->fov * (PI / 180.0f);
  cam->focal_length = (1.0f / tanf(fov_rad / 2.0f));

  return true;
}

void init_texture(renderer *renderer) {
  Image image =
      GenImageColor(renderer->screen_width, renderer->screen_height, BLANK);
  renderer->screen_texture = LoadTextureFromImage(image);
  UnloadImage(image);
}

bool init_renderer(renderer *renderer) {
  renderer->screen_width = GetScreenWidth();
  renderer->screen_height = GetScreenHeight();
  renderer->aspect_ratio =
      (float)renderer->screen_width / renderer->screen_height;

  renderer->framebuffer =
      malloc(renderer->screen_width * renderer->screen_height * sizeof(Color));
  if (!renderer->framebuffer) {
    return false;
  }

  renderer->depthbuffer =
      malloc(renderer->screen_width * renderer->screen_height * sizeof(float));

  if (!renderer->depthbuffer) {
    return false;
  }

  init_texture(renderer);

  return true;
}

bool load_objs_files(world *world, char *obj_paths[], const size_t obj_count) {
  world->mesh_count = obj_count;
  world->meshes = malloc(obj_count * sizeof(mesh));
  if (!world->meshes) {
    return false;
  }

  if (obj_paths == NULL) {
    return true;
  }

  world->obj_paths = obj_paths;
  world->obj_count = obj_count;

  for (size_t i = 0; i < obj_count; i++) {
    if (!load_obj(obj_paths[i], &world->meshes[i])) {
      return false;
    }
  }

  return true;
}

bool init_world(world *world, char *obj_paths[], const size_t obj_count) {
  renderer *rendererM = malloc(sizeof(renderer));
  if (!init_renderer(rendererM)) {
    return false;
  }

  cam *camM = malloc(sizeof(cam));
  if (!init_cam(camM)) {
    destroy_world(world);
    return false;
  }

  world->cam = camM;
  world->renderer = rendererM;

  if (!load_objs_files(world, obj_paths, obj_count)) {
      destroy_world(world);
      return false;
  }

  return true;
}

