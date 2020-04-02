#include <stdio.h>
#include <unistd.h>
#include "args.h"

int DEBUG_MODE = 0;

void printDouble(const double *src, size_t n, const char *msg) {
  if (DEBUG_MODE) {
    printf("%s %s: ", msg, "double");

    for (int i = 0; i < (int) n; i++)
      printf("[%d]=%lf ", i, src[i]);

    printf("\n-------------------------------\n");
  }
}

void printInt(const int *src, size_t n, const char *msg) {
  if (DEBUG_MODE) {
    printf("%s int: ", msg);

    for (int i = 0; i < (int) n; i++)
      printf("[%d]=%d ", i, src[i]);

    printf("\n-------------------------------\n");
  }
}

void printTYPE(const TYPE *src, size_t n, const char *msg) {
  if (DEBUG_MODE) {
    printf("%s %s: ", msg, TYPE_NAME);

    for (int i = 0; i < (int) n; i++) {
      printf("[%d] = ", i);
      printf(TYPE_FORMAT, src[i]);
      printf("\t");
    }

    printf("\n-------------------------------\n");
  }
}
