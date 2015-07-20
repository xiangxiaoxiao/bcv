//! @file bcv_imgproc.h
#ifndef BCV_IMGPROC_H_
#define BCV_IMGPROC_H_
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
//#include <functional>
#include <cassert>
#include "bcv_utils.h"

namespace bcv {
using namespace std;

//! \brief resize image
vector<float> imresize(const vector<float>& in, int in_rows, int in_cols, int out_rows, int out_cols);

//! \brief returns a grayscale image
vector<float> rgb2gray(const vector<float>& x);

void rgb2yiq(vector<float>& x);
void yiq2rgb(vector<float>& x);

//! hsv \in [0,360] x [0,1] x [0,max(x)]
void rgb2hsv(vector<float>& x);
void hsv2rgb(vector<float>& x);

//! \brief Sepia effect to an RGB image.
//! Range of the image does not matter; default args assume [0,1]
void sepia(vector<float>& x, float i_new=51.0f/256.0f, float q_new=0.0f);

//! \brief Histogram equalization
//! Image must be in {0,255}
void hist_eq(vector<uchar>& x, int chan);

//! \brief Vignetting effect to an image
//! Range of the image does not matter.
void vignette(vector<float>& x, int rows, int cols, int chan, float sigma=0.01); 

//! \brief Gamma adjustment effect.
//! Range of the image does not matter. When gamma=0, does 'autogamma':
//! chooses gamma=-log(mu)/log(2) , where mu is the average intensity.
void gamma_adjustment(vector<float>& x, float gamma=0.0f);

//! \brief Modulation effect.
//! \param val_mul: value/brightness multiplier
//! \param sat_mul: saturation multiplier
//! \param hue_rot: hue rotation in degrees
//! \param x: image is assumed to be in HSV / HSL space
void modulate(vector<float>& x, float val_mul, float sat_mul, float hue_rot);

//! \brief Applies "vintage" effect to an image.
//! the same effect as used in vintageJS (https://github.com/rendro/vintageJS).
//! if maxval (largest value in the image) is not specified,
//! it is calculated from the image.
void vintage(vector<float>& x, float maxval=0);

//! \brief Tint effect 
void tint(vector<float>& x, const vector<float>& rgb, float percent);

void inline rgb2yiq_one(float* x) {
    float y, i, q;
    y = 0.299*x[0] + 0.587*x[1] + 0.114*x[2];
    i = 0.596*x[0] - 0.275*x[1] - 0.321*x[2];
    q = 0.212*x[0] - 0.528*x[1] + 0.311*x[2];
    x[0]=y; x[1]=i; x[2]=q;
}

void inline yiq2rgb_one(float* x) {
    float r,g,b;
    r = 1.0*x[0] + 0.956*x[1] + 0.620*x[2];
    g = 1.0*x[0] - 0.272*x[1] - 0.647*x[2];
    b = 1.0*x[0] - 1.108*x[1] + 1.705*x[2];
    x[0]=r; x[1]=g; x[2]=b;	
}

void inline rgb2hsv_one(float* x) { 
    float m = min(min(x[0],x[1]),x[2]);
    float M = max(max(x[0],x[1]),x[2]);
    float C = max(1e-8f, M-m);
    float H = -1; // undefined.
    float S = 0;
    if (M != 0) { 
        S = C/M;
        if (M==x[0]) {
            H = (x[1]-x[2])/C;
        } else if (M==x[1]) {
            H = (x[2]-x[0])/C + 2.0f;
        } else {
            H = (x[0]-x[1])/C + 4.0f;
        }
        H *= 60.0f;
        if (H<0) { H += 360.0f; }
    }
    x[0]=H; x[1]=S; x[2]=M;
}

void inline hsv2rgb_one(float* x) { 
    if (x[1]==0) { // saturation=0
        x[0]=x[1]=x[2];
        return;
    }
    float H_ = x[0]/60.0f;
    int sector_index = floor(H_);
    float rem = H_-sector_index;
    float p = max(0.0f, x[2]*(1.0f - x[1]));
    float q = max(0.0f, x[2]*(1.0f - x[1]*rem));
    float t = max(0.0f, x[2]*(1.0f - x[1]*(1.0f - rem)) );

    if (sector_index==0) {
        x[0] = x[2];
        x[1] = t;
        x[2] = p;
    } else if (sector_index==1) { 
        x[0] = q;
        x[1] = x[2];
        x[2] = p; 
    } else if (sector_index==2) { 
        x[0] = p;
        x[1] = x[2];
        x[2] = t;
    } else if (sector_index==3) { 
        x[0] = p;
        x[1] = q;
    } else if (sector_index==4) { 
        x[0] = t;
        x[1] = p;
    } else { 
        x[0] = x[2];
        x[1] = p;
        x[2] = q;
    }
}

} // namespace bcv
#endif // BCV_IMGPROC_H_
