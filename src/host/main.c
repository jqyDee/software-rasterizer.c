#include <stdbool.h>
#include <stdio.h>

#ifdef STATIC_RELEASE

extern void *rasterizer(void *);
int main(void) { rasterizer(NULL); return 0; }

#else

#include <dlfcn.h>

#if defined(__APPLE__)
#define PLUGIN_PATH BIN_DIR "/rasterizer.dylib"
#else
#define PLUGIN_PATH BIN_DIR "/rasterizer.so"
#endif

int main(void) {
  void *state = NULL;

  int cycle = 0;
  while (true && cycle <= 10) {
    void *module = dlopen(PLUGIN_PATH, RTLD_NOW);

    if (module == NULL) {
      cycle++;
      continue;
    } else {
      cycle = 0;
    }

    typedef void *module_main_func(void *state);
    module_main_func *module_main = dlsym(module, "rasterizer");
    state = module_main(state);

    dlclose(module);

    if (state == NULL) {
      return 0;
    }

  }
  return 0;
}

#endif
