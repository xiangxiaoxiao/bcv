// @file tvdeblur.cpp
#include "tvdeblur.h"

tvdeblur_params::tvdeblur_params() {
    lambda = 1e-3f;
    min_lambda = 1e-5f;
    annealing_rate = 0.999f;
    max_iterations = 5;
    max_anneal_rounds = 10000;
    grad_descent_step_u = 5e-3f;
    grad_descent_step_k = 1e-3f;
    dfx_tolerance = 1e-4f;
    ker_size = 9;
    verbosity = 100;
    eps = 1e-5f;
}

void tvdeblur_params::print() { 
    printf("deblurring parameters.\n");
    printf("---------------------\n");
    printf("lambda = %f (min=%f)\n", lambda, min_lambda );
    printf("annealing rate = %f\n", annealing_rate );
    printf("grad step: img = %f, ker = %f\n", grad_descent_step_u, grad_descent_step_k );
    printf("kernel size: %d\n", ker_size);
    printf("max iterations: %d, max anneal rounds: %d\n", max_iterations, max_anneal_rounds );
    printf("dfx tolerance: %f\n", dfx_tolerance);
    printf("smoothing eps: %f\n", eps);
    printf("verbosity: %d\n", verbosity);
    printf("multiscale parameters.\n");
    printf("num levels: %d\n", pyramid_numlevels);
    printf("imsize scale: %f\n", pyramid_imsize_scaling);
    printf("lambda scale: %f\n", pyramid_lambda_scaling);
    printf("---------------------\n");
}

tvdeblur::tvdeblur() { }
tvdeblur::~tvdeblur() { }

tvdeblur::tvdeblur(const vector<float>& img, const tvdeblur_params& p) { 
    params = &p;

    // initialize multi-resolution data
    scales_data = vector<pyramid_size>( params->pyramid_numlevels );
    for (int i = 0; i < params->pyramid_numlevels; ++i) {
        float scale = pow( params->pyramid_imsize_scaling, params->pyramid_numlevels-(i+1) );
        float lamscale = pow( params->pyramid_lambda_scaling, params->pyramid_numlevels-(i+1) );
        scales_data[i].rows = (int)(p.rows*scale);
        scales_data[i].cols = (int)(p.cols*scale);
        scales_data[i].rows_ker = max(5, (int)(p.ker_size*scale) );
        scales_data[i].cols_ker = max(5, (int)(p.ker_size*scale) );
        scales_data[i].rows_truncated = scales_data[i].rows - scales_data[i].rows_ker + 1;
        scales_data[i].cols_truncated = scales_data[i].cols - scales_data[i].cols_ker + 1;
        scales_data[i].rows_padded = scales_data[i].rows + scales_data[i].rows_ker-1;
        scales_data[i].cols_padded = scales_data[i].cols + scales_data[i].cols_ker-1;
        scales_data[i].lambda = lamscale*p.lambda;
    }
    img_blur_original = img;
}

//! rows_t, cols_t - output image size.
//! cols - input image size.
//! rows_offset/cols_offset -- offsets into imput image
//! input image is 'interleaved', and output is 'stacked'.
vector<float> tvdeblur::init_blur_image(const vector<float>& img, int rows_t, int cols_t, 
                    int cols, int rows_offset, int cols_offset, int chan) {
    vector<float> out = vector<float>(rows_t*cols_t*chan, 0.0f);
    int n = rows_t*cols_t;
    for (int ch = 0; ch < chan; ++ch) {
        for (int r = 0; r < rows_t; ++r) {
            for (int c = 0; c < cols_t; ++c) {
                out[ linear_index(r,c,cols_t) + n*ch ] = 
                    img[ linear_index(r+rows_offset, c+cols_offset, ch, cols, chan) ];
            }
        }
    }
    return out;
}

