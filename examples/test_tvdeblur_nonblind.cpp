#include <cstdlib>
#include <sstream>
#include "bcv_imgproc.h"
#include "bcv_io.h"
#include "tvdeblur_nonblind.h"
#include <gflags/gflags.h>

using namespace std;
using bcv::uchar;
using namespace bcv;

DEFINE_string(input, "../images/peppers_blur_rgb.png","input image");
DEFINE_string(output, "out.jpg","output image");

int main(int argc, char** argv) { 
    gflags::SetUsageMessage("image/video processing example");
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    int rows, cols, chan;
    vector<float> img = bcv::bcv_imread<float>(
                        FLAGS_input.c_str(), &rows, &cols, &chan);
    transform( img.begin(), img.end(), img.begin(), 
                 bind1st( multiplies<float>(), 1.0f/256.0f) );
    
    tvdeblur_nonblind_params params;
    params.lambda = 1e-2f;
    params.rows = rows;
    params.cols = cols;
    params.chan = chan;
    
    int kernel_size = 15;
    params.kernel = vector<float>(kernel_size*kernel_size);
    for (int i = 0; i < kernel_size; ++i) { 
        params.kernel[kernel_size*(kernel_size/2)+i] = 1.0f/kernel_size; 
    }
    params.rows_ker = kernel_size;
    params.cols_ker = kernel_size;
    params.max_iterations = 5000;
    params.dx_tolerance = 1e-6f; 
    params.verbosity = 100;
    tvdeblur_nonblind tv;
    vector<float> out = tv.solve(img, params);
    
    transform( out.begin(), out.end(), out.begin(), 
                 bind1st( multiplies<float>(), 256.0f) );

    bcv::bcv_imwrite<float>(FLAGS_output.c_str(), out, rows, cols, chan);

    return 0;
}