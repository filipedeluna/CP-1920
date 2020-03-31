#include <string.h>
#include <assert.h>
#include "patterns.h"
#include "args.h"

void map(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2)) {
  /* To be implemented */
  assert (dest != NULL);
  assert (src != NULL);
  assert (worker != NULL);

  for (int i = 0; i < (int) nJob; i++)
    worker(&((TYPE *) dest)[i * sizeJob], &((TYPE *) src)[i * sizeJob]);
}

void reduce(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
  /* To be implemented */
  assert (dest != NULL);
  assert (src != NULL);
  assert (worker != NULL);

  if (nJob > 0) {
    memcpy(&((TYPE *) dest)[0], &((TYPE *) src)[0], sizeJob);
    for (int i = 1; i < (int) nJob; i++)
      worker(&((TYPE *) dest)[0], &((TYPE *) dest)[0], &((TYPE *) src)[i * sizeJob]);
  }

}

void scan(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
  /* To be implemented */
  assert (dest != NULL);
  assert (src != NULL);
  assert (worker != NULL);

  if (nJob > 1) {
    memcpy(&((TYPE *) dest)[0], &((TYPE *) src)[0], sizeJob);
    for (int i = 1; i < (int) nJob; i++)
      worker(&((TYPE *) dest)[i * sizeJob], &((TYPE *) dest)[(i - 1) * sizeJob], &((TYPE *) src)[i * sizeJob]);
  }
}

int pack(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
  /* To be implemented */
  assert (dest != NULL);
  assert (src != NULL);
  assert (filter != NULL);
  assert ((int) nJob >= 0);
  assert (sizeJob > 0);

  int pos = 0;
  for (int i = 0; i < (int) nJob; i++) {
    if (filter[i]) {
      memcpy(&((TYPE *) dest)[pos * sizeJob], &((TYPE *) src)[i * sizeJob], sizeJob);
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

  (void) nJob; // TODO delete

  for (int i = 0; i < nFilter; i++) {
    assert (filter[i] < (int) nJob);
    memcpy(&((TYPE *) dest)[i * sizeJob], &((TYPE *) src)[filter[i] * sizeJob], sizeJob);
  }
}

void scatter(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
  /* To be implemented */
  assert (dest != NULL);
  assert (src != NULL);
  assert (filter != NULL);
  assert ((int) nJob >= 0);
  assert (sizeJob > 0);

  for (int i = 0; i < (int) nJob; i++) {
    assert (filter[i] < (int) nJob);
    memcpy(&((TYPE *) dest)[filter[i] * sizeJob], &((TYPE *) src)[i * sizeJob], sizeJob);
  }
}

void pipeline(void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers) {
  /* To be implemented */
  assert (dest != NULL);
  assert (src != NULL);
  assert (workerList != NULL);
  assert ((int) nJob >= 0);
  assert (sizeJob > 0);

  for (int i = 0; i < (int) nJob; i++) {
    memcpy(&((TYPE *) dest)[i * sizeJob], &((TYPE *) src)[i * sizeJob], sizeJob);

    for (int j = 0; j < (int) nWorkers; j++) {
      assert (workerList[j] != NULL);
      workerList[j](&((TYPE *) dest)[i * sizeJob], &((TYPE *) dest)[i * sizeJob]);
    }
  }
}

void farm(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2), size_t nWorkers) {
  /* To be implemented */
  (void) nWorkers; // TODO delete

  map(dest, src, nJob, sizeJob, worker);  // it provides the right result, but is a very very vey bad implementation…
}
