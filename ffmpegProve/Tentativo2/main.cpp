extern "C" {
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#include "unistd.h"
}

unsigned int width = 1920;
unsigned int height = 1080;

static pthread_t thrId;
static void *handleStreamThreadFunc(void *);

static AVFormatContext *avFmtCtx = NULL;
static AVCodecContext *avRawCodecCtx = NULL;
static AVCodecContext *avEncoderCtx;
static AVDictionary *avRawOptions = NULL;
static AVCodec *avDecodec = NULL;
static AVCodec *avEncodec = NULL;
static struct SwsContext *swsCtx = NULL;
static AVPacket *avRawPkt = NULL;
static int videoIndex = -1;
static AVFrame *avYUVFrame = NULL;

int main(int argc, char **argv) {
    //av_register_all();
    avdevice_register_all();

    avFmtCtx = avformat_alloc_context();
    avRawPkt = (AVPacket *)malloc(sizeof(AVPacket));
    char *displayName = getenv("DISPLAY");
    /* Display * display = XOpenDisplay(displayName); 
    int screenNum = DefaultScreen(display);
    width = DisplayWidth(display, screenNum);
    height = DisplayHeight(display, screenNum);
    if (width % 32 != 0) {
        width = width / 32 * 32;
    }
    if (height % 2 != 0) {
        height = height / 2 * 2;
    }
    XCloseDisplay(display); */
    char val[255];
    sprintf(val, "%d*%d", width, height);
    av_dict_set(&avRawOptions, "video_size", val, 0);
    av_dict_set(&avRawOptions, "framerate", "15", 0);

    AVInputFormat *avInputFmt = av_find_input_format("x11grab");
    if (avInputFmt == NULL) {
        printf("av_find_input_format not found......\n");
        return 0;
    }

    if (avformat_open_input(&avFmtCtx, ":1.0+0,0", avInputFmt, &avRawOptions) != 0) {
        printf("Couldn't open input stream.\n");
        return 0;
    }
    if (avformat_find_stream_info(avFmtCtx, &avRawOptions) < 0) {
        printf("Couldn't find stream information.\n");
        return 0;
    }

    for (int i = 0; i < avFmtCtx->nb_streams; ++i) {
        if (avFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }

    if (videoIndex == -1 || videoIndex >= avFmtCtx->nb_streams) {
        printf("Didn't find a video stream.\n");
        return 0;
    }

    //deprecato: avRawCodecCtx = avFmtCtx->streams[videoIndex]->codecpar;
    avRawCodecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(avRawCodecCtx, avFmtCtx->streams[videoIndex]->codecpar);

    avDecodec = avcodec_find_decoder(avRawCodecCtx->codec_id);
    if (avDecodec == NULL) {
        printf("Codec not found.\n");
        return 0;
    }
    if (avcodec_open2(avRawCodecCtx, avDecodec, NULL) < 0) {
        printf("Could not open decodec . \n");
        return 0;
    }

    swsCtx = sws_getContext(avRawCodecCtx->width,
                            avRawCodecCtx->height,
                            avRawCodecCtx->pix_fmt,
                            avRawCodecCtx->width,
                            avRawCodecCtx->height,
                            AV_PIX_FMT_YUV420P,
                            SWS_BICUBIC, NULL, NULL, NULL);

    avYUVFrame = av_frame_alloc();
    //deprecata: int yuvLen = avpicture_get_size(AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height);
    int yuvLen = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height, 1);

    uint8_t *yuvBuf = new uint8_t[yuvLen];
    //deprecata: avpicture_fill((AVPicture *)avYUVFrame, (uint8_t *)yuvBuf, AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height);
    av_image_fill_arrays(avYUVFrame->data, avYUVFrame->linesize, (uint8_t *)yuvBuf, AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height, 1);

    avYUVFrame->width = avRawCodecCtx->width;
    avYUVFrame->height = avRawCodecCtx->height;
    avYUVFrame->format = AV_PIX_FMT_YUV420P;

    avEncodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!avEncodec) {
        printf("h264 codec not found\n");
        return 0;
    }

    avEncoderCtx = avcodec_alloc_context3(avEncodec);

    avEncoderCtx->codec_id = AV_CODEC_ID_H264;
    avEncoderCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    avEncoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    avEncoderCtx->width = width;
    avEncoderCtx->height = height;
    avEncoderCtx->time_base.num = 1;
    avEncoderCtx->time_base.den = 15;
    avEncoderCtx->bit_rate = 128 * 1024 * 8;
    avEncoderCtx->gop_size = 250;
    avEncoderCtx->qmin = 1;
    avEncoderCtx->qmax = 10;

    AVDictionary *param = 0;
    //H.264
    //av_dict_set(&param, "preset", "slow", 0);
    av_dict_set(&param, "preset", "superfast", 0);
    av_dict_set(&param, "tune", "zerolatency", 0);  //实现实时编码

    if (avcodec_open2(avEncoderCtx, avEncodec, &param) < 0) {
        printf("Failed to open video encoder!\n");
        return 0;
    }

    int got_picture = 0;
    int ret = 0;
    int flag = 0;
    int bufLen = 0;
    uint8_t *outBuf = NULL;
    /* int sock = -1;
	struct sockaddr_in toAddr;
	sock = socket(AF_INET, SOCK_DGRAM, 0);;
	if(sock < 0){
 		printf("failed create socket\r\n");
 		return 0;
	}

	memset(&toAddr, 0, sizeof(toAddr));	
	toAddr.sin_family = AF_INET;
	toAddr.sin_addr.s_addr = inet_addr("192.168.1.100");
	toAddr.sin_port = htons(9000); */

    bufLen = width * height * 3;
    outBuf = (uint8_t *)malloc(bufLen);

    AVFrame *avOutFrame;
    avOutFrame = av_frame_alloc();
    //deprecato: avpicture_fill((AVPicture *)avOutFrame, (uint8_t *)outBuf, avEncoderCtx->pix_fmt, avEncoderCtx->width, avEncoderCtx->height);

    av_image_fill_arrays(avOutFrame->data, avOutFrame->linesize, (uint8_t *)outBuf, avEncoderCtx->pix_fmt, avEncoderCtx->width, avEncoderCtx->height, 1);

    AVPacket pkt;  // 用来存储编码后数据
    memset(&pkt, 0, sizeof(AVPacket));
    av_init_packet(&pkt);

    unsigned int pkSn = 0;
    FILE *h264Fp = fopen("out.264", "wb");

    while (1) {
        if (av_read_frame(avFmtCtx, avRawPkt) >= 0) {
            if (avRawPkt->stream_index == videoIndex) {
                //Inizio DECODING
                //deprecato: flag = avcodec_decode_video2(avRawCodecCtx, avOutFrame, &got_picture, avRawPkt);
                flag = avcodec_send_packet(avRawCodecCtx, avRawPkt);
                if (flag < 0) {
                    printf("Errore Decoding part1");
                    break;
                }
                got_picture = avcodec_receive_frame(avRawCodecCtx, avOutFrame);
                //Fine DECODING
                if (got_picture == 0) {
                    sws_scale(swsCtx, avOutFrame->data, avOutFrame->linesize, 0, avRawCodecCtx->height, avYUVFrame->data, avYUVFrame->linesize);

                    got_picture = -1;  //forse non serve
                    //Inizio ENCODING
                    //deprecato: flag = avcodec_encode_video2(avEncoderCtx, &pkt, avYUVFrame, &got_picture);
                    flag = avcodec_send_frame(avEncoderCtx, avYUVFrame);
                    got_picture = avcodec_receive_packet(avEncoderCtx, &pkt);
                    //Fine ENCODING

                    if (flag >= 0) {
                        if (got_picture == 0) {
                            int w = fwrite(pkt.data, 1, pkt.size, h264Fp);
                            /* int len = pkt.size;
                            char* data = (char*)pkt.data;
                            while(len / PACKSIZE) {
                                ret = sendto(sock, data, PACKSIZE, 0, (struct sockaddr *)&toAddr, sizeof(toAddr));
                                if(ret == -1) {
                                   printf("send udp data failed \n");
                                } else {
                                    data += PACKSIZE;
                                    len -= PACKSIZE;
                                }
				                usleep(1000*10);
                            }
                            if(len > 0) {
                                ret = sendto(sock, data, len, 0, (struct sockaddr *)&toAddr, sizeof(toAddr));
                            } */
                        }
                    }
                    av_packet_unref(&pkt);
                    av_packet_unref(avRawPkt);
                }
            }
        }
        usleep(1000 * 500);
    }
    return 0;
}
