#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "raylib.h"
#include "types.h"
#include "vec.h"

static inline vec3f get_forward(cam *cam) {
  return (vec3f){sinf(cam->yaw), 0, cosf(cam->yaw)};
}

static inline vec3f get_right(cam *cam) {
  return (vec3f){sinf(cam->yaw - M_PI_2), 0, cosf(cam->yaw - M_PI_2)};
}

// 0 is nothing, 1 is reload .so, 2 is reload meshes and camera
int handle_user_input(world *world, const float delta_time) {
  cam *cam = world->cam;
  settings *settings = &world->settings;

  static bool is_rotating = false;

  // Use renderer display dims (not GetScreenWidth) so window_center is always
  // consistent with display_width, preventing cursor clamping and drift.
  static Vector2 window_center;
  window_center.x = (float)world->renderer->display_width / 2.0f;
  window_center.y = (float)world->renderer->display_height / 2.0f;

  const float mouse_sensitivity = 0.003f;
  const float move_speed = 5.0f;

  // MOUSE LOOK ONLY IF LEFT BUTTON HELD
  if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !settings->show_debug_gui) {
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

  // MOVE CAMERA WITH WASDQE (horizontal plane fixed, Q/E vertical)
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
  if (IsKeyDown(KEY_SPACE)) {
    cam->pos.y += move_speed * delta_time;
  }
  if (IsKeyDown(KEY_LEFT_SHIFT)) {
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

  if (world->mesh_data) {
    for (size_t i = 0; i < world->mesh_data_count; i++)
      free(world->mesh_data[i].vertices);
    free(world->mesh_data);
  }

  if (world->instances)
    free(world->instances);
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

void resize_renderer_to(world *world, int display_w, int display_h) {
  renderer *renderer = world->renderer;
  int render_width   = world->settings.render_width;

  renderer->display_width  = display_w;
  renderer->display_height = display_h;

  int new_w = render_width;
  int new_h = (int)(render_width * ((float)display_h / (float)display_w));

  renderer->screen_width  = new_w;
  renderer->screen_height = new_h;
  renderer->aspect_ratio  = (float)new_w / (float)new_h;

  free(renderer->framebuffer);
  free(renderer->depthbuffer);
  renderer->framebuffer = malloc(new_w * new_h * sizeof(Color));
  if (!renderer->framebuffer)
    exit(2);

  renderer->depthbuffer = malloc(new_w * new_h * sizeof(float));
  if (!renderer->depthbuffer)
    exit(2);

  UnloadTexture(renderer->screen_texture);
  init_texture(renderer);
}

void resize_renderer(world *world) {
  resize_renderer_to(world, GetScreenWidth(), GetScreenHeight());
}

static bool create_floor_mesh(mesh *m, float y, float size) {
  m->vertex_count = 6;
  m->vertices = malloc(6 * sizeof(vec3f));
  if (!m->vertices)
    return false;

  float s = size;
  // CCW winding from above → normal points +y (up)
  m->vertices[0] = (vec3f){-s, y, -s};
  m->vertices[1] = (vec3f){ s, y,  s};
  m->vertices[2] = (vec3f){ s, y, -s};
  m->vertices[3] = (vec3f){-s, y, -s};
  m->vertices[4] = (vec3f){-s, y,  s};
  m->vertices[5] = (vec3f){ s, y,  s};

  return true;
}

bool load_objs_files(world *world, char *obj_paths[], const size_t obj_count) {
  size_t total = obj_count + 1; // +1 for floor

  world->mesh_data_count = total;
  world->mesh_data = malloc(total * sizeof(mesh));
  if (!world->mesh_data)
    return false;

  world->instance_count = total;
  world->instances = malloc(total * sizeof(mesh_instance));
  if (!world->instances)
    return false;

  if (obj_paths != NULL) {
    world->obj_paths = obj_paths;
    world->obj_count = obj_count;

    for (size_t i = 0; i < obj_count; i++) {
      if (!load_obj(obj_paths[i], &world->mesh_data[i]))
        return false;
      world->instances[i].mesh = &world->mesh_data[i];
      world->instances[i].pos  = (vec3f){5.0f * i, 1.0f, 0.0f};
    }
  }

  if (!create_floor_mesh(&world->mesh_data[obj_count], 0.0f, 10.0f))
    return false;
  world->instances[obj_count].mesh = &world->mesh_data[obj_count];
  world->instances[obj_count].pos  = (vec3f){0, 0, 0};

  return true;
}

bool init_world(world *world, char *obj_paths[], const size_t obj_count,
                int display_w, int display_h) {
  renderer *rendererM = malloc(sizeof(renderer));
  if (!rendererM)
    return false;
  *rendererM = (renderer){0};

  cam *camM = malloc(sizeof(cam));
  if (!init_cam(camM)) {
    destroy_world(world);
    return false;
  }

  world->cam = camM;
  world->renderer = rendererM;
  world->settings = (settings){
    .show_debug_gui       = false,
    .render_width         = BASE_RENDER_WIDTH,
    .parallel_cutoff_rows = CUT_OFF_PARALLEL_DRAWING,
    .near_plane           = NEAR_PLANE,
  };

  resize_renderer_to(world, display_w, display_h);

  if (!load_objs_files(world, obj_paths, obj_count)) {
    destroy_world(world);
    return false;
  }

  return true;
}
