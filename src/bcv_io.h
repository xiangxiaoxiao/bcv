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

namespace bcv {
using namespace std;

#ifndef BCV_UCHAR_TYPEDEF_
#define BCV_UCHAR_TYPEDEF_
typedef unsigned char uchar;
#endif

//! Writes 'jpg' or 'png' image.
template <class T> void bcv_imwrite(const char* fname, 
                const vector<T>& x, int rows, int cols, int chan=1, bool normalize=false) {
    vector<uchar> img = vector<uchar>(x.size());
    if (normalize) { 
        double ratio = 255.0/(*max_element(x.begin(), x.end() )); 
        for (int i = 0; i < x.size(); ++i) {
            T val = min( max( T(0), T(x[i]*ratio) ), T(255) );
            img[i] = (uchar)val;
        }
    } else {
        for (int i = 0; i < x.size(); ++i) {
            T val = min( max( T(0), T(x[i]) ), T(255) );
            img[i] = (uchar)val;
        }
    }

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

//! Loads image from 'fname' and converts to grayscale if needed
template <class T> vector<T> bcv_imread(const char* fname, int* rows, int* cols) { 
    int channels;
    vector<T> x = bcv_imread<T>(fname, rows, cols, &channels);
    if (channels==1) { return x; }
    // convert image to grayscale    
    int n = (*rows)*(*cols);
    vector<T> y = vector<T>(n);
    for (int i = 0; i < n; ++i) { 
        double u = 0.0f;
        for (int k = 0; k < channels; ++k) { 
            u += x[i*(channels)+k];
        }
        y[i] = u/=(channels);
    }
    return y;
}


//! Returns 1 if file exists, 0 otherwise.
inline int file_exists(const char* fname) {
  struct stat buffer;
  return (stat(fname, &buffer) == 0); 
}

//! Changes the end of s into 'ext'
inline void set_extension(string& s, const char* ext) { 
    size_t n = strlen(ext);
    if (s.size() < n) { return; }
    for (size_t i=s.size()-n,j=0; i<s.size(); ++i, ++j) { s[i]=ext[j]; }
}

//! given 'sup.blabla' returns 'sup'
inline string strip_extension(const string& s) { 
    return s.substr(0, s.find('.'));
}

//! Returns a vector of lines from file.
vector<string> read_file_lines(const char* fname);

//! \brief Returns true if extension matches a typical image extension 
bool is_image_file(const char* fname);
//! \brief Returns true if extension matches a typical video extension 
bool is_video_file(const char* fname);

} // namespace bcv
#endif // BCV_IO_H_
