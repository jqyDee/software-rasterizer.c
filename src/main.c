#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "draw.h"
#include "types.h"
#include "vec.h"

struct directions_s {
  vec3f *directions;
  size_t vertex_count;
};

// Utility Functions
void clear_framebuffer(Color *framebuffer, Color clearColor) {
  for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
    framebuffer[i] = clearColor;
  }
}

// Random Mesh + Direction Generation
void generate_random_mesh(struct mesh_s *mesh, struct directions_s *directions, size_t triangle_count) {
    if (triangle_count == 0) triangle_count = DEFAULT_TRIANGLE_COUNT;

    mesh->vertex_count = triangle_count * 3;
    mesh->positions = malloc(mesh->vertex_count * sizeof(vec3f));

    directions->vertex_count = mesh->vertex_count;
    directions->directions = malloc(mesh->vertex_count * sizeof(vec3f));

    for (size_t i = 0; i < triangle_count; i++) {
        // One random direction for this triangle
        vec3f dir = {
            ((rand() % 200) - 100) * 0.01f, // dx in [-1, 1)
            ((rand() % 200) - 100) * 0.01f,
            0.f,
        };

        // Random triangle center
        float cx = rand() % SCREEN_WIDTH;
        float cy = rand() % SCREEN_HEIGHT;
        float size = 10.0f + (rand() % 10);

        // Create triangle points and assign same direction to all 3
        for (int j = 0; j < 3; j++) {
            float angle = 2.0f * PI * j / 3.0f;
            float px = cx + cosf(angle) * size;
            float py = cy + sinf(angle) * size;

            size_t index = i * 3 + j;
            mesh->positions[index] = (vec3f){px, py, 0};
            directions->directions[index] = dir;  // same for all 3
        }
    }
}

// Vertex Movement with Edge Detection
void move_vertices(struct mesh_s *mesh, struct directions_s *directions) {
  for (size_t i = 0; i < directions->vertex_count; i++) {
    mesh->positions[i] = vec_add(mesh->positions[i], directions->directions[i]);

    if (mesh->positions[i].x < 0 || mesh->positions[i].x >= SCREEN_WIDTH ||
        mesh->positions[i].y < 0 || mesh->positions[i].y >= SCREEN_HEIGHT) {

      if (mesh->positions[i].x < 0)
        mesh->positions[i].x = 0;
      if (mesh->positions[i].x >= SCREEN_WIDTH)
        mesh->positions[i].x = SCREEN_WIDTH - 1;
      if (mesh->positions[i].y < 0)
        mesh->positions[i].y = 0;
      if (mesh->positions[i].y >= SCREEN_HEIGHT)
        mesh->positions[i].y = SCREEN_HEIGHT - 1;

      float dx = ((rand() % 200) - 100) * 0.01f;
      float dy = ((rand() % 200) - 100) * 0.01f;
      directions->directions[i] = (vec3f){dx, dy, 0.f};
    }
  }
}

int main(void) {
  srand(time(NULL));
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);
  if (GetMonitorHeight(0) > SCREEN_HEIGHT ||
      GetMonitorWidth(0) > SCREEN_WIDTH) {
    CloseWindow();
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);
  }

  struct mesh_s mesh;
  struct directions_s directions;
  generate_random_mesh(&mesh, &directions, 20);

  Color *framebuffer = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Color));

  Image image = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLANK);
  Texture2D screenTexture = LoadTextureFromImage(image);
  UnloadImage(image);

  while (!WindowShouldClose()) {
    BeginDrawing();
    {
      ClearBackground(BLACK);

      clear_framebuffer(framebuffer, BLACK);
      move_vertices(&mesh, &directions);

      draw_mesh(mesh, framebuffer);
      UpdateTexture(screenTexture, framebuffer);

      DrawTexture(screenTexture, 0, 0, WHITE);

      char fpsText[20];
      sprintf(fpsText, "FPS: %d", GetFPS());
      DrawText(fpsText, 10, 10, 20, RED);
    }
    EndDrawing();
  }

  free(mesh.positions);
  free(directions.directions);
  free(framebuffer);
  UnloadTexture(screenTexture);
  CloseWindow();
  return 0;
}
