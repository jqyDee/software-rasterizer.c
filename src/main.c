#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"

#include "draw.h"
#include "parser.h"
#include "types.h"

#define ASPECT_RATIO ((float)GetScreenWidth() / (float)GetScreenHeight())

int main(void) {

  // CUBE
  struct mesh_s cube;
  if (!load_obj("./obj/cube.obj", &cube)) {
    return 1;
  }

  srand(time(NULL));

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);
  if (GetMonitorHeight(0) > SCREEN_HEIGHT ||
      GetMonitorWidth(0) > SCREEN_WIDTH) {
    CloseWindow();
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);
  }

  // TRIANGLE
  // vec3f pos1 = {0, 0, 5};
  // vec3f pos2 = {5, 10, 5};
  // vec3f pos3 = {10, 0, 5};
  // vec3f positions[] = {
  //     pos1,
  //     pos2,
  //     pos3,
  // };
  // struct mesh_s triangle = {.positions = positions, .vertex_count = 3};

  cam cam = {(vec3f){0, 0, -10}, 0, 0};

  float *depthbuffer = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(float));

  Color *framebuffer = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Color));

  Image image = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLANK);
  Texture2D screenTexture = LoadTextureFromImage(image);
  UnloadImage(image);

  while (!WindowShouldClose()) {
    BeginDrawing();
    {
      // INPUT HANDLING
      float deltaTime = GetFrameTime();
      float speed = 5.0f;

      if (IsKeyDown(KEY_W))
        cam.pos.z += speed * deltaTime;
      if (IsKeyDown(KEY_S))
        cam.pos.z -= speed * deltaTime;
      if (IsKeyDown(KEY_A))
        cam.pos.x -= speed * deltaTime;
      if (IsKeyDown(KEY_D))
        cam.pos.x += speed * deltaTime;
      if (IsKeyDown(KEY_Q))
        cam.pos.y += speed * deltaTime;
      if (IsKeyDown(KEY_E))
        cam.pos.y -= speed * deltaTime;

      // RENDERING
      ClearBackground(WHITE);

      clear_framebuffer(framebuffer, WHITE);
      clear_depthbuffer(depthbuffer);

      render_mesh(cube, framebuffer, depthbuffer, cam);

      UpdateTexture(screenTexture, framebuffer);
      DrawTexture(screenTexture, 0, 0, WHITE);

      // FPS
      char fpsText[20];
      sprintf(fpsText, "FPS: %d", GetFPS());
      DrawText(fpsText, 10, 10, 20, RED);
    }
    EndDrawing();
  }

  free(cube.positions);
  free(framebuffer);
  free(depthbuffer);
  UnloadTexture(screenTexture);
  CloseWindow();
  return 0;
}
