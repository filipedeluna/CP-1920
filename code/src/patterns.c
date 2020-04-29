#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <math.h>
#include <omp.h>
#include "patterns.h"
#include "args.h"

/*
 *  UTILS
*/
size_t min(size_t a, size_t b) {
  if ((long) a < 0)
    a = 0;

  if (a < b)
    return a;
  return b;
}

size_t max(size_t a, size_t b) {
  if ((long) a < 0)
    a = 0;

  if (a > b)
    return a;
  return b;
}

size_t getTileIndex(int tile, int leftOverTiles, size_t tileSize) {
  if (tile == 0)
    return 0;

  return tile < leftOverTiles
         ? tile * (tileSize + 1)
         : leftOverTiles * (tileSize + 1) + (tile - leftOverTiles) * tileSize;
}

static void workerAddForPack(void *a, const void *b, const void *c) {
  // a = b + c
  *(TYPE *) a = *(TYPE *) b + *(TYPE *) c;
}

void basicAsserts(void *dest, void *src, void (*worker)(void *v1, const void *v2)) {
  assert (dest != NULL);
  assert (src != NULL);
  assert (worker != NULL);
}

void basicAsserts2(void *dest, void *src, void (*worker)(void *v1, const void *v2, const void *v3)) {
  assert (dest != NULL);
  assert (src != NULL);
  assert (worker != NULL);
}

void filteredAsserts(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
  assert (dest != NULL);
  assert (src != NULL);
  assert (filter != NULL);
  assert ((int) nJob >= 0);
  assert (sizeJob > 0);
}

void pipelineAsserts(void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers) {
  assert (dest != NULL);
  assert (src != NULL);
  assert (workerList != NULL);
  assert ((int) nJob >= 0);
  assert (sizeJob > 0);
  for (size_t i = 0; i < nWorkers; i++)
    assert (workerList[i] != NULL);
}

struct treeNode {
    TYPE sum;
    TYPE fromLeft;
} treeNode;

/*
 * TEMP TEST
*/
void printTree(struct treeNode *tree, size_t nJob) {
  int h = 1;
  for (size_t i = 0; i < nJob; i++) {
    int currentHeight = (int) log2(i + 1) + 1;

    if (currentHeight != h) {
      printf("\n");
      h++;
    }

    printf("%.0lf ", tree[i].sum);
  }

  printf("\n\n");
}

/*
 *  Parallel Patterns
*/

// Implementation of map
void mapImpl(void *dest, void *src, size_t nJob, void (*worker)(void *v1, const void *v2), int nThreads) {
  basicAsserts(dest, src, worker);
  assert (nThreads >= 1);

  TYPE *d = dest;
  TYPE *s = src;

  #pragma omp parallel default(none) if(nThreads > 1) \
  shared(worker, nJob, d, s) num_threads(nThreads)
  #pragma omp for schedule(static)
  for (size_t i = 0; i < nJob; i++)
    worker(&d[i], &s[i]);
}

// Standalone map for tests
void map(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2)) {
  // Avoid unused parameter error for useless parameter
  (void) sizeJob;

  mapImpl(dest, src, nJob, worker, omp_get_max_threads());
}

// Implementation of reduce
void
reduceImpl(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3), int nThreads) {
  basicAsserts2(dest, src, worker);
  assert (nThreads >= 1);

  /*
   * Implementation based on Structured Parallel Programming by Michael McCool et al.
   * The two phase implementation of reduce can be found on chapter 5
   */

  // Zero destination variable
  memset(dest, 0, sizeJob);

  // If no jobs, return 0
  if (nJob == 0)
    return;

  TYPE *result = calloc(1, sizeJob);

  TYPE *s = src;

  // Set size of tiles in relation to number of threads
  // set how many left over jobs, making a few threads work an extra job
  size_t tileSize = nJob / nThreads;
  int leftOverJobs = (int) (nJob % nThreads);
  int nTiles = min(nJob, nThreads);

  // Allocate space to hold the reduction of phase 1
  // Set first position as the first value of the src array
  TYPE *phase1reduction = calloc(nTiles, sizeJob);

  #pragma omp parallel default(none) num_threads(nTiles) if(nTiles > 1) \
    shared(leftOverJobs, phase1reduction, worker, tileSize, nTiles, result, s, sizeJob)
  #pragma omp for schedule(static)
  for (int tile = 0; tile < nTiles; tile++) {
    // Calculate if this tile needs to do extra job
    // use tile size to create tile reduction array
    size_t tileSizeWithOffset = tileSize + (tile < leftOverJobs ? 1 : 0);

    // Get tile index
    size_t tileIndex = getTileIndex(tile, leftOverJobs, tileSize);

    for (size_t i = 0; i < tileSizeWithOffset; i++)
      worker(&phase1reduction[tile], &phase1reduction[tile], &s[i + tileIndex]);
  }

  // Do phase 2 reduction
  for (int tile = 0; tile < nTiles; tile++)
    worker(result, result, &phase1reduction[tile]);

  memcpy(dest, result, sizeJob);

  // Free everything
  free(result);
  free(phase1reduction);
}

