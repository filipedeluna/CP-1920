#include <stdlib.h>
#include <string.h>
#include "patterns.h"

#include "debug.h"
#include "args.h"

#define FMT "%lf"

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
}

/*
static void workerSubtract(void* a, const void* b, const void* c) {
    // a = n - c
    *(TYPE *)a = *(TYPE *)b - *(TYPE *)c;
}
*/

/*
static void workerMultiply(void* a, const void* b, const void* c) {
    // a = b * c
    *(TYPE *)a = *(TYPE *)b + *(TYPE *)c;
}
*/

static void workerAddOne(void *a, const void *b) {
  // a = b + 1
  *(TYPE *) a = *(TYPE *) b + 1;
}

static void workerMultTwo(void *a, const void *b) {
  // a = b * 2
  *(TYPE *) a = *(TYPE *) b * 2;
}

static void workerDivTwo(void *a, const void *b) {
  // a = b / 2
  *(TYPE *) a = *(TYPE *) b / 2;
}

//=======================================================
// Unit testing funtions
//=======================================================

void testMap(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  map(dest, src, n, size, workerAddOne);

  printDouble(dest, n, __FUNCTION__);

  free(dest);
}

void testReduce(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(size);

  reduce(dest, src, n, size, workerAdd);

  printDouble(dest, 1, __FUNCTION__);

  free(dest);
}

void testScan(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  scan(dest, src, n, size, workerAdd);

  printDouble(dest, n, __FUNCTION__);

  free(dest);
}

void testPack(void *src, size_t n, size_t size) {
  int nFilter = 3;

  TYPE *dest = malloc(nFilter * size);
  int *filter = calloc(n, sizeof(*filter));

  for (int i = 0; i < (int) n; i++)
    filter[i] = (i == 0 || i == (int) n / 2 || i == (int) n - 1);

  int newN = pack(dest, src, n, size, filter);

  printInt(filter, n, "filter");

  printDouble(dest, newN, __FUNCTION__);

  free(filter);
  free(dest);
}

void testGather(void *src, size_t n, size_t size) {
  int nFilter = 3;
  int filter[nFilter];

  TYPE *dest = malloc(nFilter * size);

  for (int i = 0; i < nFilter; i++)
    filter[i] = rand() % n;

  printInt(filter, nFilter, "filter");

  gather(dest, src, n, size, filter, nFilter);

  printDouble(dest, nFilter, __FUNCTION__);

  free(dest);
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

  printDouble(dest, nDest, __FUNCTION__);

  free(filter);
  free(dest);
}

void testPipeline(void *src, size_t n, size_t size) {
  void (*pipelineFunction[])(void *, const void *) = {
      workerMultTwo,
      workerAddOne,
      workerDivTwo
  };

  int nPipelineFunction = sizeof(pipelineFunction) / sizeof(pipelineFunction[0]);

  TYPE *dest = malloc(n * size);

  pipeline(dest, src, n, size, pipelineFunction, nPipelineFunction);

  printDouble(dest, n, __FUNCTION__);

  free(dest);
}

void testFarm(void *src, size_t n, size_t size) {
  TYPE *dest = malloc(n * size);

  farm(dest, src, n, size, workerAddOne, 3);

  printDouble(dest, n, __FUNCTION__);

  free(dest);
}

//=======================================================
// List of unit test functions
//=======================================================

typedef void (*TESTFUNCTION)(void *, size_t, size_t);

TESTFUNCTION testFunction[] = {
    testMap,
    testReduce,
    testScan,
    testPack,
    testGather,
    testScatter,
    testPipeline,
    testFarm,
};

char *testNames[] = {
    "testMap",
    "testReduce",
    "testScan",
    "testPack",
    "testGather",
    "testScatter",
    "testPipeline",
    "testFarm",
};

int nTestFunction = sizeof(testFunction) / sizeof(testFunction[0]);


