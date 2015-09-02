#include "tvdeblur.h"
namespace bcv {

void tvdeblur_params::print() { 
    printf("deblurring parameters.\n");
    printf("---------------------\n");
    printf("lambda = %f (min=%f)\n", lambda, min_lambda );
    printf("annealing rate = %f\n", annealing_rate );
    printf("grad step: img = %f, ker = %f\n", grad_descent_step_u, grad_descent_step_k );
    printf("kernel size: %zu\n", ker_size);
    printf("max iterations: %zu, max anneal rounds: %zu\n", max_inner_iterations, max_anneal_rounds );
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
        scales_data[i].rows_padded = scales_data[i].rows + scales_data[i].rows_ker-1;
        scales_data[i].cols_padded = scales_data[i].cols + scales_data[i].cols_ker-1;
        scales_data[i].lambda = lamscale*p.lambda;
    }
    img_blur_original = img;
}


vector<float> tvdeblur::pad_image(const vector<float>& in, 
                        size_t rows_in, size_t cols_in, size_t rows_out, size_t cols_out) {
    vector<float> out = vector<float>(rows_out*cols_out*chan, 1.0f);
    return out;

    size_t n_in = rows_in*cols_in;
    size_t n_out = rows_out*cols_out;
    int r_offset = (rows_out-rows_in)/2;
    int c_offset = (cols_out-cols_in)/2;

    assert( (r_offset >= 0 && c_offset >= 0) && "sane offsets" );
    assert( (rows_in*cols_in*chan == in.size()) && "sane input size" );
    for (size_t ch = 0; ch < chan; ++ch) {
        float* out_ch = &out[ch*n_out];
        const float* in_ch = &in[ch*n_in];
        
        for (size_t r = 0; r < rows_in; ++r) { 
            for (size_t c = 0; c < cols_in; ++c) {
                out_ch[ linear_index(r+r_offset, c+c_offset, cols_out) ] = 
                in_ch[ linear_index(r, c, cols_in) ]; 
            }
        }
        for (size_t r = 0; r < rows_out; ++r) {
            for (size_t c = 0; c < cols_out; ++c) {
                if ((int(r) >= r_offset) && (r < r_offset+rows_in) && 
                    (int(c) >= c_offset) && (c < c_offset+cols_in)) {
                    continue;
                }                
                int r_ = min(max(0, int(r)-r_offset), int(rows_in-1) );
                int c_ = min(max(0, int(c)-c_offset), int(cols_in-1) );
                out_ch[ linear_index(r, c, cols_out) ] = 
                    in_ch[ linear_index( r_, c_, cols_in) ];
            }
        }
    }
    return out;
}

vector<float> tvdeblur::unpad_image(const vector<float>& in, 
                        size_t rows_in, size_t cols_in, size_t rows_out, size_t cols_out) {
    vector<float> out = vector<float>(rows_out*cols_out*chan, 1.0f);
    size_t n_in = rows_in*cols_in;
    size_t n_out = rows_out*cols_out;
    int r_offset = (rows_in-rows_out)/2;
    int c_offset = (cols_in-cols_out)/2;

    assert( (r_offset >= 0 && c_offset >= 0) && "sane offsets" );
    assert( (rows_in*cols_in*chan == in.size()) && "sane input size" );
    for (size_t ch = 0; ch < chan; ++ch) {
        for (size_t r = 0; r < rows_out; ++r) { 
            for (size_t c = 0; c < cols_out; ++c) {
                out[ linear_index(r, c, cols_out) + ch*n_out] = 
                in[ linear_index(r+r_offset, c+c_offset, cols_in) + ch*n_in]; 
            }
        }
    }
    return out;
}

