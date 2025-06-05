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

  vec3f temp_vertices[4096];
  size_t temp_vertex_count = 0;

  int face_indices[16384][3]; // Bigger to fit split quads
  size_t face_count = 0;

  char line[256];
  while (fgets(line, sizeof(line), file)) {
    if (strncmp(line, "v ", 2) == 0) {
      vec3f v;
      sscanf(line, "v %f %f %f", &v.x, &v.y, &v.z);
      temp_vertices[temp_vertex_count++] = v;
    } else if (strncmp(line, "f ", 2) == 0) {
      int vi[4] = {-1, -1, -1, -1};
      int count = 0;

      char *token = strtok(line + 2, " ");
      while (token && count < 4) {
        // Parse only vertex index before any slash
        sscanf(token, "%d", &vi[count]);
        count++;
        token = strtok(NULL, " ");
      }

      if (count == 3) {
        // Triangle
        for (int i = 0; i < 3; i++) {
          face_indices[face_count][i] = vi[i] - 1;
        }
        face_count++;
      } else if (count == 4) {
        // Quad split into two triangles
        face_indices[face_count][0] = vi[0] - 1;
        face_indices[face_count][1] = vi[1] - 1;
        face_indices[face_count][2] = vi[2] - 1;
        face_count++;

        face_indices[face_count][0] = vi[0] - 1;
        face_indices[face_count][1] = vi[2] - 1;
        face_indices[face_count][2] = vi[3] - 1;
        face_count++;
      }
    }
  }
  fclose(file);

  mesh->vertex_count = face_count * 3;
  mesh->vertices = malloc(mesh->vertex_count * sizeof(vec3f));
  if (!mesh->vertices)
    return false;

  for (size_t i = 0; i < face_count; ++i) {
    for (int j = 0; j < 3; ++j) {
      mesh->vertices[i * 3 + j] = temp_vertices[face_indices[i][j]];
    }
  }

  return true;
}
