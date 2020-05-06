#!/usr/bin/env python3.7
import os
import sys

import matplotlib.pyplot as pyplot


def create_graph(image_dir):
    # Read iterations length and number of iterations
    iterations_length = int(input_file.readline())
    iterations = []
    for x in range(0, iterations_length):
        iterations.append(int(input_file.readline()))

    # Read threads length and number of iterations
    threads_length = int(input_file.readline())
    threads = []
    for y in range(0, threads_length):
        threads.append(int(input_file.readline()))

    test_name = input_file.readline()

    # Compute results into array
    results = []

    for it in range(0, iterations_length):
        temp = []
        for th in range(0, threads_length):
            temp.append(float(input_file.readline()))
        results.append(temp)

    # Data for plotting
    fig = pyplot.figure()
    ax = fig.add_subplot(111)

    for z in range(0, len(iterations)):
        ax.plot(threads, results[z], label=f"{iterations[z]} Iterations")

    ax.set_xscale("log", basex=2)
    ax.set_xticks(threads)
    ax.set_xticklabels(threads)
    ax.set_yscale("log")

    ax.set(xlabel='Number of Threads', ylabel='Time (microseconds)',
           title=f"{test_name} Algorithm Test")
    ax.grid()

    ax.legend(loc='upper right', fancybox=True, shadow=True, prop={'size': 6})

    # Crate directory if it does not exist and save graph file
    if not os.path.isdir(image_dir):
        os.mkdir(image_dir)

    fig.savefig(f"{image_dir}/{test_name}-test.png")


# Handle args -------------------------------------------------------------------------
if len(sys.argv) != 3:
    print("Invalid arguments - INPUT_FILE and OUTPUT_FOLDER.")
    sys.exit(-1)

if not os.path.isfile(sys.argv[1]):
    print("Expected valid input file.")
    sys.exit(-1)

if not sys.argv[2].isalpha():
    print("Expected output folder name.")
    sys.exit(-1)

file_location = str(sys.argv[1])
image_directory = str(sys.argv[2])

input_file = ""

try:
    if not os.path.isdir(image_directory):
        os.mkdir(image_directory)

    input_file = open(file_location, 'r')
except OSError as err:
    print(f"Invalid output path - {err}.")
    sys.exit(-1)

# Create Graphs -------------------------------------------------------------------------
testCount = int(input_file.readline())

for i in range(0, testCount):
    create_graph(image_directory)

input_file.close()

print(f"Graphs created successfully..")
