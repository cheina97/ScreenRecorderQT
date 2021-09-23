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

AVFormatContext *avFmtCtx = nullptr, *avFmtCtxOut = nullptr;
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
int fps;

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

void initScreenSource(X11parameters x11pmt, bool fullscreen, int fps_arg, float scaling_factor) {
    fps = fps_arg;
    avdevice_register_all();
    //avcodec_register_all();
    avFmtCtx = avformat_alloc_context();

    //aggiunto da me
    //avFmtCtx->flags |= AVFMT_FLAG_FLUSH_PACKETS;

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

    //NOTE: fine di start recording, inizio di init output file

    fmt = av_guess_format(NULL, out_file.c_str(), NULL);
    avformat_alloc_output_context2(&avFmtCtxOut, fmt, fmt->name, out_file.c_str());

    avEncodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!avEncodec) {
        cout << "Encoder codec not found"
             << endl;
        exit(-1);
    }

    video_st = avformat_new_stream(avFmtCtxOut, avEncodec);
    // av_dump_format(avFmtCtx, 0, out_file.c_str(), 1);

    //avEncoderCtx = video_st->codec;
    avEncoderCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(avEncoderCtx, video_st->codecpar);

    avEncoderCtx->codec_id = AV_CODEC_ID_H264;
    avEncoderCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    avEncoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    avEncoderCtx->bit_rate = 80000;
    avEncoderCtx->width = x11pmt.width * scaling_factor;
    avEncoderCtx->height = x11pmt.height * scaling_factor;
    avEncoderCtx->time_base.num = 1;
    avEncoderCtx->time_base.den = fps;
    avEncoderCtx->gop_size = 50;
    avEncoderCtx->qmin = 5;
    avEncoderCtx->qmax = 10;
    avEncoderCtx->max_b_frames = 2;

    //TODO: analizzare se serve
    if (avEncoderCtx->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(avEncoderCtx, "preset", "ultrafast", 0);
        av_opt_set(avEncoderCtx, "tune", "zerolatency", 0);
        av_opt_set(avEncoderCtx, "cabac", "1", 0);
        av_opt_set(avEncoderCtx, "ref", "3", 0);
        av_opt_set(avEncoderCtx, "deblock", "1:0:0", 0);
        av_opt_set(avEncoderCtx, "analyse", "0x3:0x113", 0);
        av_opt_set(avEncoderCtx, "subme", "7", 0);
        av_opt_set(avEncoderCtx, "chroma_qp_offset", "4", 0);
        av_opt_set(avEncoderCtx, "rc", "crf", 0);
        av_opt_set(avEncoderCtx, "rc_lookahead", "40", 0);
        av_opt_set(avEncoderCtx, "crf", "10.0", 0);
    }

    if (avFmtCtxOut->oformat->flags & AVFMT_GLOBALHEADER) {
        avEncoderCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    //NOTE Loro sta cosa non la fanno
    if (avcodec_open2(avRawCodecCtx, avDecodec, nullptr) < 0) {
        cout << "Could not open decodec. "
             << endl;
        exit(-1);
    }

    if (avcodec_open2(avEncoderCtx, avEncodec, NULL) < 0) {
        cout << "Failed to open video encoder!"
             << endl;
        exit(-1);
    }

    //NOTE: Sfoltisci
    //Sta roba non l'abbiamo aggiunta
    int outVideoStreamIndex = -1;
    for (int i = 0; (size_t)i < avFmtCtx->nb_streams; i++) {
        if (avFmtCtxOut->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            outVideoStreamIndex = i;
        }
    }
    if (outVideoStreamIndex < 0) {
        //cerr << "Error: cannot find a free stream index for video output" << endl;
        //exit(-1);
        cout << "Error: cannot find a free stream index for video output" << endl;
        exit(-1);
    }
    avcodec_parameters_from_context(avFmtCtxOut->streams[outVideoStreamIndex]->codecpar, avEncoderCtx);

    //Open output URL
    if (avio_open(&avFmtCtxOut->pb, out_file.c_str(), AVIO_FLAG_READ_WRITE) < 0) {
        printf("Failed to open output file! \n");
        exit(-1);
    }

    swsCtx = sws_getContext(avRawCodecCtx->width,
                            avRawCodecCtx->height,
                            avRawCodecCtx->pix_fmt,
                            (int)avRawCodecCtx->width * scaling_factor,
                            (int)avRawCodecCtx->height * scaling_factor,
                            AV_PIX_FMT_YUV420P,
                            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    avYUVFrame = av_frame_alloc();
    //deprecata: int yuvLen = avpicture_get_size(AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height);
    int yuvLen = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height, 1);

    uint8_t *yuvBuf = new uint8_t[yuvLen];
    //deprecata: avpicture_fill((AVPicture *)avYUVFrame, (uint8_t *)yuvBuf, AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height);
    av_image_fill_arrays(avYUVFrame->data, avYUVFrame->linesize, (uint8_t *)yuvBuf, AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height, 1);

    avYUVFrame->width = avRawCodecCtx->width;
    avYUVFrame->height = avRawCodecCtx->height;
    avYUVFrame->format = AV_PIX_FMT_YUV420P;

    //av_dump_format(avFmtCtx, 0, out_file.c_str(), 1);
}

