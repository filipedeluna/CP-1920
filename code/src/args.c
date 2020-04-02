#include <stdlib.h>
#include "args.h"
#include "unit.h"

// Util
int getInt(struct argp_state *state, char *arg, char *arg_name);

// Parser
int argp_option_parser(int key, char *arg, struct argp_state *state) {
  argp_args *args = state->input;

  switch (key) {
    case 'd':
      args->debug_mode = 1;
      break;
    case 'k':
      args->test_id = getInt(state, arg, "test id");

      if (args->test_id < 1 || args->test_id > nTestFunction)
        argp_failure(state, 1, 0, "invalid test id. pick from 1 to %d", nTestFunction);
      break;
    case 'i':
      args->iterations = getInt(state, arg, "number of iterations");
      break;
    case 't':
      args->num_threads = getInt(state, arg, "number of threads");
      break;
    case ARGP_KEY_INIT:
      args->debug_mode = 0;
      args->test_id = 0;
      args->iterations = 0;
      args->num_threads = 1;
      break;
    case ARGP_KEY_END:
      if (args->iterations < 1)
        argp_failure(state, 1, 0, "invalid number of iterations");

      if (args->num_threads < 1 || args->num_threads > 16)
        argp_failure(state, 1, 0, "invalid number of threads");
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

struct argp_option argp_options[] = {
    {
        "debug",
        'd',
        0,
        0,
        "Use this flag to enable debug mode. Optional ID of test to run.",
        0
    },
    {
        "iterations",
        'i',
        "NUM_ITERATIONS",
        0,
        "Number of iterations to run. Must be a positive integer",
        0
    },
    {
        "threads",
        't',
        "NUM_THREADS",
        0,
        "Number of threads to use. Must be a positive integer",
        0
    },
    {
        "test_id",
        'k',
        "TEST_ID",
        0,
        "ID of test to run.",
        0
    },
    {   0}
};

// Util
int getInt(struct argp_state *state, char *arg, char *arg_name) {
  char *ptr = malloc(sizeof(char));
  int val = (int) strtol(arg, &ptr, 10);

  if (*ptr != 0)
    argp_failure(state, 1, 0, "invalid format for %s", arg_name);

  return val;
}
