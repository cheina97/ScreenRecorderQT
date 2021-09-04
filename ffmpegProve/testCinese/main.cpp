//TODO : passare da NULL a nullptr
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>

#include "libavutil/imgutils.h"
}

using namespace std;

//Inizializzate in initDataStructures
AVFormatContext *pFormatCtx;  //contiene lo stream del desktop
AVCodecContext *pCodecCtx;    // codec dello stream che arriva dal desktop
AVCodec *pCodec;
AVDictionary *options = NULL;
AVStream *video_st;  //opzioni con cui viene aperto xgrab11

//Serve da setupOutput in poi
AVFrame *pFrame;
int picture_size;
uint8_t *picture_buf;
AVPacket pkt;
int framenum = 300;  //Frames to encode

typedef struct {
    int screen_number;
    int x_offset;
    int y_offset;
    int width;
    int height;
} X11_GrabParameters;

void initDataStructures(X11_GrabParameters x11gp) {
    // cout << string(getenv("DISPLAY")) + "." + to_string(x11gp.screen_number) + "+" + to_string(x11gp.x_offset) + "," + to_string(x11gp.y_offset) << endl;
    int i, videoindex;

    pFormatCtx = avformat_alloc_context();

    avdevice_register_all();

    if (av_dict_set(&options, "framerate", "30", 0) < 0) {
        cout << "\nerror in setting framerate value" << endl;
        exit(1);
    }
    if (av_dict_set(&options, "video_size", (to_string(x11gp.width) + "x" + to_string(x11gp.height)).c_str(), 0) < 0) {
        cout << "\nerror in setting video_size" << endl;
        exit(1);
    }

    if (av_dict_set(&options, "preset", "medium", 0) < 0) {
        cout << "\nerror in setting preset values" << endl;
        exit(1);
    }

    AVInputFormat *ifmt = av_find_input_format("x11grab");
    if (avformat_open_input(&pFormatCtx, (string(getenv("DISPLAY")) + "." + to_string(x11gp.screen_number) + "+" + to_string(x11gp.x_offset) + "," + to_string(x11gp.y_offset)).c_str(), ifmt, &options) != 0) {
        cout << "Couldn't open input stream." << endl;
        exit(-1);
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        cout << "Couldn't find stream information." << endl;
        exit(-1);
    }

    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams && videoindex == -1; i++)
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
        }
    if (videoindex == -1) {
        cout << "Didn't find a video stream." << endl;
        exit(-1);
    }
    video_st = pFormatCtx->streams[videoindex];
    //FIXME : EHHHH
    video_st->time_base.num = 1;
    video_st->time_base.den = 30;
    //deprecato: pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);

    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        cout << "Codec not found."
             << endl;
        exit(-1);
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        cout << "Could not open codec.\n"
             << endl;
        exit(-1);
    }

    cout << "Data structures inited" << endl;
}