void tvdeblur::solve() {
    chan = params->chan;
    for (int s = 0; s < scales_data.size(); ++s) {
        // get sizes:
        rows = scales_data[s].rows;
        cols = scales_data[s].cols;
        rows_ker = scales_data[s].rows_ker;
        cols_ker = scales_data[s].cols_ker;
        rows_truncated = scales_data[s].rows_truncated;
        cols_truncated = scales_data[s].cols_truncated;
        rows_padded = scales_data[s].rows_padded;
        cols_padded = scales_data[s].cols_padded;
        float tv_penalty = scales_data[s].lambda;

        // resize image to appropriate size, to serve as "noisy image"...
        vector<float> img = imresize(img_blur_original, params->rows, params->cols, rows, cols);

        int rows_offset = rows_ker/2;
        int cols_offset = cols_ker/2;
        img_og = init_blur_image(img, rows_truncated, cols_truncated, cols, 
                                        rows_offset, cols_offset, chan);
        // initialize image variable
        printf("solving at %dx%d with kernel=%d, lambda=%f\n", rows, cols, rows_ker, tv_penalty);
        printf("truncated: %dx%d, padded: %dx%d\n", rows_truncated, cols_truncated, rows_padded, cols_padded);
        if (s == 0) {
            u = stack_channels(img, chan); 
            //kernel = vector<float>(rows_ker*cols_ker, 1.0f/(rows_ker*cols_ker) );
            kernel = vector<float>(rows_ker*cols_ker, 0.0f);
            int mid_y = (rows_ker % 2 == 0) ? rows_ker/2+1 : rows_ker/2;
            int mid_x = (cols_ker % 2 == 0) ? cols_ker/2+1 : cols_ker/2;
            kernel[ linear_index(mid_y, mid_x, cols_ker) ] = 1.0f;
        } else { // initialize from the previous iteration
            u = interleave_channels(u, chan);
            u = imresize(u, scales_data[s-1].rows, scales_data[s-1].cols, rows, cols);
            u = stack_channels(u, chan);
            
            kernel = imresize(kernel, scales_data[s-1].rows_ker, scales_data[s-1].cols_ker, rows_ker, cols_ker);
            project_kernel_onto_feasible_set(kernel);
        }
        // 
        solve_inner(tv_penalty);
    }
}

void tvdeblur::solve_inner(float tv_penalty) {
    float scale = 1.0f;
    double t1, elapsed;
    float fx, fxprev, avg_dfx;
    vector<float> recent_dfx = vector<float>(10, 1.0);
    bool fx_updated;

    for (int nn = 0; nn < params->max_anneal_rounds; ++nn) {
        tv_penalty = max(params->min_lambda, tv_penalty * params->annealing_rate );
        int k = 0;
        for (int iter = 0; iter < params->max_iterations; ++iter) {
            // apply gradient descent step wrt image
            // step is: max|u| / max| \nabla u |
            ////double t_ = now_ms();
            t1 = now_ms();
            vector<float> gu = calc_grad_img(u, kernel, img_og, tv_penalty);
            scale = max(1e-3f, 
                        abs(*max_element(u.begin(), u.end())) / 
                        max(1e-10f, abs(*max_element(gu.begin(), gu.end() )) ) );
            for (int i = 0; i < u.size(); ++i ) {
                u[i] = u[i] - params->grad_descent_step_u*scale*gu[i];
            }
            ////printf("for image step: %f\n", now_ms()-t_);

            
            // apply gradient descent step wrt kernel variable
            // step is: max|k| / max| \nabla k |
            ////t_ = now_ms();
            vector<float> gk = calc_grad_kernel(u, kernel, img_og);
            scale = max(1e-3f, 
                    abs(*max_element(kernel.begin(), kernel.end())) / 
                    max(1e-10f, abs(*max_element(gk.begin(), gk.end() )) ) );
            for (int i = 0; i < kernel.size(); ++i ) {
                kernel[i] = kernel[i] - params->grad_descent_step_k*scale*gk[i];
            }
            ////printf("for kernel step: %f\n", now_ms()-t_);
            project_kernel_onto_feasible_set(kernel);
            elapsed = now_ms()-t1;
        }   
        fx_updated = false;
        if ((nn>0) && (nn % 10 ==0)) {
            fxprev = fx;
            fx = calc_func_value(u, kernel, img_og, tv_penalty);
            for (int i = 0; i < recent_dfx.size()-1; ++i) { recent_dfx[i]=recent_dfx[i+1]; }
            recent_dfx[ recent_dfx.size()-1 ] = (fxprev-fx)/fxprev; // >= 0
            fx_updated = true;
            avg_dfx = accumulate( recent_dfx.begin(), 
                                        recent_dfx.end(), 0.0f) / recent_dfx.size();
            if ( (nn > 10*recent_dfx.size()) && (avg_dfx < params->dfx_tolerance) ) {
                printf("Stopping: df(x)= %f < %f\n", avg_dfx, params->dfx_tolerance);
                break;
            }            
        }

        if ((params->verbosity>0) && (nn % params->verbosity == 0) && (nn>0)) {
            if (!fx_updated) {
                fx = calc_func_value(u, kernel, img_og, tv_penalty);
            }
            printf("%3d) fx= %.9g | dfx = %9g | elapsed: %f\n", nn, fx, avg_dfx, elapsed); 
            //
            // char fname[256];
            // vector<float> u_;
            // sprintf(fname, "out_%04d.png", nn);
            // u_ = interleave_channels(u, chan);
            // transform(u_.begin(), u_.end(), u_.begin(), bind1st( multiplies<float>(), 256.0f));
            // bcv_imwrite<float>(fname, u_, rows, cols, chan);

            // sprintf(fname, "kernel_%04d.png", nn);
            // u_ = kernel;
            // transform(u_.begin(), u_.end(), u_.begin(), bind1st( multiplies<float>(), 256.0f));
            // bcv_imwrite<float>(fname, u_, rows_ker, cols_ker, 1);
        }
    }
}

