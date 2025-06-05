#include <stdbool.h>
#include <stdio.h>

#include <dlfcn.h>

int main(void) {
  void *state = NULL;

  int cycle = 0;
  while (true && cycle <= 10) {
    void *module = dlopen("./bin/rasterizer.so", RTLD_NOW);

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
