#include <cstdlib>
#include <cstdio>
#include "tvdn.h"
#include "bcv_io.h"
#include "bcv_imgproc.h"
#include <algorithm>

#include <gflags/gflags.h>

using namespace std;

DEFINE_int32(max_iters, 100, "Maximum number of iterations");
DEFINE_double(lambda, 20.0, "TV weight");
DEFINE_double(dx_tolerance, 1e-4f, "dx tolerance");
DEFINE_int32(verbosity, 100000, "verbosity");
DEFINE_bool(isotropic, true, "isotropic? ");
DEFINE_bool(accelerated, true, "accelerated variant?");
DEFINE_double(gamma, 0.1, "acceleration gamma");
DEFINE_bool(grayscale, false, "grayscale image?");
DEFINE_string(input, "../images/arches.jpg", "Input image filename");
DEFINE_string(output, "out.png", "Output image filename");

int main(int argc, char** argv) { 
    gflags::SetUsageMessage("TV-regularized segmentation example");
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    double t1, t2;
    int rows, cols, chan;
    vector<float> img = bcv_imread<float>(FLAGS_input.c_str(), &rows, &cols, &chan);
    if (FLAGS_grayscale) {
        img = rgb2gray(img);
        chan = 1;
    }

    printf("Loaded from '%s'\n", FLAGS_input.c_str() );
    tvdn_params params = tvdn_params();
    params.lambda = FLAGS_lambda;
    params.isotropic = FLAGS_isotropic;    
    params.max_iterations = FLAGS_max_iters;
    params.dx_tolerance = FLAGS_dx_tolerance;
    params.verbosity = FLAGS_verbosity;
    params.accelerated = FLAGS_accelerated;
    params.gamma = FLAGS_gamma;
    params.rows = rows;
    params.cols = cols;
    params.chan = chan;

    params.print();
    // ------------------------------------------------------------------------
    t1 = now_ms();
    tvdn tv = tvdn(img, params);
    vector<float> out = tv.result();
    t2 = now_ms();
    bcv_imwrite<float>(FLAGS_output.c_str(), out, rows, cols, chan);
    printf("Wrote the result to '%s'\n", FLAGS_output.c_str() );
    printf("took %f ms\n", t2-t1);
    return 0;
}
