#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libavdevice/avdevice.h"

int main() {
    int ret;
    AVFrame *frame;
    AVPacket pkt;

    //apro l'input
    AVInputFormat* schermo=av_find_input_format("x11grab") ;
    AVFormatContext* schermoContext;
    
    ret=avformat_open_input(&schermoContext,":0.0+10,250",schermo,NULL);

    //FINE 

    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!codec) {
        printf("Codec not found\n");
        exit(1);
    }

    AVCodecContext *c = avcodec_alloc_context3(codec);
    if (!c) {
        printf("Could not allocate video codec context\n");
        exit(1);
    }

    c->bit_rate = 400000;
    c->width = 320;
    c->height = 200;
    c->time_base = (AVRational){1, 25};
    c->pix_fmt = AV_PIX_FMT_YUVJ420P;

    if (avcodec_open2(c, codec, NULL) < 0) {
        printf("Could not open codec\n");
        exit(1);
    }

    frame = av_frame_alloc();

    if (!frame) {
        printf("Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32);
    if (ret < 0) {
        printf("Could not allocate raw picture buffer\n");
        exit(1);
    }

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    /* prepare a dummy image */
    /* Y */
    for (int y = 0; y < c->height; y++) {
        for (int x = 0; x < c->width; x++) {
            frame->data[0][y * frame->linesize[0] + x] = x + y;
        }
    }

    /* Cb and Cr */
    for (int y = 0; y < c->height / 2; y++) {
        for (int x = 0; x < c->width / 2; x++) {
            frame->data[1][y * frame->linesize[1] + x] = 128 + y;
            frame->data[2][y * frame->linesize[2] + x] = 64 + x;
        }
    }

    frame->pts = 1;

    //avcodec_encode_video2(pCodecCtx, pPacket, pFrame, &got_picture);

    //avcodec_send_frame(pCodecCtx, pFrame);
    //avcodec_receive_packet(pCodecCtx, pPacket);

    //ret = avcodec_encode_video2(c, &pkt, frame, &got_output);

    avcodec_send_frame(c, frame);
    ret = avcodec_receive_packet(c, &pkt);

    if (ret < 0) {
        printf("Error encoding frame\n");
        exit(1);
    }

    printf("got frame\n");
    FILE *f = fopen("x.jpg", "wb");
    fwrite(pkt.data, 1, pkt.size, f);
    av_packet_unref(&pkt);

    avcodec_close(c);
    av_free(c);
    av_freep(&frame->data[0]);
    av_frame_free(&frame);

    printf("\n");

    return 0;
}
