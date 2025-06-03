#include <stdlib.h>

#include "generators.h"
#include "types.h"
#include "vec.h"


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
