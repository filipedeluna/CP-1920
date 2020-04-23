#ifndef __DEBUG_H
#define __DEBUG_H

extern int DEBUG_MODE;

void printDouble(const double *src, size_t n, const char *msg);

void printInt(const int *src, size_t n, const char *msg);

void printTYPE(void *src, size_t n, const char *msg);

#endif
