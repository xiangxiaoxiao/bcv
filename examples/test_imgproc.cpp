#include <cstdlib>
#include <sstream>
#include "bcv_imgproc.h"
#include "bcv_io.h"
#include <gflags/gflags.h>

using namespace std;
using bcv::uchar;

DEFINE_string(input, "./images/arches.jpg","input image");
DEFINE_string(output, "out.jpg","output image");
DEFINE_bool(sepia, false, "do sepia?");
DEFINE_bool(hist_eq, false, "do histogram equalization?");
DEFINE_bool(vignette, false, "do vignetting?");
DEFINE_double(gamma, -1.0, "do gamma adjustment?");
DEFINE_double(vignette_sigma, 1, "vignette sigma");
DEFINE_double(sepia_i, 51.0, "sepia I-channel");
DEFINE_double(sepia_q, 0.0, "sepia Q-channel");
DEFINE_bool(rgb2hsv, false, "test hsv2rgb conversion.\n");
DEFINE_bool(vintage, false, "apply vintage effect?.\n");
DEFINE_string(modulate, "1,1,0", "modulate by (val-mul, sat-mul, hue-rot).\n");
DEFINE_string(tint, "0,0,0,0", "tint color (rgba).\n");

int main(int argc, char** argv) { 
    gflags::SetUsageMessage("image processing example");
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    double t1, t2;
    int rows, cols, chan;

    vector<float> img = bcv::bcv_imread<float>(
    							FLAGS_input.c_str(), &rows, &cols, &chan);

    if (FLAGS_rgb2hsv) { // rgb to hsv and back
        t1 = bcv::now_ms();
        bcv::rgb2hsv(img);
	    t2 = bcv::now_ms();
        printf("rgb2hsv took: %f ms on %dx%d image\n", t2-t1, rows, cols);
    
        t1 = bcv::now_ms();
        bcv::hsv2rgb(img);
	    t2 = bcv::now_ms();
        printf("hsv2rgb took: %f ms on %dx%d image\n", t2-t1, rows, cols);
    }

    if (FLAGS_hist_eq) { // histogram equalization
        vector<uchar> I;
        I.assign(img.begin(), img.end());
        t1 = bcv::now_ms();
        bcv::hist_eq(I, chan);  
	    t2 = bcv::now_ms();
        printf("histogram equalization took: %f ms on %dx%d image\n", t2-t1, rows, cols);
        img.assign(I.begin(), I.end());
    }
    if (!FLAGS_modulate.empty()) { // modulation in HSV space
        // parse string.
        vector<float> mod_vals;
        float val;
        stringstream ss(FLAGS_modulate);
        while (ss >> val) {
            mod_vals.push_back(val);
            if (ss.peek() == ',') { ss.ignore(); }
        }
        if (mod_vals.size() !=3) { 
            printf("incorrect modulate string!\n");
            printf("should be comma-separated list of three values.\n");
            printf("got %s\n", FLAGS_modulate.c_str());
        } else if (!((mod_vals[0]==1) && (mod_vals[1]==1) && (mod_vals[2]==0))) {
            t1 = bcv::now_ms();
            bcv::rgb2hsv(img);
            bcv::modulate(img, mod_vals[0], mod_vals[1], mod_vals[2]); 
            bcv::hsv2rgb(img);
            t2 = bcv::now_ms();
            printf("modulation took: %f ms on %dx%d image\n", t2-t1, rows, cols);
        }
    }
    if (!FLAGS_tint.empty()) { // tint
        // parse string.
        vector<float> mod_vals;
        float val;
        stringstream ss(FLAGS_tint);
        while (ss >> val) {
            mod_vals.push_back(val);
            if (ss.peek() == ',') { ss.ignore(); }
        }
        if (mod_vals.size() !=4) { 
            printf("incorrect tint string!\n");
            printf("should be comma-separated list of four values.\n");
            printf("got %s\n", FLAGS_tint.c_str());
        } else if (mod_vals[3]!=0) {
            vector<float> rgb_val = vector<float>(3);
            rgb_val[0] = mod_vals[0];
            rgb_val[1] = mod_vals[1];
            rgb_val[2] = mod_vals[2];
            float alpha = mod_vals[3];
            t1 = bcv::now_ms();
            bcv::tint(img, rgb_val, alpha);
            t2 = bcv::now_ms();
            printf("tint took: %f ms on %dx%d image\n", t2-t1, rows, cols);
        }
    }



    if (FLAGS_gamma >= 0) { // gamma adjustment
        t1 = bcv::now_ms();
        bcv::gamma_adjustment(img, FLAGS_gamma);
        t2 = bcv::now_ms();
        printf("gamma adjustment took: %f ms on %dx%d image\n", t2-t1, rows, cols);
    }
    if (FLAGS_sepia) { // sepia
        t1 = bcv::now_ms();
        bcv::sepia(img, FLAGS_sepia_i, FLAGS_sepia_q);
        t2 = bcv::now_ms();
        printf("sepia took: %f ms on %dx%d image\n", t2-t1, rows, cols);
    }
    if (FLAGS_vintage) { // vintage-esque
        t1 = bcv::now_ms();
        bcv::vintage(img);
        t2 = bcv::now_ms();
        printf("vintage took: %f ms on %dx%d image\n", t2-t1, rows, cols);
    }

    if (FLAGS_vignette) { 
        t1 = bcv::now_ms();
        bcv::vignette(img, rows, cols, chan, FLAGS_vignette_sigma);
	    t2 = bcv::now_ms();
        printf("vignette took: %f ms on %dx%d image\n", t2-t1, rows, cols);
    }

    bcv::bcv_imwrite<float>(FLAGS_output.c_str(), img, rows, cols, chan);
    return 0;
}
