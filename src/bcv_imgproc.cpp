#include "bcv_imgproc.h"

void rgb2hsv(vector<float>& x) {
    assert( (x.size() % 3) == 0 && "image has 3 channels." );
    for (int i = 0; i < x.size(); i+=3) { rgb2hsv_one(&x[i]); }
}

void hsv2rgb(vector<float>& x) {
    assert( (x.size() % 3) == 0 && "image has 3 channels." );
    for (int i = 0; i < x.size(); i+=3) { hsv2rgb_one(&x[i]); }
}

void rgb2yiq(vector<float>& x) {
    assert( (x.size() % 3) == 0 && "image has 3 channels." );
    for (int i = 0; i < x.size(); i+=3) { rgb2yiq_one(&x[i]); }
}

void yiq2rgb(vector<float>& x) {
    assert( (x.size() % 3) == 0 && "image has 3 channels." );
    for (int i = 0; i < x.size(); i+=3) { yiq2rgb_one(&x[i]); }
}

void sepia(vector<float>& x, float i_new, float q_new) {
    assert( (x.size() % 3) == 0 && "image has 3 channels." );
    float* x_;
    float y;
    float s[3]; 
    s[0] = +0.956*i_new + 0.620*q_new;                                     
    s[1] = -0.272*i_new - 0.647*q_new;                                     
    s[2] = -1.108*i_new + 1.705*q_new;                                     
    for (int j = 0; j < x.size(); j+=3) { 
        x_ = &x[j];
        y = 0.299*x_[0] + 0.587*x_[1] + 0.114*x_[2];                                   
        x_[0] = 1.0*y + s[0];
        x_[1] = 1.0*y + s[1];
        x_[2] = 1.0*y + s[2];
    }
}

void hist_eq(vector<uchar>& x, int chan) { 
    vector<float> pdf = vector<float>(256,0);
    vector<float> cdf = vector<float>(256);
    float inv_sum = 1.0f/(x.size()/chan);
    for (int c = 0; c < chan; ++c) { 
        for (int i = c; i < x.size(); i+=chan) { pdf[ x[i] ]++; }
        transform(pdf.begin(), pdf.end(), pdf.begin(),
                       bind1st(multiplies<float>(), inv_sum));
        cdf[0] = pdf[0];
        for (int i = 0; i < pdf.size();++i) { cdf[i] = cdf[i-1] + pdf[i]; }
        for (int i = c; i < x.size(); i+=chan) { x[i] = 255*cdf[ x[i] ]; }
    }
}

void vignette(vector<float>& x, int rows, int cols, int chan, float sigma) { 
    vector<float> wx = vector<float>(cols);
    vector<float> wy = vector<float>(rows);
    int cx = cols/2;
    int cy = rows/2;
    float sigma_sq_horiz = (sigma*sigma)/(cols*cols);
    float sigma_sq_vert = (sigma*sigma)/(rows*rows);
    for (int i = 0; i < cols; ++i) { wx[i] = exp(-sigma_sq_horiz*(i-cx)*(i-cx)); }
    for (int i = 0; i < rows; ++i) { wy[i] = exp(-sigma_sq_vert*(i-cy)*(i-cy)); }
    for (int i = 0; i < x.size(); ++i) { 
        int r = getrow(i, cols, chan);
        int c = getcol(i, cols, chan);
        x[i] *= (wy[r]*wx[c]);
    }
}

void gamma_adjustment(vector<float>& x, float gamma) {
    if ((gamma < 0) || (gamma==1)) { return; }

    float maxval = *max_element(x.begin(), x.end());
    if (maxval==0) { return; }
    transform(x.begin(), x.end(), x.begin(), bind1st(multiplies<float>(), 1.0f/maxval));
    if (gamma==0) { // use auto-gamma..
       float mu = accumulate(x.begin(),x.end(), 0.0f)/x.size();
       gamma = -log(mu)/0.693147f; //log(2.0f);
    }
    vector<float> lut = vector<float>(256);
    for (int i = 0; i < lut.size(); ++i) { 
        lut[i] = pow( ((i+1)/256.0f), 1.0f/gamma );
    }
    for (int i = 0; i < x.size(); ++i) { x[i] = lut[ 255*x[i] ]; }
    transform(x.begin(), x.end(), x.begin(), bind1st(multiplies<float>(), maxval));
}

void modulate(vector<float>& x, float val_mul, float sat_mul, float hue_rot) {
    assert( (x.size() % 3) == 0 && "image has 3 channels." );
    if ((val_mul==1) && (sat_mul==1) && (hue_rot==0)) { return; }
    for (int i = 0; i < x.size(); i+=3) { 
        x[i+2]*=val_mul;
        x[i+1]*=sat_mul;
        x[i+0]+=hue_rot;
        if (x[i+0]>360.0f) { x[i+0]-=360.0f; }
        else if (x[i+0]<0) { x[i+0]+=360.0f; }
    }
}

void vintage(vector<float>& x) { 
    assert( (x.size() % 3) == 0 && "image has 3 channels." );
    float maxval = *max_element(x.begin(), x.end());
    if (maxval==0) { return; }
    transform(x.begin(), x.end(), x.begin(), bind1st(multiplies<float>(), 1.0f/maxval));
    float r,g,b;
    for (int i = 0; i < x.size(); i+=3) { 
        r = x[i+0];
        g = x[i+1];
        b = x[i+2];
        x[i+0] = -0.2*sqrt(r)*sin(M_PI*(-0.0000195*r*r + 0.0125*r))+r;
        x[i+1] = -0.001045*g*g+1.12665*g;
        x[i+2] = 0.573*b+0.2078;
    }
    transform(x.begin(), x.end(), x.begin(), bind1st(multiplies<float>(), maxval));
}

void tint(vector<float>&x, const vector<float>& rgb, float percent) {
    assert( (x.size() % 3) == 0 && "image has 3 channels." );
    assert( (rgb.size() == 3) && "rgb value has three values." );
    float maxval = *max_element(x.begin(), x.end());
    if (maxval==0) { return; }
    transform(x.begin(), x.end(), x.begin(), bind1st(multiplies<float>(), 1.0f/maxval));
    // create multiplication fn:
    vector<float> mulf = vector<float>(256);
    for (int i = 0; i < mulf.size(); ++i) { 
        float x = float(i)/float(mulf.size());
        mulf[i]=percent*(1.0f-(4.0f*((x-0.5)*(x-0.5))));
    }
    // ensure rgb value is within sane range.
    vector<float> rgb_ = rgb;
    if ((rgb_[0]>1) || (rgb_[1]>1) || (rgb_[2]>1)) {
        rgb_[0]/=255.0; 
        rgb_[1]/=255.0; 
        rgb_[2]/=255.0; 
    }
    for (int i = 0; i < x.size(); i+=3) {
        for (int j = 0; j < 3; ++j) { 
            float val = mulf[ floor(x[i+j]*255.99) ];
            x[i+j] = val*rgb_[j]+(1-val)*x[i+j];
        }
    }
    transform(x.begin(), x.end(), x.begin(), bind1st(multiplies<float>(), maxval));
}
