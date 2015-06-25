#include <cstdlib>
#include "bcv_io.h"
using std::vector;
using bcv::uchar;

int main() { 
    const char* in_fname = "../images/arches.jpg";
    const char* out_fname = "out.jpg";
    int rows, cols, chan;
   
    vector<uchar> img; 
    printf("Trying to read %s\n", in_fname);
    img = bcv::bcv_imread<uchar>(in_fname, &rows, &cols, &chan);
    printf("read image, size: %dx%dx%d\n", rows, cols, chan); 
    printf("Trying to write %s\n", out_fname);
    bcv::bcv_imwrite<uchar>(out_fname, img, rows, cols, chan);

    printf("Trying to read grayscale image %s\n", in_fname);
    img = bcv::bcv_imread<uchar>(in_fname, &rows, &cols);
    printf("read image, size: %dx%d\n", rows, cols); 
    printf("Trying to write %s\n", out_fname);
    bcv::bcv_imwrite<uchar>(out_fname, img, rows, cols);
 
    return 0;
}
