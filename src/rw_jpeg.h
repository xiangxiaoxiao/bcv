#ifndef RW_JPEG_H_
#define RW_JPEG_H_

#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <cstring>
#include "jpeglib.h"
#include "setjmp.h"

namespace bcv {
using namespace std;
/* This code is based on example.c included in libjpeg package. */

/*! helper for reading jpeg images */
void put_scanline(unsigned char*, int, unsigned char*, int);

/*! write a jpeg image to file 'filename' using data in 'img'. */
void write_jpeg_file(const char * filename, unsigned char* img, int width, int height, int channels,
                     int quality);

/*! reads in a jpeg image from file 'filename' into an array 'img'
    of unsigned chars. memory is allocated inside the function.
    image size (width, height, channels) are returned as well. */
int read_jpeg_file(const char * filename, unsigned char** img, int* width, int* height,
                   int* channels);

struct my_error_mgr {
    struct jpeg_error_mgr pub;  /* "public" fields */
    jmp_buf setjmp_buffer;      /* for return to caller */
};

bool isJPG(const char* fname);

} // namespace bcv
#endif  // RW_JPEG_H_
