#include <time.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

extern "C" {
#include <X11/Xlib.h>
#include <stdlib.h>

#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#include "unistd.h"
}

using namespace std;

typedef struct {
    int width;
    int height;
    int offset_x;
    int offset_y;
    int screen_number;
} X11parameters;

//mem measures
int parseLine(char *line) {
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char *p = line;
    while (*p < '0' || *p > '9') p++;
    line[i - 3] = '\0';
    i = atoi(p);
    return i;
}

int getCurrentVMemUsedByProc() {  //Note: this value is in KB!
    FILE *file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != nullptr) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    //cout << "CurrMemUsedByProc: " << (double)result / 1024 / 1024 << "GB" << endl;
    return result;
}

//speed measures
int readframe_clock = 0;
int readframe_clock_start = 0;
int readframe_clock_end = 0;
int decode_clock = 0;
int decode_clock_start = 0;
int decode_clock_end = 0;
int scaling_clock = 0;
int scaling_clock_start = 0;
int scaling_clock_end = 0;
int encode_clock = 0;
int encode_clock_start = 0;
int encode_clock_end = 0;

AVFormatContext *avFmtCtx = nullptr;
AVCodecContext *avRawCodecCtx = nullptr;
AVCodecContext *avEncoderCtx;
AVDictionary *avRawOptions = nullptr;
AVCodec *avDecodec = nullptr;
AVCodec *avEncodec = nullptr;
struct SwsContext *swsCtx = nullptr;
queue<AVPacket *> avRawPkt_queue;
mutex avRawPkt_queue_mutex;
int videoIndex = -1;
AVFrame *avYUVFrame = nullptr;
X11parameters x11pmt;
bool stop;
string out_file = "out.mp4";
AVOutputFormat *fmt;
AVStream *video_st;
int64_t pts_offset = 0;

FILE *debug;
int memory_limit;
bool memory_error = false;

void memoryCheck_init() {
    memory_limit = getCurrentVMemUsedByProc() + (1024 * 1024);
}

bool memoryCheck_limitSurpassed() {
    if (getCurrentVMemUsedByProc() > memory_limit) {
        return true;
    } else {
        return false;
    }
}

