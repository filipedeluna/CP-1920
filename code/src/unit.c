#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "patterns.h"
#include <errno.h>

#include "debug.h"
#include "args.h"

#define FMT "%lf"
#define SLEEP_WEIGHT 250

int WEIGHTED_MODE = 0;
int ITERATIONS = 0;

// This allows workers to simulate work
void addWeight() {
  if (WEIGHTED_MODE)
    for (int i = 0; i < SLEEP_WEIGHT; i++)
      (void) i;
}

//=======================================================
// Workers
//=======================================================

/*
static void workerMax(void* a, const void* b, const void* c) {
    // a = max (b, c)
    *(TYPE *)a = *(TYPE *)b;
    if (*(TYPE *)c > *(TYPE *)a)
        *(TYPE *)a = *(TYPE *)c;
}
*/

/*
static void workerMin(void* a, const void* b, const void* c) {
    // a = min (b, c)
    *(TYPE *)a = *(TYPE *)b;
    if (*(TYPE *)c < *(TYPE *)a)
        *(TYPE *)a = *(TYPE *)c;
}
*/

static void workerAdd(void *a, const void *b, const void *c) {
  // a = b + c
  *(TYPE *) a = *(TYPE *) b + *(TYPE *) c;

  addWeight();
}

/*
static void workerSubtract(void* a, const void* b, const void* c) {
    // a = n - c
    *(TYPE *)a = *(TYPE *)b - *(TYPE *)c;

    addWeight();
}
*/

/*
static void workerMultiply(void* a, const void* b, const void* c) {
    // a = b * c
    *(TYPE *)a = *(TYPE *)b + *(TYPE *)c;

        addWeight();
}
*/

static void workerAddOne(void *a, const void *b) {
  // a = b + 1
  *(TYPE *) a = *(TYPE *) b + 1;

  addWeight();
}

static void workerAccum(void *a, const void *b) {
  // a += b
  *(TYPE *) a += *(TYPE *) b;

  addWeight();
}

static void workerMultTwo(void *a, const void *b) {
  // a = b * 2
  *(TYPE *) a = *(TYPE *) b * 2;

  addWeight();
}

static void workerDivTwo(void *a, const void *b) {
  // a = b / 2
  *(TYPE *) a = *(TYPE *) b / 2;

  addWeight();
}

//=======================================================
// Unit testing funtions
//=======================================================

double testMap(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  double time = omp_get_wtime();

  map(dest, src, n, size, workerAddOne);

  printTYPE(dest, n, __func__);

  free(dest);

  return time;
}

double testReduce(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(size);

  double time = omp_get_wtime();

  reduce(dest, src, n, size, workerAdd);

  printTYPE(dest, 1, __func__);

  free(dest);

  return time;
}

double testInclusiveScan(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  double time = omp_get_wtime();

  scan(dest, src, n, size, workerAdd);

  printTYPE(dest, n, __func__);

  free(dest);

  return time;
}

double testExclusiveScan(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  double time = omp_get_wtime();

  exclusiveScan(dest, src, n, size, workerAdd);

  printTYPE(dest, n, __func__);

  free(dest);

  return time;
}

double testPack(void *src, size_t n, size_t size) {
  int *filter = calloc(n, sizeof(*filter));

  int count = 0;
  for (int i = 0; i < (int) n; i++) {
    filter[i] = i % 2;
    count += i % 2;
  }

  TYPE *dest = calloc(count, size);

  double time = omp_get_wtime();

  int newN = pack(dest, src, n, size, filter);

  printInt(filter, n, "filter");

  printTYPE(dest, newN, __func__);

  free(filter);
  free(dest);

  return time;
}

double testGather(void *src, size_t n, size_t size) {
  int nFilter = ITERATIONS / 2;
  int *filter = calloc(nFilter, sizeof(int));

  TYPE *dest = malloc(nFilter * size);

  for (long i = 0; i < nFilter; i++)
    filter[i] = rand() % n;

  printInt(filter, nFilter, "filter");

  double time = omp_get_wtime();

  gather(dest, src, n, size, filter, nFilter);

  printTYPE(dest, nFilter, __func__);

  free(dest);
  free(filter);

  return time;
}

double testScatter(void *src, size_t n, size_t size) {
  int nDest = 10;

  TYPE *dest = malloc(nDest * size);

  memset(dest, 0, nDest * size);

  int *filter = calloc(n, sizeof(*filter));

  for (int i = 0; i < (int) n; i++)
    filter[i] = rand() % nDest;

  printInt(filter, n, "filter");

  double time = omp_get_wtime();

  scatter(dest, src, n, size, filter);

  printTYPE(dest, nDest, __func__);

  free(filter);
  free(dest);

  return time;
}

double testPriorityScatter(void *src, size_t n, size_t size) {
  int nDest = 6;

  TYPE *dest = malloc(nDest * size);

  memset(dest, 0, nDest * size);

  int *filter = calloc(n, sizeof(*filter));

  for (int i = 0; i < (int) n; i++)
    filter[i] = rand() % nDest;

  printInt(filter, n, "filter");

  double time = omp_get_wtime();

  priorityScatter(dest, src, n, size, filter);

  printTYPE(dest, nDest, __func__);

  free(filter);
  free(dest);

  return time;
}