// Standalone reduce for tests
void reduce(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
  reduceImpl(dest, src, nJob, sizeJob, worker, omp_get_max_threads());
}

void scan(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
  basicAsserts2(dest, src, worker);

  /*
  * Implementation based on Structured Parallel Programming by Michael McCool et al.
  * The Three-phase tiled implementation of scan can be found on chapter 5.6
  * This is the inclusive scan
  */

  char *d = dest;
  char *s = src;

  if (nJob == 0)
    return;

  memcpy(d, s, sizeJob);

  if (nJob == 1)
    return;

  // Set size of tiles in relation to number of threads
  // set how many left over jobs, making a few threads work an extra job
  size_t tileSize = (nJob - 1) / omp_get_max_threads();
  int leftOverJobs = (int) ((nJob - 1) % omp_get_max_threads());
  int nTiles = min((nJob - 1), omp_get_max_threads());

  // Allocate space to hold the reductions of phase 1 and 2
  // Set first position for both as the first value of the src array
  char *phase1reduction = calloc(nTiles, sizeJob);
  char *phase2reduction = calloc(nTiles, sizeJob);
  memcpy(phase1reduction, s, sizeJob);
  memcpy(phase2reduction, s, sizeJob);


  // Start phase 1 for each tile with one tile per processor
  // If there are less jobs than processors, only start the necessary tiles
  #pragma omp parallel default(none) num_threads(nTiles) if(nTiles > 1) \
    shared(leftOverJobs, worker, tileSize, phase1reduction, nTiles, s, sizeJob)
  #pragma omp for schedule(static)
  for (int tile = 0; tile < nTiles - 1; tile++) {
    // Calculate if this tile needs to do extra job
    // use tile size to create tile reduction array
    size_t tileSizeWithOffset = tileSize + (tile < leftOverJobs ? 1 : 0);

    // Get tile index with + 1 offset
    size_t tileIndex = getTileIndex(tile, leftOverJobs, tileSize) + 1;

    // Reduce tile
    reduceImpl(&phase1reduction[(tile + 1) * sizeJob], &s[tileIndex * sizeJob], tileSizeWithOffset, sizeJob, worker, 1);
  }

  // Do phase 2 reductions
  for (int tile = 1; tile < nTiles; tile++)
    worker(&phase2reduction[tile * sizeJob], &phase2reduction[(tile - 1) * sizeJob], &phase1reduction[tile * sizeJob]);

  free(phase1reduction);

  // Do final phase
  #pragma omp parallel default(none) num_threads(nTiles) if(nTiles > 1) \
    shared(leftOverJobs, worker, tileSize, phase2reduction, nTiles, d, s, sizeJob)
  #pragma omp for schedule(static)
  for (int tile = 0; tile < nTiles; tile++) {
    // Calculate if this tile needs to do extra job
    // use tile size to create tile reduction array
    size_t tileSizeWithOffset = tileSize + (tile < leftOverJobs ? 1 : 0);

    // Get tile index with + 1 offset
    size_t tileIndex = getTileIndex(tile, leftOverJobs, tileSize) + 1;

    // Set value of first value of tile from phase 2
    worker(&d[tileIndex * sizeJob], &phase2reduction[tile * sizeJob], &s[tileIndex * sizeJob]);

    for (size_t i = 1; i < tileSizeWithOffset; i++)
      worker(&d[(i + tileIndex) * sizeJob], &d[(i - 1 + tileIndex) * sizeJob], &s[(i + tileIndex) * sizeJob]);
  }

  free(phase2reduction);
}

