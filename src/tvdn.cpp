// @file tvdn.cpp
#include "tvdn.h"

tvdn_params::tvdn_params() {
    lambda = 1;
    max_iterations = 1000;
    verbosity = numeric_limits<int>::max();
    dx_tolerance = 1e-5f;
    isotropic = 1;
    accelerated = true;
    gamma = 1.0f;

    sigma_x = 0.95f/sqrt(8.0f);
    sigma_y = 0.95f/sqrt(8.0f);
    rows = 0;
    cols = 0;
    chan = 0;
}
void tvdn_params::print() {
    printf("lambda: %f\n", lambda);
    printf("isotropic: %d\n", isotropic);
    if (accelerated) {
        printf("accelerated: 1 (gamma=%f)\n", gamma);
    } else { printf("accelerated: 0\n"); }
    printf("max-iterations: %f\n", max_iterations);
    printf("dx-tolerance: %f\n", dx_tolerance);
    printf("verbosity: %d\n", verbosity);
    printf("image size: %dx%dx%d\n", rows, cols, chan);
    printf("sigma_x/y: %f %f\n", sigma_x, sigma_y);
}

tvdn::tvdn() { }	
tvdn::~tvdn() { }
vector<float> tvdn::result() { return x; }

tvdn::tvdn(const vector<float>& img, const tvdn_params& p) {
    int n = img.size();
    int num_pts = n / p.chan;
    x       = vector<float>(n, 0.0f);
    x_bar   = vector<float>(n, 0.0f);
    DTy		= vector<float>(n, 0.0f);
    y	    = vector<float>(2*n, 0.0f);
    Dx		= vector<float>(2*n, 0.0f);

    float sigma_x = p.sigma_x;
    float sigma_y = p.sigma_y;
    float theta = 1.0f;
    float inv_lambda = 1.0f/p.lambda;
    vector<float> dxvec = vector<float>(5, 1.0f);
    float one_plus_sigmax_inv = 1.0f/(1.0f + sigma_x);      
    for (size_t iter = 0; iter < p.max_iterations; ++iter) {

        if (p.accelerated) {
            theta = 1.0f/sqrt(1.0f + 2*p.gamma*sigma_x);
            sigma_x = theta*sigma_x;
            sigma_y = sigma_y/theta;
            one_plus_sigmax_inv = 1.0f/(1.0f + sigma_x);
        }

        apply_pixelwise_gradient_op_transpose(DTy, y, p.rows, p.cols, p.chan);
        float dx = solve_x(img, theta, sigma_x);
        apply_pixelwise_gradient_op(Dx, x_bar, p.rows, p.cols, p.chan);
        solve_y(sigma_y, p.lambda, p.chan, p.isotropic);
        // ---------------------------------------------------------------------
        // performance diagnostics, stopping criteria, etc
        // calculate distance from previous iteration:
        memcpy(&dxvec[0], &dxvec[1], sizeof(float)*4 );
        dxvec[dxvec.size()-1] = dx;
        // average step over the last few iterations:
        dx = accumulate(dxvec.begin(), dxvec.end(), 0.0f)/dxvec.size();

        if ((iter>0) && (p.verbosity>0) && (iter % p.verbosity == 0)) {
            printf("i=%4d | dx = %4e\n", iter, dx);
        }
        if ((iter > 10) && (dx < p.dx_tolerance)) {
            printf("Change in x is too small: %.2e < %.2e\n", dx, p.dx_tolerance);
            break;
        }
        if (iter >= p.max_iterations) {
            printf("Reached maximum number of iterations: %d\n", p.max_iterations);
            break;
        }
    }	
}

