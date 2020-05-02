#!/usr/bin/env python3.7
import os
import sys
import time

import matplotlib.pyplot as pyplot
import numpy

# Constants
NUM_ALGORITHMS = 15
ITERATIONS_HEAVY = [100, 1000, 10000, 50000, 100000]
ITERATIONS_LIGHT = [50, 100, 500, 1000, 5000]
HEAVY_ALGS = [15]

THREADS = [1, 2, 4, 8, 16, 32, 64, 128]
REPETITIONS = 5

PROGRAM = "../code/cmake-build-debug/main"
IMAGE_DIRECTORY = "graphs"

# Functions -------------------------------------------------------------------------
def run_test(alg_id, single_test_run):
    test_name = ""
    results = []

    # Set default iterations
    iterations = ITERATIONS_HEAVY

    # Check if test is heavy, if so, set lighter iterations
    if alg_id in HEAVY_ALGS:
        iterations = ITERATIONS_LIGHT

    total_tests = len(iterations) * len(THREADS)

    for i in range(0, len(iterations)):
        it_results = []
        for t in range(0, len(THREADS)):
            thread_results = []
            for r in range(0, REPETITIONS):
                # Run and extract result from program
                stream = os.popen(f"{PROGRAM} -i {iterations[i]} -w -k {alg_id} -t {THREADS[t]}")
                output = stream.read().split("Done!\n\n")

                # Extract time from program output
                running_time = output[1].split(":\t")[1].split(" micro")[0]
                thread_results.append(int(running_time))

                # Extract test name from program output
                test_name = output[1].split(":\t")[0][4:]

            # Calculate mean of repetitions
            it_results.append(numpy.mean(thread_results))

            # Print progress
            curr_test = i * (len(THREADS)) + t + 1

            if not single_test_run:
                print(f"Finished test {curr_test}/{total_tests} of algorithm {alg_id}/{NUM_ALGORITHMS} - {test_name}.")
            else:
                print(f"Finished test {curr_test}/{total_tests}.")

        # Append mean time to execute for each thread count
        results.append(it_results)

    # Create graph with results
    create_graph(results, test_name, iterations)


def create_graph(results, test_name, iterations):
    # Data for plotting
    fig = pyplot.figure()
    ax = fig.add_subplot(111)

    for i in range(0, len(iterations)):
        ax.plot(THREADS, results[i], label=f"{iterations[i]} Iterations")

    ax.set_xscale("log", basex=2)
    ax.set_xticks(THREADS)
    ax.set_xticklabels(THREADS)
    ax.set_yscale("log")

    ax.set(xlabel='Number of Threads', ylabel='Time (microseconds)',
           title=f"{test_name} Algorithm Test")
    ax.grid()

    ax.legend(loc='upper right', fancybox=True, shadow=True, prop={'size': 6})

    # Crate directory if it does not exist and save graph file
    if not os.path.isdir(IMAGE_DIRECTORY):
        os.mkdir(IMAGE_DIRECTORY)

    fig.savefig(f"{IMAGE_DIRECTORY}/{test_name}-test.png")

# Handle args -------------------------------------------------------------------------
algorithm_id = -1

if len(sys.argv) > 2:
    print("Too many arguments - expected test number or nothing for all tests")
    sys.exit(-1)

if len(sys.argv) == 2:
    if len(sys.argv) == 2 and not sys.argv[1].isdecimal():
        print("Expected test number - must be integer")
        sys.exit(-1)

    algorithm_id = int(sys.argv[1])

    if algorithm_id < 1 or algorithm_id > NUM_ALGORITHMS:
        print(f"Invalid test number. Please choose from 1 - {NUM_ALGORITHMS}")
        sys.exit(-1)

# Start tests -------------------------------------------------------------------------
start_time = time.time()

# Run all algorithms or single
if algorithm_id == -1:
    for alg in range(1, NUM_ALGORITHMS + 1):
        run_test(alg, False)
else:
    run_test(algorithm_id, True)

totalTime = round(time.time() - start_time)
print(f"Tests completed successfully in {totalTime} seconds.")
