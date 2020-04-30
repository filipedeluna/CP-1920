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

    printf("(%.0lf, %.0lf) ", tree[i].sum, tree[i].fromLeft);
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

  // Create tree structure ----------------------
  // Calculate how many elems tree has in order to have nJob elements at base
  size_t nTreeElems = nJob <= 1
                      ? nJob
                      : nJob % 2 == 0
                        ? nJob * 2 - 1
                        : nJob * 2;

  // Calculate how many levels tree will have and verify if it is odd or not (one less element)
  int treeHeight = (int) log2(nTreeElems) + 1;
  struct treeNode *tree = calloc(nTreeElems, sizeof(treeNode));
  int nThreads = omp_get_max_threads();

  TYPE *s = src;
  TYPE *d = dest;

  // Begin up pass
  // Travel each level and do computations
  for (int level = treeHeight; level > 0; level--) {
    // Calculate current and next levels
    size_t firstNode = pow(2, level - 1) - 1;
    size_t lastNode = level == treeHeight
                      ? nTreeElems - 1
                      : pow(2, level) - 2;

    #pragma omp parallel default(none) if(nThreads > 1) num_threads(nThreads) \
    shared(worker, nJob, s, sizeJob, tree, treeHeight, level, firstNode, lastNode, nTreeElems)
    #pragma omp for schedule(static)
    for (size_t node = firstNode; node <= lastNode; node++) {
      // Check if node has left and/or right children - not leaf
      if (node * 2 + 1 < nTreeElems) {
        if (node * 2 + 2 < nTreeElems)
          worker(&tree[node].sum, &tree[node * 2 + 1].sum, &tree[node * 2 + 2].sum);
        else
          memcpy(&tree[node].sum, &tree[node * 2 + 1].sum, sizeJob);
        continue;
      }

      // If node has no children - its a leaf - assign value -------
      // Check if last level. If not - ignore unused node
      if (level == treeHeight)
        memcpy(&tree[node].sum, &s[node - firstNode], sizeJob);
    }
  }

  // Begin down pass
  // Travel each level and do computations
  for (int level = 1; level <= treeHeight; level++) {
    // Calculate current and next levels
    size_t firstNode = pow(2, level - 1) - 1;
    size_t lastNode = level == treeHeight
                      ? nTreeElems - 1
                      : pow(2, level) - 2;

    #pragma omp parallel default(none) if(nThreads > 1) num_threads(nThreads) \
    shared(worker, nJob, d, sizeJob, tree, treeHeight, level, firstNode, lastNode, nTreeElems)
    #pragma omp for schedule(static)
    for (size_t node = firstNode; node <= lastNode; node++) {
      // If first node in level, assign value of 0 from left
      if (node == firstNode) {
        tree[node].fromLeft = 0;

        if (level == 1)
          continue;
      }

      // If its not root, check if node is right or left node
      if (node % 2 == 0)
        worker(&tree[node].fromLeft, &tree[node - 1].fromLeft, &tree[node - 1].sum);
      else
        worker(&tree[node].fromLeft, &tree[node].fromLeft, &tree[(node - 1) / 2].fromLeft);

      // If at last level, assign value to destiny array
      if (level == treeHeight) {
        worker(&d[node - firstNode], &tree[node].fromLeft, &tree[node].sum);
        continue;
      }
    }
  }

  free(tree);
}

// Standalone map for tests
void hyperplane(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
  basicAsserts2(dest, src, worker);
  assert(nJob >= 2);

  /*
   * Based on McCool book - Structured Parallel Programming - Chapter 7.5.
  */

  int nThreads = omp_get_max_threads();

  TYPE *d = dest;
  TYPE *s = src;

  // Calculate height and width
  size_t height = nJob / 2;
  size_t width = nJob / 2 + nJob % 2;

  // Create computation matrix
  TYPE *compMatrix = calloc(pow(width, 2), sizeJob);

  #pragma omp parallel default(none) if(nThreads > 1) num_threads(nThreads) \
    shared(worker, nJob, d, s, sizeJob, width, height, compMatrix)
  for (size_t i = 0; i < width + height - 1; i++) {
    // Calculate number of cycles for this sweep
    size_t nCycles = i < width ? i : width - (i - width);

    // Calculate base node vertical and horizontal position
    // Derive base node position in array
    // The root (0,0) is the top-left corner
    size_t baseV = min(i, height);
    size_t baseH = i < width ? 0 : height;
    size_t basePos = baseH * width + baseH;

    #pragma omp single
    for (size_t j = 0; j < nCycles; j++) {
      // Calculate current node
      size_t currV = baseV - 1;
      size_t currH = baseH - 1;
      size_t currPos = currH * width + currH;

      // Deal with root case
      if (currV == 0 && currH == 0) {
        worker(&compMatrix[0], &s[0], &s[width]);
        continue;
      }

      // Deal with top and left edge-cases
      if (currV == 0) {
        #pragma omp task default(none) shared(s, currH, currPos, worker, compMatrix)
        worker(&compMatrix[currPos], &compMatrix[basePos - 1], &s[currH]);
        continue;
      }

      if (currH == 0) {
        #pragma omp task default(none) shared(s, currV, currPos, worker, compMatrix, width)
        worker(&compMatrix[currPos], &compMatrix[basePos - width], &s[width + baseV - 1]);
        continue;
      }

      // Normal case
      #pragma omp task default(none) shared(j, s, currH, currPos, worker, compMatrix)
      worker(&compMatrix[currPos - (j * width - i)], &compMatrix[basePos - 1], &s[currH]);

      // Deal with bottom and right edge-cases
      if (currV == height - 1) {
        #pragma omp task default(none) shared(d, currH, currPos, sizeJob, compMatrix)
        memcpy(&d[currH], &compMatrix[basePos], sizeJob);
      }

      if (currH == width - 1) {
        #pragma omp task default(none) shared(d, currH, currV, currPos, sizeJob, compMatrix)
        memcpy(&d[currH + baseV + 1], &compMatrix[basePos], sizeJob);
      }

      // If last node
      if (currV == height - 1 && currH == width - 1) {
        memcpy(&d[width - 1], &compMatrix[currPos], sizeJob);
        memcpy(&d[width], &compMatrix[basePos], sizeJob);
      }
    }
  }
}
