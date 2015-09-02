#include <cstdlib>
#include <sstream>
#include <string>
#include "bcv_io.h"
#include "hog.h"

#include <gflags/gflags.h>

using namespace std;
using namespace bcv;

DEFINE_string(input, "../images/pedestrian.jpg","input image");
DEFINE_string(output, "out.jpg","output image");
DEFINE_int32(cell_size, 8, "HOG cell size");
DEFINE_int32(num_orientations, 9, "HOG num orientations");
DEFINE_bool(timing, false, "do a timing experiment?");

void do_imgproc(vector<float>& img, int rows, int cols, int chan);
int main(int argc, char** argv) { 
    gflags::SetUsageMessage("HOG example");
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    double t1,t2;
    int rows, cols, chan;
    vector<float> img = bcv_imread<float>(FLAGS_input.c_str(), &rows, &cols, &chan);

    t1 = now_ms();
    Hog descr(img, rows, cols, FLAGS_cell_size, FLAGS_num_orientations);
    t2 = now_ms();
    printf("computed hog on %dx%d image, took %f ms\n", rows, cols, t2-t1);
    vector<float> hog_vis = descr.vis();
    int out_rows = descr.vis_rows();
    int out_cols = descr.vis_cols();
    printf("%d %d\n", out_rows, out_cols);
    bool normalize_image = true;
    bcv_imwrite(FLAGS_output.c_str(), hog_vis, out_rows, out_cols, 1, normalize_image);
    printf("saved HOG visualization %s\n", FLAGS_output.c_str());

    if (FLAGS_timing) {
        // this is a poor timing test (who knows how the compiler will optimize this loop)
        // but it will work for our purposes.
        t1 = now_ms();
        for (int k=0; k < 1000; ++k) {
            Hog descr(img, rows, cols, FLAGS_cell_size, FLAGS_num_orientations);
        }
        t2 = now_ms();
        printf("computed hog for timing purposes: %f ms\n", (t2-t1)/1000 );
    }
}

