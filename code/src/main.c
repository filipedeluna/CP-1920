#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "args.h"
#include "unit.h"
#include "debug.h"
#include "omp.h"

// You may replace this with opm_get_wtime() //TODO
static long wall_clock_time(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (long) tv.tv_sec * 1e6 + tv.tv_usec;
}

// Global Argp Vars
const char *argp_program_bug_address = "<f.luna@campus.fct.unl.pt> or <gl.batista@campus.fct.unl.pt>";
const char *argp_program_version = "0.1";

// Example documentation
static const char *argp_doc = "Parallel Patterns with C and OpenMP - CP 2019.";

int main(int argc, char *argv[]) {
  // Set up argp and extract configs
  struct argp argp = {argp_options, argp_option_parser, 0,
                      argp_doc, 0, 0, 0};

  argp_args args;
  argp_parse(&argp, argc, argv, 0, 0, &args);

  if (args.debug_mode)
    DEBUG_MODE = 1;

  // Setup OpenMP
  omp_set_num_threads(args.num_threads);

  // Generate random values to fill src array
  srand(time(NULL));
  srand48(time(NULL));

  // Initialize src array for all iterations
  printf("Initializing SRC array\n");
  TYPE *src = malloc(TYPE_SIZE * args.iterations);

  for (int i = 0; i < args.iterations; i++) {
    if (strcmp(TYPE_NAME, "int") == 0) {
      src[i] = (TYPE) (drand48() * INT_MAX);
    } else if (strcmp(TYPE_NAME, "char") == 0) {
      src[i] = (TYPE) (drand48() * CHAR_MAX);;
    } else
      src[i] = drand48();
  }

  printf("Done!\n\n");

  printTYPE(src, args.iterations, "SRC");

  if (args.test_id == 0) {
    for (int i = 0; i < nTestFunction; i++) {
      long start = wall_clock_time();

      testFunction[i](src, args.iterations, TYPE_SIZE);

      long end = wall_clock_time();

      printf("%s:\t%8ld\tmicroseconds\n", testNames[i], end - start);

      if (DEBUG_MODE)
        printf("\n\n");
    }
  } else {
    long start = wall_clock_time();

    testFunction[args.test_id - 1](src, args.iterations, TYPE_SIZE);

    long end = wall_clock_time();

    printf("%s:\t%8ld\tmicroseconds\n", testNames[args.test_id - 1], end - start);

    if (DEBUG_MODE)
      printf("\n\n");
  }

  return 0;
}
