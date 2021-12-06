#ifndef constants
#define constants
#include <stdint.h>
// inorder to facilitate, align any addresses to word boundary if malloc doesn't
#if UINTPTR_MAX == 0xFFFF
  typedef uint16_t ssm_word_t;
#elif INTPTR_MAX == INT32_MAX
  typedef uint32_t ssm_word_t;
#elif INTPTR_MAX == INT64_MAX
  typedef uint64_t ssm_word_t;
#else
  #error Unknown pointer size
#endif

#define WORD_BOUNDARY sizeof(ssm_word_t)

#define INFO_DEST stdout
//#define DEBUG_DEST stdout
#define DEBUG_DEST fopen("/dev/null","w")
//#uncomment above to suppress debug lines
#endif
