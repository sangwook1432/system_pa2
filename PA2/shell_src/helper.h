#ifndef HELPER
#define HELPER

#include <stdint.h>
#if defined __STDC_VERSION__
#if __STDC_VERSION__ < 202311L
#include <stdbool.h>
#endif
#endif

typedef int32_t status_t;

#define SAFELY_RUN(x)     \
  do {                    \
    if ((x) == -1) {      \
      perror(#x);         \
      exit(EXIT_FAILURE); \
    }                     \
  } while (0)

#include "builtin_commands.h"
#include "job.h"
#include "jobs.h"
#include "parser.h"

char* get_bin_dir();
#endif