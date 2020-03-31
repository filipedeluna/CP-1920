// Argument structure
typedef struct argp_args {
    int debug_mode;
    int iterations;
    int num_threads;
    size_t count;
} argp_args;

struct argp_option argp_options[4];

int argp_option_parser(int key, char *arg, struct argp_state *state);