void inclusiveScan(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
  scan(dest, src, nJob, sizeJob, worker);
}

void exclusiveScan(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
  scan((TYPE *) dest + 1, src, nJob - 1, sizeJob, worker);
}


int pack(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
  filteredAsserts(dest, src, nJob, sizeJob, filter);

  TYPE *d = dest;
  TYPE *s = src;

  int *bitSumArray = calloc(nJob, sizeof(int));
  exclusiveScan(bitSumArray, (void *) filter, nJob, sizeof(int), workerAddForPack);

  #pragma omp parallel default(none) shared(nJob, d, s, filter, bitSumArray, sizeJob)
  #pragma omp for schedule(static)
  for (int i = 0; i < (int) nJob; i++) {
    if (filter[i]) {
      memcpy(&d[bitSumArray[i]], &s[i], sizeJob);
    }
  }

  return bitSumArray[nJob] + 1;
}

void gatherImpl(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter, int nFilter, int nThreads) {
  filteredAsserts(dest, src, nJob, sizeJob, filter);
  assert (nFilter >= 0);

  TYPE *d = dest;
  TYPE *s = src;

  #pragma omp parallel default(none) if(nThreads > 1) \
  shared(filter, nFilter, d, s, sizeJob, nJob, stderr) num_threads(nThreads)
  #pragma omp for schedule(static)
  for (int i = 0; i < nFilter; i++) {
    // This assertion fails due to a bug - error: ‘__PRETTY_FUNCTION__’ not specified in enclosing ‘parallel’
    // assert (filter[i] < (int) nJob);
    // I replaced it with a closely equivalent solution
    if (filter[i] >= (int) nJob) {
      fprintf(stderr, "Invalid filter index in Gather");
      exit(1);
    }

    memcpy(&d[i], &s[filter[i]], sizeJob);
  }
}

void gather(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter, int nFilter) {
  gatherImpl(dest, src, nJob, sizeJob, filter, nFilter, omp_get_max_threads());
}

void scatter(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
  filteredAsserts(dest, src, nJob, sizeJob, filter);

  char *d = dest;
  char *s = src;

  #pragma omp parallel default(none) shared(filter, nJob, sizeJob, d, s)
  #pragma omp for
  for (int i = 0; i < (int) nJob; i++) {
    // assert (filter[i] < (int) nJob);
    memcpy(&d[filter[i] * sizeJob], &s[i * sizeJob], sizeJob);
  }
}

void mapPipeline(void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers) {
  pipelineAsserts(dest, src, nJob, sizeJob, workerList, nWorkers);

  TYPE *d = dest;
  TYPE *s = src;

  if (nWorkers == 0)
    return;

  int nThreads = omp_get_max_threads();

  // Do first cycle
  mapImpl(d, s, nJob, workerList[0], nThreads);

  // Following cycles
  for (size_t j = 1; j < nWorkers; j++)
    mapImpl(d, d, nJob, workerList[j], nThreads);
}

void itemBoundPipeline(void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers) {
  pipelineAsserts(dest, src, nJob, sizeJob, workerList, nWorkers);

  /*
   * In this version of the algorithm, a worker accompanies
   * one block through all the transformations for better data locality
   * https://ipcc.cs.uoregon.edu/lectures/lecture-10-pipeline.pdf
  */

  TYPE *d = dest;
  TYPE *s = src;

  if (nWorkers == 0)
    return;

  int nThreads = omp_get_max_threads();

  #pragma omp parallel default(none) if(nThreads > 1) \
  shared(workerList, nJob, nWorkers, d, s) num_threads(nThreads)
  #pragma omp for schedule(static)
  for (size_t i = 0; i < nJob; i++) {

    // Do first worker
    workerList[0](&d[i], &s[i]);

    // Do subsequent workers
    for (size_t j = 1; j < nWorkers; j++)
      workerList[j](&d[i], &d[i]);
  }
}