float tvdn::solve_x(const vector<float>& img, float theta, float sigma_x) {
    #ifdef HAVE_SSE
    return solve_x_sse(img, theta, sigma_x);
    #endif
    //--------------------------  normal CPU version ---------------------------
    float dx = 0;
    float one_plus_sigmax_inv = 1.0f/(1.0f + sigma_x);
    int n = x.size();
    for (int i = 0; i < n; ++i) {
        float x_old = x[i]; 
        x[i] = (x[i]-sigma_x*DTy[i] + sigma_x*img[i])*one_plus_sigmax_inv;
        x_bar[i] = x[i] + theta*(x[i]-x_old);
        dx += (x_old-x[i])*(x_old-x[i]);
    }
    dx /= n;
    return dx;
}

void tvdn::solve_y(float sigma_y, float lambda, int chan, int isotropic) {
    #ifdef HAVE_SSE
    solve_y_sse(sigma_y, lambda, chan, isotropic);
    return;
    #endif
    int n = x.size();
    if (isotropic == 0) {
        for (int i = 0; i < 2*n; ++i) { 
            y[i] += sigma_y*Dx[i];
            y[i] = min(lambda, abs(y[i])) * BCV_SIGN(y[i]);
        } 
    } else if (isotropic == 1) {
        float inv_lambda = 1.0f/lambda;
        for (int i = 0; i < n; ++i) {
            float dz1 = y[i] + sigma_y*Dx[i];
            float dz2 = y[i + n] + sigma_y*Dx[i + n];
            float nz = 1.0f/max(1.0f, sqrt(dz1*dz1+dz2*dz2)*inv_lambda);
            y[i] = dz1*nz;
            y[i + n] = dz2*nz;
        }
    }
}

#ifdef HAVE_SSE
float tvdn::solve_x_sse(const vector<float>& img, float theta, float sigma_x) {
    float one_plus_sigmax_inv = 1.0f/(1.0f + sigma_x);
    int n = x.size();
    int nloops = n/4;
    float* px = &x[0];
    float* px_bar = &x_bar[0];
    float* pDTy = &DTy[0];
    const float* pimg = &img[0];

    __m128 m_dty, m_x, m_img, m_x_old, m_xbar, temp;
    __m128 m_one_plus_sigmax_inv = _mm_set1_ps( one_plus_sigmax_inv );
    __m128 m_sigma = _mm_set1_ps( sigma_x );
    __m128 m_theta = _mm_set1_ps( theta );
    __m128 m_dx = _mm_set1_ps( 0.0f );
    __m128 m_zero = _mm_set1_ps( 0.0f );

    float dx = 0;
    for (int i = 0; i < nloops; ++i) {
        m_dty    = _mm_loadu_ps(pDTy+4*i);
        m_x      = _mm_loadu_ps(px+4*i);
        m_img    = _mm_loadu_ps(pimg+4*i);
        m_x_old  = m_x;        
        temp = _mm_mul_ps(m_sigma, _mm_sub_ps(m_img, m_dty));
        temp = _mm_add_ps(m_x, temp);
        m_x = _mm_mul_ps(temp, m_one_plus_sigmax_inv);
        // compute dx:
        temp = _mm_sub_ps(m_x, m_x_old);
        m_dx = _mm_add_ps(m_dx, _mm_mul_ps(temp, temp));
        // compute xbar:
        m_xbar = _mm_add_ps(m_x, _mm_mul_ps(m_theta, _mm_sub_ps(m_x, m_x_old)));
        _mm_storeu_ps( px+4*i, m_x);
        _mm_storeu_ps( px_bar+4*i, m_xbar );
    }
    // need to verify:
    float dxsum_[4];
    _mm_storeu_ps(dxsum_, m_dx);
    dx = (dxsum_[0] + dxsum_[1] + dxsum_[2] + dxsum_[3]);
    
    for (int i = 4*nloops; i < n; ++i) {
        float x_old = x[i]; 
        x[i] = (x[i]-sigma_x*DTy[i] + sigma_x*img[i])*one_plus_sigmax_inv;
        x_bar[i] = x[i] + theta*(x[i]-x_old);
        dx += (x_old-x[i])*(x_old-x[i]);
    }
    dx /= n;
    return dx;
}