void setupOutput(string outFileName) {
    pFormatCtx->oformat = av_guess_format(NULL, outFileName.c_str(), NULL);
    if (pFormatCtx->oformat == NULL) {
        cout << "Impossible to guess output format" << endl;
        exit(-1);
    }
    if (avio_open(&pFormatCtx->pb, outFileName.c_str(), AVIO_FLAG_READ_WRITE) < 0) {
        cout << "Failed to open output file! \n"
             << endl;
        exit(-1);
    }
    //video_st = avformat_new_stream(pFormatCtx, 0);
    //if (video_st == NULL) {
    //    exit(-1);
    //}

    //NOTE: Show some Information
    av_dump_format(pFormatCtx, 0, outFileName.c_str(), 1);

    pFrame = av_frame_alloc();

    //Return the size in bytes of the amount of data required to store an image with the given parameters.
    picture_size = av_image_get_buffer_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, 1);

    picture_buf = (uint8_t *)av_malloc(picture_size);

    //Setup the data pointers and linesizes based on the specified image parameters and the provided array.
    if (av_image_fill_arrays(pFrame->data, pFrame->linesize, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, 1) < 0) {
        cout << "Error in filling image arrays" << endl;
        exit(-1);
    }

    if (avformat_write_header(pFormatCtx, NULL) < 0) {
        cout << "Error in writing outputfile header" << endl;
        exit(-1);
    }

    av_new_packet(&pkt, picture_size);

    for (int i = 0; i < framenum; i++) {

        //Decode
        // deprecata : int ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        int ret = av_read_frame(pFormatCtx, &pkt);

        if (ret < 0) {
            cout << "av_read_frame error" << endl;
        }

        ret = avcodec_send_packet(pCodecCtx, &pkt);

        if (ret < 0) {
            printf("Decode Error part 1.\n");
            exit(-1);
        }

        ret = avcodec_receive_frame(pCodecCtx, pFrame);  //got_picture

        if (ret != 0) {
            printf("Decode Error part 1.\n");
            exit(-1);
        }
        //TODO: alloc di pkt data?

        /*  pFrame->data[0] = &(pkt.data[0]);
        pFrame->data[1] = &(pkt.data[1]);
        pFrame->data[2] = &(pkt.data[2]); */

        //PTS
        //pFrame->pts=i;
        pFrame->pts = i * (video_st->time_base.den) / ((video_st->time_base.num) * 30);
        cout << "qui ci sono" << endl;
        

        //Encode
        // deprecata: int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
        ret = avcodec_send_frame(pCodecCtx, pFrame);
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN)) cout << "input is not accepted in the current state - user must read output with avcodec_receive_packet() (once all output is read, the packet should be resent, and the call will not fail with EAGAIN)." << endl;
            if (ret == AVERROR_EOF) cout << "the encoder has been flushed, and no new frames can be sent to it" << endl;
            if (ret == AVERROR(EINVAL)) cout << "codec not opened, refcounted_frames not set, it is a decoder, or requires flush" << endl;
            if (ret == AVERROR(ENOMEM)) cout << "failed to add packet to internal queue, or similar" << endl;

            cout << "Failed to encode! error: " << ret << endl;
            exit(-1);
        }

        ret = avcodec_receive_packet(pCodecCtx, &pkt);

        if (ret == 0) {
            cout << "Succeed to encode frame: " << i << "size: " << pkt.size << endl;
            pkt.stream_index = video_st->index;
            ret = av_write_frame(pFormatCtx, &pkt);
            av_packet_unref(&pkt);
        }
    }

    /* int ret = flush_encoder(pFormatCtx, 0);
    if (ret < 0) {
        cout<<"Flushing encoder failed\n"<<endl;
        exit(-1);
    } */

    //Write file trailer
    av_write_trailer(pFormatCtx);

    //Clean
    if (video_st) {
        avcodec_close(pCodecCtx);
        av_free(pFrame);
        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
    //aggiunti da noi
    avformat_close_input(&pFormatCtx);
}

int main(int argc, char const *argv[]) {
    X11_GrabParameters x11gp;
    x11gp.screen_number = 0;
    x11gp.x_offset = 0;
    x11gp.y_offset = 0;
    x11gp.height = 200;
    x11gp.width = 424;
    initDataStructures(x11gp);
    //cout << "Stampa parametri pCodecCtx 1" << endl
    //<< "pCodecCtx->codec_id: " << pCodecCtx->codec_id << endl
    //<< "pCodecCtx->codec_type: " << pCodecCtx->codec_type << endl
    //<< "pCodecCtx->pix_fmt: " << pCodecCtx->pix_fmt << endl
    //<< "pCodecCtx->width: " << pCodecCtx->width << endl
    //<< "pCodecCtx->height: " << pCodecCtx->height << endl
    //<< "pCodecCtx->bit_rate: " << pCodecCtx->bit_rate << endl
    //<< "pCodecCtx->gop_size: " << pCodecCtx->gop_size << endl
    //<< "pCodecCtx->time_base.num: " << pCodecCtx->time_base.num << endl
    //<< "pCodecCtx->time_base.den: " << pCodecCtx->time_base.den << endl
    //<< "pCodec name " << string(pCodec->name) << endl
    //<< "pCodec descr->name " << string(pCodecCtx->codec_descriptor->name) << endl
    //<< "pCodec descr->long name " << string(pCodecCtx->codec_descriptor->long_name) << endl
    //<< "pCodec descr->mime " << pCodecCtx->codec_descriptor->mime_types << endl
    //<< "pCodec descr->id " << pCodecCtx->codec_descriptor->id << endl
    //<< "-----------------------------------------------" << endl;
    setupOutput("output.h264");

    cout << "Termine" << endl;
    return 0;
}
