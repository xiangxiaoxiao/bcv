#include "video_reader.h"

video_reader::video_reader(const char* fname) {
    av_log_set_level(AV_LOG_WARNING);    
    av_register_all();

    if (fname != NULL) { open(fname); }
}

bool video_reader::open(const char* fname) {
    if (reader_success) { return false; } // file is already open, do nothing

    filename = std::string(fname);
    reader_success = prepare();
    return reader_success;
}
//! Returns true on success.
//! This function reads the video codecs and must be executed prior to getting
//! any frames from the video.
bool video_reader::prepare() {
    if (avformat_open_input(&pFormatCtx, filename.c_str(), NULL, NULL)!=0) {
        printf("Couldn't open %s, does it exist?\n", filename.c_str());
        return false;
    }
    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
        return false; // Couldn't find stream information
    }
    av_dump_format(pFormatCtx, 0, filename.c_str(), 0);
    // Find the first video stream
    for(unsigned int i=0; i<pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            video_stream=i;
            break;
        }
    }
    if(video_stream==-1) {
        printf("Did not find a video stream.\n");
        return false;
    }
    // Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[video_stream]->codec;

    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        printf("Unsupported codec (codec not found)!\n");
        return false; // Codec not found
    }
    // Copy context
    pCodecCtx2 = avcodec_alloc_context3(pCodec);
    if(avcodec_copy_context(pCodecCtx2, pCodecCtx) != 0) {
        printf("Couldn't copy codec context");
        return false;
    }
    // Open codec
    if(avcodec_open2(pCodecCtx2, pCodec, NULL)<0) {
        printf("Couldnt open codec.\n");
        return false;
    }
    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(pCodecCtx2->width, pCodecCtx2->height, 
            pCodecCtx2->pix_fmt, pCodecCtx2->width, pCodecCtx2->height,
            PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL );

    width = pCodecCtx2->width;
    height = pCodecCtx2->height;
    
    return true;
}

void video_reader::alloc_frame_internal() {
    int numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx2->width,
                                        pCodecCtx2->height);
    buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();    
}
void video_reader::dealloc_frame_internal() {
    av_free(buffer);
    av_free(pFrame);
    av_free(pFrameRGB);        
}

uint8_t* video_reader::get_frame_internal() {
    if (!reader_success) { 
        printf("Video was not successfully opened, cannot get a frame.\n");
        return NULL;
    }
    //--------------------------------------------------------------------------
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, width, height);

    // Keep trying to read a frame until we succeed...
    while (true) {
        // Read frame into a packet
        if (av_read_frame(pFormatCtx, &packet)<0) {
            // Free the packet that was allocated by av_read_frame
            av_free_packet(&packet);
            return NULL;
        }
        // If the packet is from a wrong stream, just continue looking...
        if(packet.stream_index!=video_stream) {
            av_free_packet(&packet);
            continue;
        }
        // Decode video frame
        int frameFinished;
        avcodec_decode_video2(pCodecCtx2, pFrame, &frameFinished, &packet);        
        if (!frameFinished) {
            av_free_packet(&packet);
            continue; // is this the right behavior??
        }
        break;
    }
    
    // managed to get a frame, so convert from its native format to RGB
    sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                pFrame->linesize, 0, pCodecCtx2->height,
                pFrameRGB->data, pFrameRGB->linesize);       
    return pFrameRGB->data[0];
} 

float video_reader::get_fps() const {
    if (video_stream < 0) { return 0.0f; }
    if (pFormatCtx) {
        return av_q2d(pFormatCtx->streams[video_stream]->r_frame_rate);
    } else { return 0.0f; }
}

void video_reader::close() { 
    if (!reader_success) { return; } // no video open..
    
    // Close the scaling context:
    if (sws_ctx) { sws_freeContext(sws_ctx); sws_ctx = NULL; }
    // Close the codecs
    if (pCodecCtx) { avcodec_close(pCodecCtx); pCodecCtx = NULL; }
    if (pCodecCtx2) { avcodec_close(pCodecCtx2); pCodecCtx2 = NULL; }
    // Close the video file
    if (pFormatCtx) { avformat_close_input(&pFormatCtx); pFormatCtx = NULL; }
    // apparently this does not need to be closed
    pCodec = NULL;
    // set fields to default values.
    filename = "";
    width = 0;
    height = 0;
    chan = 0;
    video_stream = -1;
    reader_success = false;
}
