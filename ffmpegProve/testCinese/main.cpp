//TODO : passare da NULL a nullptr
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
}

using namespace std;

//Inizializzate in initDataStructures
AVFormatContext *pFormatCtx;  //contiene lo stream del desktop
AVCodecContext *pCodecCtx;    // codec dello stream che arriva dal desktop
AVCodec *pCodec;
AVDictionary *options = NULL;  //opzioni con cui viene aperto xgrab11

//Serve da setupOutput in poi
AVStream *video_st;

typedef struct {
    int screen_number;
    int x_offset;
    int y_offset;
    int width;
    int height;
} X11_GrabParameters;

void initDataStructures(X11_GrabParameters x11gp) {
    cout << string(getenv("DISPLAY")) + "." + to_string(x11gp.screen_number) + "+" + to_string(x11gp.x_offset) + "," + to_string(x11gp.y_offset) << endl;
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
        printf("\nerror in setting preset values");
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
        printf("Could not open codec.\n");
        exit(-1);
    }

    cout << "Data structures inited" << endl;
}

void setupOutput(string outFileName) {
    pFormatCtx->oformat = av_guess_format(NULL, outFileName.c_str(), NULL);
    if (avio_open(&pFormatCtx->pb, outFileName.c_str(), AVIO_FLAG_READ_WRITE) < 0) {
        printf("Failed to open output file! \n");
        exit(-1);
    }
    video_st = avformat_new_stream(pFormatCtx, 0);
    if (video_st == NULL) {
        exit(-1);
    }

    //pCodecCtx = video_st->codec;
    //pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
    pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    //pCodecCtx->width = in_w;
    //pCodecCtx->height = in_h;
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->gop_size = 250;

    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 30;
}

int main(int argc, char const *argv[]) {
    X11_GrabParameters x11gp;
    x11gp.screen_number = 0;
    x11gp.x_offset = 0;
    x11gp.y_offset = 0;
    x11gp.height = 1080;
    x11gp.width = 1920;
    initDataStructures(x11gp);
    setupOutput("output.mp4");
    
    cout << "Stampa parametri pCodecCtx" << endl
         << "pCodecCtx->codec_id: " << pCodecCtx->codec_id << endl
         << "pCodecCtx->codec_type: " << pCodecCtx->codec_type << endl
         << "pCodecCtx->pix_fmt: " << pCodecCtx->pix_fmt << endl
         << "pCodecCtx->width: " << pCodecCtx->width << endl
         << "pCodecCtx->height: " << pCodecCtx->height << endl
         << "pCodecCtx->bit_rate: " << pCodecCtx->bit_rate << endl
         << "pCodecCtx->gop_size: " << pCodecCtx->gop_size << endl
         << "pCodecCtx->time_base.num: " << pCodecCtx->time_base.num << endl
         << "pCodecCtx->time_base.den: " << pCodecCtx->time_base.den << endl;


    return 0;
}
