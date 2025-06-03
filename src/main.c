#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"

#include "draw.h"
#include "parser.h"
#include "types.h"

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

  cam cam = {(vec3f){0, 0, -20}, 0, 0};

  int screen_width = GetScreenWidth();
  int screen_height = GetScreenHeight();

  float *depthbuffer = malloc(screen_width * screen_height * sizeof(float));
  Color *framebuffer = malloc(screen_width * screen_height * sizeof(Color));

  Image image = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLANK);
  Texture2D screenTexture = LoadTextureFromImage(image);
  UnloadImage(image);

  while (!WindowShouldClose()) {
    // RENDERING
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

      ClearBackground(WHITE);

      clear_framebuffer(framebuffer, screen_width, screen_height, WHITE);
      clear_depthbuffer(depthbuffer, screen_width, screen_height);

      render_mesh(cube, framebuffer, depthbuffer, cam, screen_width,
                  screen_height);

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
