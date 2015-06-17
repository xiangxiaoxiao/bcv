//! TV deblurring
//! This is adapted from MATLAB code accompanying a paper:
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

using namespace std;

#ifndef BCV_SIGN
#define BCV_SIGN(x) ( ((x)>0) ? +1 : -1)
#endif

enum truncation_type { TRUNCATION_FULL, TRUNCATION_SAME, TRUNCATION_VALID };
//! deblurring parameters
class tvdeblur_params {
public:
    float lambda; //! (initial) weight of TV penalty
    float min_lambda; //! (final) weight of TV penalty (after annealing)
    float annealing_rate; //! rate by which lambda decreases
    int max_iterations; //! max number of iterations
    int max_anneal_rounds; //! max number of annealing rounds
    float grad_descent_step_u; //! step length in gradient descent (over image) 
    float grad_descent_step_k; //! step length in gradient descent (over kernel)
    float dfx_tolerance; //! smallest ( f(x)-f(x_+) )/f(x)
    int ker_size; // kernel size 
    int verbosity;
    // pyramid parameters
    int pyramid_numlevels;
    float pyramid_imsize_scaling; // how much to resize the image at each time? (<1)
    float pyramid_lambda_scaling; // how much to increase lambda at each time (>1)

    // other parameters
    int rows;
    int cols;
    int chan;
    float eps; // smoothing parameter of the L1 norm
    tvdeblur_params();
    ~tvdeblur_params() {};
    void print();
};

struct pyramid_size {
    int rows, cols;
    int rows_ker, cols_ker;
    int rows_truncated, cols_truncated;
    int rows_padded, cols_padded;
    float lambda;
};

class tvdeblur {
public:
    tvdeblur(const vector<float>& img, const tvdeblur_params& p);
    tvdeblur();
    ~tvdeblur();

    vector<float> result();
    void solve_inner(float lambda);
    void solve();

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

    void project_kernel_onto_feasible_set(vector<float>& k);

    vector<float> init_blur_image(const vector<float>& img, int rows_t, int cols_t, 
                        int cols, int rows_offset, int cols_offset, int chan);


    vector<float> fft_imfilter( const vector<float>& im1, int rows1, int cols1, int chan1, 
                                const vector<float>& im2, int rows2, int cols2, int chan2,
                                truncation_type type);

    vector<float> time_reverse_signal(const vector<float>& in, int rows, int cols, int chan);
    void print_kernel();

    int get_rows_truncated() { return rows_truncated; };
    int get_cols_truncated() { return cols_truncated; };

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
    int rows_truncated, cols_truncated;
private:
    vector<float> stack_channels(const vector<float>& in, int chan);
    vector<float> interleave_channels(const vector<float>& in, int chan);
};

#endif // BCV_TVDEBLUR_H_
