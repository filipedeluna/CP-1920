#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <omp.h>
#include "patterns.h"
#include "args.h"

/*
 *  UTILS
 */
size_t min(size_t a, size_t b) {
  if (a < b)
    return a;
  else return b;
}

size_t max(size_t a, size_t b) {
  if (a > b)
    return a;
  else return b;
}


void map(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2)) {
  assert (dest != NULL);
  assert (src != NULL);
  assert (worker != NULL);

  char *d = dest;
  char *s = src;

  #pragma omp parallel default(none) shared(worker, nJob, sizeJob, d, s)
  #pragma omp for
  for (int i = 0; i < (int) nJob; i++)
    worker(&d[i * sizeJob], &s[i * sizeJob]);
}

void reduce(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
  assert (dest != NULL);
  assert (src != NULL);
  assert (worker != NULL);

  TYPE result = 0;
  char *s = src;

  if (nJob > 0) {
    result = *((TYPE *) src);

    #pragma omp parallel default(none) shared(worker, nJob, sizeJob, result, s)
    #pragma omp for reduction(+:result)
    for (int i = 1; i < (int) nJob; i++) {
      worker(&result, &result, &s[i * sizeJob]);
    }
  }

  *((TYPE *) dest) = result;
}

// inclusive scan
void scan(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
  assert (dest != NULL);
  assert (src != NULL);
  assert (worker != NULL);
  /*
  * Implementation based on Structured Parallel Programming by Michael McCool et al.
  * The Three-phase tiled implementation of scan can be found on chapter 5.6
  * This is the inclusive scan
  */

  char *d = dest;
  char *s = src;

  for (int i = 1; i < (int) nJob; i++)
    worker(&d[i * sizeJob], &d[(i - 1) * sizeJob], &s[i * sizeJob]);

  if (nJob == 0)
    return;

  memcpy(&d[0], &s[0], sizeJob);

  if (nJob == 1)
    return;

  // Set size of tiles in relation to number of threads
  size_t tileSize = (nJob - 1) / omp_get_num_threads();
  int leftOverJobs = (nJob - 1) % omp_get_num_threads();
  int nTiles = min((nJob - 1), omp_get_num_threads());

  // Allocate space to hold the reductions of phase 1 and 2
  TYPE *phase1reduction = calloc(nTiles - 1, sizeJob);
  TYPE *phase2reduction = calloc(nTiles - 1, sizeJob);

  // Start phase 1 for each tile with one tile per processor
  // If there are less jobs than processors, only start the necessary tiles
  #pragma omp parallel default(none) shared(leftOverJobs, worker) num_threads(nTiles)
  #pragma omp for schedule(static)
  for (int tile = 0; tile < nTiles; tile++) {
    // Calculate if this tile needs to do extra job
    // use tile size to create tile reduction array
    int tileSizeWithOffset = tileSize + (tile < leftOverJobs ? 1 : 0);

    // Allocate space to reduce tile
    TYPE *reduction = calloc(tileSizeWithOffset, sizeJob);

    // Do jobs for this tile. If there are jobs leftover, the first few tiles
    // will have to do one additional job
    for (long i = 0; i < tileSizeWithOffset; i++)
      worker(&reduction[i], &reduction[(i - 1) * tile], &s[i * tile * sizeJob]);

    phase1reduction[tile] = reduction[tileSizeWithOffset - 1];
    free(reduction);
  }

  // Do phase 2 reduction
  phase2reduction[0] = s[0];

  for (int tile = 1; tile < nTiles; tile++) {
    worker(&phase2reduction[tile], &phase2reduction[tile - 1], &phase1reduction[tile]);
  }

  free(phase1reduction);

  // Do final phase
  for (int tile = 0; tile < nTiles; tile++) {

  }

  free(phase2reduction);
}

int pack(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
  /* To be implemented */
  assert (dest != NULL);
  assert (src != NULL);
  assert (filter != NULL);
  assert ((int) nJob >= 0);
  assert (sizeJob > 0);

  char *d = dest;
  char *s = src;

  int pos = 0;
  for (int i = 0; i < (int) nJob; i++) {
    if (filter[i]) {
      memcpy(&d[pos * sizeJob], &s[i * sizeJob], sizeJob);
      pos++;
    }
  }

  return pos;
}

void gather(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter, int nFilter) {
  /* To be implemented */
  assert (dest != NULL);
  assert (src != NULL);
  assert (filter != NULL);
  assert ((int) nJob >= 0);
  assert (sizeJob > 0);
  assert (nFilter >= 0);

  char *d = dest;
  char *s = src;

  for (int i = 0; i < nFilter; i++) {
    assert (filter[i] < (int) nJob);
    memcpy(&d[i * sizeJob], &s[filter[i] * sizeJob], sizeJob);
  }
}

void scatter(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
  assert (dest != NULL);
  assert (src != NULL);
  assert (filter != NULL);
  assert ((int) nJob >= 0);
  assert (sizeJob > 0);

  char *d = dest;
  char *s = src;

  #pragma omp parallel default(none) shared(filter, nJob, sizeJob, d, s)
  #pragma omp for
  for (int i = 0; i < (int) nJob; i++) {
    // assert (filter[i] < (int) nJob);
    memcpy(&d[filter[i] * sizeJob], &s[i * sizeJob], sizeJob);
  }
}

void pipeline(void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers) {
  /* To be implemented */
  assert (dest != NULL);
  assert (src != NULL);
  assert (workerList != NULL);
  assert ((int) nJob >= 0);
  assert (sizeJob > 0);

  char *d = dest;
  char *s = src;

  for (int i = 0; i < (int) nJob; i++) {
    memcpy(&d[i * sizeJob], &s[i * sizeJob], sizeJob);

    for (int j = 0; j < (int) nWorkers; j++) {
      assert (workerList[j] != NULL);
      workerList[j](&d[i * sizeJob], &d[i * sizeJob]);
    }
  }
}

void farm(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2), size_t nWorkers) {
  /* To be implemented */
  (void) nWorkers; // TODO delete

  map(dest, src, nJob, sizeJob, worker);  // it provides the right result, but is a very very vey bad implementation…
}