void sequentialPipeline(void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers) {
  pipelineAsserts(dest, src, nJob, sizeJob, workerList, nWorkers);

  /*
   * In this version, the data is processed sequentially.
   * https://ipcc.cs.uoregon.edu/lectures/lecture-10-pipeline.pdf
  */

  TYPE *d = dest;
  TYPE *s = src;

  if (nWorkers == 0)
    return;

  int nThreads = omp_get_max_threads();

  #pragma omp parallel default(none) if(nThreads > 1) \
  shared(workerList, nJob, nWorkers, d, s) num_threads(nThreads)
  for (size_t i = 0; i < nJob - nWorkers; i++) {
    #pragma omp single
    for (size_t j = 0; j <= min(j, nWorkers); j++) {
      #pragma omp task
      mapImpl(d, j % nJob == 0 ? s : d, nJob, workerList[j], nWorkers);
    }
  }
}

void farm(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2), size_t nWorkers) {
  /* To be implemented */
  (void) nWorkers; // TODO delete

  map(dest, src, nJob, sizeJob, worker);  // it provides the right result, but is a very very vey bad implementation…
}

void stencil(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2), int nShift) {
  basicAsserts(dest, src, worker);
  assert(nShift >= 0);
  /*
  * Based on McCool book - Structured Parallel Programming - Chapter 7.1.
  */

  TYPE *d = dest;
  TYPE *s = src;

  int nThreads = omp_get_max_threads();

  #pragma omp parallel default(none) if(nThreads > 1) \
  shared(worker, nJob, d, s, nShift, sizeJob) num_threads(nThreads)
  #pragma omp for schedule(static)
  for (size_t i = 0; i < nJob; i++) {
    TYPE result = 0;

    for (size_t j = max(i - nShift, 0); j <= min(i + nShift, nJob); j++)
      worker(&result, &s[j]);

    memcpy(&d[i], &result, sizeJob);
  }
}

void parallelPrefix(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
  basicAsserts2(dest, src, worker);

  /*
  * Up/Down pass implementation based on the slides and
  * https://www.cs.princeton.edu/courses/archive/fall13/cos326/lec/23-parallel-scan.pdf
  * This is an inclusive scan
  */

  if (nJob == 0)
    return;

  if (nJob == 1) {
    memcpy(dest, src, sizeJob);
    return;
  }

  // Create tree structure
  // Calculate how many levels tree will have and verify if it is odd or not (one less element)
  int treeHeight = (int) log2(nJob) + 1;
  struct treeNode *tree = calloc(nJob, sizeof(treeNode));
  int nThreads = omp_get_max_threads();

  //TYPE *d = dest;
  TYPE *s = src;

  // Begin up pass
  // Travel each level and do computations
  for (int level = treeHeight - 1; level >= 0; level--) {
    // Calculate current and next levels
    size_t firstNode = pow(2, level) - 1;
    size_t lastNode = min(pow(2, level + 1) - 1, nJob);

    #pragma omp parallel default(none) if(nThreads > 1) num_threads(nThreads) \
    shared(worker, nJob, s, sizeJob, tree, treeHeight, level, firstNode, lastNode)
    #pragma omp for schedule(static)
    for (size_t node = firstNode; node < lastNode; node++) {
      // Last level ----------------------------
      if (level == treeHeight - 1) {
        tree[node].sum = s[node];
        continue;
      }

      // Upper levels ----------------------------
      // Check if node has left child
      if (node * 2 + 1 < nJob)
        tree[node].sum = s[node * 2 + 1];

      // Check if node has right child
      if (node * 2 + 2 < nJob) {
        worker(&tree[node], &tree[node * 2 + 1], &tree[node * 2 + 2]);
        continue;
      }

      if (node * 2 + 2 < nJob)
        worker(&tree[node], &tree[node * 2 + 1], &tree[node * 2 + 2]);
      else
        tree[node].sum = s[node * 2 + 1];
    } else
    tree[node].sum = 0;
  }
}


printTree(tree, nJob
);

free(tree);
}
