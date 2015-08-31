#include "tvdeblur_nonblind.h"
namespace bcv {

vector<float> tvdeblur_nonblind::solve(const vector<float>& img, 
                              const tvdeblur_nonblind_params& p) { 
    params = &p;
    // TODO: some basic checks!!!
    
    vector<float> img_degraded = stack_channels(img, params->chan);

    persistent_data_setup(img_degraded, p.kernel);
    //vector<float> out = solve_inner(img_degraded);
    //persistent_data_free();

    //return interleave_channels(out, params->chan);
    
    return img;
}

vector<float> tvdeblur_nonblind::solve_inner(const vector<float>& init) {
    int n = params->rows*params->cols;
    int nedges = n*2;

    vector<float> u; 
    if (init.size()==params->chan*n) { u = init; }
    else { u = vector<float>(params->chan*n, 0); }
    vector<float> ubar = u;
    vector<float> y(params->chan*nedges, 0);

    vector<float> dxvec(5,1);
    double elapsed = 0;
    double total_elapsed = 0;
    double tic;
    for (int iter = 0; iter < params->max_iterations; ++iter) {
        // compute D u
        tic = now_ms();
        vector<float> Dubar(nedges*params->chan);
        for (int ch = 0; ch < params->chan; ++ch) {
            apply_pixelwise_gradient_op(
                &Dubar[0]+nedges*ch, &ubar[0]+n*ch, params->rows, params->cols);
        }
        for ( size_t i = 0; i < y.size(); ++i) { y[i] += params->sigma_y * Dubar[i]; } 
        
        // 1b. apply prox operator:
        for (size_t ch = 0; ch < params->chan; ++ch) {
            for (size_t i = 0; i < n; ++i) {
                float y1 = y[i+0+2*n*ch]; 
                float y2 = y[i+n+2*n*ch];
                float ynorm = sqrt(y1*y1 + y2*y2);
                y[i+0+2*n*ch]  = y1*min(1.0f, params->lambda/ynorm);
                y[i+n+2*n*ch]  = y2*min(1.0f, params->lambda/ynorm);
            }
        }
        // compute D^T y
        vector<float> Dty(n*params->chan, 0);
        for (int ch = 0; ch < params->chan; ++ch) {
            apply_pixelwise_gradient_op_transpose(
                    &Dty[0]+n*ch, &y[0]+nedges*ch, params->rows, params->cols);
        }
        vector<float> uprev = u;
        for (size_t i = 0; i < u.size(); ++i) { u[i] -= params->sigma_x * Dty[i]; }
        
        // 2b. apply prox operator:
        apply_prox_gu(u);    
        float dxL2 = 0;
        float xL2 = 0;
        for (int i = 0; i < u.size(); ++i) { 
            ubar[i] = 2*u[i]-uprev[i]; 
            dxL2 += (u[i]-uprev[i])*(u[i]-uprev[i]);
            xL2 += u[i]*u[i];
        }
        memcpy(&dxvec[0], &dxvec[1], sizeof(float)*(dxvec.size()-1)); 
        dxvec[ dxvec.size()-1 ] = sqrt(dxL2)/sqrt(xL2);
        float dx_mu = accumulate( dxvec.begin(), dxvec.end(), 0.0)/dxvec.size();

        elapsed = now_ms()-tic;
        total_elapsed += elapsed;
        // process output and stopping criteria and such
        if ((params->verbosity>0) && (iter % params->verbosity==0)) {
            printf("%3d) dx=%4g \t| %4g ms \t| total: %4g s\n", 
                    iter, dxvec[ dxvec.size()-1 ], elapsed, total_elapsed/1000.0 );
        }
        if (dx_mu < params->dx_tolerance) {
            if (params->verbosity>0) {
                printf("%3d) dx=%4g < %4g, stopping\n", 
                                            iter, dx_mu, params->dx_tolerance) ;
                break;
            }
        }
    }
    return u;
}

void tvdeblur_nonblind::apply_prox_gu(vector<float>& u) {
    fftwf_complex Fx, Ffk; float invden;
    int n = params->rows*params->cols;
    int fft_dim = params->cols*params->rows; //(params->rows/2+1);
    for (int ch = 0; ch < params->chan; ++ch) {
        memcpy( precomputed.p, &u[0]+n*ch, sizeof(float)*n );
        fftwf_execute( precomputed.p_forward );
        for (int i = 0; i < fft_dim; ++i) {
            Fx[0] = precomputed.Fp[i][0];
            Fx[1] = precomputed.Fp[i][1];
            Ffk[0] = precomputed.Ffk[ch][i][0];
            Ffk[1] = precomputed.Ffk[ch][i][1];
            invden = precomputed.Fkk[i];

            precomputed.Fp[i][0] = (Ffk[0]+Fx[0])*invden;
            precomputed.Fp[i][1] = (Ffk[1]+Fx[1])*invden;
        }
        fftwf_execute( precomputed.p_inverse );        
        memcpy( &u[0]+n*ch, precomputed.p, sizeof(float)*n );
    }
}

//! Allocates memory for F(f)*conj(F(k)) and F(k)*conj(F(k)), and creates those
//! variables..
void tvdeblur_nonblind::persistent_data_setup(const vector<float>& img_degraded,
                                              const vector<float>& kernel) { 

    // first allocate memory. F(f)*conj(F(k)) needs to be allocated for each channel.
    int fft_dim = params->cols*params->rows; //(params->rows/2+1);
    int n = params->rows*params->cols;
    //precomputed.Ffk = (fftwf_complex**)fftwf_malloc(sizeof(fftwf_complex*)*params->chan);
    for (int k = 0; k < params->chan; ++k) {
        precomputed.Ffk[k] = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*fft_dim );
    }
    precomputed.Fkk = (float*)fftwf_malloc( sizeof(float)*fft_dim );

