#define png_infopp_NULL (png_infopp)NULL
#define int_p_NULL (int*)NULL
#include "rw_png.h"

namespace bcv {
int read_png_file(const char *file_name, unsigned char** img, int* width, int* height,
                  int* channels) {
    int ERROR = 1;
    int OK = 0;
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned int sig_read = 0;
    png_bytepp row_pointers;
    FILE *fp;

    if ((fp = fopen(file_name, "rb")) == NULL) {
        fprintf(stderr, "can't open %s\n", file_name);
        return (ERROR);
    }
    /* Create and initialize the png_struct with the desired error handler
     * functions.  If you want to use the default stderr and longjump method,
     * you can supply NULL for the last three parameters.  We also supply the
     * the compiler header file version, so that we know if the application
     * was compiled with a compatible version of the library.  REQUIRED
     */

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fclose(fp);
        return (ERROR);
    }

    /* Allocate/initialize the memory for image information.  REQUIRED. */
    // TODO???
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
        return (ERROR);
    }

    /* Set error handling if you are using the setjmp/longjmp method (this is
     * the normal method of doing things with libpng).  REQUIRED unless you
     * set up your own error handlers in the png_create_read_struct() earlier.
     */
    if (setjmp(png_jmpbuf(png_ptr))) {
        /* Free all of the memory associated with the png_ptr and info_ptr */
        png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
        fclose(fp);
        /* If we get here, we had a problem reading the file */
        return (ERROR);
    }

    /* Set up the input control if you are using standard C streams */
    png_init_io(png_ptr, fp);

    /* If we have already read some of the signature */
    png_set_sig_bytes(png_ptr, sig_read);
    /*
     * If you have enough memory to read in the entire image at once,
     * and you need to specify only transforms that can be controlled
     * with one of the PNG_TRANSFORM_* bits (this presently excludes
     * dithering, filling, setting background, and doing gamma
     * adjustment), then you can read the entire image (including
     * pixels) into the info structure with this call:
     */
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    /* At this point you have read the entire image */
    /* clean up after the read, and free any memory allocated - REQUIRED */

    *width = (int)png_get_image_width(png_ptr, info_ptr);
    *height = (int)png_get_image_height(png_ptr, info_ptr);
    *channels = (int)png_get_channels(png_ptr, info_ptr);
    // TODO(vasiliy)- we never use these
    //int color_type = png_get_color_type(png_ptr, info_ptr);
    //int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    int w = *width;
    int h = *height;
    int chan_in = *channels;
    int chan = *channels;
    if (chan == 4) { // dont worry about RGBA crap
        *channels = 3;
        chan = 3;
    }
    /* GET IMAGE DATA*/
    *img = (unsigned char*)malloc(sizeof(unsigned char)*w*h*chan);
    row_pointers = png_get_rows(png_ptr, info_ptr);

    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            for (int k = 0; k < chan; ++k) {
                img[0][w*chan*i + j*chan + k] = row_pointers[i][j*chan_in+k];
            }
        }
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
    /* close the file */
    fclose(fp);
    /* that's it */
    return (OK);
}

int write_png_file(const char *file_name, unsigned char* img, int width, int height,
                   int channels) {
    int ERROR = 1;
    int OK = 0;
    FILE *fp;
    png_bytepp row_pointers;
    png_structp png_ptr;
    png_infop info_ptr;

    /* open the file */
    fp = fopen(file_name, "wb");
    if (fp == NULL)
        return (ERROR);

    /* Create and initialize the png_struct with the desired error handler
     * functions.  If you want to use the default stderr and longjump method,
     * you can supply NULL for the last three parameters.  We also check that
     * the library version is compatible with the one used at compile time,
     * in case we are using dynamically linked libraries.  REQUIRED.
     */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (png_ptr == NULL) {
        fclose(fp);
        return (ERROR);
    }

    /* Allocate/initialize the image information data.  REQUIRED */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, png_infopp_NULL);
        return (ERROR);
    }

    /* Set error handling.  REQUIRED if you aren't supplying your own
     * error handling functions in the png_create_write_struct() call.
     */
    if (setjmp(png_jmpbuf(png_ptr))) {
        /* If we get here, we had a problem reading the file */
        fclose(fp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return (ERROR);
    }

    /* One of the following I/O initialization functions is REQUIRED */
    png_init_io(png_ptr, fp);

    /* write header */
    int bit_depth = 8;
    int color_type;
    if (channels == 3)
        color_type = PNG_COLOR_TYPE_RGB;
    else
        color_type = PNG_COLOR_TYPE_GRAY;

    png_set_IHDR(png_ptr, info_ptr, width, height,
                 bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);
    /* write image data in row-pointer format */
    row_pointers = (png_bytepp)malloc(sizeof(png_bytep) * height);
    for (int i = 0; i < height; ++i) {
        row_pointers[i] = (png_bytep)malloc(sizeof(png_byte) * channels * width);
        for (int j = 0; j < width*channels; ++j)
            row_pointers[i][j] = img[width*channels*i + j];
    }
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);

    for (int i = 0; i < height; ++i)
        free(row_pointers[i]);
    free(row_pointers);

    /* clean up after the write, and free any memory allocated */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return (OK);
}

bool isPNG(const char *fname) {
    int n = strlen(fname);
    return ( (strcmp(fname+(n-3),"png")==0) || (strcmp(fname+(n-3),"PNG")==0) );
}

} // namespace bcv
