#include <math.h>
#include <stdlib.h>

#include "types.h"
#include "parser.h"

void handle_user_input(cam *cam, float delta_time) {
  float deltaTime = GetFrameTime();
  float speed = 5.0f;

  if (IsKeyDown(KEY_W))
    cam->pos.z += speed * deltaTime;
  if (IsKeyDown(KEY_S))
    cam->pos.z -= speed * deltaTime;
  if (IsKeyDown(KEY_A))
    cam->pos.x -= speed * deltaTime;
  if (IsKeyDown(KEY_D))
    cam->pos.x += speed * deltaTime;
  if (IsKeyDown(KEY_Q))
    cam->pos.y += speed * deltaTime;
  if (IsKeyDown(KEY_E))
    cam->pos.y -= speed * deltaTime;
}

bool init_cam(cam *cam) {
  if (!cam)
    return false;

  cam->pos = (vec3f){0, 0, -5};

  cam->pitch = 0.0f;
  cam->yaw = 0.0f;

  cam->fov = 60.0f;

  float fov_rad = cam->fov * (PI / 180.0f);
  cam->focal_length = (1.0f / tanf(fov_rad / 2.0f));

  return true;
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
    free(renderer->framebuffer);
    return false;
  }

  Image image = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLANK);
  renderer->screen_texture = LoadTextureFromImage(image);
  UnloadImage(image);

  return true;
}

bool init_world(world *world, char *obj_paths[], size_t obj_count) {
  renderer *rendererM = malloc(sizeof(renderer));
  if (!init_renderer(rendererM)) {
    return false;
  }

  cam *camM = malloc(sizeof(cam));
  if (!init_cam(camM)) {
    free(rendererM);
    return false;
  }

  world->cam = camM;
  world->renderer = rendererM;
  world->mesh_count = obj_count;
  world->meshes = malloc(obj_count * sizeof(mesh));
  if (!world->meshes) {
    free(rendererM);
    free(camM);
    return false;
  }

  if (obj_paths == NULL || obj_count == 0) {
    return true;
  }

  for (size_t i = 0; i < obj_count; i++) {
    if (!load_obj(obj_paths[i], &world->meshes[i])) {
      free(rendererM);
      free(camM);
      free(world->meshes);
      return false;
    }
  }

  return true;
}

void destroy_world(world *world) {
  free(world->renderer->depthbuffer);
  free(world->renderer->framebuffer);
  free(world->renderer);
  free(world->cam);
  free(world->meshes);
}
