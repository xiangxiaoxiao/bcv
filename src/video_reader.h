//! Class for reading videos, using ffmpeg.
//! This is loosely based on example tutorials packaged with the library
#include <cstdlib>
#include <vector>
#include <string>
#include <cassert>

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

//! Video reader
class video_reader {  
public:
    video_reader(const video_reader& other) = delete; // no copy constructor!
    video_reader(const video_reader&& other) = delete; // no move constructor!
    video_reader& operator=(const video_reader& other) = delete; // no copy assignment op!
    video_reader& operator=(const video_reader&& other) = delete; // no move assignment op!

    //! constructor: input filename. 
    //! If no filename is specified, you have to later open the file manually,
    //! otherwise it is opened when the object is created. 
    video_reader(const char* filename=NULL);
    //! nontrivial destructor
    ~video_reader() { close(); };

    //! Returns true on success, false otherwise. Returns false and does nothing
    //! if the video is already opened.
    bool open(const char* filename);

    // Returns true if the video has successfully been opened, false otherwise.
    bool is_opened() const { return reader_success; };
    
    //! Closes a video if one is open, otherwise does nothing.
    void close();

    //! Get a frame from the video sequence.
    //! Returns empty vector on failure (or e.g. when video is finished)    
    template <typename T> std::vector<T> get_frame() { 
        alloc_frame_internal();
        uint8_t* data = get_frame_internal();
        std::vector<T> out;
        if (data) { 
            out = std::vector<T>(width*height*chan);
            for (int i = 0; i < width*height*chan; ++i) { out[i]=data[i]; }
        }
        dealloc_frame_internal();
        return out; 
    };

    //! Returns image width
    int get_width() const { return width; };
    //! Returns image height
    int get_height() const { return height; };
    //! Returns video FPS
    float get_fps() const;
private:
    bool reader_success = false; 
    bool prepare();
    uint8_t* get_frame_internal();
    void alloc_frame_internal();
    void dealloc_frame_internal();

    int width = 0;
    int height = 0;
    int chan = 3;
    int video_stream = -1;

    std::string filename = "";
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVCodecContext* pCodecCtx2 = NULL;
    AVCodec *pCodec = NULL;
    SwsContext *sws_ctx = NULL;

    uint8_t* buffer;
    AVPacket packet;
    AVFrame *pFrame;
    AVFrame *pFrameRGB;
};
