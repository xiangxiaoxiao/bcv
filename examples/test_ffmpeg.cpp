#include <cstdlib>
#include "bcv_io.cpp"
#include "math.h"
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
}
#define MAX_PATH 2048

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);
int video_to_images();
int images_to_video();

int main() { 
    images_to_video();
    //video_to_images();
}

int images_to_video() {
    int rows, cols, chan;
    vector<vector<uchar> > images;
    char in_fname[256];
    for (int i = 0; i < 5; ++i) {
        sprintf(in_fname, "frame%d.ppm.png", i);
        printf("loading %s\n", in_fname);
        vector<uchar> img = bcv_imread<uchar>(in_fname, &rows, &cols, &chan);
        images.push_back( img );
    }
    // -------------------------------------------------------------------------
    char filename[256];
    sprintf(filename, "output.mp4");
    AVCodecID codec_id = AV_CODEC_ID_MPEG4;

    AVCodec *codec;
    AVCodecContext *c= NULL;
    int i, ret, x, y, got_output;
    FILE *f;
    AVFrame *frame;
    AVPacket pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7};
    printf("Encode video file %s\n", filename);

    /* find the mpeg1 video encoder */
    //avcodec_register_all();
    av_register_all();
    AVOutputFormat* avOutputFormat = av_guess_format(NULL, filename, NULL);
    if (!avOutputFormat) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avOutputFormat = av_guess_format("mpeg", NULL, NULL);
    }    
    if (!avOutputFormat) {
        fprintf(stderr, "Could not find suitable output format\n");
        exit(1);
    }

    AVFormatContext* avFormatContext = avformat_alloc_context();
    if (!avFormatContext) {
        fprintf(stderr, "Memory error\n");
        exit(1);
    }
    avFormatContext->oformat = avOutputFormat;

    codec_id = avOutputFormat->video_codec;
    codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }


    AVStream* video_avstream = avformat_new_stream(avFormatContext, codec);
    if (!video_avstream) {
        fprintf(stderr, "Could not alloc stream\n");
        exit(1);
    }

    if (video_avstream->codec == NULL) {
    fprintf(stderr, "AVStream codec is NULL\n");
    exit(1);
    }

    /*
    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }*/

    c = video_avstream->codec;
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = cols;
    c->height = rows;
    /* frames per second */
    c->time_base = (AVRational){1,25};
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    //if (codec_id == AV_CODEC_ID_H264)
    //  av_opt_set(c->priv_data, "preset", "slow", 0);

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    //frame = av_frame_alloc();
    //if (!frame) {
    //    fprintf(stderr, "Could not allocate video frame\n");
    //    exit(1);
    //}
    
    frame = avcodec_alloc_frame();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;

    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
                         c->pix_fmt, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        exit(1);
    }

    /* open the output file, if needed */
    if (!(avOutputFormat->flags & AVFMT_NOFILE)) {
        if (avio_open(&avFormatContext->pb, filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Could not open '%s'\n", filename);
            return 1;
        }
    }

    // some formats want stream headers to be separate
    if(avFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    avformat_write_header(avFormatContext, NULL);




    /* encode 1 second of video */
    for (i = 0; i < images.size(); i++) {
        av_init_packet(&pkt);
        pkt.data = NULL;    
        // packet data will be allocated by the encoder
        pkt.size = 0;

        fflush(stdout);
        /* prepare a dummy image */
        /* Y */
        for (y = 0; y < c->height; y++) {
            for (x = 0; x < c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = images[i][3*x+3*y*cols];
            }
        }
        printf("wrote image. %d\n", i);

        frame->pts = i;

        /* encode the image */
        ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }

        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            
            ret = av_write_frame(avFormatContext, &pkt);
            if (ret < 0) {
                fprintf(stderr, "Error writing frame\n");
                exit(1);
            }

            //fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }

    /* get the delayed frames */
    for (got_output = 1; got_output; i++) {
        fflush(stdout);

        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }

        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            ret = av_write_frame(avFormatContext, &pkt);
            if (ret < 0) {
                fprintf(stderr, "Error writing frame\n");
                exit(1);
            }
            //fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }

    /* add sequence end code to have a real mpeg file */
    //fwrite(endcode, 1, sizeof(endcode), f);
    //fclose(f);
    printf("now i am here.\n");

    /* free the streams */

    for(i = 0; i < avFormatContext->nb_streams; i++) {
        av_freep(&avFormatContext->streams[i]->codec);
        av_freep(&avFormatContext->streams[i]);
    }
    
    avcodec_close(c);
    //av_free(c);
    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    
    if (!(avOutputFormat->flags & AVFMT_NOFILE)) {
        /* close the output file */
        avio_close(avFormatContext->pb);
        printf("closing.\n");
    }

    /* free the stream */
    av_free(avFormatContext);

    printf("\n");
}


int video_to_images() {
    av_register_all();
    const char* fname = "CRBVSE1.mp4";
    AVFormatContext *pFormatCtx = NULL;
    if (avformat_open_input(&pFormatCtx, fname, NULL, NULL)!=0) {
        printf("couldnts open %s\n", fname);
        return -1;
    }
    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
        return -1; // Couldn't find stream information
    }
    av_dump_format(pFormatCtx, 0, fname, 0);
    //

    AVCodecContext *pCodecCtxOrig = NULL;
    AVCodecContext *pCodecCtx = NULL;

    // Find the first video stream
    int videoStream=-1;
    for(int i=0; i<pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    }
    if(videoStream==-1) {
        printf("didnt find a video stream.\n");
        return -1; // Didn't find a video stream
    }
    printf("got video stream %d\n", videoStream);
    // Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    printf("got pointer to codec context.\n");
    // The stream's information about the codec is in what we call the 
    // "codec context." This contains all the information about the codec that 
    // the stream is using, and now we have a pointer to it. 
    // But we still have to find the actual codec and open it:
    AVCodec *pCodec = NULL;
    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    } else {
        printf("found decoder.\n");
    }
    // Copy context
    AVCodecContext* pCodecCtx2 = avcodec_alloc_context3(pCodec);
    printf("allocated context.\n");
    if(avcodec_copy_context(pCodecCtx2, pCodecCtx) != 0) {
        printf("Couldn't copy codec context");
        return -1; // Error copying codec context
    }
    // Open codec
    printf("opening codec.\n");
    if(avcodec_open2(pCodecCtx2, pCodec, NULL)<0) {
        return -1; // Could not open codec
    }
    //--------------------------------------------------------------------------
    // actually get a place to store the Frame
    // Allocate video frame
    AVFrame *pFrame=av_frame_alloc();
    AVFrame *pFrameRGB=av_frame_alloc();
    if (pFrameRGB == NULL) { return -1; }

    printf("width:%d height:%d\n", pCodecCtx2->width, pCodecCtx2->height);
    //printf("width:%d height:%d\n", pCodec->width, pCodec->height);

    // get a place to put raw data when we convert it.
    // Determine required buffer size and allocate buffer
    int numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx2->width,
                                pCodecCtx2->height);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
                    pCodecCtx2->width, pCodecCtx2->height);

    printf("done.\n");

    // -----------------------------------------------------------------------//
    //                          read the data
    // -----------------------------------------------------------------------//
    int frameFinished;
    AVPacket packet;
    // initialize SWS context for software scaling
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx2->width,
        pCodecCtx2->height, pCodecCtx2->pix_fmt,
        pCodecCtx2->width, pCodecCtx2->height,
        PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL );

    int i=0;
    while(av_read_frame(pFormatCtx, &packet)>=0) {
      // Is this a packet from the video stream?
      if(packet.stream_index==videoStream) {
        // Decode video frame
        avcodec_decode_video2(pCodecCtx2, pFrame, &frameFinished, &packet);
        
        // Did we get a video frame?
        if(frameFinished) {
        // Convert the image from its native format to RGB
            sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                pFrame->linesize, 0, pCodecCtx2->height,
                pFrameRGB->data, pFrameRGB->linesize);
        
            // Save the frame to disk
            if(i<=5) {
                SaveFrame(pFrameRGB, pCodecCtx2->width, pCodecCtx2->height, i);
                i++;
            }
        }
      }
      // Free the packet that was allocated by av_read_frame
      av_free_packet(&packet);
    }
    // Free the RGB image
    av_free(buffer);
    av_free(pFrameRGB);

    // Free the YUV frame
    av_free(pFrame);

    // Close the codecs
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtx2);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return 0;
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;
  
  // Open file
  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;
  
  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
  
  // Write pixel data
  for(y=0; y<height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
  
  // Close file
  fclose(pFile);
}
// ---------------------------------------------------------------------------//



