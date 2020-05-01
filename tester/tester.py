import os
import array
import numpy

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

results = []

for test in range(1, TESTS + 1):
    for i in range(0, len(ITERATIONS)):
        for t in range(0, len(THREADS)):
            for r in range(0, REPETITIONS):
                stream = os.popen(f"{PROGRAM} -i {ITERATIONS[i]} -k {test} -t {THREADS[t]}")
                output = stream.read().split("Done!\n\n")

                repResults = []
                repResults.append(output[1].split(":\t")[1].split(" micro")[0])
                print(repResults)

                testName = output[1].split(":\t")[0]

    results[test][i] = 0
    print(testName + " - " + 0)
