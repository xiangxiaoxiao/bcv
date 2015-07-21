#include <cstdlib>
#include <sstream>
#include "bcv_imgproc.h"
#include "bcv_io.h"
#include <gflags/gflags.h>

#ifdef HAVE_FFMPEG
#include "video_reader.h"
#include "video_writer.h"
#endif

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

void do_imgproc(vector<float>& img, int rows, int cols, int chan);
int main(int argc, char** argv) { 
    gflags::SetUsageMessage("image/video processing example");
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    int rows, cols, chan;

    if ( (bcv::is_image_file(FLAGS_input.c_str())) && 
         (bcv::is_image_file(FLAGS_output.c_str())) ) {
        vector<float> img = bcv::bcv_imread<float>(
        							FLAGS_input.c_str(), &rows, &cols, &chan);
        do_imgproc(img, rows, cols, chan);
        bcv::bcv_imwrite<float>(FLAGS_output.c_str(), img, rows, cols, chan);
    } else if ( (bcv::is_video_file(FLAGS_input.c_str())) &&
                (bcv::is_video_file(FLAGS_output.c_str())) ) {
        #ifndef HAVE_FFMPEG
        printf("Trying to read a video file, but no FFMPEG support.\n");
        printf("Cannot do anything.\n");            
        return 0;
        #else

        video_reader vin( FLAGS_input.c_str() );
        if (!vin.is_opened()) { return -1; }

        int width  = vin.get_width();
        int height = vin.get_height();
        rows = height;
        cols = width;
        chan = 3;
        video_writer vout( FLAGS_output.c_str(), width, height);
        if (!vout.is_opened()) { return -1; }
        vout.set_fps( vin.get_fps() );
        if (!vout.prepare()) { return -1; }

        while (true) {
            vector<float> img = vin.get_frame<float>();
            if (img.empty()) { break; }
            do_imgproc(img, rows, cols, chan);

            vout.add_frame(img);
        }
        vin.close(); // alternatively, let destructor do the same 
        vout.close(); // alternatively, let destructor do the same
        #endif
    } else {
        printf("Check input/output consistency: \nin: %s\nout: %s\n", 
                        FLAGS_input.c_str(), FLAGS_output.c_str() );
    } 

    return 0;
}

void do_imgproc(vector<float>& img, int rows, int cols, int chan) {
    double t1, t2;
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
}