void initScreenSource(X11parameters x11pmt, bool fullscreen, int fps, float scaling_factor) {
    avdevice_register_all();
    avFmtCtx = avformat_alloc_context();

    //aggiunto da me
    //avFmtCtx->flags |= AVFMT_FLAG_FLUSH_PACKETS;

    fmt = av_guess_format("mpeg1video", NULL, NULL);
    avFmtCtx->oformat = fmt;

    char *displayName = getenv("DISPLAY");

    if (fullscreen) {
        x11pmt.offset_x = 0;
        x11pmt.offset_y = 0;
        Display *display = XOpenDisplay(displayName);
        int screenNum = DefaultScreen(display);
        x11pmt.width = DisplayWidth(display, screenNum);
        x11pmt.height = DisplayHeight(display, screenNum);
        XCloseDisplay(display);
    }

    if (x11pmt.width % 32 != 0) {
        x11pmt.width = x11pmt.width / 32 * 32;
    }
    if (x11pmt.height % 2 != 0) {
        x11pmt.height = x11pmt.height / 2 * 2;
    }

    av_dict_set(&avRawOptions, "video_size", (to_string(x11pmt.width) + "*" + to_string(x11pmt.height)).c_str(), 0);
    av_dict_set(&avRawOptions, "framerate", to_string(fps).c_str(), 0);
    av_dict_set(&avRawOptions, "show_region", "1", 0);
    av_dict_set(&avRawOptions, "probesize", "30M", 0);

    AVInputFormat *avInputFmt = av_find_input_format("x11grab");

    if (avInputFmt == nullptr) {
        cout << "av_find_input_format not found......"
             << endl;
        exit(-1);
    }

    if (avformat_open_input(&avFmtCtx, (string(displayName) + "." + to_string(x11pmt.screen_number) + "+" + to_string(x11pmt.offset_x) + "," + to_string(x11pmt.offset_y)).c_str(), avInputFmt, &avRawOptions) != 0) {
        cout << "Couldn't open input stream."
             << endl;
        exit(-1);
    }

    if (avformat_find_stream_info(avFmtCtx, &avRawOptions) < 0) {
        cout << "Couldn't find stream information."
             << endl;
        exit(-1);
    }

    for (int i = 0; i < (int)avFmtCtx->nb_streams; ++i) {
        if (avFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }

    if (videoIndex == -1 || videoIndex >= (int)avFmtCtx->nb_streams) {
        cout << "Didn't find a video stream."
             << endl;
        exit(-1);
    }

    //deprecato: avRawCodecCtx = avFmtCtx->streams[videoIndex]->codecpar;
    avRawCodecCtx = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(avRawCodecCtx, avFmtCtx->streams[videoIndex]->codecpar);

    avDecodec = avcodec_find_decoder(avRawCodecCtx->codec_id);
    if (avDecodec == nullptr) {
        cout << "Codec not found."
             << endl;
        exit(-1);
    }
    if (avcodec_open2(avRawCodecCtx, avDecodec, nullptr) < 0) {
        cout << "Could not open decodec . "
             << endl;
        exit(-1);
    }

    //Open output URL
    if (avio_open(&avFmtCtx->pb, out_file.c_str(), AVIO_FLAG_READ_WRITE) < 0) {
        printf("Failed to open output file! \n");
        exit(-1);
    }

    swsCtx = sws_getContext(avRawCodecCtx->width,
                            avRawCodecCtx->height,
                            avRawCodecCtx->pix_fmt,
                            (int)avRawCodecCtx->width * scaling_factor,
                            (int)avRawCodecCtx->height * scaling_factor,
                            AV_PIX_FMT_YUV420P,
                            SWS_BICUBIC, nullptr, nullptr, nullptr);

    avYUVFrame = av_frame_alloc();
    //deprecata: int yuvLen = avpicture_get_size(AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height);
    int yuvLen = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height, 1);

    uint8_t *yuvBuf = new uint8_t[yuvLen];
    //deprecata: avpicture_fill((AVPicture *)avYUVFrame, (uint8_t *)yuvBuf, AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height);
    av_image_fill_arrays(avYUVFrame->data, avYUVFrame->linesize, (uint8_t *)yuvBuf, AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height, 1);

    avYUVFrame->width = avRawCodecCtx->width;
    avYUVFrame->height = avRawCodecCtx->height;
    avYUVFrame->format = AV_PIX_FMT_YUV420P;

    avEncodec = avcodec_find_encoder(fmt->video_codec);
    if (!avEncodec) {
        cout << "Encoder codec not found"
             << endl;
        exit(-1);
    }

    video_st = avFmtCtx->streams[videoIndex];

    avEncoderCtx = avcodec_alloc_context3(avEncodec);

    avEncoderCtx->codec_id = fmt->video_codec;
    //cout << fmt->video_codec << endl;
    //cout << AV_CODEC_ID_MPEG4 << endl;
    //cout << AV_CODEC_ID_H264 << endl;

    //avEncoderCtx->codec_id =AV_CODEC_ID_HEVC;
    avEncoderCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    avEncoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    avEncoderCtx->width = x11pmt.width * scaling_factor;
    avEncoderCtx->height = x11pmt.height * scaling_factor;
    avEncoderCtx->time_base.num = 1;
    avEncoderCtx->time_base.den = fps;
    avEncoderCtx->bit_rate = 128 * 1024 * 8;
    avEncoderCtx->gop_size = 0;
    avEncoderCtx->qmin = 1;
    avEncoderCtx->qmax = 10;
    avEncoderCtx->max_b_frames = 0;

    //avEncoderCtx->me_range = 16;
    //avEncoderCtx->max_qdiff = 4;
    //avEncoderCtx->qcompress = 0.6;

    AVDictionary *param = 0;
    //H.264
    //av_dict_set(&param, "preset", "slow", 0);
    av_dict_set(&param, "preset", "ultrafast", 0);
    av_dict_set(&param, "tune", "zerolatency", 0);  //实现实时编码

    if (avcodec_open2(avEncoderCtx, avEncodec, &param) < 0) {
        cout << "Failed to open video encoder!"
             << endl;
        exit(-1);
    }

    av_dump_format(avFmtCtx, 0, out_file.c_str(), 1);
}

