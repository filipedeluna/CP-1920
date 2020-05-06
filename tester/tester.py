#!/usr/bin/env python3.7
import os
import sys
import time
import datetime
import numpy

now = datetime.datetime.now()

# Constants
NUM_ALGORITHMS = 17
ITERATIONS_HEAVY = [10000, 50000, 100000, 500000, 1000000]
ITERATIONS_MED = [500, 1000, 5000, 10000, 50000]
ITERATIONS_LIGHT = [50, 100, 500, 1000, 5000]

LIGHT_ALGS = [
    11,  # Serial Pipeline
    15  # Hyperplane
]

MED_ALGS = [
    7,  # Scatter
    12,  # Farm
    13,  # Stencil
    14,  # Parallel Prefix
]

WEIGHTED = [
    2,  # Reduce
    12,  # Farm
    11,  # Serial Pipeline
]

THREADS = [1, 2, 4, 8, 16, 32, 64, 128]
REPETITIONS = 5

FILE_NAME = f"paralell_tests {now.day}-{now.month}-{now.year} {now.hour}:{now.minute}:{now.second}.txt"


# Functions -------------------------------------------------------------------------
def file_write(file_buffer, value):
    file_buffer.append(f"{str(value)}\n")


def run_test(alg_id):
    test_name = ""
    results = []

    file_buffer = []

    # Set default iterations
    iterations = ITERATIONS_HEAVY

    # Check if test is heavy, if so, set lighter iterations
    if alg_id in LIGHT_ALGS:
        iterations = ITERATIONS_LIGHT

    if alg_id in MED_ALGS:
        iterations = ITERATIONS_MED

    total_tests = len(iterations) * len(THREADS)

    # Write iterations and total tests to file
    file_write(file_buffer, len(iterations))
    for x in range(0, len(iterations)):
        file_write(file_buffer, iterations[x])

    file_write(file_buffer, len(THREADS))
    for x in range(0, len(THREADS)):
        file_write(file_buffer, THREADS[x])

    test_start_time = time.time()

    for i in range(0, len(iterations)):
        it_results = []
        for t in range(0, len(THREADS)):
            thread_results = []
            for r in range(0, REPETITIONS):
                # Run and extract result from program
                command = f"{program} -i {iterations[i]} -k {alg_id} -t {THREADS[t]}"

                if WEIGHTED.count(alg_id) != 0:
                    command += " -w"

                stream = os.popen(command)
                output = stream.read().split("Done!\n\n")

                # Extract time from program output
                running_time = output[1].split(":\t")[1].split(" micro")[0]
                thread_results.append(int(running_time))

                # Extract test name from program output and write to file
                if test_name == "":
                    test_name = output[1].split(":\t")[0][6:]
                    if WEIGHTED.count(alg_id) != 0:
                        test_name += " (Weighted)"

                    file_write(file_buffer, test_name)

            # Calculate mean of repetitions and write to file
            file_write(file_buffer, numpy.mean(thread_results))

            # Print progress
            curr_test = i * (len(THREADS)) + t + 1

            print(f"Finished test {curr_test}/{total_tests} of algorithm {alg_id}/{NUM_ALGORITHMS} - {test_name}.")

        # Append mean time to execute for each thread count
        results.append(it_results)

    # Write results to file
    output_file.writelines(file_buffer)
    print(f"Time to complete \"{test_name}\"': {time.time() - test_start_time} seconds.")


# Handle args -------------------------------------------------------------------------
if len(sys.argv) != 4:
    print("Invalid arguments - PROGRAM_LOCATION TEST_NUMBER (or 0 for all) and OUTPUT_FOLDER.")
    sys.exit(-1)

if not os.path.isfile(str(sys.argv[1])):
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
file_dir = str(sys.argv[3])

try:
    if not os.path.isdir(file_dir):
        os.mkdir(file_dir)

    output_file = open(f"{file_dir}/{FILE_NAME}", 'x')
except OSError as err:
    print(f"Invalid output path - {err}.")
    sys.exit(-1)

# Start tests -------------------------------------------------------------------------
start_time = time.time()

# Run all algorithms or single
if algorithm_id == 0:
    output_file.write(f"{NUM_ALGORITHMS}\n")

    for alg in range(1, NUM_ALGORITHMS + 1):
        run_test(alg)
else:
    output_file.write("1\n")

    run_test(algorithm_id)

output_file.close()

totalTime = round(time.time() - start_time)
print(f"Tests completed successfully in {totalTime} seconds.")
