#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <cstring>

#include "png.h"
namespace bcv {
using namespace std;

/* This code is based on example.c included in libpng package. */

/*! reads in a png image from file 'file_name' into an array 'img'
    of unsigned chars. memory is allocated inside the function.
    image size (width, height, channels) are returned as well. */
int read_png_file(const char *file_name, unsigned char** img, int* width, int* height,
                  int* channels);

/*! write a png image to file 'file_name' using data in 'img'. 
	returns 0 if OK, 1 if ERROR.
*/
int write_png_file(const char *file_name, unsigned char* img, int width, int height, int channels);

/*! check the extension as the MOST BASIC POSSIBLE check for
    whether a file is png or not. */
bool isPNG(const char* fname);

} // namespace bcv 