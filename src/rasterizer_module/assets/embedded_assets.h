#pragma once

#include <stddef.h>

typedef struct {
  const char *path;
  const unsigned char *data;
  size_t size;
} embedded_asset_t;

extern const embedded_asset_t embedded_assets[];
extern const int embedded_asset_count;

const embedded_asset_t *find_embedded_asset(const char *path);
