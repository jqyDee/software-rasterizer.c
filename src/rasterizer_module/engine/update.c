#include "update.h"

#include "../world.h"
#include <stddef.h>

void update(world *world) {
  // for (size_t i = 0; i < world->light_count; i++) {
  //   float angle =
  //       (float)GetTime() * LIGHT_ORBIT_SPEED +
  //       (float)i *
  //           (2.0f * PI / (float)world->light_count); // phase offset per light
  //   world->lights[i].light_dir = vec_normalize(((vec3f){
  //       cosf(angle),
  //       1.0f, // fixed height — keeps it "above" the scene like a sun
  //       sinf(angle),
  //   }));
  // }
}
