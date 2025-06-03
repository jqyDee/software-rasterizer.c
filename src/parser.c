#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

bool load_obj(const char *filename, struct mesh_s *mesh) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "Could not open %s\n", filename);
    return false;
  }

  vec3f temp_vertices[2048];
  size_t temp_vertex_count = 0;

  // Temporarily store triangle indices (we'll allocate once we know how many)
  int face_indices[4096][3];
  size_t face_count = 0;

  char line[256];
  while (fgets(line, sizeof(line), file)) {
    if (strncmp(line, "v ", 2) == 0) {
      vec3f v;
      sscanf(line, "v %f %f %f", &v.x, &v.y, &v.z);
      temp_vertices[temp_vertex_count++] = v;
    } else if (strncmp(line, "f ", 2) == 0) {
      int vi[3];
      if (sscanf(line, "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d", &vi[0], &vi[1],
                 &vi[2]) == 3) {
        for (int i = 0; i < 3; i++) {
          face_indices[face_count][i] = vi[i] - 1; // OBJ is 1-based
        }
        face_count++;
      }
    }
  }
  fclose(file);

  // Allocate once with exact size
  mesh->vertex_count = face_count * 3;
  mesh->positions = malloc(mesh->vertex_count * sizeof(vec3f));
  if (!mesh->positions)
    return false;

  for (size_t i = 0; i < face_count; ++i) {
    for (int j = 0; j < 3; ++j) {
      mesh->positions[i * 3 + j] = temp_vertices[face_indices[i][j]];
    }
  }

  return true;
}
