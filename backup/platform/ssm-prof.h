/**
 * SSM GPIO profiler interface.
 */
#ifndef _PLATFORM_SSM_PROF_H
#define _PLATFORM_SSM_PROF_H

/** Platform-specific bindings.
 *
 * May define SSM_PROF(sym)
 *
 */
#include <platform-specific/ssm-prof.h>

#ifndef SSM_PROF_INIT
#define SSM_PROF_INIT(...) do ; while (0)
#endif

#ifndef SSM_PROF
#define SSM_PROF(...) do ; while (0)
#endif

/** Symbols given to SSM_PROF to indicate where the program is executing. */
enum {
  SSM_PROF_MAIN_SETUP,
  SSM_PROF_MAIN_BEFORE_LOOP_CONTINUE,
  SSM_PROF_MAIN_BEFORE_INPUT_CONSUME,
  SSM_PROF_MAIN_BEFORE_TICK,
  SSM_PROF_MAIN_BEFORE_SLEEP_ALARM,
  SSM_PROF_MAIN_BEFORE_SLEEP_BLOCK,
  SSM_PROF_MAIN_BEFORE_WAKE_RESET,
  SSM_PROF_INPUT_BEFORE_READTIME,
  SSM_PROF_INPUT_BEFORE_ALLOC,
  SSM_PROF_INPUT_BEFORE_COMMIT,
  SSM_PROF_INPUT_BEFORE_WAKE,
  SSM_PROF_INPUT_BEFORE_LEAVE,
  SSM_PROF_USER_BEFORE_STEP,
  SSM_PROF_USER_BEFORE_YIELD,
  SSM_PROF_USER_BEFORE_LEAVE,
  SSM_PROF_LIMIT,
};

#endif /* ifndef _PLATFORM_SSM_PROF_H */
