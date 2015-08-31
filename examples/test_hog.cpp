#include <cstdlib>
#include <sstream>
#include <string>
#include "bcv_io.h"
#include "hog.h"

#include <gflags/gflags.h>

using namespace std;
using namespace bcv;
/*
DEFINE_string(input, "/home/vasiliy/data/INRIAPerson/train_64x128_H96/pos.lst","input data");
DEFINE_string(output, "out.jpg","output image");
DEFINE_int32(cell_size, 8, "HOG cell size");
DEFINE_int32(num_orientations, 9, "HOG num orientations");

int main(int argc, char** argv) { 
    gflags::SetUsageMessage("HOG example");
    gflags::ParseCommandLineFlags(&argc, &argv, false);
   
    string base_path("/home/vasiliy/data/INRIAPerson/");
    string img_path( base_path + "96X160H96/");
    string fname_pos( base_path + "train_64x128_H96/pos.lst");
    string fname_neg( base_path + "train_64x128_H96/neg.lst");
    vector<string> pos_files = read_file_lines( fname_pos.c_str() );
    int rows, cols, chan;
    double t1, t2;
    for (int i = 0; i < pos_files.size(); ++i) { 
        string fname(img_path + pos_files[i]);
        printf("%d) %s\n", i, fname.c_str() );
        
        string fname_hog(img_path+pos_files[i]);
        set_extension(fname_hog, "hog");
        // check if HOG file exists for this file.
        if ( file_exists(fname_hog.c_str()) ) { 
            //continue;
            Hog descr(fname_hog.c_str());
            int rows_vis, cols_vis;
            vector<float> vis = descr.vis(&rows_vis, &cols_vis);
            char temp[128]; sprintf(temp, "%04d.png", i);
            bcv_imwrite(temp, vis, rows_vis, cols_vis, 1, true);
        } else {

            // compute hog descriptor
            vector<float> img = bcv_imread<float>(fname.c_str(), &rows, &cols, &chan);
            t1 = now_ms();
            Hog descr(img, rows, cols, FLAGS_cell_size, FLAGS_num_orientations);
            t2 = now_ms();
            descr.write( fname_hog.c_str() );
            printf("computed hog on %dx%d image, took %f ms\n", rows, cols, t2-t1);
        }
    }
}
 */

DEFINE_string(input, "../images/pedestrian.jpg","input image");
DEFINE_string(output, "out.jpg","output image");
DEFINE_int32(cell_size, 8, "HOG cell size");
DEFINE_int32(num_orientations, 9, "HOG num orientations");

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
}

