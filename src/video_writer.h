//! Class for writing videos, using ffmpeg.
//! This is loosely based on:
//! http://docs.thefoundry.co.uk/nuke/70/ndkreference/examples/ffmpegWriter.cpp
#include <cstdlib>
#include <vector>
#include <string>
#include <cassert>
#include <algorithm> // for max/min
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
#include <stdint.h>

extern "C" {
#include <libavutil/opt.h>                                                      
#include <libavcodec/avcodec.h>                                                 
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>                                           
#include <libavutil/common.h>                                                   
#include <libavutil/imgutils.h>                                                 
#include <libavutil/mathematics.h>                                              
#include <libavutil/samplefmt.h> 
#include <libswscale/swscale.h>
#include <libavutil/audioconvert.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
}

class video_writer {  
public:
    video_writer() = delete; // no default constructor
    video_writer(const video_writer& other) = delete; // no copy constructor!
    video_writer(const video_writer&& other) = delete; // no move constructor!
    video_writer& operator=(const video_writer& other) = delete; // no copy assignment op!
    video_writer& operator=(const video_writer&& other) = delete; // no move assignment op!

    //! constructor: output filename, video size
    video_writer(const char* filename, int width, int height);
    //! nontrivial destructor
    ~video_writer();

    //! add a frame to the video sequence. If a frame has incorrect dimensions,
    //! we will add a black frame.
    template <typename T> void add_frame(const std::vector<T>& img) {
        uint8_t* buf = NULL;
        if (img.size() != width*height*3) { 
            printf("incorrect image size.");
        } else {
            buf = (uint8_t*)malloc( sizeof(uint8_t)*img.size() );
            for (int i = 0; i < img.size(); ++i) { 
                buf[i] = std::min(std::max(T(0), img[i]), T(255));
            }
        }
        add_frame_internal(buf);
        if (buf) { free(buf); }
    }

    //! set fps (can only do PRIOR TO ADDING THE FIRST FRAME)
    void set_fps(float fps);
    //! set bitrate (can only do PRIOR TO ADDING THE FIRST FRAME)
    void set_bitrate(int br);
private:
    void add_frame_internal(const uint8_t* data);
    void free_format_context();
    void prepare();  
    void finish();
    bool set_codec_context(AVCodecContext* codec, AVCodec* videoCodec);
    
    bool video_prepared = false;
    bool video_finished = false;

    AVCodecContext*   codec_context = NULL;
    AVFormatContext*  format_context = NULL;
    AVStream* stream_ = NULL;
    AVOutputFormat* fmt = NULL;

    int width = 0;
    int height = 0;
    std::string filename = "";
    float fps_ = 15;
    int format_ = 0;
    int codec_ = 0;
    int bitrate_ = 100000000;
    int bitrateTolerance_ = 2000000000;
    int gopSize_ = 12;
};