void getRawPackets(int frameNumber) {
    cout << "Capturing from x11..." << endl;
    readframe_clock_start = clock();
    auto begin = std::chrono::high_resolution_clock::now();
    AVPacket *avRawPkt;
    //NOTE: unused
    // int64_t cur_pts = 0;

    for (int i = 0; i < frameNumber; i++) {
        avRawPkt = av_packet_alloc();
        if (av_read_frame(avFmtCtx, avRawPkt) < 0) {
            cout << "Error in getting RawPacket from x11" << endl;
        } else {
            cout << "Captured " << i << " raw packets" << endl;
            //getCurrentVMemUsedByProc();
        }
        //prove pts

        //cout << "avRawPkt->pts: " << avRawPkt->pts << endl;
        //cout << "avRawPkt->dts: " << avRawPkt->dts << endl;
        //cout << "avRawPkt->duration: " << avRawPkt->duration << endl;

        //cur_pts = avRawPkt->pts;
        //avRawPkt->pts = av_rescale_q_rnd(avRawPkt->pts, avFmtCtxOut->streams[videoIndex]->time_base, {1, 30}, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        //avRawPkt->dts = av_rescale_q_rnd(avRawPkt->dts, avFmtCtxOut->streams[videoIndex]->time_base, {1, 30}, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        //avRawPkt->duration = av_rescale_q(avRawPkt->duration, avFmtCtxOut->streams[videoIndex]->time_base, {1, 30});
        //av_packet_rescale_ts()

        //cout << "avRawPkt->pts: " << avRawPkt->pts << endl;
        //cout << "avRawPkt->dts: " << avRawPkt->dts << endl;
        //cout << "avRawPkt->duration: " << avRawPkt->duration << endl;

        avRawPkt_queue_mutex.lock();
        avRawPkt_queue.push(avRawPkt);
        avRawPkt_queue_mutex.unlock();

        //avRawPkt->stream_index = videoIndex;

        if (memoryCheck_limitSurpassed()) {
            memory_error = true;
            break;
        }

        if (i == frameNumber / 2) sleep(5);
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

    flag = avformat_write_header(avFmtCtxOut, NULL);
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

    AVPacket pkt;
    //memset(&pkt, 0, sizeof(AVPacket));
    av_init_packet(&pkt);

    AVPacket *avRawPkt;
    //NOTE: unused
    // int64_t avRawPkt_dts;
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
                // avRawPkt_dts = avRawPkt->dts;
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

                    avYUVFrame->pts = i * 30 * 30 * 100 / fps;
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
                            //pkt.pts = avFmtCtx->streams[videoIndex]->cur_dts+1;
                            //pkt.dts = avFmtCtx->streams[videoIndex]->cur_dts+1;
                            //int64_t rescaled_dts = (int64_t)avRawPkt_dts / 1000 * 30;
                            //cout << "PTS: " << rescaled_dts << endl;

                            pkt.pts = i * 30 * 30 * 100 / fps;
                            ;
                            pkt.dts = i * 30 * 30 * 100 / fps;
                            ;

                            //av_packet_rescale_ts(&pkt, avEncoderCtx->time_base, avFmtCtxOut->streams[videoIndex]->time_base);
                            //pkt.pts=i*fps*100;
                            //pkt.dts=i*fps*100;
                            //pkt.duration=1;
                            //pkt.pos=i;

                            fprintf(debug, "%ld\n", pkt.pts);

                            //pkt.pts = pts_offset;
                            //pkt.dts = pts_offset;
                            //cout << "pkt.pts =" << pts_offset << " + " << avFmtCtxOut->start_time << " + " << av_rescale_q(pkt.pts, video_st->codec->time_base, video_st->time_base) << endl;
                            //pkt.pts = avFmtCtxOut->start_time + avFmtCtxOut->start_time + av_rescale_q(pkt.pts, video_st->codec->time_base, video_st->time_base);
                            //pkt.dts = avFmtCtxOut->start_time + av_rescale_q(pkt.dts, video_st->codec->time_base, video_st->time_base);
                            //pkt.duration = 1;

                            //int w = fwrite(pkt.data, 1, pkt.size, mp4Fp);
                            if (av_write_frame(avFmtCtxOut, &pkt) < 0) {
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

    /* int ret = flush_encoder(avFmtCtxOut, 0);
    if (ret < 0) {
        printf("Flushing encoder failed\n");
        return -1;
    } */
    av_packet_unref(&pkt);
    av_write_trailer(avFmtCtxOut);

    //NOTE: aggiunto da rhami ///////////
    avformat_close_input(&avFmtCtx);
    /////////////////////////////////////
    avio_close(avFmtCtxOut->pb);
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
