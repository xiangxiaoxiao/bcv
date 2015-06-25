// Example 1: Read an image and perform superpixel segmentation.
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <ctime>

#include "Slic.h"
#include "SlicGraph.h"
#include "bcv_io.h"
#include "bcv_utils.h"

#include <gflags/gflags.h>

DEFINE_int32(K, 2000, "Number of superpixels");
DEFINE_int32(M, 5, "Geometry weight");
DEFINE_int32(num_iters, 10, "Number of iterations");
DEFINE_string(input, "../images/arches.jpg", "Input image filename");
DEFINE_int32(max_levels, 1, "Number of hierarchy levels");

using namespace std;
using bcv::uchar; //

int main(int argc, char** argv) {
    gflags::SetUsageMessage("SLIC superpixels example");
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    double t1, t2;
    int rows, cols, chan;
    vector<uchar> img = bcv::bcv_imread<uchar>(FLAGS_input.c_str(), &rows, &cols, &chan);

    bcv::Slic slic = bcv::Slic(img, rows, cols, chan, 
                        FLAGS_K, FLAGS_M, FLAGS_num_iters, FLAGS_max_levels);
    t1 = bcv::now_ms();
    vector<int> sp = slic.segment(); // basic segmentation call.
    t2 = bcv::now_ms();
    printf("Took %f ms to superpixellize\n", t2-t1);
    
    vector<uchar> matout = slic.get_boundary_image(img);
    bcv::bcv_imwrite<uchar>("out.png", matout, rows, cols, chan);
    printf("Writing visualization of segmentation to out.png\n");
    
    vector<bcv::SlicNode<uchar> > graph = bcv::construct_slic_graph<uchar>(img, sp, rows, cols, chan); 
    vector<uchar> avg_img = slic_average_image(graph, rows, cols, chan);
    bcv::bcv_imwrite<uchar>("out_avg.png", avg_img, rows, cols, chan);
    printf("Writing averaged image to out_avg.png\n");    
    return 0;
}
