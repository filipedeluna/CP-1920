#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>

#include "args.h"
#include "unit.h"
#include "debug.h"
#include "omp.h"

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

  if (args.weighted)
    WEIGHTED_MODE = 1;

  ITERATIONS = args.iterations;

  srand48(time(NULL));
  srand48(time(NULL));

  // Setup OpenMP
  omp_set_num_threads(args.num_threads);
  omp_set_nested(1);
  omp_set_dynamic(0);

  // Initialize src array for all iterations
  printf("Initializing SRC array\n");
  TYPE *src = malloc(TYPE_SIZE * args.iterations);

  for (int i = 0; i < args.iterations; i++) {
    if (strcmp(TYPE_NAME, "int") == 0) {
      src[i] = (TYPE) (drand48() * INT_MAX);
    } else if (strcmp(TYPE_NAME, "char") == 0) {
      src[i] = (TYPE) (drand48() * CHAR_MAX);;
    } else
      src[i] = (int) (drand48() * 10);
  }

  printf("Done!\n\n");

  printTYPE(src, args.iterations, "SRC");

  double start_time = testFunction[args.test_id - 1](src, args.iterations, TYPE_SIZE);

  double timeElapsed = (omp_get_wtime() - start_time) * 1e6;

  printf("%s:\t%.0lf microseconds\n", testNames[args.test_id - 1], timeElapsed);

  if (DEBUG_MODE)
    printf("\n\n");

  free(src);

  return 0;
}
