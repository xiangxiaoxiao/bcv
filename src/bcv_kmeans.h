//! @file bcv_kmeans.h
#ifndef BCV_KMEANS_H_
#define BCV_KMEANS_H_

#include <cstdlib>
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <iostream>
#include <vector>
#include <bitset>
#include <limits>
#include <algorithm>
#include <numeric>

namespace bcv {
using namespace std;

//! A very basic kmeans implementation.
class bcv_kmeans {
public:
    //! kmeans cluster initialization options
    enum {
        INIT_RANDOM=0,
        INIT_FURTHEST_FIRST=1 };

    //! kmeans algorithm options
    enum {
        METHOD_LLOYD=0,
        METHOD_ELKAN=1 };

    int num_pts = 0;
    int dim = 0;
    int K = 0;
    int num_iterations = 0; 
    int verbosity = 0;
    float dfx_tolerance = 1e-5f; 
    int kmeans_init_method = INIT_RANDOM; // initialization type
    int kmeans_method = METHOD_LLOYD; // lloyd or elkan?
 
    const float* data = NULL;
    float* distance = NULL; // distance of point to nearest cluster
    int* assignments = NULL;
    float* centers = NULL;
    int* count = NULL; // number of points belonging to cluster

    bcv_kmeans();
    bcv_kmeans(const bcv_kmeans& that);
    bcv_kmeans(bcv_kmeans&& that); 
    bcv_kmeans& operator=(const bcv_kmeans& that);
    bcv_kmeans& operator=(bcv_kmeans&& that);
    bcv_kmeans(const vector<float>& d, int num_pts_, int dim_, int K_, 
            int num_iterations_=100, int verbosity_=0, float dfx_tol=1e-5f, 
            int init_method = INIT_RANDOM, int solve_method = METHOD_LLOYD);


    bcv_kmeans(const char* fname);
    ~bcv_kmeans(); 
    void init_centers();
    void init_centers_furthest_first();

    void get_centers(vector<float>& data);
    void get_assignments(vector<int>& a);
    void get_assignments(vector<int>& a, const vector<float>&d);

    void kmeans();
    void elkan_kmeans(); 

    void save(const char* fname);
private:
    int check_parameters();
    float eval_function_value();

    float inline get_distance_sq(const float* a, const float* b, int n);

    void elkan_compute_cluster_distance(float* D, float* S);
};

} // namespace bcv
#endif // BCV_KMEANS_H_
