//! @file: tvdn.h
//! TV denoising:
//!   x^* = \arg\min_x 0.5*\| x- u \|_2^2 + \lambda TV(x)
//!
//! Optimization is performed using the primal-dual method from:
//! A. Chambolle and T.Pock's "A first-order primal-dual algorithm
//! for convex problems with applications to imaging"
//! Journal of Mathematical Imaging and Vision 40.1 (2011)

#ifndef BCV_TVDN_H_
#define BCV_TVDN_H_

#include <cstdlib>
#include <cmath>
#include <limits>
#include <numeric>
#include "bcv_utils.h"
#include "bcv_diff_ops.h"

namespace bcv {
using namespace std;

#ifdef HAVE_SSE
#include <xmmintrin.h>
#include <pmmintrin.h>
#endif

#ifndef BCV_SIGN
#define BCV_SIGN(x) ( ((x)>0) ? +1 : -1)
#endif

//! TV denoising parameters
class tvdn_params {
public:
    float lambda = 1.0f; //! weight of TV penalty
    int max_iterations = 1000; //! maximum number of iterations
    float dx_tolerance = 1e-5f; //! step tolerance per pixel
    int isotropic = 1; //! 0=anisotropic, 1=isotropic xy
    int verbosity = numeric_limits<int>::max();
    bool accelerated = true; // acceleration 
    float gamma = 1.0f; // acceleration parameter
    float sigma_x = 0.95f/sqrt(8.0f);
    float sigma_y = 0.95f/sqrt(8.0f);
    int rows = 0;
    int cols = 0;
    int chan = 0;

    void print();
};

//! TV denoising solver
class tvdn {
public:
    tvdn(const vector<float>& img, const tvdn_params& p);
    tvdn();
    tvdn(const tvdn& other) = delete; // no copy constructor!
    tvdn(const tvdn&& other) = delete; // no move constructor!
    tvdn& operator=(const tvdn& other) = delete; // no copy assignment op!
    tvdn& operator=(const tvdn&& other) = delete; // no move assignment op!
    ~tvdn();

    vector<float> result();
private:
    vector<float> x;    
    vector<float> x_bar;
    vector<float> y;
    vector<float> DTy;
    vector<float> Dx;

    float solve_x(const vector<float>& img, float theta, float sigma_x);
    void solve_y(float sigma_y, float lambda, int chan, int isotropic);
    #ifdef HAVE_SSE
    float solve_x_sse(const vector<float>& img, float theta, float sigma_x);
    void solve_y_sse(float sigma_y, float lambda, int chan, int isotropic);
    #endif
};

} // namespace bcv
#endif // BCV_TVDN_H_