void tvdeblur::project_kernel_onto_feasible_set(vector<float>& kernel) {
    for (int i = 0; i < kernel.size(); ++i) {
        kernel[i] = max(0.0f, kernel[i]);
    }
    float sum_ker = accumulate(kernel.begin(), kernel.end(), 1e-14f);
    transform(kernel.begin(), kernel.end(), kernel.begin(),
                            bind1st( multiplies<float>(), 1.0f/sum_ker ));
}

vector<float> tvdeblur::result() { return interleave_channels(u, chan); }


// applies a gradient step to the grayscale image
// u      - optimization variable (rows_padded, cols_padded)
// kernel - blur function         (rows_ker, cols_ker)
// img_og - original blurry image (rows, cols)
vector<float> tvdeblur::calc_grad_img(const vector<float>& u, const vector<float>& kernel,
                            const vector<float>& img_og, float lambda) {
    
    vector<float> gu = fft_imfilter(u, rows, cols, chan, kernel, rows_ker, cols_ker, 1, 
                                                            TRUNCATION_VALID);
    for (int i = 0; i < gu.size(); ++i) { gu[i] -= img_og[i]; }

    vector<float> kernel_trans = time_reverse_signal( kernel, rows_ker, cols_ker, 1);
    
    gu = fft_imfilter(gu, rows_truncated, cols_truncated, chan, kernel_trans, 
                                        rows_ker, cols_ker, 1, TRUNCATION_FULL);
    // calculate TV gradient:
    int n = rows*cols;
    for (int ch = 0; ch < chan; ++ch) {
        vector<float> u_( u.begin() + n*ch, u.begin()+n*(ch+1) );
        vector<float> Du, Dtu;
        apply_pixelwise_gradient_op(Du, u_, rows, cols);
        for (int i = 0; i < n; ++i) {
            float den = sqrt( Du[i]*Du[i] + Du[i+n]*Du[i+n] ) + params->eps;
            Du[i]   /= den;
            Du[i+n] /= den;
        }
        apply_pixelwise_gradient_op_transpose(Dtu, Du, rows, cols); 
        // combine gradient
        for (int i = 0; i < n; ++i) { gu[i+ch*n] += lambda * Dtu[i]; }
    }
    return gu;
}

float tvdeblur::calc_func_value(const vector<float>& u, 
        const vector<float>& kernel, const vector<float>& img_og, float lambda) {
    
    float fx = 0.0f;
    vector<float> gu = fft_imfilter(u, rows, cols, chan, kernel, rows_ker, cols_ker, 1, 
                                                            TRUNCATION_VALID);
    for (int i = 0; i < gu.size(); ++i) { 
        fx += (gu[i]-img_og[i])*(gu[i]-img_og[i]);    
    }

    int n = rows*cols;
    for (int ch = 0; ch < chan; ++ch) {
        vector<float> Du;
        vector<float> u_(u.begin()+n*ch, u.begin()+n*(ch+1) );
        apply_pixelwise_gradient_op(Du, u_, rows, cols);
        for (int i = 0; i < n; ++i) { 
            float absval = sqrt( Du[i]*Du[i] + Du[i+n]*Du[i+n] );
            fx += lambda*absval;
        }
    }
    return fx;
}

