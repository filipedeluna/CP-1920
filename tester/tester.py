#!/usr/bin/env python3.7
import os
import sys
import time
import datetime

import matplotlib.pyplot as pyplot
import numpy

now = datetime.datetime.now()

# Constants
FILE_NAME = f"paralell_tests {now.day}-{now.month}-{now.year} {now.hour}-{now.minute}-{now.second}"
NUM_ALGORITHMS = 15
ITERATIONS_HEAVY = [100, 1000, 10000, 50000, 100000]
ITERATIONS_LIGHT = [50, 100, 500, 1000, 5000]
HEAVY_ALGS = [
    11,  # Serial Pipeline
    15  # Hyperplane
]

THREADS = [1, 2, 4, 8, 16, 32, 64, 128]
REPETITIONS = 5


# Functions -------------------------------------------------------------------------
def run_test(alg_id, single_test_run, output_file, program):
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
                stream = os.popen(f"{program} -i {iterations[i]} -w -k {alg_id} -t {THREADS[t]}")
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

    # Write results to file
    create_graph(results, test_name, iterations)


# Handle args -------------------------------------------------------------------------
if len(sys.argv) != 4:
    print("Invalid arguments - PROGRAM_LOCATION TEST_NUMBER (or 0 for all) and OUTPUT_FOLDER.")
    sys.exit(-1)

if not sys.argv[1].isalpha() and not os.path.isfile(str(sys.argv[1])):
    print("Expected valid program.")
    sys.exit(-1)

program = str(sys.argv[1])

if not sys.argv[2].isdecimal():
    print("Expected test number - must be integer.")
    sys.exit(-1)

algorithm_id = int(sys.argv[2])

if algorithm_id < 0 or algorithm_id > NUM_ALGORITHMS:
    print(f"Invalid test number. Please choose from 1 - {NUM_ALGORITHMS} or 0 for all.")
    sys.exit(-1)

if not sys.argv[3].isalpha():
    print("Expected directory name.")
    sys.exit(-1)

output_file = ""

try:
    output_file = open(f"{str(sys.argv[2])}/{FILE_NAME}", 'w')
except:
    print("Invalid path.")
    sys.exit(-1)

# Start tests -------------------------------------------------------------------------
start_time = time.time()

# Run all algorithms or single
if algorithm_id == 0:
    for alg in range(1, NUM_ALGORITHMS + 1):
        run_test(alg, False, output_file, program)
else:
    run_test(algorithm_id, True, output_file, program)

output_file.close()

totalTime = round(time.time() - start_time)
print(f"Tests completed successfully in {totalTime} seconds.")
