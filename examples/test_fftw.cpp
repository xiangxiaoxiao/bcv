#include <cstdlib>
#include <algorithm>
#include <fftw3.h>
#include "bcv_io.h"
#include "bcv_utils.h"
int main() { 
    const char* in_fname = "../images/arches.jpg";
    const char* out_fname1 = "out1.jpg";
    const char* out_fname2 = "out2.jpg";
    const char* out_fname3 = "out.jpg";
    int rows, cols;
    double t1, t2;
    printf("Trying to read %s\n", in_fname);
    vector<float> img = bcv_imread<float>(in_fname, &rows, &cols);
    printf("read image, size: %dx%d\n", rows, cols); 
    fftwf_complex *in_fftw, *out_fftw;
    fftwf_plan p;
    float maxval;

    // ------------------------------------------------------------------------
    // take fft2 of "complex" data
    printf("taking fftwf of 'complex' data.\n");
    in_fftw  = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * rows*cols);
    out_fftw = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * rows*cols );
    
    for (int i = 0; i < rows*cols; ++i) { 
        in_fftw[i][0] = img[i];
        in_fftw[i][1] = 0.0f;
    }
    t1 = now_ms();
    p = fftwf_plan_dft_2d(rows, cols, in_fftw, out_fftw, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_execute(p); 
    t1 = (now_ms()-t1);
    printf("took %f ms\n", t1);

    // make a abs-fftw image:
    for (int i = 0; i < rows*cols; ++i) { 
        img[i] = sqrt(out_fftw[i][0]*out_fftw[i][0]+out_fftw[i][1]*out_fftw[i][1]);
        img[i] = sqrt(sqrt(sqrt(img[i])));
    }
    maxval = *max_element(img.begin(), img.end() );
    transform(img.begin(), img.end(), img.begin(),
                  bind1st(multiplies<float>(),255.0/maxval ));

    fftwf_destroy_plan(p);
    fftwf_free(in_fftw);
    fftwf_free(out_fftw);    
    bcv_imwrite(out_fname1, img, rows, cols);
    // ----------------------------------------------------------------------//
    // take a fft2 of a real data:
    printf("taking fftwf of real data.\n");
    out_fftw = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * rows*(cols/2+1) );
    memset(out_fftw, 0, sizeof(float)*rows*(cols/2+1)*2);
    t2 = now_ms();
    p = fftwf_plan_dft_r2c_2d(rows, cols, (float*)(&img[0]), out_fftw, FFTW_ESTIMATE);
    fftwf_execute(p); 
    t2 = (now_ms()-t2);
    printf("took %f ms\n", t2);

    // make a abs-fftw image:
    memset(&img[0], 0, sizeof(float)*rows*cols);
    for (int i = 0; i < rows*(cols/2+1); ++i) { 
        img[i] = sqrt(out_fftw[i][0]*out_fftw[i][0]+out_fftw[i][1]*out_fftw[i][1]);
        img[i] = sqrt(sqrt(sqrt(img[i])));
    }
    maxval = *max_element(img.begin(), img.end() );
    transform(img.begin(), img.end(), img.begin(),
                  bind1st(multiplies<float>(),255.0/maxval ));
    fftwf_destroy_plan(p);
    bcv_imwrite(out_fname2, img, rows, cols/2+1);
    // ------------------------------------------------------------------------
    // take inverse fft2 of complex data..
    printf("taking inverse fftwf of real data.\n");
    t2 = now_ms();
    p = fftwf_plan_dft_c2r_2d(rows, cols, out_fftw, (float*)(&img[0]), FFTW_ESTIMATE );
    fftwf_execute(p);
    t2 = (now_ms()-t2);
    printf("took %f ms\n", t2);
    transform(img.begin(), img.end(), img.begin(),
                  bind1st(multiplies<float>(),1.0f/(rows*cols) ) );
    fftwf_destroy_plan(p);
    fftwf_free(out_fftw);    
    bcv_imwrite(out_fname3, img, rows, cols);
    return 0;
}
