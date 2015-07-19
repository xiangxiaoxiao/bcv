#include <cstdlib>
#include <vector>
#include "bcv_io.cpp"
#include "math.h"

#include "video_writer.h"
#include "video_reader.h"
#include "bcv_imgproc.h"

#include <gflags/gflags.h>

DEFINE_string(input, "mad_men_peggy.mp4", "Input video filename");

using namespace bcv;

int video_to_images(const char* fname);
int images_to_video(const char* fname);
int video_to_video(const char* fname);

int images_to_video(const char* fname) {
    int width = 400;
    int height = 400;
    std::vector<uchar> img = std::vector<uchar>(width*height*3);

    video_writer f(fname, width, height);
    f.set_fps(10);
    f.set_bitrate(10000000);
    for (int k = 0; k < 100; ++k) {
        for (auto &x : img) { x = rand() % 255; }    
        f.add_frame( img );
    }
}

int main(int argc, char** argv) {
    gflags::SetUsageMessage("FFMPEG/AVCONV test");
    gflags::ParseCommandLineFlags(&argc, &argv, false);
   
    // Uncomment this to generate a video with randomly colored pixels 
    //images_to_video0();
    // Uncomment this to split a video into images (and save them...)
    //video_to_images();
    video_to_video( FLAGS_input.c_str() );
}

int video_to_images(const char* in_fname) {
    char fname[1024];
    video_reader f(in_fname);
    int width  = f.get_width();
    int height = f.get_height();

    for (int k = 0; k < 200; ++k) {
        vector<uchar> img = f.get_frame<uchar>();
        if (img.size() > 0) {
            sprintf(fname, "image%03d.jpg", k);
            bcv_imwrite(fname, img, height, width, 3);
            printf("wrote %s\n", fname);
        }
    }
}

int video_to_video(const char* in_fname) {
    
    video_reader fr(in_fname);
    int width  = fr.get_width();
    int height = fr.get_height();
    float fps = fr.get_fps();

    char out_fname[1024];
    sprintf(out_fname, "out-%s", in_fname);
    video_writer fw(out_fname, width, height);
    fw.set_fps(fps);
    int i = 0;
    while (true) {
        vector<float> img = fr.get_frame<float>();
        if (img.empty()) { break; }

        vintage(img, 255.0f);
        vignette(img, height, width, 3, 2.0f);

        fw.add_frame(img);
        printf("added frame %d\n", i); i++;
    }
}
