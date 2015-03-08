//! @file bcv_io.cpp
#ifndef BCV_IO_H_
#define BCV_IO_H_
 
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include <sys/stat.h>

#include "rw_png.h"
#include "rw_jpeg.h"
using namespace std;

#ifndef BCV_UCHAR_TYPEDEF_
#define BCV_UCHAR_TYPEDEF_
typedef unsigned char uchar;
#endif

//! Writes 'jpg' or 'png' image.
template <class T> void bcv_imwrite(const char* fname, 
                const vector<T>& x, int rows, int cols, int chan) {
    vector<unsigned char> img;
    img.assign(x.begin(), x.end());
    if (isJPG(fname)) {
        int quality = 100;
        write_jpeg_file(fname, &img[0], cols, rows, chan, quality);
    } else if (isPNG(fname)) {
        if (write_png_file(fname, &img[0], cols, rows, chan)) {
            printf("couldnt write image %s\n", fname);
        }
    } else {
        printf("extension of %s doesnt appear to be 'png' or 'jpg'.\n", fname);
        printf("cannot write image.\n");
    }
}

//! Loads image from 'fname'.
template <class T> vector<T> bcv_imread(const char* fname, int* rows, int* cols, int* chan) { 
    unsigned char* img=NULL;
    if (isJPG(fname)) {
        if (read_jpeg_file(fname, &img, cols, rows, chan)) {
            printf("couldnt read jpg file %s.\n", fname);
            return vector<T>();
        }
    } else if (isPNG(fname)) {
        if (read_png_file(fname, &img, cols, rows, chan)) {
            printf("couldnt read png file %s.\n", fname);
            return vector<T>();
        }
   } else {
        printf("extension of %s doesnt appear to be 'png' or 'jpg'.\n", fname);
        printf("cannot read image.\n");
        return vector<T>();
    }
    int n = (*rows)*(*cols)*(*chan);    
    vector<T> x;
    x.assign(img, img+n);
    if (img!=NULL) { free(img); }
    return x;
}

//! Returns 1 if file exists, 0 otherwise.
inline int file_exists(const char* fname) {
  struct stat buffer;
  return (stat(fname, &buffer) == 0); 
}



//! Returns a vector of lines from file.
vector<string> read_file_lines(const char* fname);

#endif
