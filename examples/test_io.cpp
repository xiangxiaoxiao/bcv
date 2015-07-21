#include <cstdlib>
#include <cassert>
#include "bcv_io.h"

#ifdef HAVE_FFMPEG
#include "video_reader.h"
#include "video_writer.h"
#endif

using std::vector;
using bcv::uchar;

void write_random_video(const char* fname);

int main() { 
    const char* in_fname = "../images/arches.jpg";
    const char* out_fname = "out.jpg";
    int rows, cols, chan;
   
    // Try image IO
    printf("\n>>> 1. read existing image. \n");
    vector<uchar> img; 
    printf("Trying to read %s\n", in_fname);
    img = bcv::bcv_imread<uchar>(in_fname, &rows, &cols, &chan);
    printf("read image, size: %dx%dx%d\n", rows, cols, chan); 

    printf("\n>>> 2. write image. \n");
    printf("Trying to write %s\n", out_fname);
    bcv::bcv_imwrite<uchar>(out_fname, img, rows, cols, chan);

    printf("\n>>> 3. read grayscale image\n");
    printf("Trying to read grayscale image %s\n", in_fname);
    img = bcv::bcv_imread<uchar>(in_fname, &rows, &cols);
    printf("read image, size: %dx%d\n", rows, cols); 

    printf("\n>>> 4. write grayscale image.\n");
    printf("Trying to write %s\n", out_fname);
    bcv::bcv_imwrite<uchar>(out_fname, img, rows, cols);
    
    printf("\n>>> 5. Try to open a non-existing file.\n");
    img = bcv::bcv_imread<uchar>("nonsense_filename", &rows, &cols, &chan);
    assert( (img.size() == 0) && "Returned empty vector.");

    printf("\n>>> 6.Try to open a non-existing file with a valid filename.\n");
    img = bcv::bcv_imread<uchar>("nonsense_filename.png", &rows, &cols, &chan);
    assert( (img.size() == 0) && "Returned empty vector.");

#ifdef HAVE_FFMPEG
    // Tests with video opening / writing.
    printf("\n>>> 1. write a video populated with random pixels\n");
    write_random_video("random.mp4");
    
    printf("\n>>> 2. can we write non-mp4 videos??\n");
    write_random_video("random.avi");
    
    printf("\n>>> 3. videos with nonsense formats?\n");
    // THIS FAILS!!! MAKE SURE THAT IT DOES NOT!!!
    write_random_video("random.supdog");
    
    printf("\n>>> 4. Can we open a video?\n");
    video_reader f;
    f.open("random.mp4");
    assert( f.is_opened() && "Can successfully open random.mp4" );
    printf("Video width x height = %d x %d, fps=%f\n", 
                        f.get_width(), f.get_height(), f.get_fps() );
    int numframes = 0; while (!f.get_frame<uchar>().empty() ) { numframes++; }
    printf("video has %d frames.\n", numframes);
    // at this stage, we are at the end of the video, and should close it.
    // (currently there is no rewind capability; cant bring back the past).
    f.close();
    assert( !f.is_opened() && "Video was closed.");

    printf("\n>>> 5. Can we open a nonexistent video using the same object?\n");
    f.open("supdog.mp4");
    assert( !f.is_opened() && "video was not opened." );

    printf("\n>>> 6. Can we open an existing object and close it immediately?\n");
    video_reader f2("random.mp4");
    assert( f2.is_opened() && "video was opened successfully." );
    f2.close();
    assert( !f2.is_opened() && "video was successfully closed." );
    //
#endif
    return 0;
}

#ifdef HAVE_FFMPEG
void write_random_video(const char* fname) {
    int width = 200;
    int height = 200;
    std::vector<uchar> img(width*height*3);

    video_writer f(fname, width, height);
    if (!f.is_opened()) { 
        printf("could not open %s\n", fname);
        return;
    }
    f.set_fps(10);
    f.set_bitrate(10000000);
    if (!f.prepare()) { 
        printf("could not prepare video %s.\n", fname);
        return; 
    }

    for (int k = 0; k < 100; ++k) {
        for (auto &x : img) { x = rand() % 255; }    
        f.add_frame( img );
    }
}
#endif