void tvdeblur::solve() {
    chan = params->chan;
    
    for (size_t s = 0; s < scales_data.size(); ++s) {
        printf("scale = %zu\n", s);
        // get sizes:
        rows = scales_data[s].rows;
        cols = scales_data[s].cols;
        rows_ker = scales_data[s].rows_ker;
        cols_ker = scales_data[s].cols_ker;
        rows_padded = scales_data[s].rows_padded;
        cols_padded = scales_data[s].cols_padded;
        float tv_penalty = scales_data[s].lambda;

        u = stack_channels(img_blur_original, chan); 
        u = pad_image(u, rows, cols, rows_padded, cols_padded);

        // resize image to appropriate size, to serve as "noisy image"...
        vector<float> img = imresize(img_blur_original, params->rows, params->cols, rows, cols);
        img_og = stack_channels(img, chan);

        if (s == 0) {
            u = stack_channels(img, chan); 
            u = pad_image(u, rows, cols, rows_padded, cols_padded);
  
            if (params->kernel_init_type == tvdeblur_params::KERNEL_INIT_FLAT) { 
                // initialize with a flat kernel
                kernel = vector<float>(rows_ker*cols_ker, 1.0f/(rows_ker*cols_ker) );
            } else if (params->kernel_init_type == tvdeblur_params::KERNEL_INIT_DELTA) {
                // initialize with a delta kernel
                kernel = vector<float>(rows_ker*cols_ker, 0.0f);
                int mid_y = (rows_ker % 2 == 0) ? rows_ker/2+1 : rows_ker/2;
                int mid_x = (cols_ker % 2 == 0) ? cols_ker/2+1 : cols_ker/2;
                kernel[ linear_index(mid_y, mid_x, cols_ker) ] = 1.0f;
            } else {
                printf("unrecognized kernel init type\n.");
                kernel = vector<float>(rows_ker*cols_ker, 0.0f);
            }
        } else { // initialize from the previous iteration
            u = interleave_channels(u, chan);
            u = imresize(u, scales_data[s-1].rows_padded, scales_data[s-1].cols_padded, 
                            rows_padded, cols_padded);
            u = stack_channels(u, chan);
            
            kernel = imresize(kernel, scales_data[s-1].rows_ker, scales_data[s-1].cols_ker, rows_ker, cols_ker);
            project_kernel_onto_feasible_set(kernel);
        }
        solve_inner(tv_penalty);
    }
}

void tvdeblur::solve_inner(float tv_penalty) {
    float scale, bls_step_u, bls_step_k; scale = bls_step_u = bls_step_k = 1.0f;
    double t1, elapsed;
    float fx=0, fxprev=0, avg_dfx=0;
    vector<float> recent_dfx = vector<float>(10, 1.0);
    bool fx_updated;

    for (size_t nn = 0; nn < params->max_anneal_rounds; ++nn) {
        tv_penalty = max(params->min_lambda, tv_penalty * params->annealing_rate );
        //size_t k = 0; // TODO: why is this here?
        for (size_t iter = 0; iter < params->max_inner_iterations; ++iter) {
            // apply gradient descent step wrt image
            // step is: max|u| / max| \nabla u |
            t1 = now_ms();
            vector<float> gu = calc_grad_img(u, kernel, img_og, tv_penalty);
            scale = params->grad_descent_step_u * max(1e-7f, 
                        abs(*max_element(u.begin(), u.end())) / 
                        max(1e-10f, abs(*max_element(gu.begin(), gu.end() )) ) );

            for (size_t i = 0; i < u.size(); ++i ) {
                u[i] = u[i] - scale*gu[i];
            }
            // apply gradient descent step wrt kernel variable
            // step is: max|k| / max| \nabla k |
            ////t_ = now_ms();
            vector<float> gk = calc_grad_kernel(u, kernel, img_og);
            scale = params->grad_descent_step_k * max(1e-7f, 
                    abs(*max_element(kernel.begin(), kernel.end())) / 
                    max(1e-10f, abs(*max_element(gk.begin(), gk.end() )) ) );

            for (size_t i = 0; i < kernel.size(); ++i ) {
                kernel[i] = kernel[i] - scale*gk[i];
            }
            project_kernel_onto_feasible_set(kernel);
            elapsed = now_ms()-t1;
        }   
        fx_updated = false;
        if ((nn>0) && (nn % 10 ==0)) {
            fxprev = fx;
            fx = calc_func_value(u, kernel, img_og, tv_penalty);
            for (size_t i = 0; i < recent_dfx.size()-1; ++i) { recent_dfx[i]=recent_dfx[i+1]; }
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
            printf("%3zu) fx= %.9g | dfx = %9g | elapsed: %f\n", nn, fx, avg_dfx, elapsed); 
            //
            if (params->vis_results) {
                char fname[256];
                vector<float> u_;
                sprintf(fname, "out_%04zu.png", nn);
                u_ = interleave_channels(u, chan);
                transform(u_.begin(), u_.end(), u_.begin(), bind1st( multiplies<float>(), 256.0f));
                bcv_imwrite(fname, u_, rows_padded, cols_padded, chan);

                sprintf(fname, "kernel_%04zu.png", nn);
                u_ = kernel;
                transform(u_.begin(), u_.end(), u_.begin(), 
                    bind1st( multiplies<float>(), 256.0f/ *max_element(u_.begin(), u_.end()) ));
                bcv_imwrite(fname, u_, rows_ker, cols_ker, 1);
            }
        }
    }
    solve_nonblind(kernel, tv_penalty, params->max_nonblind_iterations);
}

