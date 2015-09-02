//! @file tvdeblur_nonblind.h
//! Non-blind TV deblurring, based on 
//! A. Chambolle and T.Pock's "A first-order primal-dual algorithm              
//! for convex problems with applications to imaging"                           
//! Journal of Mathematical Imaging and Vision 40.1 (2011)  

#ifndef BCV_TVDEBLUR_NONBLIND_H_
#define BCV_TVDEBLUR_NONBLIND_H_


#include <cstdlib>
#include <cmath>
#include <limits>
#include <numeric>
#include <algorithm>
#include <fftw3.h>
#include "bcv_utils.h"
#include "bcv_imgutils.h"
#include "bcv_diff_ops.h"
#include "bcv_imgproc.h" // for circshift
#ifdef HAVE_SSE
#include <xmmintrin.h>
#include <pmmintrin.h>
#endif
#include <iostream>

namespace bcv {
using namespace std;

//! TV deblurring parameters
class tvdeblur_nonblind_params {
public:
    float lambda = 1e-3;
    float sigma_x = 0.350; // ~1.0/sqrt(8)
    float sigma_y = 0.350; // ~1.0/sqrt(8)
    
    int max_iterations = 1000;
    float dx_tolerance = 1e-6f; // |x-x_|/|x| < threshold --> quit
    int verbosity = 1;

    size_t rows = 0;
    size_t cols = 0;
    size_t chan = 0;

    size_t cols_ker = 0;
    size_t rows_ker = 0;
    vector<float> kernel;
};

//! \class Non-blind TV deblurring
class tvdeblur_nonblind {
private:
    //! Structure to store FFTW data that is reused over and over again..
    struct persistent_fft_data { 
        fftwf_complex* Ffk[10]; // = NULL; // (multiple channels) F(f) \circ conj( F(k) ) 
        float* Fkk = NULL; // 1.0/( (F(k) \circ conj( F(k) ) + 1)/n )
        
        // storage for fft, used at each iteration
        fftwf_complex* Fp = NULL; // (multiple channels)
        float* p = NULL;

        fftwf_plan p_forward; // plan for prox-point forward fft
        fftwf_plan p_inverse; // plan for the inverse, which gives the minimizer
    };    

    persistent_fft_data precomputed; 
    void persistent_data_setup(const vector<float>& img_degraded,
                               const vector<float>& kernel);
    void persistent_data_free();

    vector<float> solve_inner(const vector<float>& init = vector<float>() );
    void apply_prox_gu(vector<float>& u);

    const tvdeblur_nonblind_params* params = NULL;

public:
    tvdeblur_nonblind() {};
    ~tvdeblur_nonblind() {};
    tvdeblur_nonblind(const tvdeblur_nonblind& other) = delete; // no copy constructor!
    tvdeblur_nonblind(const tvdeblur_nonblind&& other) = delete; // no move constructor!
    tvdeblur_nonblind& operator=(const tvdeblur_nonblind& other) = delete; // no copy assignment op!
    tvdeblur_nonblind& operator=(const tvdeblur_nonblind&& other) = delete; // no move assignment op!
    //!\brief in comes the blurry image, out comes the result.
    vector<float> solve(const vector<float>& img, const tvdeblur_nonblind_params& p);
};

} // namespace bcv
#endif // BCV_TVDEBLUR_NONBLIND_H_
