import os
import sys
import array

import numpy
import matplotlib
import matplotlib.pyplot as pyplot

# Constants
HYPERPLANE_ID = 15

TESTS = 14
ITERATIONS = [100, 1000, 10000, 50000, 100000, 500000]
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

# Set lighter iterations for expensive memory tests
if testID == HYPERPLANE_ID:
    ITERATIONS = [100, 1000, 10000, 50000, 100000, 500000]

results = []
totalTests = len(ITERATIONS) * len(THREADS)
testName = ""

for i in range(0, len(ITERATIONS)):
    itResults = []
    for t in range(0, len(THREADS)):
        threadResults = []
        for r in range(0, REPETITIONS):
            stream = os.popen(f"{PROGRAM} -i {ITERATIONS[i]} -w -k {testID} -t {THREADS[t]}")
            output = stream.read().split("Done!\n\n")
            time = output[1].split(":\t")[1].split(" micro")[0]

            threadResults.append(int(time))
            testName = output[1].split(":\t")[0]

        currTest = t * (len(ITERATIONS)) + i + 1
        print(f"Finished test {currTest}/{totalTests}")

        itResults.append(numpy.mean(threadResults))

    results.append(itResults)

print(results)

# Data for plotting
fig = pyplot.figure()
ax = fig.add_subplot(111)

for i in range(0, len(ITERATIONS)):
    ax.plot(THREADS, results[i], label=f"{ITERATIONS[i]} Iterations")

ax.set_xscale("log", basex=2)
ax.set_xticks(THREADS)
ax.set_xticklabels(THREADS)
ax.set_yscale("log")

ax.set(xlabel='Number of Threads', ylabel='Time (microseconds)',
       title=f"{testName}")
ax.grid()

ax.legend(loc='upper right', fancybox=True, shadow=True, prop={'size': 6})
fig.savefig(f"{testName}.png")
