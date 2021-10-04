#include "ScreenRecorder.h"

#include <iostream>

using namespace std;

ScreenRecorder::ScreenRecorder(RecordingRegionSettings rrs, VideoSettings vs, AudioSettings as) : rrs(rrs), vs(vs), as(as), status(RecordingStatus::stopped) {
    initCommon();
    cout << "-> Finita initCommon" << endl;
    initVideoSource();
    cout << "-> Finita initVideoSource" << endl;
    initVideoVariables();
    cout << "-> Finita initiVideoVariables" << endl;
}

ScreenRecorder::~ScreenRecorder() {
    cout << "Distruttore Screen Recorder" << endl;
}

void ScreenRecorder::initCommon() {
    avdevice_register_all();
    avFmtCtx = avformat_alloc_context();
}

void ScreenRecorder::initVideoSource() {
    char *displayName = getenv("DISPLAY");

    if (rrs.fullscreen) {
        rrs.offset_x = 0;
        rrs.offset_y = 0;
        Display *display = XOpenDisplay(displayName);
        int screenNum = DefaultScreen(display);
        rrs.width = DisplayWidth(display, screenNum);
        rrs.height = DisplayHeight(display, screenNum);
        XCloseDisplay(display);
    }

    if (rrs.width % 32 != 0) {
        rrs.width = rrs.width / 32 * 32;
    }
    if (rrs.height % 2 != 0) {
        rrs.height = rrs.height / 2 * 2;
    }

    avRawOptions = nullptr;
    av_dict_set(&avRawOptions, "video_size", (to_string(rrs.width) + "*" + to_string(rrs.height)).c_str(), 0);
    av_dict_set(&avRawOptions, "framerate", to_string(fps).c_str(), 0);
    av_dict_set(&avRawOptions, "show_region", "1", 0);
    av_dict_set(&avRawOptions, "probesize", "30M", 0);

    AVInputFormat *avInputFmt = av_find_input_format("x11grab");

    if (avInputFmt == nullptr) {
        cout << "av_find_input_format not found......"
             << endl;
        exit(-1);
    }

    if (avformat_open_input(&avFmtCtx, (string(displayName) + "." + to_string(rrs.screen_number) + "+" + to_string(rrs.offset_x) + "," + to_string(rrs.offset_y)).c_str(), avInputFmt, &avRawOptions) != 0) {
        cerr << "Couldn't open input stream."
             << endl;
        exit(-1);
    }

    if (avformat_find_stream_info(avFmtCtx, &avRawOptions) < 0) {
        cout << "Couldn't find stream information."
             << endl;
        exit(-1);
    }

    videoIndex = -1;
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

    avRawCodecCtx = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(avRawCodecCtx, avFmtCtx->streams[videoIndex]->codecpar);

    avDecodec = avcodec_find_decoder(avRawCodecCtx->codec_id);
    if (avDecodec == nullptr) {
        cout << "Codec not found."
             << endl;
        exit(-1);
    }
}

void ScreenRecorder::initVideoVariables() {
    fmt = av_guess_format(NULL, out_file.c_str(), NULL);
    avformat_alloc_output_context2(&avFmtCtxOut, fmt, fmt->name, out_file.c_str());

    avEncodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!avEncodec) {
        cout << "Encoder codec not found"
             << endl;
        exit(-1);
    }
    video_st = avformat_new_stream(avFmtCtxOut, avEncodec);
    avEncoderCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(avEncoderCtx, video_st->codecpar);

    avEncoderCtx->codec_id = AV_CODEC_ID_H264;
    avEncoderCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    avEncoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    avEncoderCtx->bit_rate = 80000;
    avEncoderCtx->width = rrs.width * vs.quality;
    avEncoderCtx->height = rrs.height * vs.quality;
    avEncoderCtx->time_base.num = 1;
    avEncoderCtx->time_base.den = fps;
    avEncoderCtx->gop_size = 50;
    avEncoderCtx->qmin = 5;
    avEncoderCtx->qmax = 10;
    avEncoderCtx->max_b_frames = 2;

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

    int outVideoStreamIndex = -1;
    for (int i = 0; (size_t)i < avFmtCtx->nb_streams; i++) {
        if (avFmtCtxOut->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            outVideoStreamIndex = i;
        }
    }

    if (outVideoStreamIndex < 0) {
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
                            (int)avRawCodecCtx->width * vs.quality,
                            (int)avRawCodecCtx->height * vs.quality,
                            AV_PIX_FMT_YUV420P,
                            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    avYUVFrame = av_frame_alloc();
    int yuvLen = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height, 1);

    uint8_t *yuvBuf = new uint8_t[yuvLen];
    av_image_fill_arrays(avYUVFrame->data, avYUVFrame->linesize, (uint8_t *)yuvBuf, AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height, 1);

    avYUVFrame->width = avRawCodecCtx->width;
    avYUVFrame->height = avRawCodecCtx->height;
    avYUVFrame->format = AV_PIX_FMT_YUV420P;
}