void tvdeblur::solve_nonblind(const vector<float>& k, float tv_penalty, size_t num_iters) {
    float scale = 1.0f;
    for (size_t iter = 0; iter < num_iters; ++iter) {
        vector<float> gu = calc_grad_img(u, k, img_og, tv_penalty);
        scale = max(1e-3f, 
                    abs(*max_element(u.begin(), u.end())) / 
                    max(1e-10f, abs(*max_element(gu.begin(), gu.end() )) ) );
        for (size_t i = 0; i < u.size(); ++i ) { u[i] -= scale*gu[i]; }  
    }   
}

void tvdeblur::project_kernel_onto_feasible_set(vector<float>& kernel) {
    for (size_t i = 0; i < kernel.size(); ++i) {
        kernel[i] = max(0.0f, kernel[i]);
    }
    float sum_ker = accumulate(kernel.begin(), kernel.end(), 1e-14f);
    transform(kernel.begin(), kernel.end(), kernel.begin(),
                            bind1st( multiplies<float>(), 1.0f/sum_ker ));
}

vector<float> tvdeblur::result() { 
    vector<float> I = unpad_image(u, rows_padded, cols_padded, rows, cols);
    return interleave_channels(I, chan); 
}

vector<float> tvdeblur::get_kernel() { return kernel; }

// applies a gradient step to the grayscale image
// u      - optimization variable (rows_padded, cols_padded)
// kernel - blur function         (rows_ker, cols_ker)
// img_og - original blurry image (rows, cols)
vector<float> tvdeblur::calc_grad_img(const vector<float>& u, const vector<float>& kernel,
                            const vector<float>& img_og, float lambda) {

    vector<float> gu = fft_imfilter(u, rows_padded, cols_padded, chan, kernel, rows_ker, cols_ker, 1, 
                                                            TRUNCATION_VALID);
    for (size_t i = 0; i < gu.size(); ++i) { gu[i] -= img_og[i]; }
    vector<float> kernel_trans = time_reverse_signal( kernel, rows_ker, cols_ker, 1);    
    gu = fft_imfilter(gu, rows, cols, chan, kernel_trans, 
                                        rows_ker, cols_ker, 1, TRUNCATION_FULL);

    // calculate TV gradient:
    size_t n = rows_padded*cols_padded;
    vector<float> Du(2*n);
    vector<float> Dtu(n);
    for (size_t ch = 0; ch < chan; ++ch) {
        apply_pixelwise_gradient_op(&Du[0], &u[0]+n*ch, rows_padded, cols_padded);
        for (size_t i = 0; i < n; ++i) {
            float den = sqrt( Du[i]*Du[i] + Du[i+n]*Du[i+n] ) + params->eps;
            Du[i]   /= den;
            Du[i+n] /= den;
        }
        apply_pixelwise_gradient_op_transpose(Dtu, Du, rows_padded, cols_padded); 
        // combine gradient
        for (size_t i = 0; i < n; ++i) { gu[i+ch*n] += lambda * Dtu[i]; }
    }
    return gu;
}

