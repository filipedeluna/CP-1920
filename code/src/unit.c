#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "patterns.h"

#include "debug.h"
#include "args.h"

#define FMT "%lf"

int WEIGHTED_MODE = 0;
int ITERATIONS = 0;

// Add a bit of weight to worker functions
void addWeight() {
  for (int i = 0; i < 1000; i++)
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

  if (WEIGHTED_MODE)
    addWeight();
}

/*
static void workerSubtract(void* a, const void* b, const void* c) {
    // a = n - c
    *(TYPE *)a = *(TYPE *)b - *(TYPE *)c;

    if (WEIGHTED_MODE)
      addWeight();
}
*/

/*
static void workerMultiply(void* a, const void* b, const void* c) {
    // a = b * c
    *(TYPE *)a = *(TYPE *)b + *(TYPE *)c;

    if (WEIGHTED_MODE)
      addWeight();
}
*/

static void workerAddOne(void *a, const void *b) {
  // a = b + 1
  *(TYPE *) a = *(TYPE *) b + 1;

  if (WEIGHTED_MODE)
    addWeight();
}

static void workerAccum(void *a, const void *b) {
  // a += b
  *(TYPE *) a += *(TYPE *) b;

  if (WEIGHTED_MODE)
    addWeight();
}

static void workerMultTwo(void *a, const void *b) {
  // a = b * 2
  *(TYPE *) a = *(TYPE *) b * 2;

  if (WEIGHTED_MODE)
    addWeight();
}

static void workerDivTwo(void *a, const void *b) {
  // a = b / 2
  *(TYPE *) a = *(TYPE *) b / 2;

  if (WEIGHTED_MODE)
    addWeight();
}

//=======================================================
// Unit testing funtions
//=======================================================

void testMap(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  map(dest, src, n, size, workerAddOne);

  printTYPE(dest, n, __func__);

  free(dest);
}

void testReduce(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(size);

  reduce(dest, src, n, size, workerAdd);

  printTYPE(dest, 1, __func__);

  free(dest);
}

void testInclusiveScan(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  inclusiveScan(dest, src, n, size, workerAdd);

  printTYPE(dest, n, __func__);

  free(dest);
}

void testExclusiveScan(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  exclusiveScan(dest, src, n, size, workerAdd);

  printTYPE(dest, n, __func__);

  free(dest);
}

void testPack(void *src, size_t n, size_t size) {
  int *filter = calloc(n, sizeof(*filter));

  int count = 0;
  for (int i = 0; i < (int) n; i++){
    filter[i] = i % 2;
    count += i % 2;
  }

  TYPE *dest = calloc(count, size);

  int newN = pack(dest, src, n, size, filter);

  printInt(filter, n, "filter");

  printTYPE(dest, newN, __func__);

  free(filter);
  free(dest);
}

void testGather(void *src, size_t n, size_t size) {
  int nFilter = ITERATIONS / 2;
  int *filter = calloc(nFilter, sizeof(int));

  TYPE *dest = malloc(nFilter * size);

  for (long i = 0; i < nFilter; i++)
    filter[i] = rand() % n;

  printInt(filter, nFilter, "filter");

  gather(dest, src, n, size, filter, nFilter);

  printTYPE(dest, nFilter, __func__);

  free(dest);
  free(filter);
}

void testScatter(void *src, size_t n, size_t size) {
  int nDest = 6;

  TYPE *dest = malloc(nDest * size);

  memset(dest, 0, nDest * size);

  int *filter = calloc(n, sizeof(*filter));

  for (int i = 0; i < (int) n; i++)
    filter[i] = rand() % nDest;

  printInt(filter, n, "filter");

  scatter(dest, src, n, size, filter);

  printTYPE(dest, nDest, __func__);

  free(filter);
  free(dest);
}

void testPriorityScatter(void *src, size_t n, size_t size) {
  int nDest = 6;

  TYPE *dest = malloc(nDest * size);

  memset(dest, 0, nDest * size);

  int *filter = calloc(n, sizeof(*filter));

  for (int i = 0; i < (int) n; i++)
    filter[i] = rand() % nDest;

  printInt(filter, n, "filter");

  priorityScatter(dest, src, n, size, filter);

  printTYPE(dest, nDest, __func__);

  free(filter);
  free(dest);
}

void testMapPipeline(void *src, size_t n, size_t size) {
  void (*pipelineFunction[])(void *, const void *) = {
      workerMultTwo,
      workerAddOne,
      workerDivTwo
  };

  int nPipelineFunction = sizeof(pipelineFunction) / sizeof(pipelineFunction[0]);

  TYPE *dest = malloc(n * size);

  mapPipeline(dest, src, n, size, pipelineFunction, nPipelineFunction);

  printTYPE(dest, n, __func__);

  free(dest);
}

void testItemBoundPipeline(void *src, size_t n, size_t size) {
  void (*pipelineFunction[])(void *, const void *) = {
      workerMultTwo,
      workerAddOne,
      workerDivTwo
  };

  int nPipelineFunction = sizeof(pipelineFunction) / sizeof(pipelineFunction[0]);

  TYPE *dest = malloc(n * size);

  itemBoundPipeline(dest, src, n, size, pipelineFunction, nPipelineFunction);

  printTYPE(dest, n, __func__);

  free(dest);
}

void testSequentialPipeline(void *src, size_t n, size_t size) {
  void (*pipelineFunction[])(void *, const void *) = {
      workerMultTwo,
      workerAddOne,
      workerDivTwo
  };

  int nPipelineFunction = sizeof(pipelineFunction) / sizeof(pipelineFunction[0]);

  TYPE *dest = malloc(n * size);

  sequentialPipeline(dest, src, n, size, pipelineFunction, nPipelineFunction);

  printTYPE(dest, n, __func__);

  free(dest);
}

void testFarm(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  farm(dest, src, n, size, workerAddOne, 3);

  printTYPE(dest, n, __func__);

  free(dest);
}

void testStencil(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  // Need static value for accurate tests, although random is fun
  // srand(time(0));
  // int nShift = (rand() % 5) + 1;

  printf("Stencil shift size: %d\n", 5 /* nShift */);

  stencil(dest, src, n, size, workerAccum, 5);

  printTYPE(dest, n, __func__);

  free(dest);
}

void testParallelPrefix(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  parallelPrefix(dest, src, n, size, workerAdd);

  printTYPE(dest, n, __func__);

  free(dest);
}

void testHyperplane(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  hyperplane(dest, src, n, size, workerAdd);

  printTYPE(dest, n, __func__);

  free(dest);
}

//=======================================================
// List of unit test functions
//=======================================================

typedef void (*TESTFUNCTION)(void *, size_t, size_t);

TESTFUNCTION testFunction[] = {
    testMap,
    testReduce,
    testInclusiveScan,
    testExclusiveScan,
    testPack,
    testGather,
    testScatter,
    testPriorityScatter,
    testItemBoundPipeline,
    testMapPipeline,
    testSequentialPipeline,
    testFarm,
    testStencil,
    testParallelPrefix,
    testHyperplane
};

char *testNames[] = {
    "testMap",
    "testReduce",
    "testInclusiveScan",
    "testExclusiveScan",
    "testPack",
    "testGather",
    "testScatter",
    "testPriorityScatter",
    "testItemBoundPipeline",
    "testMapPipeline",
    "testSequentialPipeline",
    "testFarm",
    "testStencil",
    "testParallelPrefix",
    "testHyperplane"
};

int nTestFunction = sizeof(testFunction) / sizeof(testFunction[0]);