void tvdn::solve_y_sse(float sigma_y, float lambda, int chan, int isotropic) {
    int n = x.size();

    float* py = &y[0];
    float* pDx = &Dx[0];
    __m128 m_dx, m_y, yhat;
    __m128 m_sigma = _mm_set1_ps( sigma_y );
    //--------------------------------------------------------------------------
    if (isotropic == 0) {
        int nloops = 2*n/4;
        __m128 m_lambda = _mm_set1_ps( lambda );
        __m128 m_one = _mm_set1_ps( 1.0f );
        // init sign vectors:
        unsigned int ABS_MASK[4] __attribute__((aligned(32)));
        unsigned int SIGN_MASK[4] __attribute__((aligned(32)));
        for (int i = 0; i < 4; ++i) { 
            ABS_MASK[i]  = 0x80000000;
            SIGN_MASK[i] = 0x7FFFFFFF;
        }
        __m128i m_signmask = _mm_load_si128( (__m128i*)SIGN_MASK );
        __m128i m_absmask  = _mm_load_si128( (__m128i*)ABS_MASK );
        __m128 abs_yhat, sign_yhat;
        for (int i = 0; i < nloops; ++i) { 
            m_dx    = _mm_loadu_ps(pDx+4*i);
            m_y     = _mm_loadu_ps(py+4*i);
            yhat    = _mm_add_ps(m_y, _mm_mul_ps(m_sigma, m_dx));
            // sign(y) min(w, abs(y) )
            abs_yhat    = _mm_andnot_ps( (__m128)m_absmask, yhat);   
            sign_yhat   = _mm_or_ps( _mm_andnot_ps( (__m128)m_signmask, yhat), m_one);   
            yhat = _mm_mul_ps(sign_yhat, _mm_min_ps(m_lambda, abs_yhat) );
            _mm_storeu_ps(py + 4*i, yhat);
        }
        for (int i = 4*nloops; i < 2*n; ++i) { 
            y[i] += sigma_y*Dx[i];
            y[i] = min(lambda, abs(y[i])) * BCV_SIGN(y[i]);
        }
    //--------------------------------------------------------------------------
    } else if (isotropic == 1) {
        int nloops = n/4;        
        float inv_lambda = 1.0f/lambda;
        __m128 m_one        = _mm_set1_ps( 1.0f );        
        __m128 m_invlambda  = _mm_set1_ps( inv_lambda );
        __m128 m_dz1, m_dz2, m_nz;
        for (int i = 0; i < nloops; ++i) {
            m_y     = _mm_loadu_ps(py + 4*i);
            m_dx    = _mm_loadu_ps(pDx + 4*i);
            m_dz1   = _mm_add_ps(_mm_mul_ps(m_sigma, m_dx), m_y);
            m_y     = _mm_loadu_ps(py + 4*i + n);
            m_dx    = _mm_loadu_ps(pDx + 4*i + n);
            m_dz2   = _mm_add_ps(_mm_mul_ps(m_sigma, m_dx), m_y);
            m_nz    = _mm_sqrt_ps( _mm_add_ps(_mm_mul_ps(m_dz1,m_dz1), 
                                              _mm_mul_ps(m_dz2,m_dz2)));
            m_nz = _mm_div_ps(m_one, _mm_max_ps(m_one, _mm_mul_ps(m_nz,m_invlambda )));
            m_dz1 = _mm_mul_ps(m_dz1, m_nz);
            m_dz2 = _mm_mul_ps(m_dz2, m_nz);
            _mm_storeu_ps(py + 4*i, m_dz1);
            _mm_storeu_ps(py + 4*i + n, m_dz2);
        }
        for (int i = 4*nloops; i < n; ++i) {
            float dz1 = y[i] + sigma_y*Dx[i];
            float dz2 = y[i + n] + sigma_y*Dx[i + n];
            float nz = 1.0f/max(1.0f, sqrt(dz1*dz1+dz2*dz2)*inv_lambda);
            y[i] = dz1*nz;
            y[i + n] = dz2*nz;
        }
    }
}

#endif