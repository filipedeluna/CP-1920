#ifndef __PATTERNS_H
#define __PATTERNS_H

void map(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    void (*worker)(void *v1, const void *v2) // [ v1 = op (v2) ]
);

void reduce(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    void (*worker)(void *v1, const void *v2, const void *v3) // [ v1 = op (v2, v3) ]
);

void scan(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    void (*worker)(void *v1, const void *v2, const void *v3) // [ v1 = op (v2, v3) ]
);

void exclusiveScan(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    void (*worker)(void *v1, const void *v2, const void *v3) // [ v1 = op (v2, v3) ]
);

int pack(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    const int *filter     // Filer for pack
);

void gather(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    const int *filter,    // Filter for gather
    int nFilter           // # elements in the filter
);

void scatter(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    const int *filter     // Filter for scatter
);

void priorityScatter(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    const int *filter     // Filter for scatter
);

void itemBoundPipeline(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    void (*workerList[])(void *v1, const void *v2), // one function for each stage of the pipeline
    size_t nWorkers       // # stages in the pipeline
);

void pipeline(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    void (*workerList[])(void *v1, const void *v2), // one function for each stage of the pipeline
    size_t nWorkers       // # stages in the pipeline
);

void serialPipeline(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    void (*workerList[])(void *v1, const void *v2), // one function for each stage of the pipeline
    size_t nWorkers       // # stages in the pipeline
);

void farm(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    void (*worker)(void *v1, const void *v2),  // [ v1 = op (22) ]
    size_t nWorkers       // # workers in the farm
);

void stencil(
    void *dest,        // Target array
    void *src,         // Source array
    size_t nJob,       // # elements in the source array
    size_t sizeJob,       // # elements in the source array
    void (*worker)(void *v1, const void *v2),
    int nShift // stencil shift
);

void parallelPrefix(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    void (*worker)(void *v1, const void *v2, const void *v3) // [ v1 = op (v2, v3) ]
);

void hyperplane(
    void *dest,           // Target array
    void *src,            // Source array
    size_t nJob,          // # elements in the source array
    size_t sizeJob,       // Size of each element in the source array
    void (*worker)(void *v1, const void *v2, const void *v3) // [ v1 = op (v2, v3) ]
);

void quickSort(
    int *arr, // Source int array
    size_t arrSize // # elements in the source int array
);


#endif