/*
bool SaveFrame(int nszBuffer, uint8_t *buffer, char cOutFileName[]);
bool WriteJPEG (AVCodecContext *pCodecCtx, AVFrame *pFrame, char cFileName[], PixelFormat pix, uint8_t *buffer, int numBytes);
bool PullFrame( char cInPath[], char cFileName[], char cImgOutPath[], char cImgOutName[], int nTimeStamp );

int main() { 
    //const char* in_fname = "./images/arches.jpg";
    //const char* out_fname = "out.jpg";
    //int rows, cols, chan;
    //printf("Trying to read %s\n", in_fname);
    //vector<uchar> img = bcv_imread<uchar>(in_fname, &rows, &cols, &chan);
    //printf("read image, size: %dx%dx%d\n", rows, cols, chan); 
    //printf("Trying to write %s\n", out_fname);
    //bcv_imwrite<uchar>(out_fname, img, rows, cols, chan);
    PullFrame( "", "CRBVSE1.mp4", "", "out.jpeg", 10);
    return 0;
}


bool SaveFrame(int nszBuffer, uint8_t *buffer, char cOutFileName[]) {
   bool bRet = false;
   FILE *pFile;
   if( nszBuffer > 0 ) {
      pFile = fopen(cOutFileName,"wb");
      if( pFile != 0 ) {
         fwrite(buffer, sizeof(uint8_t), nszBuffer, pFile);
         bRet = true;
         fclose(pFile);
      }
   }
   return bRet;
}

bool WriteJPEG (AVCodecContext *pCodecCtx, AVFrame *pFrame, char cFileName[], PixelFormat pix, uint8_t *buffer, int numBytes) {
   bool bRet = false;
   AVCodec *pMJPEGCodec = avcodec_find_encoder( pCodecCtx->codec_id );
   AVCodecContext *pMJPEGCtx   = avcodec_alloc_context3(pMJPEGCodec);
   if( pMJPEGCtx )
   {
      pMJPEGCtx->bit_rate = pCodecCtx->bit_rate;
      pMJPEGCtx->width = pCodecCtx->width;
      pMJPEGCtx->height = pCodecCtx->height;
      pMJPEGCtx->pix_fmt = pix;
      pMJPEGCtx->codec_id = pCodecCtx->codec_id;
      pMJPEGCtx->codec_type = AVMEDIA_TYPE_VIDEO;
      pMJPEGCtx->time_base.num = pCodecCtx->time_base.num;
      pMJPEGCtx->time_base.den = pCodecCtx->time_base.den;

      if( pMJPEGCodec && (avcodec_open2( pMJPEGCtx, pMJPEGCodec, NULL) >= 0) )
      {
         pMJPEGCtx->qmin = pMJPEGCtx->qmax = 3;
         pMJPEGCtx->mb_lmin = pMJPEGCtx->lmin = pMJPEGCtx->qmin * FF_QP2LAMBDA;
         pMJPEGCtx->mb_lmax = pMJPEGCtx->lmax = pMJPEGCtx->qmax * FF_QP2LAMBDA;
         pMJPEGCtx->flags |= CODEC_FLAG_QSCALE;
         pFrame->quality = 10;
         pFrame->pts = 0;
         int szBufferActual = avcodec_encode_video(pMJPEGCtx, buffer, numBytes, pFrame);
            
         printf("ready to save.\n");
         if( SaveFrame(szBufferActual, buffer, cFileName ) )
            bRet = true;
         avcodec_close(pMJPEGCtx);
      }
   }
   return bRet;
} 

bool PullFrame( char cInPath[], char cFileName[], char cImgOutPath[], char cImgOutName[], int nTimeStamp ) {
   bool bRet = false;
   int videoStream   = -1;
    AVFrame *pFrame=NULL; 
    AVFrame *pFrameRGB=NULL;
   AVPacket packet;
    int frameFinished=0;

   AVDictionary *optionsDict = NULL;
  
   char cFileLocale[MAX_PATH];
   sprintf( cFileLocale, "%s%s", cInPath, cFileName);

   avformat_network_init();
    av_register_all();

   AVFormatContext *pFormatCtx=avformat_alloc_context();
    if(avformat_open_input(&pFormatCtx, cFileLocale, NULL, NULL) == 0 )
   {
      if(avformat_find_stream_info(pFormatCtx, NULL) >= 0 )
      {
         av_dump_format(pFormatCtx, 0, cFileName, 0);
         for(int i=0; i<(int)pFormatCtx->nb_streams; i++)
         {
            if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
            {
               videoStream=i;
               break;
            }
         }	

         if(videoStream >= 0 )  
         {
            AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;
            printf("width=%d, height=%d\n", pCodecCtx->width, pCodecCtx->height);

            AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
   
            if(pCodec != NULL)
            {
               if( avcodec_open2(pCodecCtx, pCodec, &optionsDict) >= 0 )
               {
                  pFrame=avcodec_alloc_frame();
                  pFrameRGB=avcodec_alloc_frame();
                  if(pFrameRGB != NULL)
                  {   
                  	 printf("frame not null!\n");
                     int nFramesSaved = 0;
                     int numBytes=avpicture_get_size(pFormatCtx->PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height);
                     uint8_t *buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

                     avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height);
                     nTimeStamp = nTimeStamp * AV_TIME_BASE;
                     int nRet = av_seek_frame(pFormatCtx, -1, nTimeStamp + pFormatCtx->start_time, AVSEEK_FLAG_ANY );
                     if( nRet < 0 )
                        nRet = av_seek_frame(pFormatCtx, -1, nTimeStamp + pFormatCtx->start_time, AVSEEK_FLAG_BACKWARD );
                     if( nRet >= 0 )
                     {
                        int i=0;
                        while(av_read_frame(pFormatCtx, &packet)>=0) 
                        {
		                   printf("reading frame!\n");
                           if(packet.stream_index==videoStream) 
                           {
                              avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
                              if(frameFinished) 
                              {
		                   		 printf("frame is finished!\n");
                                 if(++i<=5)
                                 {
                                    char cFileName[MAX_PATH];
                                    sprintf( cFileName, "%sframe%d.jpg", cImgOutPath, i );
                                    printf("filename %s\n", cFileName);
                                    WriteJPEG(pCodecCtx, pFrame, cFileName, PIX_FMT_YUVJ420P, buffer, numBytes);
                                 }
                                 else
                                 {
                                    av_free_packet(&packet);
                                    break;
                                 }
                              }
                           }
                           av_free_packet(&packet);
                        }      
                     }
                  }
                  else
                     bRet = false;
                  
                  av_free(pFrameRGB);
                  av_free(pFrame);
                  avcodec_close(pCodecCtx);
               }
               else
                  bRet = false;
            }
            else
               bRet = false;
         }
         else
            bRet = false;
      }
      else
         bRet = false;
      av_close_input_file(pFormatCtx);
   }
   return bRet;
}
*/