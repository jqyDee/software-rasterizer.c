#include <math.h>
#include <stdlib.h>

#include "raylib.h"

#include "generators.h"
#include "types.h"

// Random Mesh + Direction Generation
void generate_random_mesh(struct mesh_s *mesh, struct directions_s *directions,
                          size_t triangle_count) {
  if (triangle_count == 0)
    triangle_count = DEFAULT_TRIANGLE_COUNT;

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
      directions->directions[index] = dir; // same for all 3
    }
  }
}
