//! @file tvdeblur.h
//! TV deblurring
//! This code is adapted from MATLAB code accompanying a paper:
//!   "Total Variation Blind Deconvolution: The Devil is in the Details", 
//!    D. Perrone and P. Favaro, CVPR, 2014. 

#ifndef BCV_TVDEBLUR_H_
#define BCV_TVDEBLUR_H_

#include <cstdlib>
#include <cmath>
#include <limits>
#include <numeric>
#include <algorithm>
#include <fftw3.h>
#include "bcv_utils.h"
#include "bcv_diff_ops.h"
#include "bcv_io.h" // temporary
#include "bcv_imgproc.h" // for imresize
#ifdef HAVE_SSE
#include <xmmintrin.h>
#include <pmmintrin.h>
#endif

namespace bcv {
using namespace std;

#ifndef BCV_SIGN
#define BCV_SIGN(x) ( ((x)>0) ? +1 : -1)
#endif

//! TV deblurring parameters
class tvdeblur_params {
public:
    enum { KERNEL_INIT_FLAT, KERNEL_INIT_DELTA };
    float lambda = 1e-3f; //! (initial) weight of TV penalty
    float min_lambda = 1e-5f; //! (final) weight of TV penalty (after annealing)
    float annealing_rate = 0.999f; //! rate by which lambda decreases
    int max_inner_iterations = 1; //! max number of iterations
    int max_anneal_rounds = 10000; //! max number of annealing rounds
    int max_nonblind_iterations = 100; //! maximum number of iterations used in NONBLIND step
    float grad_descent_step_u = 5e-3f; //! step length in gradient descent (over image) 
    float grad_descent_step_k = 1e-3f; //! step length in gradient descent (over kernel)
    float dfx_tolerance = 1e-4f; //! smallest ( f(x)-f(x_+) )/f(x), stopping criterion
    int ker_size = 9; // kernel width/height 
    int verbosity = 100;
    // pyramid parameters
    int pyramid_numlevels = 1;
    float pyramid_imsize_scaling = 1; // how much to resize the image at each time? (<1)
    float pyramid_lambda_scaling = 1; // how much to increase lambda at each time (>1)
    // backtracking line search:
    bool use_bls = false;
    float bls_beta = 0.5f;
    float bls_alpha = 0.001f; 
    int bls_max_iters = 10;
    // other parameters
    int rows = 0;
    int cols = 0;
    int chan = 0;
    bool vis_results = true; // write out images at every step?
    int kernel_init_type = KERNEL_INIT_FLAT;

    float eps = 1e-4; // smoothing parameter of the L1 norm

    void print();
};

//! TV deblurring solver
class tvdeblur {
    enum truncation_type { TRUNCATION_FULL, TRUNCATION_SAME, TRUNCATION_VALID };
public:
    struct pyramid_size {
        int rows, cols;
        int rows_ker, cols_ker;
        int rows_padded, cols_padded;
        float lambda;
    }; // structure for multiscale processing

    tvdeblur(const vector<float>& img, const tvdeblur_params& p);   
    tvdeblur();
    ~tvdeblur();
    tvdeblur(const tvdeblur& other) = delete; // no copy constructor!
    tvdeblur(const tvdeblur&& other) = delete; // no move constructor!
    tvdeblur& operator=(const tvdeblur& other) = delete; // no copy assignment op!
    tvdeblur& operator=(const tvdeblur&& other) = delete; // no move assignment op!

    vector<float> result();
    //
    void solve_inner(float lambda);
    // for a given kernel, solve the non-blind deconvolution problem
    // (just using gradient descent)
    void solve_nonblind(const vector<float>& k, float tv_penalty, int num_iters);
    void solve();
    void debug();

    vector<float> calc_grad_img(const vector<float>& u, 
                                const vector<float>& kernel,
                                const vector<float>& img_og,
                                float lambda);
    vector<float> calc_grad_kernel(const vector<float>& u, 
                                const vector<float>& kernel,
                                const vector<float>& img_og);

    float calc_func_value(const vector<float>& u, 
                          const vector<float>& kernel, 
                          const vector<float>& img_og, float lambda);

    // backtracking line search over image and kernel
    float bls_u(const vector<float>& gu, float lambda, float bls_step);
    float bls_k(const vector<float>& gk, float lambda, float bls_step);

    void project_kernel_onto_feasible_set(vector<float>& k);

    vector<float> pad_image(const vector<float>& in, 
                        int rows_in, int cols_in, int rows_out, int cols_out);
    vector<float> unpad_image(const vector<float>& in, 
                        int rows_in, int cols_in, int rows_out, int cols_out);

    vector<float> fft_imfilter( const vector<float>& im1, int rows1, int cols1, int chan1, 
                                const vector<float>& im2, int rows2, int cols2, int chan2,
                                truncation_type type);

    vector<float> time_reverse_signal(const vector<float>& in, int rows, int cols, int chan);

    // variables..
    const tvdeblur_params* params;
    vector<pyramid_size> scales_data;
    vector<float> img_blur_original;
    vector<float> img_og;
    vector<float> u;
    vector<float> kernel; // filter kernel 
    // memory pointers..
    float* img_p1;
    fftwf_complex* img_p2;
    float* ker_p1;
    fftwf_complex* ker_p2;
    // data sizes:
    int rows, cols, chan;
    int rows_ker, cols_ker;
    int rows_padded, cols_padded;    
private:
    vector<float> stack_channels(const vector<float>& in, int chan);
    vector<float> interleave_channels(const vector<float>& in, int chan);
};

} // namespace bcv
#endif // BCV_TVDEBLUR_H_