double testMapPipeline(void *src, size_t n, size_t size) {
  void (*pipelineFunction[])(void *, const void *) = {
      workerMultTwo,
      workerAddOne,
      workerDivTwo
  };

  int nPipelineFunction = sizeof(pipelineFunction) / sizeof(pipelineFunction[0]);

  TYPE *dest = malloc(n * size);

  double time = omp_get_wtime();

  pipeline(dest, src, n, size, pipelineFunction, nPipelineFunction);

  printTYPE(dest, n, __func__);

  free(dest);

  return time;
}

double testItemBoundPipeline(void *src, size_t n, size_t size) {
  void (*pipelineFunction[])(void *, const void *) = {
      workerMultTwo,
      workerAddOne,
      workerDivTwo
  };

  int nPipelineFunction = sizeof(pipelineFunction) / sizeof(pipelineFunction[0]);

  TYPE *dest = malloc(n * size);

  double time = omp_get_wtime();

  itemBoundPipeline(dest, src, n, size, pipelineFunction, nPipelineFunction);

  printTYPE(dest, n, __func__);

  free(dest);

  return time;
}

double testSerialPipeline(void *src, size_t n, size_t size) {
  size_t nWorkers = 128;

  void (**pipelineFunction)(void *, const void *) = calloc(nWorkers, sizeof(pipelineFunction[0]));

  for (size_t i = 0; i < nWorkers; i++)
    pipelineFunction[i] = workerAddOne;

  TYPE *dest = malloc(n * size);

  double time = omp_get_wtime();

  serialPipeline(dest, src, n, size, pipelineFunction, nWorkers);

  printTYPE(dest, n, __func__);

  free(dest);

  return time;
}

double testFarm(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  double time = omp_get_wtime();

  farm(dest, src, n, size, workerAddOne, 3);

  printTYPE(dest, n, __func__);

  free(dest);

  return time;
}

double testStencil(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  // Need static value for accurate tests, although random is fun
  // srand(time(0));
  // int nShift = (rand() % 5) + 1;

  // printf("Stencil shift size: %d\n", 5 /* nShift */);

  double time = omp_get_wtime();

  stencil(dest, src, n, size, workerAccum, 5);

  printTYPE(dest, n, __func__);

  free(dest);

  return time;
}

double testParallelPrefix(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  double time = omp_get_wtime();

  parallelPrefix(dest, src, n, size, workerAdd);

  printTYPE(dest, n, __func__);

  free(dest);

  return time;
}

double testHyperplane(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  double time = omp_get_wtime();

  hyperplane(dest, src, n, size, workerAdd);

  printTYPE(dest, n, __func__);

  free(dest);

  return time;
}

double testQuickSort(void *src, size_t n, size_t size) {
  // Ignore input array and size to make use of existing signature
  (void) src;
  (void) size;

  // Generate random numbers for ordering and fill array
  int *randArr = calloc(n, sizeof(int));

  for (size_t i = 0; i < n; i++)
    randArr[i] = rand() % n;

  printInt(randArr, n, __func__);

  double time = omp_get_wtime();

  quickSort(randArr, n);

  printInt(randArr, n, __func__);

  free(randArr);

  return time;
}

double testQuickSort2(void *src, size_t n, size_t size) {
  // Ignore input array and size to make use of existing signature
  (void) src;
  (void) size;

  // Generate random numbers for ordering and fill array
  int *randArr = calloc(n, sizeof(int));

  for (size_t i = 0; i < n; i++)
    randArr[i] = rand() % n;

  printInt(randArr, n, __func__);

  double time = omp_get_wtime();

  quickSort2(randArr, src, size, n);

  printTYPE(src, n, __func__);

  printInt(randArr, n, __func__);

  free(randArr);

  return time;
}


//=======================================================
// List of unit test functions
//=======================================================

typedef double (*TESTFUNCTION)(void *, size_t, size_t);

TESTFUNCTION testFunction[] = {
    testMap,
    testReduce,
    testInclusiveScan,
    testExclusiveScan,
    testPack,
    testGather,
    testScatter,
    testPriorityScatter,
    testMapPipeline,
    testItemBoundPipeline,
    testSerialPipeline,
    testFarm,
    testStencil,
    testParallelPrefix,
    testHyperplane,
    testQuickSort,
    testQuickSort2
};

char *testNames[] = {
    "test: Map",
    "test: Reduce",
    "test: Inclusive Scan",
    "test: Exclusive Scan",
    "test: Pack",
    "test: Gather",
    "test: Scatter",
    "test: Priority Scatter",
    "test: Map Pipeline",
    "test: Item-Bound Pipeline",
    "test: Serial Pipeline",
    "test: Farm",
    "test: Stencil",
    "test: Parallel Prefix",
    "test: Hyperplane",
    "test: Int Quick Sort",
    "test: Int Double Quick Sort"
};

int nTestFunction = sizeof(testFunction) / sizeof(testFunction[0]);
