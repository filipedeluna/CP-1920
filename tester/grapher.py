#!/usr/bin/env python3.7
import os
import sys
import time

import matplotlib.pyplot as pyplot
import numpy

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