    // compute FFTs 
    float* fft_src = (float*)fftwf_malloc( sizeof(float)*n );
    fftwf_complex* fft_dst    = (fftwf_complex*)fftwf_malloc( 
                                            sizeof(fftwf_complex)*fft_dim );
    fftwf_complex* fft_kernel = (fftwf_complex*)fftwf_malloc( 
                                            sizeof(fftwf_complex)*fft_dim );

    //fftwf_plan p0 = fftwf_plan_dft_r2c_2d(params->rows, params->cols, fft_src, fft_dst, FFTW_MEASURE);
    // kernel is circshifted here
    
    vector<float> kernel_image(n, 0);
    for (int r = 0; r < params->rows_ker; ++r) {
        memcpy( &kernel_image[ linear_index(r,0,params->cols) ], 
                      &kernel[ linear_index(r,0,params->cols_ker) ], 
                            sizeof(float)*params->cols_ker );
    }
    circshift(kernel_image, params->rows, params->cols, 
                                        params->cols_ker/2, params->rows_ker/2);
    
    //memcpy(fft_src, &kernel_image[0], sizeof(float)*n);
    
    //fftwf_execute(p0);
    //memcpy(fft_kernel, fft_dst, sizeof(fftw_complex)*fft_dim );

    // fill Fkk:
    for (int i = 0; i < fft_dim; ++i) {
        float val = (fft_kernel[i][0]*fft_kernel[i][0] + 
                     fft_kernel[i][1]*fft_kernel[i][1])*params->sigma_x;
        precomputed.Fkk[i] = 1.0f/((val+1.0f)*n ); // store inverse denominator
    }
    
    // compute FFTs of the degraded image:
    
    for (int k = 0; k < params->chan; ++k) {
        //memcpy(fft_src, &img_degraded[0]+n*k, sizeof(float)*n);
        //fftwf_execute(p0);
        for (int i = 0; i < fft_dim; ++i) {
            precomputed.Ffk[k][i][0] =( fft_dst[i][0]*fft_kernel[i][0] +
                                        fft_dst[i][1]*fft_kernel[i][1] )*params->sigma_x;
            precomputed.Ffk[k][i][1] =(-fft_dst[i][0]*fft_kernel[i][1] +
                                        fft_dst[i][1]*fft_kernel[i][0] )*params->sigma_x;
        }
    }
    //fftwf_destroy_plan(p0);

    fftwf_free(fft_kernel);
    fftwf_free(fft_dst);
    fftwf_free(fft_src);
    
    // Initialize storage for ffts that will be computed at each iteration.
    precomputed.p = (float*)fftwf_malloc( sizeof(float)*n );
    precomputed.Fp = (fftwf_complex*)fftwf_malloc( sizeof(fftwf_complex)*fft_dim );
    precomputed.p_forward = fftwf_plan_dft_r2c_2d(params->rows, params->cols, 
                                    precomputed.p, precomputed.Fp, FFTW_MEASURE);
    precomputed.p_inverse = fftwf_plan_dft_c2r_2d(params->rows, params->cols, 
                                    precomputed.Fp, precomputed.p, FFTW_MEASURE);
}

void tvdeblur_nonblind::persistent_data_free() { 
    if (precomputed.Ffk) {
        for (int k = 0; k < params->chan; ++k) {
            if (precomputed.Ffk[k]) {
                fftwf_free(precomputed.Ffk[k]); precomputed.Ffk[k] = NULL;
            }
        }
        //fftwf_free(precomputed.Ffk); precomputed.Ffk = NULL;
    }
    if (precomputed.Fkk) { fftwf_free(precomputed.Fkk); precomputed.Fkk = NULL; }

    // storage that was used for FFTs at each iteration
    if (precomputed.p) { fftwf_free(precomputed.p); precomputed.p = NULL; }
    if (precomputed.Fp) { fftwf_free(precomputed.Fp); precomputed.Fp = NULL; }
    fftwf_destroy_plan(precomputed.p_forward);
    fftwf_destroy_plan(precomputed.p_inverse);
}

} // namespace bcv
