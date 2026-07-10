#pragma once

#define ASSETS_OBJ_DIR "assets/obj"
#define ASSETS_TEX_DIR "assets/textures"

char **scan_dir(const char *dir, const char **exts, int n_exts,
                       int *out_count);
