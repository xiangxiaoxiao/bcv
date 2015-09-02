//! @file bcv_basic.cpp
#include "bcv_utils.h"

namespace bcv {
//! returns time in microseconds
unsigned long now_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long ret = tv.tv_usec;
    ret += (tv.tv_sec * 1000*1000);
    return ret;
}
//! returns time in milliseconds
double now_ms() {
    return double(now_us())/1000.0f;
}


//! Returns a randomly chosen subset S, s.t. |S|=k, sampled from {0,...,n-1}
//! TODO: rewrite this with Knuth's sampling algorithm
vector<int> choose_random_subset(size_t k, size_t n) {
    assert( (k<=n) && "asked for subset smaller than n");
    
    vector<int> s;
    s.reserve(k);
    if (k==n) { 
        for (size_t i = 0; i < n; ++i) { s[i] = i; }
        return s;
    }
    // TODO: rewrite this with UNORDERED set
    set<int> s_;
    while (s_.size() < k) { s_.insert( rand() % n); }
    // move to vector
    for (set<int>::iterator it=s_.begin(); it!=s_.end(); ++it) {
        s.push_back( *(it) );
    }
    return s;
}

} // namespace bcv
