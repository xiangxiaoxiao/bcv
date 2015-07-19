#include "video_writer.h"

video_writer::video_writer(const char* fname, int width, int height) {
    this->filename = std::string(fname);
    this->width = width;
    this->height = height;
    av_log_set_level(AV_LOG_WARNING);
    av_register_all();
}

video_writer::~video_writer() {
    if (!video_finished) { finish(); }
    if (codec_context) { av_free(codec_context); }
}

void video_writer::set_fps(float fps) {
    if (!video_prepared) { fps_ = fps; }
    else { printf("Cannot change video FPS.\n"); }
}

void video_writer::set_bitrate(int br) {
    if (!video_prepared) { bitrate_ = br; }
    else { printf("Cannot change video bitrate.\n"); }
}

void video_writer::prepare() {
    assert( !video_prepared && "video not prepared." );
    video_prepared = true;

    if (!format_) {
        fmt = av_guess_format(NULL, filename.c_str(), NULL);
        if (!fmt) { printf("Could not guess format."); return; }
    }
    if (!format_context) {
        avformat_alloc_output_context2(&format_context, fmt, NULL, filename.c_str());
    }
    snprintf(format_context->filename, sizeof(format_context->filename), 
                                                    "%s", filename.c_str());

    AVCodecID codecId = fmt->video_codec; //AV_CODEC_ID_MPEG4;
    AVCodec* videoCodec = avcodec_find_encoder(codecId);
    if (!videoCodec) { 
        printf("Could not find codec");
        free_format_context();
        return;
    }
  
    if ( avformat_query_codec(fmt, codecId, FF_COMPLIANCE_NORMAL) != 1) {
        printf("the selected codec is not supported in this container.");
        free_format_context();
        return;
    }
      
    if (!stream_) {
        stream_ = avformat_new_stream(format_context, NULL);
        if (!stream_) {
            printf("Could not create stream.\n");
            free_format_context();
            return;
        }
    }

    // set video parameters
    if (!set_codec_context(stream_->codec, videoCodec)) {
        free_format_context();
        return;
    }
    avformat_write_header(format_context, NULL);
}

//! Returns "true" on success
bool video_writer::set_codec_context(AVCodecContext* codec, AVCodec* videoCodec) {

    PixelFormat pixFMT = PIX_FMT_YUV420P; // set default format.
    if (videoCodec->pix_fmts != NULL) {
        pixFMT = *videoCodec->pix_fmts;
    } else {
        printf("Could not set pixel format\n");
        free_format_context();
        return false;
    }

    codec_context = codec;
    avcodec_get_context_defaults3(codec_context, videoCodec);
    
    codec_context->pix_fmt = pixFMT;
    codec_context->bit_rate = bitrate_;
    codec_context->bit_rate_tolerance = bitrateTolerance_;
    codec_context->width = width;
    codec_context->height = height;
    // apparently deprecated
    codec_context->time_base.num = 1000;
    codec_context->time_base.den = int(fps_ * 1000.0f);
    stream_->time_base.num = 1000;
    stream_->time_base.den = int(fps_ * 1000.0f);

    codec_context->gop_size = gopSize_;
    codec_context->mb_decision = FF_MB_DECISION_SIMPLE;

    if (!strcmp(format_context->oformat->name, "mp4") || 
        !strcmp(format_context->oformat->name, "mov") || 
        !strcmp(format_context->oformat->name, "3gp")) {
        codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    
    if (format_context->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    
    if (avcodec_open2(codec_context, videoCodec, NULL) < 0) {
        printf("unable to open codec");
        return false;
    }
                  
    if (!(fmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&format_context->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
          printf("unable to open file");
          return false;
        }
    }
    return true;
}


void video_writer::add_frame_internal(const uint8_t* data) {
    // on first frame, prepare the video by setting up all info and headers
    if (!video_prepared) { 
        prepare(); 
    }
    AVPicture picture;

    // buffer for input image
    int picSize = avpicture_get_size(PIX_FMT_RGB24, width, height);
    uint8_t* buffer = (uint8_t*)av_malloc(picSize);
    
    // blank the values - this initialises stuff and seems to be needed
    avpicture_fill(&picture, buffer, PIX_FMT_RGB24, width, height);
    
    // copy data from input image
    if (data == NULL) {
        memset( picture.data[0], 0, sizeof(uint8_t)*height*width*3 );
    } else {
        memcpy( picture.data[0], data, sizeof(uint8_t)*height*width*3 );
    }

    // now allocate an image frame for the image in the output codec's format...
    AVFrame* output = avcodec_alloc_frame();
    output->format = PIX_FMT_RGB24; //AV_PIX_FMT_YUV420P;
    output->width = width;
    output->height = height;

    // buffer for output image
    picSize = avpicture_get_size(codec_context->pix_fmt, width, height);
    uint8_t* outBuffer = (uint8_t*)av_malloc(picSize);

    av_image_alloc(output->data, output->linesize, width, height, codec_context->pix_fmt, 1);
    
    SwsContext* convertCtx = sws_getContext(width, height, 
                                PIX_FMT_RGB24, width, height,
                                codec_context->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
    
    sws_scale(convertCtx, picture.data, picture.linesize, 0, height, output->data, output->linesize);
  
    int ret = 0;
    if ((format_context->oformat->flags & AVFMT_RAWPICTURE) != 0) {
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = stream_->index;
        pkt.data = (uint8_t*)output;
        pkt.size = sizeof(AVPicture);
        ret = av_interleaved_write_frame(format_context, &pkt);
    } else {
        uint8_t* outbuf = (uint8_t*)av_malloc(picSize);
        ret = avcodec_encode_video(codec_context, outbuf, picSize, output);
        if (ret > 0) {
            AVPacket pkt;
            av_init_packet(&pkt);
            if (codec_context->coded_frame && (unsigned long)(codec_context->coded_frame->pts) != AV_NOPTS_VALUE) {
                pkt.pts = av_rescale_q(codec_context->coded_frame->pts, codec_context->time_base, stream_->time_base);
            }
            if (codec_context->coded_frame && codec_context->coded_frame->key_frame) {
                pkt.flags |= AV_PKT_FLAG_KEY;
            }
            pkt.stream_index = stream_->index;
            pkt.data = outbuf;
            pkt.size = ret;
            ret = av_interleaved_write_frame(format_context, &pkt);
        } else {
            char szError[1024];
            av_strerror(ret, szError, 1024);
            printf(szError);
        }
        av_free(outbuf);
    }
    av_free(outBuffer);
    av_free(buffer);
    av_free(output);
  
    if (ret) { printf("error writing frame to file"); }
}

void video_writer::finish() {
    assert( !video_finished && "video not finished." );
    video_finished = true;

    av_write_trailer(format_context);
    avcodec_close(codec_context);
    if (!(format_context->oformat->flags & AVFMT_NOFILE)) {
        avio_close(format_context->pb);
    }
    free_format_context();
}

void video_writer::free_format_context() {
    if (format_context) { // clear format context
        for (int i = 0; i < format_context->nb_streams; ++i) {
            av_freep(&format_context->streams[i]);
        }
        av_free(format_context);
        format_context = NULL;
    }
    stream_ = NULL;
}