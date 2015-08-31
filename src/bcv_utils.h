#ifndef BCV_UTILS_H_
#define BCV_UTILS_H_
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <vector>
#include <cstring>
#include <string>
#include <set>
#include <cassert>

namespace bcv {
using namespace std;

#ifndef BCV_UCHAR_TYPEDEF_
#define BCV_UCHAR_TYPEDEF_
typedef unsigned char uchar;
#endif

typedef pair<int,int> intpair;

//#define BCV_SIGN(x) ( ((x)>0) ? +1 : -1)
#define BCV_SIGN(x) (((x)>0) - ((x)<0))

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
int inline getrow(int i, int cols, int chan) { 
    return (i / chan) / cols;
}
int inline getcol(int i, int cols, int chan) { 
    return (i / chan) - (i / chan / cols)*cols;
}

//
vector<int> choose_random_subset(int k, int n);

template <typename T>
void keep_indices_if(vector<T>& x, const vector<bool>& pred) {
    assert(x.size() == pred.size());
    int j = 0;
    for (int i = 0; i < pred.size(); ++i) { if (pred[i]) { x[j]=x[i]; j++; } }
    x.erase(x.begin()+j, x.end());
}
template <typename T>
void erase_indices_if(vector<T>& x, const vector<bool>& pred) {
    assert(x.size() == pred.size());
    int j = 0;
    for (int i = 0; i < pred.size(); ++i) { if (!pred[i]) { x[j]=x[i]; j++; } }
    x.erase(x.begin()+j, x.end());
}

unsigned long now_us();
double now_ms();

} // namespace bcv

#endif // BCV_UTILS_H_
