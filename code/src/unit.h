#ifndef __UNIT_H
#define __UNIT_H

extern int WEIGHTED_MODE;
extern int ITERATIONS;

typedef void (*TESTFUNCTION)(void *, size_t, size_t);

extern TESTFUNCTION testFunction[];

extern char *testNames[];

extern int nTestFunction;

#endif
