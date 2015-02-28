#include <cstdlib>
#include "bcv_io.cpp"

int main() { 
    const char* in_fname = "./images/arches.jpg";
    const char* out_fname = "out.jpg";
    int rows, cols, chan;
    
    printf("Trying to read %s\n", in_fname);
    vector<uchar> img = bcv_imread<uchar>(in_fname, &rows, &cols, &chan);
    printf("read image, size: %dx%dx%d\n", rows, cols, chan); 
    printf("Trying to write %s\n", out_fname);
    bcv_imwrite<uchar>(out_fname, img, rows, cols, chan);
    return 0;
}