vector<float> tvdeblur::calc_grad_kernel(const vector<float>& u, 
                                const vector<float>& kernel,
                                const vector<float>& img_og) {

    vector<float> gu = fft_imfilter(u, rows, cols, chan, kernel, rows_ker, cols_ker, 1, 
                                                            TRUNCATION_VALID);
    for (int i = 0; i < gu.size(); ++i) { gu[i] -= img_og[i]; }    
    vector<float> u_trans = time_reverse_signal(u, rows, cols, chan);
    vector<float> gk_ = fft_imfilter(u_trans, rows, cols, chan, gu, 
                            rows_truncated, cols_truncated, chan, TRUNCATION_VALID);
    vector<float> gk = vector<float>(kernel.size(), 0.0f);
    for (int ch = 0; ch < chan; ++ch) {
        for (int i = 0; i < kernel.size(); ++i) {
            gk[i] += gk_[i + kernel.size()*ch];
        }
    }
    return gk;
}

vector<float> tvdeblur::fft_imfilter(
                            const vector<float>& im1, int rows1, int cols1, int chan1, 
                            const vector<float>& im2, int rows2, int cols2, int chan2,
                            truncation_type type) {
    fftwf_plan p0, p1, p2;
    int rows_padded = rows1 + rows2 - 1;
    int cols_padded = cols1 + cols2 - 1;

    img_p1 = (float*)fftwf_malloc( sizeof(float)*(rows_padded*cols_padded) );
    ker_p1 = (float*)fftwf_malloc( sizeof(float)*(rows_padded*cols_padded) );

    img_p2 = (fftwf_complex*)fftwf_malloc( sizeof(fftwf_complex)*(rows_padded*(cols_padded/2+1) ) );
    ker_p2 = (fftwf_complex*)fftwf_malloc( sizeof(fftwf_complex)*(rows_padded*(cols_padded/2+1) ) );

    // get data out.
    vector<float> out;
    if (type == TRUNCATION_FULL) {
        out = vector<float>(rows_padded*cols_padded*chan1);
    } else if (type == TRUNCATION_SAME) { // output is the same size as the original    
        out = vector<float>(rows1*cols1*chan1);
    } else if (type == TRUNCATION_VALID) { // output image is trunctated
        int rows_truncated = rows1-rows2+1;
        int cols_truncated = cols1-cols2+1;
        out = vector<float>(rows_truncated*cols_truncated*chan1);
    } else {
        printf("error (unknown truncation_type).\n");
        return vector<float>();
    }

    // -------------------------------------------------------------------------
    //                          take FFT of the kernel 2.
    // -------------------------------------------------------------------------
    if (chan2 == 1) {
        memset(ker_p1, 0, sizeof(float)*rows_padded*cols_padded);
        for (int r = 0; r < rows2; ++r) {
            memcpy( &ker_p1[ linear_index(r,0,cols_padded) ], 
                    &im2[linear_index(r,0,cols2)], sizeof(float)*cols2 );
        }       
        p0 = fftwf_plan_dft_r2c_2d(rows_padded, cols_padded, ker_p1, ker_p2, FFTW_MEASURE);
        fftwf_execute(p0);
        fftwf_destroy_plan(p0);
    }

    p1 = fftwf_plan_dft_r2c_2d(rows_padded, cols_padded, img_p1, img_p2, FFTW_MEASURE);
    p2 = fftwf_plan_dft_c2r_2d(rows_padded, cols_padded, img_p2, img_p1, FFTW_MEASURE);
    for (int ch = 0; ch < chan1; ++ch) {
        if (chan2 > 1) {
            memset(ker_p1, 0, sizeof(float)*rows_padded*cols_padded);
            for (int r = 0; r < rows2; ++r) {
                memcpy( &ker_p1[ linear_index(r,0,cols_padded) ], 
                        &im2[linear_index(r,0,cols2)+rows2*cols2*ch], sizeof(float)*cols2 );
            }
            if (ch == 0) {
                p0 = fftwf_plan_dft_r2c_2d(rows_padded, cols_padded, 
                                            ker_p1, ker_p2, FFTW_MEASURE);
            }
            fftwf_execute(p0);
            if (ch == chan2-1) { fftwf_destroy_plan(p0); }
        }

        memset( img_p1, 0, sizeof(float)*rows_padded*cols_padded );
        for (int r = 0; r < rows1; ++r) { 
            memcpy( &img_p1[ linear_index(r,0,cols_padded) ], 
                    &im1[linear_index(r,0,cols1)+ch*rows1*cols1], sizeof(float)*cols1 );
        }
        fftwf_execute(p1);
        if (ch == chan1-1) { fftwf_destroy_plan(p1); }
        // multiply two together
        for (int i = 0; i < rows_padded*(cols_padded/2+1); ++i) {
            float r_ = (img_p2[i][0] * ker_p2[i][0] - 
                        img_p2[i][1] * ker_p2[i][1])/(rows_padded*cols_padded);
            float c_ = (img_p2[i][0] * ker_p2[i][1] + 
                        img_p2[i][1] * ker_p2[i][0])/(rows_padded*cols_padded);
            img_p2[i][0] = r_;
            img_p2[i][1] = c_;
        }
        // -------------------------------------------------------------------------
        //                           take inverse fft
        // -------------------------------------------------------------------------
        fftwf_execute(p2);
        if (ch == chan1-1) { fftwf_destroy_plan(p2); }

        if (type == TRUNCATION_FULL) {
            int n = rows_padded*cols_padded;
            memcpy(&out[ch*n], img_p1, sizeof(float)*rows_padded*cols_padded);
        } else if (type == TRUNCATION_SAME) { // output is the same size as the original    
            int r_offset = rows2/2;
            int c_offset = cols2/2;
            int n = rows1*cols1;
            for (int r = 0; r < rows1; ++r) {
                memcpy( &out[ ch*n + linear_index(r,0,cols1) ], 
                        &img_p1[ linear_index(r+r_offset, c_offset, cols_padded) ],
                        sizeof(float)*cols1 );
            }
        } else if (type == TRUNCATION_VALID) { // output image is trunctated
            int rows_truncated = rows1-rows2+1;
            int cols_truncated = cols1-cols2+1;
            int r_offset = rows2-1;
            int c_offset = cols2-1;
            int n = rows_truncated*cols_truncated;
            for (int r = 0; r < rows_truncated; ++r) {
                memcpy( &out[ ch*n + linear_index(r,0,cols_truncated) ], 
                        &img_p1[ linear_index(r+r_offset, c_offset, cols_padded) ],
                        sizeof(float)*cols_truncated );
            }
        }
    }

    fftwf_free(img_p1); img_p1 = NULL;
    fftwf_free(img_p2); img_p2 = NULL;
    fftwf_free(ker_p1); ker_p1 = NULL;
    fftwf_free(ker_p2); ker_p2 = NULL;
    return out;
}

