#ifndef BCV_UTILS_H_
#define BCV_UTILS_H_
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <vector>
#include <cstring>
#include <string>
#include <set>
#include "assert.h"

using namespace std;

// image element access operations
int inline linear_index(int r, int c, int k, int cols, int chan) {
    return k + (c + r*cols)*chan;
}
int inline linear_index(int r, int c, int cols) {
    return c + r*cols;
}
int inline getrow(int i, int cols) {
    return i / cols;
}
int inline getcol(int i, int cols) {
    return i - (i / cols)*cols;
}
//
vector<int> choose_random_subset(int k, int n);

unsigned long now_us();
double now_ms();


#endif // BCV_UTILS_H_
