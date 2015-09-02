#ifndef HOG_H_
#define HOG_H_

#include <cstdlib>
#include <cstdio>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include "bcv_utils.h"
#include "bcv_imgutils.h"
#include "bcv_imgproc.h"
#include "bcv_bbox.h"

namespace bcv {
using namespace std;

//! \brief HOG descriptor data structure
class Hog {
    public:
    vector<float> data;
    int rows = 0; // vertically number of cells 
    int cols = 0; // horizontally number of cells
    int num_orientations = 9;
    int cell_size = 8;  
    // block size is ALWAYS twice the cell size (as in Dalal Triggs 2005)
    // block stride is ALWAYS the cell_size (as in Dalal Triggs 2005)
   
    //! default constructor does nothing 
    Hog() {}
    //! constructs a descriptor given the image
    Hog(const vector<float>& img, int rows, int cols, int cell_size=8, int num_orientations=9);
    Hog(const vector<float>& img_gradnorm, const vector<float>& img_theta, 
        int rows, int cols, int cell_size=8, int num_orientations=9) {
        init(img_gradnorm, img_theta, rows, cols, cell_size, num_orientations);
    }
    //! constructs a descriptor from data file
    Hog(const char* fname) { read(fname); }
    Hog(const string& fname) { read(fname.c_str()); }
    //!

    //! hog visualization routine
    vector<float> vis();
    //! returns number of rows of the visualization image
    int vis_rows() { return this->rows*out_block_sz/2; }
    //! returns number of columsn of the visualiztion image
    int vis_cols() { return this->cols*out_block_sz/2; }
    //! write hog descriptor to file
    void write(const char* fname);
    //! read hog descriptor from file
    void read(const char* fname);

    private:
    void init(const vector<float>& img_gradnorm, const vector<float>& img_theta,
                        int rows, int cols, int cell_size, int num_orientations) noexcept;
    int out_block_sz = 16; // for visualization
    bool is_initialized();
};

} // namespace bcv

#endif // HOG_H_
    
    
