#include "rw_jpeg.h"
namespace bcv {

void write_jpeg_file(const char * filename, unsigned char* img, int width, int height, int channels,
                     int quality) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE * outfile;             /* target file */
    JSAMPROW row_pointer[1];    /* pointer to JSAMPLE row[s] */
    int row_stride;             /* physical row width in image buffer */

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    if ((outfile = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "can't open %s\n", filename);
        exit(1);
    }

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;  /* image width and height, in pixels */
    cinfo.image_height = height;
    cinfo.input_components = channels;  /* # of color components per pixel */

    if (channels == 3)
        cinfo.in_color_space = JCS_RGB;  /* colorspace of input image */
    else
        cinfo.in_color_space = JCS_GRAYSCALE;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
    jpeg_start_compress(&cinfo, TRUE);
    row_stride = width * channels; /* JSAMPLEs per row in image_buffer */

    // unsigned char* row = (unsigned char*)malloc( sizeof(unsigned char)*width*channels );
    // for (size_t j=0; j<width*channels; ++j)
    // row[j] = img[row_stride]
    while (cinfo.next_scanline < cinfo.image_height)
    {
        row_pointer[0] = &img[ cinfo.next_scanline * row_stride];
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    // free(row);

    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
}

int read_jpeg_file (const char * filename, unsigned char** img, int* width, int* height,
                    int* channels) {
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    FILE * infile;              /* source file */
    JSAMPARRAY buffer;          /* Output row buffer */
    int row_stride;             /* physical row width in output buffer */
    int scanline;

    if ((infile = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "can't open %s\n", filename);
        return 1;
    }

    cinfo.err = jpeg_std_error(&jerr.pub);
    //jerr.pub.error_exit = my_error_exit;
    jerr.pub.error_exit = [](j_common_ptr cinfo) {
        my_error_mgr* myerr = (my_error_mgr*) cinfo->err;
        (*cinfo->err->output_message)(cinfo);
        longjmp(myerr->setjmp_buffer, 1);
    };

   
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return 1;
    }
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    (void) jpeg_read_header(&cinfo, TRUE);
    (void) jpeg_start_decompress(&cinfo);

    row_stride = cinfo.output_width * cinfo.output_components;
    buffer = (*cinfo.mem->alloc_sarray)
                 ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    scanline = 0;
    *img = (unsigned char*)malloc(
        sizeof(unsigned char)*cinfo.output_width*cinfo.output_height*cinfo.output_components);
    while (cinfo.output_scanline < cinfo.output_height) {
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);
        put_scanline((unsigned char*)buffer[0], row_stride, *img, scanline);
        scanline++;
    }
    *width = cinfo.output_width;
    *height = cinfo.output_height;
    *channels = cinfo.output_components;
    (void) jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return 0;
}

void put_scanline(unsigned char* buf, int row_stride, unsigned char* img, int scanline) {
    for (int i = 0; i < row_stride; ++i)
        img[row_stride*scanline + i] = buf[i];
}

bool isJPG(const char* fname) {
    int n = strlen(fname);
    return ( (strcmp(fname+(n-3),"jpg")==0) || (strcmp(fname+(n-3),"JPG")==0) );
}

} // namespace bcv























