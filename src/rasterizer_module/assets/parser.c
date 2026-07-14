#include "parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../math/vec.h"

/* Grow a heap buffer by 2× when full. Returns false on alloc failure. */
#define GROW(ptr, count, cap, type)                                            \
  ((count) >= (cap) ? (cap) *= 2,                                              \
   (ptr) = realloc((ptr), (cap) * sizeof(type)), (ptr) != NULL : true)

bool load_obj(const char *filename, struct mesh_s *mesh) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "Could not open %s\n", filename);
    return false;
  }

  /* dynamic temp arrays */
  size_t vert_cap = 4096, uv_cap = 4096, normal_cap = 4096, face_cap = 8192;
  vec3f *temp_verts = malloc(vert_cap * sizeof(vec3f));
  vec2f *temp_uvs = malloc(uv_cap * sizeof(vec2f));
  vec3f *temp_normals = malloc(normal_cap * sizeof(vec3f));
  int *face_vi = malloc(face_cap * 3 * sizeof(int));
  int *face_ti = malloc(face_cap * 3 * sizeof(int));
  int *face_ni = malloc(face_cap * 3 * sizeof(int));
  if (!temp_verts || !temp_uvs || !temp_normals || !face_vi || !face_ti ||
      !face_ni)
    goto oom;

  size_t temp_vc = 0, temp_uvc = 0, temp_nc = 0, face_count = 0;
  bool has_uvs = true;
  bool has_normals = true;

  char line[512];
  while (fgets(line, sizeof(line), file)) {
    if (strncmp(line, "vt ", 3) == 0) {
      if (temp_uvc >= uv_cap) {
        uv_cap *= 2;
        vec2f *t = realloc(temp_uvs, uv_cap * sizeof(vec2f));
        if (!t)
          goto oom;
        temp_uvs = t;
      }
      vec2f uv = {0, 0};
      sscanf(line, "vt %f %f", &uv.x, &uv.y);
      temp_uvs[temp_uvc++] = uv;

    } else if (strncmp(line, "vn ", 3) == 0) {
      if (temp_nc >= normal_cap) {
        normal_cap *= 2;
        vec3f *t = realloc(temp_normals, normal_cap * sizeof(vec3f));
        if (!t)
          goto oom;
        temp_normals = t;
      }
      vec3f n = {0, 0, 0};
      sscanf(line, "vn %f %f %f", &n.x, &n.y, &n.z);
      temp_normals[temp_nc++] = n;

    } else if (strncmp(line, "v ", 2) == 0) {
      if (temp_vc >= vert_cap) {
        vert_cap *= 2;
        vec3f *t = realloc(temp_verts, vert_cap * sizeof(vec3f));
        if (!t)
          goto oom;
        temp_verts = t;
      }
      vec3f v = {0, 0, 0};
      sscanf(line, "v %f %f %f", &v.x, &v.y, &v.z);
      temp_verts[temp_vc++] = v;

    } else if (strncmp(line, "f ", 2) == 0) {
      int vi[4] = {0}, ti[4] = {0}, ni[4] = {0};
      int count = 0;

      char *token = strtok(line + 2, " \r\n");
      while (token && count < 4) {
        int v_idx = 0, t_idx = 0, n_idx = 0;
        if (sscanf(token, "%d/%d/%d", &v_idx, &t_idx, &n_idx) ==
            3) { /* v/vt/vn */
        } else if (sscanf(token, "%d//%d", &v_idx, &n_idx) == 2) {
          t_idx = 0;
        } else if (sscanf(token, "%d/%d", &v_idx, &t_idx) == 2) { /* v/vt   */
        } else
          sscanf(token, "%d", &v_idx);

        vi[count] = v_idx - 1;
        ti[count] = t_idx - 1;
        ni[count] = n_idx - 1;
        if (t_idx == 0)
          has_uvs = false;
        if (n_idx == 0)
          has_normals = false;
        count++;
        token = strtok(NULL, " \r\n");
      }

      /* ensure space for up to 2 new faces (quad split) */
      if (face_count + 2 > face_cap) {
        face_cap *= 2;
        int *fv = realloc(face_vi, face_cap * 3 * sizeof(int));
        int *ft = realloc(face_ti, face_cap * 3 * sizeof(int));
        int *fn = realloc(face_ni, face_cap * 3 * sizeof(int));
        if (!fv || !ft || !fn)
          goto oom;
        face_vi = fv;
        face_ti = ft;
        face_ni = fn;
      }

      if (count >= 3) {
        face_vi[face_count * 3 + 0] = vi[0];
        face_ti[face_count * 3 + 0] = ti[0];
        face_ni[face_count * 3 + 0] = ni[0];
        face_vi[face_count * 3 + 1] = vi[1];
        face_ti[face_count * 3 + 1] = ti[1];
        face_ni[face_count * 3 + 1] = ni[1];
        face_vi[face_count * 3 + 2] = vi[2];
        face_ti[face_count * 3 + 2] = ti[2];
        face_ni[face_count * 3 + 2] = ni[2];
        face_count++;
      }
      if (count == 4) {
        face_vi[face_count * 3 + 0] = vi[0];
        face_ti[face_count * 3 + 0] = ti[0];
        face_ni[face_count * 3 + 0] = ni[0];
        face_vi[face_count * 3 + 1] = vi[2];
        face_ti[face_count * 3 + 1] = ti[2];
        face_ni[face_count * 3 + 1] = ni[2];
        face_vi[face_count * 3 + 2] = vi[3];
        face_ti[face_count * 3 + 2] = ti[3];
        face_ni[face_count * 3 + 2] = ni[3];
        face_count++;
      }
    }
  }
  fclose(file);

  mesh->vertex_count = face_count * 3;
  mesh->vertices = malloc(mesh->vertex_count * sizeof(vec3f));
  if (!mesh->vertices)
    goto oom;

  for (size_t i = 0; i < face_count; i++) {
    for (int j = 0; j < 3; j++) {
      int vi_idx = face_vi[i * 3 + j];
      if (vi_idx >= 0 && (size_t)vi_idx < temp_vc)
        mesh->vertices[i * 3 + j] = temp_verts[vi_idx];
    }
  }

  mesh->uvs = NULL;
  if (has_uvs && temp_uvc > 0) {
    mesh->uvs = malloc(mesh->vertex_count * sizeof(vec2f));
    if (mesh->uvs) {
      for (size_t i = 0; i < face_count; i++) {
        for (int j = 0; j < 3; j++) {
          int ti_idx = face_ti[i * 3 + j];
          if (ti_idx >= 0 && (size_t)ti_idx < temp_uvc)
            mesh->uvs[i * 3 + j] = temp_uvs[ti_idx];
          else
            mesh->uvs[i * 3 + j] = (vec2f){0, 0};
        }
      }
    }
  }

  mesh->normals = NULL;
  if (has_normals && temp_nc > 0) {
    mesh->normals = malloc(mesh->vertex_count * sizeof(vec3f));
    if (mesh->normals) {
      for (size_t i = 0; i < face_count; i++) {
        for (int j = 0; j < 3; j++) {
          int ni_idx = face_ni[i * 3 + j];
          mesh->normals[i * 3 + j] = (ni_idx >= 0 && (size_t)ni_idx < temp_nc)
                                         ? vec_normalize(temp_normals[ni_idx])
                                         : (vec3f){0, 1, 0};
        }
      }
    }
  }

  mesh->auto_tex_path[0] = '\0';

  free(temp_verts);
  free(temp_uvs);
  free(temp_normals);
  free(face_vi);
  free(face_ti);
  free(face_ni);
  return true;

