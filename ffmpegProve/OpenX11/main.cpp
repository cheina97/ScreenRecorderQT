#include <iostream>
using namespace std;

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libavcodec/avcodec.h"
#include "libavcodec/avfft.h"
#include "libavdevice/avdevice.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"

// libav resample

#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/file.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"

// lib swresample

#include "libswscale/swscale.h"
}

AVDictionary *options;
AVFormatContext *pAVFormatContext;
AVInputFormat *pAVInputFormat;
AVCodec *pAVCodec;
AVCodecContext *pAVCodecContext;
int VideoStreamIndx;

void openCamera(int display_number, int screen_number, int offset_x, int offset_y, int width, int height) {
    string url = ":" + to_string(display_number) + "." + to_string(screen_number) + "+" + to_string(offset_x) + "," + to_string(offset_y);
    // esempio =>  :1.0+10,250"
    int value = 0;
    pAVFormatContext = avformat_alloc_context();  //Allocate an AVFormatContext.
                                                  /*

X11 video input device.
To enable this input device during configuration you need libxcb installed on your system. It will be automatically detected during configuration.
This device allows one to capture a region of an X11 display. 
refer : https://www.ffmpeg.org/ffmpeg-devices.html#x11grab
*/
    /* current below is for screen recording. to connect with camera use v4l2 as a input parameter for av_find_input_format */
    pAVInputFormat = av_find_input_format("x11grab");

    /* setting up the video size (how big are the frames captured, width x height)*/
    value = av_dict_set(&options, "video_size", (to_string(width)+"x"+to_string(height)).c_str(), 0);
    if (value < 0) {
        printf("\nerror in setting video_size");
        exit(1);
    }

    //X11 sintex: :display_number.screen_number[+x_offset,y_offset]
    //display_number -> x11 server port
    //screen_number -> real screen number (in case of an extended screen setup gnome and maybe other de/wm see the group of screen as only one screen)
    value = avformat_open_input(&pAVFormatContext, url.c_str(), pAVInputFormat, &options);
    if (value != 0) {
        printf("\nerror in opening input device, errore %d", value);
        exit(1);
    }

    /* set frame per second */
    value = av_dict_set(&options, "framerate", "30", 0);
    if (value < 0) {
        printf("\nerror in setting dictionary value");
        exit(1);
    }

    value = av_dict_set(&options, "preset", "medium", 0);
    if (value < 0) {
        printf("\nerror in setting preset values");
        exit(1);
    }

    //	value = avformat_find_stream_info(pAVFormatContext,NULL);
    if (value < 0) {
        printf("\nunable to find the stream information");
        exit(1);
    }

    VideoStreamIndx = -1;

    /* find the first video stream index . Also there is an API available to do the below operations */
    for (int i = 0; i < pAVFormatContext->nb_streams; i++)  // find video stream posistion/index.
    {
        if (pAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            VideoStreamIndx = i;
            break;
        }
    }

    if (VideoStreamIndx == -1) {
        printf("\nunable to find the video stream index. (-1)");
        exit(1);
    }

    // assign pAVFormatContext to VideoStreamIndx
    /* Riga seguente deprecata 
        pAVCodecContext = pAVFormatContext->streams[VideoStreamIndx]->codec;
    e sostituita con:     */
    pAVCodecContext = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pAVCodecContext, pAVFormatContext->streams[VideoStreamIndx]->codecpar);
    /////

    pAVCodec = avcodec_find_decoder(pAVCodecContext->codec_id);
    if (pAVCodec == NULL) {
        printf("\nunable to find the decoder");
        exit(1);
    }

    value = avcodec_open2(pAVCodecContext, pAVCodec, NULL);  //Initialize the AVCodecContext to use the given AVCodec.
    if (value < 0) {
        printf("\nunable to open the av codec");
        exit(1);
    }
}

int main(int argc, char const *argv[]) {
    avdevice_register_all();

    //open camera
    openCamera(1, 0, 10, 10, 200, 200);
    printf("Open Camera ok!\n");
    ////

    //init_outputfile

    return 0;
}