float tvdeblur::calc_func_value(const vector<float>& u, 
        const vector<float>& kernel, const vector<float>& img_og, float lambda) {
    
    float fx = 0.0f;
    vector<float> gu = fft_imfilter(u, rows_padded, cols_padded, chan, 
                                    kernel, rows_ker, cols_ker, 1, 
                                                    TRUNCATION_VALID);
    for (size_t i = 0; i < gu.size(); ++i) { 
        fx += (gu[i]-img_og[i])*(gu[i]-img_og[i]);    
    }

    size_t n = rows_padded*cols_padded;
    float fxtv = 0.0f;
    vector<float> Du(2*n);
    for (size_t ch = 0; ch < chan; ++ch) {
        apply_pixelwise_gradient_op(&Du[0], &u[0]+n*ch, rows_padded, cols_padded);        
        for (size_t i = 0; i < n; ++i) { 
            float absval = sqrt( Du[i]*Du[i] + Du[i+n]*Du[i+n] );
            fxtv += lambda*absval;
        }
    }
    fx += fxtv;

    return fx;
}

vector<float> tvdeblur::calc_grad_kernel(const vector<float>& u, 
                                const vector<float>& kernel,
                                const vector<float>& img_og) {

    vector<float> gu = fft_imfilter(u, rows_padded, cols_padded, chan, 
                                    kernel, rows_ker, cols_ker, 1, 
                                                            TRUNCATION_VALID);
    for (size_t i = 0; i < gu.size(); ++i) { gu[i] -= img_og[i]; }    
    vector<float> u_trans = time_reverse_signal(u, rows_padded, cols_padded, chan);
    vector<float> gk_ = fft_imfilter(u_trans, rows_padded, cols_padded, chan, gu, 
                            rows, cols, chan, TRUNCATION_VALID);
    vector<float> gk = vector<float>(kernel.size(), 0.0f);
    
    for (size_t ch = 0; ch < chan; ++ch) {
        for (size_t i = 0; i < kernel.size(); ++i) {
            gk[i] += gk_[i + kernel.size()*ch];
        }
    }
    return gk;
}


//! im1, rows1, cols1, chan1 -- first image
//! im2, rows2, cols2, chan2 -- second image
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
        p0 = fftwf_plan_dft_r2c_2d(rows_padded, cols_padded, 
                                                ker_p1, ker_p2, FFTW_MEASURE);

        memset(ker_p1, 0, sizeof(float)*rows_padded*cols_padded);
        for (int r = 0; r < rows2; ++r) {
            memcpy( &ker_p1[ linear_index(r,0,cols_padded) ], 
                    &im2[linear_index(r,0,cols2)], sizeof(float)*cols2 );
        }
        fftwf_execute(p0);
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
            fftwf_execute(p0); // TODO: fix warning here
        }

        memset( img_p1, 0, sizeof(float)*rows_padded*cols_padded );
        for (int r = 0; r < rows1; ++r) { 
            memcpy( &img_p1[ linear_index(r,0,cols_padded) ], 
                    &im1[linear_index(r,0,cols1)+ch*rows1*cols1], sizeof(float)*cols1 );
        }
        fftwf_execute(p1);
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
    fftwf_destroy_plan(p0);
    fftwf_destroy_plan(p1);
    fftwf_destroy_plan(p2);

    fftwf_free(img_p1); img_p1 = NULL;
    fftwf_free(img_p2); img_p2 = NULL;
    fftwf_free(ker_p1); ker_p1 = NULL;
    fftwf_free(ker_p2); ker_p2 = NULL;
    return out;
}

vector<float> tvdeblur::time_reverse_signal(const vector<float>& in, int rows, int cols, int chan) {
    vector<float> out = vector<float>( in.size() );
    int n = rows*cols;
    assert( (in.size()/chan == n)  && "sane argument size" );

    for (int ch = 0; ch < chan; ++ch) {
        for (int i = 0; i < n; ++i) {
            int r = getrow(i, cols);
            int c = getcol(i, cols);
            out[ linear_index(rows-1-r, cols-1-c, cols) + ch*n] = in[i+ch*n];
        }
    }
    return out;
}

} // namespace bcv
