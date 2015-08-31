#include <cstdlib>
#include <cstdio>
#include "tvdeblur.h"
#include "tvdeblur_nonblind.h"
#include "bcv_io.h"
#include <algorithm>

#include <gflags/gflags.h>

using namespace std;

DEFINE_string(input, "../images/blur/lena_blur.png", "Input image filename");
DEFINE_string(output, "out.png", "Output image filename");
DEFINE_double(lambda, 0.5, "TV penalty");
DEFINE_int32(kernel_size, 15, "kernel size");
DEFINE_int32(verbosity, 100, "verbosity");
DEFINE_int32(max_inner_iterations, 1, "max inner iterations");
DEFINE_int32(max_nonblind_iterations, 100, "max nonblind iterations");
DEFINE_int32(max_anneal_rounds, 1, "max anneal iterations");
// pyramid parameters
DEFINE_double(annealing_rate, 0.999, "annealing rate");
DEFINE_int32(pyramid_num_levels, 1, "num pyramid levels");
DEFINE_double(pyramid_imsize_scale, 0.90, "pyramid image scaling");
DEFINE_double(pyramid_lambda_scale, 1.90, "pyramid lambda scaling");
// gradient step parameters
DEFINE_double(grad_descent_step_u, 5e-3, "pyramid image scaling");
DEFINE_double(grad_descent_step_k, 1e-3, "pyramid lambda scaling");
DEFINE_double(dfx_tolerance, 1e-5f, "pyramid lambda scaling");
DEFINE_bool(grayscale, false, "solve grayscale?");

int main(int argc, char** argv) { 
    gflags::SetUsageMessage("TV-regularized segmentation example");
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    double t1, t2;
    int rows, cols, chan;
    vector<float> img = bcv::bcv_imread<float>(FLAGS_input.c_str(), &rows, &cols, &chan);
    transform(img.begin(), img.end(), img.begin(), 
                    bind1st( multiplies<float>(), 1.0f/256.0f ) );
    if ((FLAGS_grayscale) && (chan==3)) { 
        img = bcv::rgb2gray(img); chan =1; }
    printf("Loaded from '%s'\n", FLAGS_input.c_str() );
    bcv::tvdeblur_params params;

    params.lambda = FLAGS_lambda;
    params.min_lambda = 1e-4;
    params.annealing_rate = FLAGS_annealing_rate;
    params.max_inner_iterations = FLAGS_max_inner_iterations;
    params.max_nonblind_iterations = FLAGS_max_nonblind_iterations;
    params.max_anneal_rounds = FLAGS_max_anneal_rounds;
    params.grad_descent_step_u = FLAGS_grad_descent_step_u;
    params.grad_descent_step_k = FLAGS_grad_descent_step_k;
    params.eps = 1e-5f;
    params.ker_size = FLAGS_kernel_size;
    params.dfx_tolerance = FLAGS_dfx_tolerance;
    params.verbosity = FLAGS_verbosity;
    params.pyramid_numlevels = FLAGS_pyramid_num_levels;
    params.pyramid_imsize_scaling = FLAGS_pyramid_imsize_scale;
    params.pyramid_lambda_scaling = FLAGS_pyramid_lambda_scale;
    params.rows = rows;
    params.cols = cols;
    params.chan = chan;
    params.print();
    // ------------------------------------------------------------------------
    t1 = bcv::now_ms();
    bcv::tvdeblur tvdb(img, params);
    tvdb.solve();
    vector<float> out = tvdb.result();
    vector<float> ker = tvdb.get_kernel();
    t2 = bcv::now_ms();

    /*
    //transform(out.begin(), out.end(), out.begin(), 
    //          bind1st( multiplies<float>(), 256.0f ) );
    //bcv::bcv_imwrite(FLAGS_output.c_str(), out, rows, cols, chan);
    //printf("Wrote the result to '%s'\n", FLAGS_output.c_str() );
    printf("took %.3f s\n", (t2-t1)/1000.0f );
    
    //vector<float> out; 
    // we can now try the nonblind deconvolution..
    bcv::tvdeblur_nonblind_params params_nonblind;
    params_nonblind.rows = rows;
    params_nonblind.cols = cols;
    params_nonblind.chan = chan;
    params_nonblind.lambda = 1e-2;
    params_nonblind.kernel = vector<float>(params.ker_size*params.ker_size, 1.0f);
    params_nonblind.rows_ker = params.ker_size;
    params_nonblind.cols_ker = params.ker_size;

    bcv::tvdeblur_nonblind tvdb_nonblind;
    out = tvdb_nonblind.solve(img, params_nonblind);
    //transform(out.begin(), out.end(), out.begin(), 
    //                bind1st( multiplies<float>(), 256.0f ) );
    //bcv::bcv_imwrite("out_nonblind.png", out, rows, cols, chan);
    */
    return 0;
}
