#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

using namespace std;

int main(int argc, char const *argv[]) {
    //PART1 INIT

    AVFormatContext *pFormatCtx;
    int i, videoindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;

    pFormatCtx = avformat_alloc_context();

    avdevice_register_all();

    AVDictionary *options = NULL;

    if (av_dict_set(&options, "framerate", "30", 0) < 0) {
        cout << "\nerror in setting framerate value" << endl;
        exit(1);
    }
    if (av_dict_set(&options, "video_size", "640x480", 0) < 0) {
        cout << "\nerror in setting video_size" << endl;
        exit(1);
    }

    //TODO parametrizzare
    AVInputFormat *ifmt = av_find_input_format("x11grab");
    if (avformat_open_input(&pFormatCtx, ":1.0+0,0", ifmt, &options) != 0) {
        cout << "Couldn't open input stream." << endl;
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        cout << "Couldn't find stream information." << endl;
        return -1;
    }

    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams && videoindex == -1; i++)
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
        }
    if (videoindex == -1) {
        cout << "Didn't find a video stream." << endl;
        return -1;
    }

    //deprecato: pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar );
    
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        cout << "Codec not found."
             << endl;
        return -1;
    }
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

    //FINE INIT CAMERA
    cout<<"open camera ok"<<endl;
    //INIZIO INIT CODEC
    return 0;
}
