#ifndef UNIT_TEST
#define INTERNAL static
#define INTERNAL_INLINE static inline
#else
#define INTERNAL
#define INTERNAL_INLINE
#endif