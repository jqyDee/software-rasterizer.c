#include "raylib.h"
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 400

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT,
             "raylib [core] example - basic window");

  while (!WindowShouldClose()) {
    BeginDrawing();
    {
      ClearBackground(RAYWHITE);

      DrawText("Congrats! You created your first window!", 190, 200, 20,
               LIGHTGRAY);
    }
    EndDrawing();
  }

  CloseWindow();

  return 0;
}