void getRawPackets(int frameNumber) {
    cout << "Capturing from x11..." << endl;
    readframe_clock_start = clock();
    auto begin = std::chrono::high_resolution_clock::now();
    AVPacket *avRawPkt;
    int64_t cur_pts = 0;

    for (int i = 0; i < frameNumber; i++) {
        avRawPkt = av_packet_alloc();
        if (av_read_frame(avFmtCtx, avRawPkt) < 0) {
            cout << "Error in getting RawPacket from x11" << endl;
        } else {
            cout << "Captured " << i << " raw packets" << endl;
            //getCurrentVMemUsedByProc();
        }
        //prove pts

        cout << "avRawPkt->pts: " << avRawPkt->pts << endl;
        cout << "avRawPkt->dts: " << avRawPkt->dts << endl;
        cout << "avRawPkt->duration: " << avRawPkt->duration << endl;

        cur_pts = avRawPkt->pts;
        avRawPkt->pts = av_rescale_q_rnd(avRawPkt->pts, avFmtCtx->streams[videoIndex]->time_base, {1, 30}, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        avRawPkt->dts = av_rescale_q_rnd(avRawPkt->dts, avFmtCtx->streams[videoIndex]->time_base, {1, 30}, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        avRawPkt->duration = av_rescale_q(avRawPkt->duration, avFmtCtx->streams[videoIndex]->time_base, {1, 30});

        cout << "avRawPkt->pts: " << avRawPkt->pts << endl;
        cout << "avRawPkt->dts: " << avRawPkt->dts << endl;
        cout << "avRawPkt->duration: " << avRawPkt->duration << endl;

        avRawPkt_queue_mutex.lock();
        avRawPkt_queue.push(avRawPkt);
        avRawPkt_queue_mutex.unlock();

        avRawPkt->stream_index = videoIndex;

        if (memoryCheck_limitSurpassed()) {
            memory_error = true;
            break;
        }
        // test for memory av_packet_unref(avRawPkt);
        //if (i == frameNumber / 2 && i % 2 == 0) usleep(5 * 1000 * 1000);
    }

    avRawPkt_queue_mutex.lock();
    stop = true;
    avRawPkt_queue_mutex.unlock();

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
    readframe_clock_end = clock();
    readframe_clock += (readframe_clock_end - readframe_clock_start);
    cout << "Capturing ended successfully, captured " << frameNumber << "raw packets in " << elapsed.count() * 1e-9 << " seconds" << endl
         << endl;
}

void decodeAndEncode(void) {
    cout << "Decoding and encoding raw packets ..." << endl;

    int got_picture = 0;
    int flag = 0;
    int bufLen = 0;
    uint8_t *outBuf = nullptr;

    flag = avformat_write_header(avFmtCtx, NULL);
    if (flag < 0) {
        cout << "Error in writing header" << endl;
        exit(-1);
    }

    bufLen = x11pmt.width * x11pmt.height * 3;
    outBuf = (uint8_t *)malloc(bufLen);

    AVFrame *avOutFrame;
    avOutFrame = av_frame_alloc();
    //deprecato: avpicture_fill((AVPicture *)avOutFrame, (uint8_t *)outBuf, avEncoderCtx->pix_fmt, avEncoderCtx->width, avEncoderCtx->height);

    av_image_fill_arrays(avOutFrame->data, avOutFrame->linesize, (uint8_t *)outBuf, avEncoderCtx->pix_fmt, avEncoderCtx->width, avEncoderCtx->height, 1);

    AVPacket pkt;  // 用来存储编码后数据
    //memset(&pkt, 0, sizeof(AVPacket));
    av_init_packet(&pkt);

    AVPacket *avRawPkt;
    int64_t avRawPkt_dts;
    int i = 0;

    avRawPkt_queue_mutex.lock();
    while (!stop || !avRawPkt_queue.empty()) {
        if (!avRawPkt_queue.empty()) {
            cout << "Remaining " << avRawPkt_queue.size() << " frames" << endl;
            avRawPkt = avRawPkt_queue.front();
            avRawPkt_queue.pop();
            avRawPkt_queue_mutex.unlock();
            if (avRawPkt->stream_index == videoIndex) {
                cout << "Elaborating frame" << i << endl;
                //Inizio DECODING
                //deprecato: flag = avcodec_decode_video2(avRawCodecCtx, avOutFrame, &got_picture, avRawPkt);
                decode_clock_start = clock();
                flag = avcodec_send_packet(avRawCodecCtx, avRawPkt);
                avRawPkt_dts = avRawPkt->dts;
                av_packet_unref(avRawPkt);
                av_packet_free(&avRawPkt);

                if (flag < 0) {
                    cout << "Errore Decoding: sending packet" << endl;
                }
                got_picture = avcodec_receive_frame(avRawCodecCtx, avOutFrame);
                decode_clock_end = clock();
                decode_clock += (decode_clock_end - decode_clock_start);
                //Fine DECODING
                if (got_picture == 0) {
                    scaling_clock_start = clock();
                    sws_scale(swsCtx, avOutFrame->data, avOutFrame->linesize, 0, avRawCodecCtx->height, avYUVFrame->data, avYUVFrame->linesize);
                    scaling_clock_end = clock();
                    scaling_clock += (scaling_clock_end - scaling_clock_start);

                    //got_picture = -1;  //forse non serve
                    //Inizio ENCODING
                    //deprecato: flag = avcodec_encode_video2(avEncoderCtx, &pkt, avYUVFrame, &got_picture);
                    encode_clock_start = clock();

                    flag = avcodec_send_frame(avEncoderCtx, avYUVFrame);
                    got_picture = avcodec_receive_packet(avEncoderCtx, &pkt);
                    encode_clock_end = clock();
                    encode_clock += (encode_clock_end - encode_clock_start);
                    //Fine ENCODING

                    if (flag >= 0) {
                        if (got_picture == 0) {
                            //cout << pts_offset << " += " << avRawPkt_dts << endl;
                            //pts_offset += avRawPkt_dts;
                            //cout << avRawPkt_dts << endl;
                            //pkt.pts = avFmtCtx->streams[videoIndex]->cur_dts + 1;
                            //pkt.dts = avFmtCtx->streams[videoIndex]->cur_dts + 1;
                            //int64_t rescaled_dts = (int64_t)avRawPkt_dts / 1000 * 30;
                            //cout << "PTS: " << rescaled_dts << endl;

                            pkt.pts = avRawPkt_dts;
                            pkt.dts = avRawPkt_dts;
                            pkt.duration = 1 / 30;

                            fprintf(debug, "%ld\n", pkt.pts);
                            //pkt.pts = pts_offset;
                            //pkt.dts = pts_offset;
                            //cout << "pkt.pts =" << pts_offset << " + " << avFmtCtx->start_time << " + " << av_rescale_q(pkt.pts, video_st->codec->time_base, video_st->time_base) << endl;
                            //pkt.pts = avFmtCtx->start_time + avFmtCtx->start_time + av_rescale_q(pkt.pts, video_st->codec->time_base, video_st->time_base);
                            //pkt.dts = avFmtCtx->start_time + av_rescale_q(pkt.dts, video_st->codec->time_base, video_st->time_base);
                            //pkt.duration = 1;

                            //int w = fwrite(pkt.data, 1, pkt.size, mp4Fp);
                            if (av_interleaved_write_frame(avFmtCtx, &pkt) < 0) {
                                cout << "Error in writing file" << endl;
                                exit(-1);
                            }
                            //cout << "Elaborated " << i << " frames" << endl;
                        }
                    }

                } else {
                    cout << "Errore Decoding: receiving packet " << got_picture << endl;
                }
            }
            i++;
        } else {
            avRawPkt_queue_mutex.unlock();
        }
        avRawPkt_queue_mutex.lock();
    }

    /* int ret = flush_encoder(avFmtCtx, 0);
    if (ret < 0) {
        printf("Flushing encoder failed\n");
        return -1;
    } */
    av_packet_unref(&pkt);
    av_write_trailer(avFmtCtx);
    avio_close(avFmtCtx->pb);

    avformat_free_context(avFmtCtx);
    //avcodec_close(video_st->codec);

    cout << "Decoded and Encoded successfully" << endl
         << endl;
}

int main(int argc, char const *argv[]) {
    debug = fopen("debug.out", "w");
    if (argc < 9) {
        cout << "Errore: parametri mancanti" << endl
             << "Utilizzo: ./main width height offset_x offset_y screen_num fps capturetime_seconds quality" << endl;
    }

    stop = false;
    x11pmt.width = atoi(argv[1]);
    x11pmt.height = atoi(argv[2]);
    x11pmt.offset_x = atoi(argv[3]);
    x11pmt.offset_y = atoi(argv[4]);
    x11pmt.screen_number = atoi(argv[5]);

    initScreenSource(x11pmt, false, atoi(argv[6]), atof(argv[8]));
    memoryCheck_init();

    thread capture_thread{getRawPackets, atoi(argv[6]) * atoi(argv[7])};
    thread elaborate_thread{decodeAndEncode};
    capture_thread.join();
    elaborate_thread.join();
    //getCurrentVMemUsedByProc();

    cout << "Finished" << endl
         << endl;
    int total_clock = readframe_clock + encode_clock + decode_clock + scaling_clock;
    cout << "ReadFrame clock: " << readframe_clock << " " << (float)100 * readframe_clock / total_clock << "%" << endl;
    cout << "Decode clock: " << decode_clock << " " << (float)100 * decode_clock / total_clock << "%" << endl;
    cout << "Scaling clock: " << scaling_clock << " " << (float)100 * scaling_clock / total_clock << "%" << endl;
    cout << "Encode clock: " << encode_clock << " " << (float)100 * encode_clock / total_clock << "%" << endl;

    if (memory_error) {
        cerr << "Process ended because memory limit have been surpassed" << endl;
    }
    return 0;
}