vector<float> tvdeblur::time_reverse_signal(const vector<float>& in, int rows, int cols, int chan) {
    vector<float> out = vector<float>( in.size() );
    for (int ch = 0; ch < chan; ++ch) {
        for (int i = 0; i < rows*cols; ++i) {
            int r = getrow(i, cols);
            int c = getcol(i, cols);
            out[ linear_index(rows-1-r, cols-1-c, cols) + ch*rows*cols] = in[i+ch*rows*cols];
        }
    }
    return out;
}

void tvdeblur::print_kernel() {
    for (int r = 0; r < rows_ker; ++r) {
        for (int c = 0; c < cols_ker; ++c) {
            printf("%.2g ", kernel[ linear_index(r,c,cols_ker) ] );
        }
        printf("\n");
    }
}

vector<float> tvdeblur::stack_channels(const vector<float>& in, int chan) {
    if (chan == 1) { return in; }
    int n = in.size()/chan;
    vector<float> out = vector<float>(in.size());
    for (int ch = 0; ch < chan; ++ch) {
        for (int i = 0; i < n; ++i) { out[i+n*ch] = in[chan*i + ch]; }
    }
    return out;
}
vector<float> tvdeblur::interleave_channels(const vector<float>& in, int chan) {
    if (chan == 1) { return in; }
    int n = in.size()/chan;
    vector<float> out = vector<float>(in.size());
    for (int ch = 0; ch < chan; ++ch) {
        for (int i = 0; i < n; ++i) { out[chan*i + ch] = in[i+n*ch]; }
    }
    return out;
}