import os
import sys
import array

import numpy
from numpy import mean

# Constants
TESTS = 14
ITERATIONS = [10, 50, 100, 500, 1000, 5000, 10000]
THREADS = [1, 2, 4, 8, 16, 32, 64, 128]
REPETITIONS = 5

PROGRAM = "../code/cmake-build-debug/main"

# Variables for running program
testID = ""
nThreads = 0
nIterations = 0

# Handle args
if len(sys.argv) == 1:
    print("Expected test number")
    sys.exit(-1)

if len(sys.argv) > 2:
    print("Too many arguments - expected test number")
    sys.exit(-1)


if not sys.argv[1].isdecimal():
    print("Expected test number - must be integer")
    sys.exit(-1)

testID = int(sys.argv[1])

if testID < 1 or testID > TESTS + 1:
    print(f"Invalid test number. Please choose from 1 - {TESTS + 1}")
    sys.exit(-1)

results = []
totalTests = len(ITERATIONS) * len(THREADS)

for i in range(0, len(ITERATIONS)):
    iterationResults = []
    for t in range(0, len(THREADS)):
        repetitionResults = []
        for r in range(0, REPETITIONS):
            stream = os.popen(f"{PROGRAM} -i {ITERATIONS[i]} -k {testID} -t {THREADS[t]}")
            output = stream.read().split("Done!\n\n")
            time = output[1].split(":\t")[1].split(" micro")[0]

            repetitionResults.append(int(time))
            testName = output[1].split(":\t")[0]

            iterationResults.append(mean(repetitionResults))

        currTest = i * (len(ITERATIONS) + 1) + t + 1
        print(f"Finished test {currTest}/{totalTests}")

    results.append(iterationResults)

print(results)



