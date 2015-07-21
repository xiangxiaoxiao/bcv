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

//! Video writer
class video_writer {  
public:
    video_writer(const video_writer& other) = delete; // no copy constructor!
    video_writer(const video_writer&& other) = delete; // no move constructor!
    video_writer& operator=(const video_writer& other) = delete; // no copy assignment op!
    video_writer& operator=(const video_writer&& other) = delete; // no move assignment op!

    //! constructor: output filename, video size
    video_writer(const char* fname=NULL, int width=0, int height=0);
    //! nontrivial destructor
    ~video_writer() { close(); };    
    //! attempts to open an output file
    bool open(const char* fname);

    //! Check if the file has been successfully open (the format appears valid, etc)
    bool is_opened() const { return video_opened; };
    
    //! Finalize the video format (fps, image size, bitrate) and perform further
    //! check about whether the video is valid.
    //! Returns false if it looks like writing to video is infeasible.
    bool prepare();

    //! Closes a video if one is open, otherwise does nothing.
    void close();

    //! add a frame to the video sequence. If a frame has incorrect dimensions,
    //! we will add a black frame.
    //! Returns 'false' on error, and 'true' on success
    template <typename T> bool add_frame(const std::vector<T>& img) {
        uint8_t* buf = NULL;
        if (img.size() != width*height*3) { 
            printf("incorrect image size.");
        } else {
            buf = (uint8_t*)malloc( sizeof(uint8_t)*img.size() );
            for (int i = 0; i < img.size(); ++i) { 
                buf[i] = std::min(std::max(T(0), img[i]), T(255));
            }
        }
        bool success = add_frame_internal(buf);
        if (buf) { free(buf); }
        return success;
    }

    //! set fps (can only do PRIOR TO ADDING THE FIRST FRAME)
    void set_fps(float fps);
    //! set bitrate (can only do PRIOR TO ADDING THE FIRST FRAME)
    void set_bitrate(int br);
    //! set width/height (can only do PRIOR TO ADDING THE FIRST FRAME)
    void set_imsize(int width, int height);
private:
    bool add_frame_internal(const uint8_t* data);
    void free_format_context();

    bool set_codec_context(AVCodecContext* codec, AVCodec* videoCodec);
    
    bool video_opened = false;
    bool video_prepared = false;

    AVCodecContext*   codec_context = NULL;
    AVFormatContext*  format_context = NULL;
    AVStream* stream_ = NULL;
    AVOutputFormat* fmt = NULL;
    SwsContext* sws_ctx = NULL;

    int width = 0;
    int height = 0;
    std::string filename = "";
    float fps_ = 15;
    int codec_ = 0;
    int bitrate_ = 2000000000;
    int bitrateTolerance_ = 1000000000; //2000000000;
    int gopSize_ = 12;
};