void ScreenRecorder::getRawPackets() {
    int frameNumber = vs.fps * vs.capturetime_seconds;
    AVPacket *avRawPkt;
    for (int i = 0; i < frameNumber; i++) {
        avRawPkt = av_packet_alloc();
        if (av_read_frame(avFmtCtx, avRawPkt) < 0) {
            cout << "Error in getting RawPacket from x11" << endl;
        }
        /*else
		{
			cout << "Captured " << i << " raw packets" << endl;
		}*/
        avRawPkt_queue_mutex.lock();
        avRawPkt_queue.push(avRawPkt);
        avRawPkt_queue_mutex.unlock();
    }

    /*
    TODO: da decidere se si aggiunge
    if (memoryCheck_limitSurpassed()) {
            memory_error = true;
            break;
        }
     */

    // NOTE: ????
    //if (i == frameNumber / 2) sleep(5);

    avRawPkt_queue_mutex.lock();
    stop = true;
    avRawPkt_queue_mutex.unlock();
}

void ScreenRecorder::decodeAndEncode() {
    int got_picture = 0;
    int flag = 0;
    int bufLen = 0;
    uint8_t *outBuf = nullptr;

    flag = avformat_write_header(avFmtCtxOut, NULL);
    if (flag < 0) {
        cout << "Error in writing header" << endl;
        exit(-1);
    }

    bufLen = rrs.width * rrs.height * 3;
    outBuf = (uint8_t *)malloc(bufLen);

    AVFrame *avOutFrame;
    avOutFrame = av_frame_alloc();
    av_image_fill_arrays(avOutFrame->data, avOutFrame->linesize, (uint8_t *)outBuf, avEncoderCtx->pix_fmt, avEncoderCtx->width, avEncoderCtx->height, 1);

    AVPacket pkt;
    av_init_packet(&pkt);

    AVPacket *avRawPkt;
    int i = 0;

    avRawPkt_queue_mutex.lock();
    while (!stop || !avRawPkt_queue.empty()) {
        if (!avRawPkt_queue.empty()) {
            //cout << "Remaining " << avRawPkt_queue.size() << " frames" << endl;
            avRawPkt = avRawPkt_queue.front();
            avRawPkt_queue.pop();
            avRawPkt_queue_mutex.unlock();
            if (avRawPkt->stream_index == videoIndex) {
                //cout << "Elaborating frame" << i << endl;
                //Inizio DECODING
                flag = avcodec_send_packet(avRawCodecCtx, avRawPkt);
                av_packet_unref(avRawPkt);
                av_packet_free(&avRawPkt);

                if (flag < 0) {
                    cout << "Errore Decoding: sending packet" << endl;
                }
                got_picture = avcodec_receive_frame(avRawCodecCtx, avOutFrame);
                //Fine DECODING

                if (got_picture == 0) {
                    sws_scale(swsCtx, avOutFrame->data, avOutFrame->linesize, 0, avRawCodecCtx->height, avYUVFrame->data, avYUVFrame->linesize);
                    //Inizio ENCODING
                    avYUVFrame->pts = i * 30 * 30 * 100 / fps;
                    flag = avcodec_send_frame(avEncoderCtx, avYUVFrame);
                    got_picture = avcodec_receive_packet(avEncoderCtx, &pkt);
                    //Fine ENCODING

                    if (flag >= 0) {
                        if (got_picture == 0) {
                            pkt.pts = i * 30 * 30 * 100 / fps;
                            pkt.dts = i * 30 * 30 * 100 / fps;

                            if (av_write_frame(avFmtCtxOut, &pkt) < 0) {
                                cout << "Error in writing file" << endl;
                                exit(-1);
                            }
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

    av_packet_unref(&pkt);
    av_write_trailer(avFmtCtxOut);

    avformat_close_input(&avFmtCtx);
    avio_close(avFmtCtxOut->pb);
    avformat_free_context(avFmtCtx);
}

int ScreenRecorder::record() {
    capture_thread = move(thread{[this]() { this->getRawPackets(); }});
    elaborate_thread = move(thread{[this]() { this->decodeAndEncode(); }});
    capture_thread.join();
    elaborate_thread.join();
    return 0;
}