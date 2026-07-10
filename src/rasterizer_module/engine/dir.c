#include "dir.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../testing.h"

INTERNAL int cmp_str(const void *a, const void *b) {
  return strcmp(*(const char **)a, *(const char **)b);
}

char **scan_dir(const char *dir, const char **exts, int n_exts,
                       int *out_count) {
  *out_count = 0;
  DIR *d = opendir(dir);
  if (!d)
    return NULL;

  char **paths = NULL;
  int cap = 0, count = 0;
  struct dirent *ent;
  while ((ent = readdir(d))) {
    const char *dot = strrchr(ent->d_name, '.');
    if (!dot)
      continue;
    int match = 0;
    for (int e = 0; e < n_exts; e++)
      if (strcasecmp(dot, exts[e]) == 0) {
        match = 1;
        break;
      }
    if (!match)
      continue;
    if (count >= cap) {
      cap = cap ? cap * 2 : 16;
      char **np = realloc(paths, (size_t)cap * sizeof(char *));
      if (!np)
        break;
      paths = np;
    }
    char full[512];
    snprintf(full, sizeof(full), "%s/%s", dir, ent->d_name);
    paths[count++] = strdup(full);
  }
  closedir(d);
  if (count > 1)
    qsort(paths, (size_t)count, sizeof(char *), cmp_str);
  *out_count = count;
  return paths;
}