oom:
  fclose(file);
  free(temp_verts);
  free(temp_uvs);
  free(temp_normals);
  free(face_vi);
  free(face_ti);
  free(face_ni);
  fprintf(stderr, "load_obj: out of memory loading %s\n", filename);
  return false;
}

bool load_obj_from_memory(const char *data, size_t len, struct mesh_s *mesh) {
  FILE *file = fmemopen((void *)data, len, "r");
  if (!file) {
    fprintf(stderr, "load_obj_from_memory: fmemopen failed\n");
    return false;
  }

  size_t vert_cap = 4096, uv_cap = 4096, normal_cap = 4096, face_cap = 8192;
  vec3f *temp_verts = malloc(vert_cap * sizeof(vec3f));
  vec2f *temp_uvs = malloc(uv_cap * sizeof(vec2f));
  vec3f *temp_normals = malloc(normal_cap * sizeof(vec3f));
  int *face_vi = malloc(face_cap * 3 * sizeof(int));
  int *face_ti = malloc(face_cap * 3 * sizeof(int));
  int *face_ni = malloc(face_cap * 3 * sizeof(int));
  if (!temp_verts || !temp_uvs || !temp_normals || !face_vi || !face_ti ||
      !face_ni)
    goto oom_mem;

  size_t temp_vc = 0, temp_uvc = 0, temp_nc = 0, face_count = 0;
  bool has_uvs = true;
  bool has_normals = true;

  char line[512];
  while (fgets(line, sizeof(line), file)) {
    if (strncmp(line, "vt ", 3) == 0) {
      if (temp_uvc >= uv_cap) {
        uv_cap *= 2;
        vec2f *t = realloc(temp_uvs, uv_cap * sizeof(vec2f));
        if (!t)
          goto oom_mem;
        temp_uvs = t;
      }
      vec2f uv = {0, 0};
      sscanf(line, "vt %f %f", &uv.x, &uv.y);
      temp_uvs[temp_uvc++] = uv;

    } else if (strncmp(line, "vn ", 3) == 0) {
      if (temp_nc >= normal_cap) {
        normal_cap *= 2;
        vec3f *t = realloc(temp_normals, normal_cap * sizeof(vec3f));
        if (!t)
          goto oom_mem;
        temp_normals = t;
      }
      vec3f n = {0, 0, 0};
      sscanf(line, "vn %f %f %f", &n.x, &n.y, &n.z);
      temp_normals[temp_nc++] = n;

    } else if (strncmp(line, "v ", 2) == 0) {
      if (temp_vc >= vert_cap) {
        vert_cap *= 2;
        vec3f *t = realloc(temp_verts, vert_cap * sizeof(vec3f));
        if (!t)
          goto oom_mem;
        temp_verts = t;
      }
      vec3f v = {0, 0, 0};
      sscanf(line, "v %f %f %f", &v.x, &v.y, &v.z);
      temp_verts[temp_vc++] = v;

    } else if (strncmp(line, "f ", 2) == 0) {
      int vi[4] = {0}, ti[4] = {0}, ni[4] = {0};
      int count = 0;

      char *token = strtok(line + 2, " \r\n");
      while (token && count < 4) {
        int v_idx = 0, t_idx = 0, n_idx = 0;
        if (sscanf(token, "%d/%d/%d", &v_idx, &t_idx, &n_idx) == 3) {
        } else if (sscanf(token, "%d//%d", &v_idx, &n_idx) == 2) {
          t_idx = 0;
        } else if (sscanf(token, "%d/%d", &v_idx, &t_idx) == 2) {
        } else
          sscanf(token, "%d", &v_idx);

        vi[count] = v_idx - 1;
        ti[count] = t_idx - 1;
        ni[count] = n_idx - 1;
        if (t_idx == 0)
          has_uvs = false;
        if (n_idx == 0)
          has_normals = false;
        count++;
        token = strtok(NULL, " \r\n");
      }

      if (face_count + 2 > face_cap) {
        face_cap *= 2;
        int *fv = realloc(face_vi, face_cap * 3 * sizeof(int));
        int *ft = realloc(face_ti, face_cap * 3 * sizeof(int));
        int *fn = realloc(face_ni, face_cap * 3 * sizeof(int));
        if (!fv || !ft || !fn)
          goto oom_mem;
        face_vi = fv;
        face_ti = ft;
        face_ni = fn;
      }

      if (count >= 3) {
        face_vi[face_count * 3 + 0] = vi[0];
        face_ti[face_count * 3 + 0] = ti[0];
        face_ni[face_count * 3 + 0] = ni[0];
        face_vi[face_count * 3 + 1] = vi[1];
        face_ti[face_count * 3 + 1] = ti[1];
        face_ni[face_count * 3 + 1] = ni[1];
        face_vi[face_count * 3 + 2] = vi[2];
        face_ti[face_count * 3 + 2] = ti[2];
        face_ni[face_count * 3 + 2] = ni[2];
        face_count++;
      }
      if (count == 4) {
        face_vi[face_count * 3 + 0] = vi[0];
        face_ti[face_count * 3 + 0] = ti[0];
        face_ni[face_count * 3 + 0] = ni[0];
        face_vi[face_count * 3 + 1] = vi[2];
        face_ti[face_count * 3 + 1] = ti[2];
        face_ni[face_count * 3 + 1] = ni[2];
        face_vi[face_count * 3 + 2] = vi[3];
        face_ti[face_count * 3 + 2] = ti[3];
        face_ni[face_count * 3 + 2] = ni[3];
        face_count++;
      }
    }
  }
  fclose(file);

  mesh->vertex_count = face_count * 3;
  mesh->vertices = malloc(mesh->vertex_count * sizeof(vec3f));
  if (!mesh->vertices)
    goto oom_mem;

  for (size_t i = 0; i < face_count; i++) {
    for (int j = 0; j < 3; j++) {
      int vi_idx = face_vi[i * 3 + j];
      if (vi_idx >= 0 && (size_t)vi_idx < temp_vc)
        mesh->vertices[i * 3 + j] = temp_verts[vi_idx];
    }
  }

  mesh->uvs = NULL;
  if (has_uvs && temp_uvc > 0) {
    mesh->uvs = malloc(mesh->vertex_count * sizeof(vec2f));
    if (mesh->uvs) {
      for (size_t i = 0; i < face_count; i++) {
        for (int j = 0; j < 3; j++) {
          int ti_idx = face_ti[i * 3 + j];
          if (ti_idx >= 0 && (size_t)ti_idx < temp_uvc)
            mesh->uvs[i * 3 + j] = temp_uvs[ti_idx];
          else
            mesh->uvs[i * 3 + j] = (vec2f){0, 0};
        }
      }
    }
  }

  mesh->normals = NULL;
  if (has_normals && temp_nc > 0) {
    mesh->normals = malloc(mesh->vertex_count * sizeof(vec3f));
    if (mesh->normals) {
      for (size_t i = 0; i < face_count; i++) {
        for (int j = 0; j < 3; j++) {
          int ni_idx = face_ni[i * 3 + j];
          mesh->normals[i * 3 + j] = (ni_idx >= 0 && (size_t)ni_idx < temp_nc)
                                         ? vec_normalize(temp_normals[ni_idx])
                                         : (vec3f){0, 1, 0};
        }
      }
    }
  }

  mesh->auto_tex_path[0] = '\0';

  free(temp_verts);
  free(temp_uvs);
  free(temp_normals);
  free(face_vi);
  free(face_ti);
  free(face_ni);
  return true;

oom_mem:
  fclose(file);
  free(temp_verts);
  free(temp_uvs);
  free(temp_normals);
  free(face_vi);
  free(face_ti);
  free(face_ni);
  fprintf(stderr, "load_obj_from_memory: out of memory\n");
  return false;
}
