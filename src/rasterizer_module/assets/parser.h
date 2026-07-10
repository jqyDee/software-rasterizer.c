#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "mesh.h"

bool load_obj(const char *filename, struct mesh_s *mesh);
bool load_obj_from_memory(const char *data, size_t len, struct mesh_s *mesh);
