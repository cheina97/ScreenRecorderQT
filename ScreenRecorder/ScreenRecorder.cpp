#include "ScreenRecorder.h"

#include <exception>
#include <iostream>

#include "GetAudioDevices.h"
#if defined __linux__
#include "MemoryCheckLinux.h"
#endif

#include <time.h>

using namespace std;

ScreenRecorder::ScreenRecorder(RecordingRegionSettings rrs, VideoSettings vs, string outFilePath, string audioDevice) : rrs(rrs), vs(vs), status(RecordingStatus::recording), outFilePath(outFilePath), audioDevice(audioDevice) {
    try {
        initCommon();
        std::cout << "-> Finita initCommon" << std::endl;
        initVideoSource();
        std::cout << "-> Finita initVideoSource" << std::endl;
        initVideoVariables();
        std::cout << "-> Finita initiVideoVariables" << std::endl;
        if (vs.audioOn) {
            initAudioSource();
            initAudioVariables();
            std::cout << "-> Finita initAudioSource" << std::endl;
        }
        initOutputFile();
#if defined __linux__
        memoryCheck_init(4000);  // ERROR
#endif
    } catch (const std::exception &e) { 
        throw;
    }
}

ScreenRecorder::~ScreenRecorder() {
    captureVideo_thread.get()->join();
    elaborate_thread.get()->join();
    if (vs.audioOn)
        captureAudio_thread.get()->join();

    av_write_trailer(avFmtCtxOut);
    avformat_close_input(&avFmtCtx);
    avio_close(avFmtCtxOut->pb);
    avformat_free_context(avFmtCtx);
    handler_thread.get()->join();
    cout << "Distruttore Screen Recorder" << endl;
}

void ScreenRecorder::handler() {
    bool handlerEnd = false;
    int command = 0;
    cout << "\nScreerRecorder Handler" << endl;
    cout << "Commands:\n[1] Pause\n[2] Resume\n[3] Stop" << endl;
    while (!handlerEnd) {
        cin >> command;
        switch (command) {
            case 1:
                cout << "Pause Recording..." << endl;
                pauseRecording();
                break;
            case 2:
                cout << "Resuming Recording..." << endl;
                resumeRecording();
                break;
            case 3:
                cout << "End Recording!" << endl;
                stopRecording();
                handlerEnd = true;
                break;
            default:
                cout << "Invalid Command:\n[1] Pause\n[2] Resume\n[3] Stop" << endl;
                break;
        }
    }
}

void ScreenRecorder::stopRecording() {
    cout << "trying to stop recording" << endl;
    unique_lock<mutex> ul(mu);
    if (status == RecordingStatus::recording || status == RecordingStatus::paused)
        status = RecordingStatus::stopped;
    cv.notify_all();
    cout << "status: " << statusToString() << endl;
}

void ScreenRecorder::pauseRecording() {
    cout << "trying to pause recording" << endl;
    unique_lock<mutex> ul(mu);
    if (status == RecordingStatus::recording)
        status = RecordingStatus::paused;
    cout << "status: " << statusToString() << endl;
}

void ScreenRecorder::resumeRecording() {
    cout << "trying to resume recording" << endl;
    unique_lock<mutex> ul(mu);
    if (status == RecordingStatus::paused) {
#if defined __linux__
        linuxVideoResume();
#endif
#if defined _WIN32
        windowsResumeAudio();
#endif
        status = RecordingStatus::recording;
        cv.notify_all();
    }
    cout << "status: " << statusToString() << endl;
}

string ScreenRecorder::statusToString() {
    switch (status) {
        case RecordingStatus::paused:
            return "Pause";
        case RecordingStatus::recording:
            return "Recording";
        case RecordingStatus::stopped:
            return "Stopped";
        default:
            return "Undefined Status";
    }
}

void ScreenRecorder::record() {
    //stop = false;
    audio_stop = false;
    gotFirstValidVideoPacket = false;
    queue<exception> exc_queue;
    mutex exc_queue_m;
    int terminated_threads = 0;
    condition_variable exc_queue_cv;

    /* captureVideo_thread = std::make_unique<std::thread>([this]() {
        try {
            this->getRawPackets();
            std::cout << "1. capture video thread fine!" << std::endl;
        }
        catch (const std::exception& e) {
            throw;
        }
        });

    elaborate_thread = std::make_unique<std::thread>([this]() {
        try {
            this->decodeAndEncode();
            std::cout << "2. elaborate_thread fine!" << std::endl;
        }
        catch (const std::exception& e) {
            throw;
        }
        });
    if (vs.audioOn)
        captureAudio_thread = std::make_unique<std::thread>([this]() {
        try {
            this->acquireAudio();
            std::cout << "3. captureAudio_thread!" << std::endl;
        }
        catch (const std::exception& e) {
            throw;
        }
            }); */

    elaborate_thread = make_unique<thread>([&]() {
        try {
            this->decodeAndEncode();
            lock_guard<mutex> lg{exc_queue_m};
            terminated_threads++;
            exc_queue_cv.notify_one();
        } catch (const std::exception &e) {
            lock_guard<mutex> lg{exc_queue_m};
            exc_queue.emplace(e);
            exc_queue_cv.notify_one();
        }
    });
    captureVideo_thread = make_unique<thread>([&]() {
        try {
            this->getRawPackets();
            lock_guard<mutex> lg{exc_queue_m};
            terminated_threads++;
            exc_queue_cv.notify_one();
        } catch (const std::exception &e) {
            lock_guard<mutex> lg{exc_queue_m};
            exc_queue.emplace(e);
            exc_queue_cv.notify_one();
        }
    });
    if (vs.audioOn)
        captureAudio_thread = std::make_unique<std::thread>([&]() {
            try {
                this->acquireAudio();
                lock_guard<mutex> lg{exc_queue_m};
                terminated_threads++;
                exc_queue_cv.notify_one();
            } catch (const std::exception &e) {
                lock_guard<mutex> lg{exc_queue_m};
                exc_queue.emplace(e);
                exc_queue_cv.notify_one();
            }
        });
    /* handler_thread = make_unique<thread>([this]() { this->handler(); }); */

    unique_lock<mutex> exc_queue_ul{exc_queue_m};
    exc_queue_cv.wait(exc_queue_ul, [&]() { cout<<"term_th: " <<terminated_threads<<" queue_size: "<< exc_queue.size()<<endl; return (!exc_queue.empty() || terminated_threads == (vs.audioOn?3:2)); });
    cout << "passato qui" << endl;
    if (!exc_queue.empty()) {
        throw logic_error{"dsdads"};
    }
}

void ScreenRecorder::initCommon() {
    avdevice_register_all();
    avFmtCtx = nullptr;
    avFmtCtx = avformat_alloc_context();

    fmt = av_guess_format(NULL, outFilePath.c_str(), NULL);
    if (fmt == NULL) {
        throw runtime_error{"Error: cannot guess format"};
    }
    avformat_alloc_output_context2(&avFmtCtxOut, fmt, fmt->name, outFilePath.c_str());

    avEncodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!avEncodec) {
        throw logic_error{"Encoder codec not found"};
    }
}

void ScreenRecorder::initVideoSource() {
    char *displayName = getenv("DISPLAY");

    if (rrs.fullscreen) {
        rrs.offset_x = 0;
        rrs.offset_y = 0;
#if defined _WIN32
        SetProcessDPIAware();  //A program must tell the operating system that it is DPI-aware to get the true resolution when you go past 125%.
        rrs.width = (int)GetSystemMetrics(SM_CXSCREEN);
        rrs.height = (int)GetSystemMetrics(SM_CYSCREEN);
#endif
#if defined __linux__
        Display *display = XOpenDisplay(displayName);
        int screenNum = DefaultScreen(display);
        rrs.width = DisplayWidth(display, screenNum);
        rrs.height = DisplayHeight(display, screenNum);
        XCloseDisplay(display);
#endif
    }

    rrs.width = rrs.width / 32 * 32;
    rrs.height = rrs.height / 2 * 2;

    avRawOptions = nullptr;

    av_dict_set(&avRawOptions, "video_size", (to_string(rrs.width) + "*" + to_string(rrs.height)).c_str(), 0);
    av_dict_set(&avRawOptions, "framerate", to_string(vs.fps).c_str(), 0);
    av_dict_set(&avRawOptions, "show_region", "1", 0);
    av_dict_set(&avRawOptions, "probesize", "30M", 0);
    //av_dict_set(&avRawOptions, "maxrate", "200k", 0);
    //av_dict_set(&avRawOptions, "minrate", "0", 0);
    //av_dict_set(&avRawOptions, "bufsize", "2000k", 0);

#if defined _WIN32
    AVInputFormat *avInputFmt = av_find_input_format("gdigrab");
    if (avInputFmt == nullptr) {
        throw logic_error{"av_find_input_format not found......"};
    }
    av_dict_set(&avRawOptions, "offset_x", to_string(rrs.offset_x).c_str(), 0);
    av_dict_set(&avRawOptions, "offset_y", to_string(rrs.offset_y).c_str(), 0);

    if (avformat_open_input(&avFmtCtx, "desktop", avInputFmt, &avRawOptions) != 0) {
        throw logic_error{"Couldn't open input stream"};
    }

#elif defined __linux__
    AVInputFormat *avInputFmt = av_find_input_format("x11grab");

    if (avInputFmt == nullptr) {
        throw logic_error{"av_find_input_format not found......"};
    }

    if (avformat_open_input(&avFmtCtx, (string(displayName) + "." + to_string(rrs.screen_number) + "+" + to_string(rrs.offset_x) + "," + to_string(rrs.offset_y)).c_str(), avInputFmt, &avRawOptions) != 0) {
        throw runtime_error{"Couldn't open input stream."};
    }
#else
    // Apple

    av_dict_set(&avRawOptions, "video_size", (to_string(rrs.width) + "*" + to_string(rrs.height)).c_str(), 0);
    av_dict_set(&avRawOptions, "preset", "ultrafast", 0);
    //av_dict_set(&avRawOptions, "list_devices", "true", 0); // TO SHOW DEVICE LIST
    av_dict_set(&avRawOptions, "pixel_format", "uyvy422", 0); /* yuv420p */
    av_dict_set(&avRawOptions, "vf", ("crop=1920:1080:0:0" + to_string(rrs.width) + ":" + to_string(rrs.height) + ":" + to_string(rrs.offset_x) + ":" + to_string(rrs.offset_y)).c_str(), 0);
    av_dict_set(&avRawOptions, "framerate", to_string(vs.fps).c_str(), 0);
    //av_dict_set(&avRawOptions, "show_region", "1", 0);
    av_dict_set(&avRawOptions, "probesize", "42M", 0);

    AVInputFormat *avInputFmt = nullptr;
    avInputFmt = av_find_input_format("avfoundation");
    if (avInputFmt == nullptr) {
        cout << "errore apertura" << endl;
    }
    //[video]:[audio]
    if (int value = avformat_open_input(&avFmtCtx, "1:none", avInputFmt, &avRawOptions)) {
        throw runtime_error{"Couldn't open Apple video input stream."};
    }
#endif

    if (avformat_find_stream_info(avFmtCtx, &avRawOptions) < 0) {
        throw logic_error{"Couldn't open input stream."};
    }

    videoIndex = -1;
    for (int i = 0; i < (int)avFmtCtx->nb_streams; ++i) {
        if (avFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }

    if (videoIndex == -1 || videoIndex >= (int)avFmtCtx->nb_streams) {
        throw logic_error{"Didn't find a video stream."};
    }

    avRawCodecCtx = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(avRawCodecCtx, avFmtCtx->streams[videoIndex]->codecpar);

    avDecodec = avcodec_find_decoder(avRawCodecCtx->codec_id);
    if (avDecodec == nullptr) {
        throw runtime_error{"Codec not found."};
    }
}

void ScreenRecorder::linuxVideoResume() {
    av_dict_set(&avRawOptions, "probesize", "30M", 0);

    char *displayName = getenv("DISPLAY");

    AVInputFormat *avInputFmt = av_find_input_format("x11grab");

    if (avInputFmt == nullptr) {
        throw logic_error{"av_find_input_format not found......"};
    }
    if (avFmtCtx == nullptr)

        if (avformat_open_input(&avFmtCtx, (string(displayName) + "." + to_string(rrs.screen_number) + "+" + to_string(rrs.offset_x) + "," + to_string(rrs.offset_y)).c_str(), avInputFmt, &avRawOptions) != 0) {
            throw runtime_error{"Couldn't open input stream."};
        }
    if (avformat_find_stream_info(avFmtCtx, &avRawOptions) < 0) {
        throw logic_error{"Couldn't open input stream."};
    }

    videoIndex = -1;
    for (int i = 0; i < (int)avFmtCtx->nb_streams; ++i) {
        if (avFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }

    if (videoIndex == -1 || videoIndex >= (int)avFmtCtx->nb_streams) {
        throw logic_error{"Didn't find a video stream."};
    }

    avRawCodecCtx = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(avRawCodecCtx, avFmtCtx->streams[videoIndex]->codecpar);

    avDecodec = avcodec_find_decoder(avRawCodecCtx->codec_id);
    if (avDecodec == nullptr) {
        throw runtime_error{"Codec not found."};
    }

    if (avcodec_open2(avRawCodecCtx, avDecodec, nullptr) < 0) {
        throw runtime_error{"Could not open decodec. "};
    }

    swsCtx = sws_getContext(avRawCodecCtx->width,
                            avRawCodecCtx->height,
                            avRawCodecCtx->pix_fmt,
                            (int)(avRawCodecCtx->width * vs.quality) / 32 * 32,
                            (int)(avRawCodecCtx->height * vs.quality) / 2 * 2,
                            AV_PIX_FMT_YUV420P,
                            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    /* avYUVFrame = av_frame_alloc();
    int yuvLen = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height, 1);

    uint8_t *yuvBuf = new uint8_t[yuvLen];
    av_image_fill_arrays(avYUVFrame->data, avYUVFrame->linesize, (uint8_t *)yuvBuf, AV_PIX_FMT_YUV420P, avRawCodecCtx->width, avRawCodecCtx->height, 1);

    avYUVFrame->width = avRawCodecCtx->width;
    avYUVFrame->height = avRawCodecCtx->height;
    avYUVFrame->format = AV_PIX_FMT_YUV420P; */
}

void ScreenRecorder::initVideoVariables() {
    video_st = avformat_new_stream(avFmtCtxOut, avEncodec);
    avEncoderCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(avEncoderCtx, video_st->codecpar);

    avEncoderCtx->codec_id = AV_CODEC_ID_H264;
    avEncoderCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    avEncoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    avEncoderCtx->bit_rate = 4000;
    avEncoderCtx->width = (int)(rrs.width * vs.quality) / 32 * 32;
    avEncoderCtx->height = (int)(rrs.height * vs.quality) / 2 * 2;
    avEncoderCtx->time_base.num = 1;
    avEncoderCtx->time_base.den = vs.fps;
    avEncoderCtx->gop_size = vs.fps * 2;
    avEncoderCtx->qmin = vs.compression * 5;
    avEncoderCtx->qmax = 5 + vs.compression * 5;
    avEncoderCtx->max_b_frames = 10;

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
        av_opt_set(avEncoderCtx, "threads", "8", 0);
    }

    if (avFmtCtxOut->oformat->flags & AVFMT_GLOBALHEADER) {
        avEncoderCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(avRawCodecCtx, avDecodec, nullptr) < 0) {
        throw runtime_error{"Could not open decodec. "};
    }

    if (avcodec_open2(avEncoderCtx, avEncodec, NULL) < 0) {
        throw runtime_error{"Failed to open video encoder!"};
    }

    int outVideoStreamIndex = -1;
    for (int i = 0; (size_t)i < avFmtCtx->nb_streams; i++) {
        if (avFmtCtxOut->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            outVideoStreamIndex = i;
        }
    }

    if (outVideoStreamIndex < 0) {
        throw runtime_error{"Error: cannot find a free stream index for video output"};
    }
    avcodec_parameters_from_context(avFmtCtxOut->streams[outVideoStreamIndex]->codecpar, avEncoderCtx);

    //Open output URL
    if (avio_open(&avFmtCtxOut->pb, outFilePath.c_str(), AVIO_FLAG_READ_WRITE) < 0) {
        throw runtime_error{"Failed to open output file!"};
    }

    swsCtx = sws_getContext(avRawCodecCtx->width,
                            avRawCodecCtx->height,
                            avRawCodecCtx->pix_fmt,
                            (int)(avRawCodecCtx->width * vs.quality) / 32 * 32,
                            (int)(avRawCodecCtx->height * vs.quality) / 2 * 2,
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

void ScreenRecorder::initAudioSource() {
    FormatContextAudio = avformat_alloc_context();
    AudioOptions = NULL;
    //DICTIONARY INIT
    av_dict_set(&AudioOptions, "sample_rate", "44100", 0);
    av_dict_set(&AudioOptions, "async", "25", 0);

#if defined __APPLE__
    AudioInputFormat = av_find_input_format("avfoundation");
    if (AudioInputFormat == NULL) {
        throw runtime_error{"Cannot open AVFoundation driver"};
    }
    if (avformat_open_input(&FormatContextAudio, "none:0", AudioInputFormat, &AudioOptions) < 0) {
        throw runtime_error("Couldn't open audio input stream.");
    }
#endif

#if defined __linux__
    // GET INPUT FORMAT ALSA
    AudioInputFormat = av_find_input_format("alsa");
    if (AudioInputFormat == NULL) {
        throw runtime_error{"Cannot open ALSA driver"};
    }

    if (avformat_open_input(&FormatContextAudio, audioDevice.c_str(), AudioInputFormat, NULL) < 0) {
        throw runtime_error("Couldn't open audio input stream.");
    }
#endif

#if defined _WIN32
    audioDevice = "audio=" + audioDevice;

    AudioInputFormat = av_find_input_format("dshow");
    int value = avformat_open_input(&FormatContextAudio, audioDevice.c_str(), AudioInputFormat, &AudioOptions);
    if (value != 0) {
        //cerr << "Error in opening input device (audio)" << endl;
        //exit(-1);
        throw runtime_error("Error in opening input device");
    }

#endif

    // CHECK STREAM INFO
    if (avformat_find_stream_info(FormatContextAudio, NULL) < 0) {
        throw runtime_error("Couldn't find audio stream information.");
    }
    // FIND AUDIO STREAM INDEX
    int StreamsNumber = (int)FormatContextAudio->nb_streams;
    for (int i = 0; i < StreamsNumber; i++) {
        if (FormatContextAudio->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioIndex = i;
            // SAVE AUDIO STREAM FROM AUDIO CONTEXT
            AudioStream = FormatContextAudio->streams[i];
            break;
        }
    }
    // CHECK AUDIO STREAM INDEX
    if (audioIndex == -1 || audioIndex >= StreamsNumber) {
        throw runtime_error("Didn't find a audio stream.");
    }
}

void ScreenRecorder::getRawPackets() {
    throw logic_error{"ECCEZIONE DI PROVAAAA"};
    AVPacket *avRawPkt;
    while (true) {
        // STATUS MUTEX LOCK
        unique_lock<mutex> ul(mu);

        // STOP CHECK
        if (status == RecordingStatus::stopped) {
            break;
        }
        // PAUSE CHECK
        if (status == RecordingStatus::paused) {
            cout << "Video Pause" << endl;
#if defined __linux__
            avformat_close_input(&avFmtCtx);
            if (avFmtCtx != nullptr) {
                throw runtime_error("Unable to close the avFmtCtx (before pause)");
            }
#endif
        }

        cv.wait(ul, [this]() { return status != RecordingStatus::paused; });
        // STATUS MUTEX UNLOCK
        ul.unlock();

        // ------------------
        avRawPkt = av_packet_alloc();
        if (av_read_frame(avFmtCtx, avRawPkt) < 0) {
            throw runtime_error("Error in getting RawPacket from x11");
        }

        avRawPkt_queue_mutex.lock();
        avRawPkt_queue.push(avRawPkt);
        avRawPkt_queue_mutex.unlock();

//da togliere da qui e mettere nell'app con chiamata a stop();
#if defined __linux__
        try {
            memoryCheck_limitSurpassed();
        } catch (const runtime_error &e) {
            cout << "ERROR: MEMORY LIMIT SURPASSED" << endl;
            stopRecording();
        }
#endif
    }

    /* if (status != RecordingStatus::stopped)
    {
        stopRecording();
    } */
}

void ScreenRecorder::decodeAndEncode() {
    int got_picture = 0;
    int flag = 0;
    int bufLen = 0;
    uint8_t *outBuf = nullptr;

    bufLen = rrs.width * rrs.height * 3;
    outBuf = (uint8_t *)malloc(bufLen);

    AVFrame *avOutFrame;
    avOutFrame = av_frame_alloc();
    av_image_fill_arrays(avOutFrame->data, avOutFrame->linesize, (uint8_t *)outBuf, avEncoderCtx->pix_fmt, avEncoderCtx->width, avEncoderCtx->height, 1);

    AVPacket pkt;
    av_init_packet(&pkt);

    AVPacket *avRawPkt;
    int i = 1;
    int j = 1;

    while (true) {
        /* unique_lock<mutex> ul(mu);

        if (status == RecordingStatus::paused)
        {
            cout << "Video Pause" << endl;
            afterPause = true;
        }
        cv.wait(ul, [this]()
                { return status != RecordingStatus::paused; });
        ul.unlock();
        //cout << statusToString() << endl;
        if (afterPause)
        {
            cout << "Video decode restarted" << endl;
#if defined linux
            //linuxVideoResume();
#endif
            afterPause = false;
        }
 */
        avRawPkt_queue_mutex.lock();
        if (!avRawPkt_queue.empty()) {
            avRawPkt = avRawPkt_queue.front();
            avRawPkt_queue.pop();
            avRawPkt_queue_mutex.unlock();
            if (avRawPkt->stream_index == videoIndex) {
                //Inizio DECODING
                flag = avcodec_send_packet(avRawCodecCtx, avRawPkt);

                av_packet_unref(avRawPkt);
                av_packet_free(&avRawPkt);

                if (flag < 0) {
                    throw runtime_error("Decoding Error: sending packet");
                }
                got_picture = avcodec_receive_frame(avRawCodecCtx, avOutFrame);

                //Fine DECODING
                if (got_picture == 0) {
                    sws_scale(swsCtx, avOutFrame->data, avOutFrame->linesize, 0, avRawCodecCtx->height, avYUVFrame->data, avYUVFrame->linesize);

                    //Inizio ENCODING
                    avYUVFrame->pts = (int64_t)j * (int64_t)30 * (int64_t)30 * (int64_t)100 / (int64_t)vs.fps;
                    j++;
                    flag = avcodec_send_frame(avEncoderCtx, avYUVFrame);

                    if (flag >= 0) {
                        got_picture = avcodec_receive_packet(avEncoderCtx, &pkt);
                        //Fine ENCODING
                        if (got_picture == 0) {
                            if (!gotFirstValidVideoPacket) {
                                gotFirstValidVideoPacket = true;
                            }
                            pkt.pts = (int64_t)i * (int64_t)30 * (int64_t)30 * (int64_t)100 / (int64_t)vs.fps;
                            pkt.dts = (int64_t)i * (int64_t)30 * (int64_t)30 * (int64_t)100 / (int64_t)vs.fps;

                            write_lock.lock();
                            if (av_write_frame(avFmtCtxOut, &pkt) < 0) {
                                throw runtime_error("Error in writing file");
                            }
                            write_lock.unlock();
                            i++;
                        }
                    }
                } else {
                    throw runtime_error("Error Decoding: receiving packet");
                }
            }
        } else {
            avRawPkt_queue_mutex.unlock();
            unique_lock<mutex> ul(mu);
            if (status == RecordingStatus::stopped) {
                cout << "STOP" << endl;
                break;
            }
            ul.unlock();
        }
    }

    av_packet_unref(&pkt);
}

void ScreenRecorder::initAudioVariables() {
    // FIND DECODER PARAMS
    AVCodecParameters *AudioParams = AudioStream->codecpar;
    // FIND DECODER CODEC
    AudioCodecIn = avcodec_find_decoder(AudioParams->codec_id);
    if (AudioCodecIn == NULL) {
        throw runtime_error("Didn't find a codec audio.");
    }

    // ALLOC AUDIO CODEC CONTEXT BY AUDIO CODEC
    AudioCodecContextIn = avcodec_alloc_context3(AudioCodecIn);
    if (avcodec_parameters_to_context(AudioCodecContextIn, AudioParams) < 0) {
        throw runtime_error("Audio avcodec_parameters_to_context failed");
    }
    // OPEN CODEC
    if (avcodec_open2(AudioCodecContextIn, AudioCodecIn, NULL) < 0) {
        throw runtime_error("Could not open decodec . ");
    }

    // NEW AUDIOSTREAM OUTPUT
    AVStream *AudioStreamOut = avformat_new_stream(avFmtCtxOut, NULL);
    if (!AudioStreamOut) {
        printf("cannnot create new audio stream for output!\n");
    }
    // FIND CODEC OUTPUT
    AudioCodecOut = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!AudioCodecOut) {
        throw runtime_error("Can not find audio encoder");
    }
    AudioCodecContextOut = avcodec_alloc_context3(AudioCodecOut);
    if (!AudioCodecContextOut) {
        throw runtime_error("Can not perform alloc for CodecContextOut");
    }

    if ((AudioCodecOut)->supported_samplerates) {
        AudioCodecContextOut->sample_rate = (AudioCodecOut)->supported_samplerates[0];
        for (int i = 0; (AudioCodecOut)->supported_samplerates[i]; i++) {
            if ((AudioCodecOut)->supported_samplerates[i] == AudioCodecContextIn->sample_rate)
                AudioCodecContextOut->sample_rate = AudioCodecContextIn->sample_rate;
        }
    }

    // INIT CODEC CONTEXT OUTPUT
    AudioCodecContextOut->codec_id = AV_CODEC_ID_AAC;
    AudioCodecContextOut->bit_rate = 128000;
    AudioCodecContextOut->channels = AudioCodecContextIn->channels;
    AudioCodecContextOut->channel_layout = av_get_default_channel_layout(AudioCodecContextOut->channels);
    AudioCodecContextOut->sample_fmt = AudioCodecOut->sample_fmts ? AudioCodecOut->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    AudioCodecContextOut->time_base = {1, AudioCodecContextIn->sample_rate};
    AudioCodecContextOut->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    if (avFmtCtxOut->oformat->flags & AVFMT_GLOBALHEADER)
        AudioCodecContextOut->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    if (avcodec_open2(AudioCodecContextOut, AudioCodecOut, NULL) < 0) {
        throw runtime_error("errore open audio");
    }

    //find a free stream index
    for (int i = 0; i < (int)avFmtCtxOut->nb_streams; i++) {
        if (avFmtCtxOut->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            audioIndexOut = i;
        }
    }
    if (audioIndexOut < 0) {
        throw runtime_error("cannot find a free stream for audio on the output");
    }

    avcodec_parameters_from_context(avFmtCtxOut->streams[audioIndexOut]->codecpar, AudioCodecContextOut);
}

void ScreenRecorder::initOutputFile() {
    // create an empty video file
    if (!(avFmtCtxOut->flags & AVFMT_NOFILE)) {
        if (avio_open2(&avFmtCtxOut->pb, outFilePath.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
            throw runtime_error("Error in creating the video file");
        }
    }

    if (avFmtCtxOut->nb_streams == 0) {
        throw runtime_error("Output file does not contain any stream");
    }
    if (avformat_write_header(avFmtCtxOut, NULL) < 0) {
        throw runtime_error("Error in writing the header context");
    }
}

void ScreenRecorder::windowsResumeAudio() {
    audioDevice = "audio=Microphone (Realtek High Definition Audio)";

    AudioInputFormat = av_find_input_format("dshow");
    int value = avformat_open_input(&FormatContextAudio, audioDevice.c_str(), AudioInputFormat, &AudioOptions);
    if (value != 0) {
        //cerr << "Error in opening input device (audio)" << endl;
        //exit(-1);
        throw runtime_error("Error in opening input device");
    }

    // CHECK STREAM INFO
    if (avformat_find_stream_info(FormatContextAudio, NULL) < 0) {
        throw runtime_error("Couldn't find audio stream information.");
    }

    // FIND AUDIO STREAM INDEX
    int StreamsNumber = (int)FormatContextAudio->nb_streams;
    for (int i = 0; i < StreamsNumber; i++) {
        if (FormatContextAudio->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioIndex = i;
            // SAVE AUDIO STREAM FROM AUDIO CONTEXT
            AudioStream = FormatContextAudio->streams[i];
            break;
        }
    }
    // CHECK AUDIO STREAM INDEX
    if (audioIndex == -1 || audioIndex >= StreamsNumber) {
        throw runtime_error("Didn't find a audio stream.");
    }
}

void ScreenRecorder::acquireAudio() {
    int ret;
    AVPacket *inPacket, *outPacket;
    AVFrame *rawFrame, *scaledFrame;
    uint8_t **resampledData;

    init_fifo();
    //allocate space for a packet
    inPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
    if (!inPacket) {
        throw runtime_error("Cannot allocate an AVPacket for encoded video");
    }
    av_init_packet(inPacket);

    //allocate space for a packet
    rawFrame = av_frame_alloc();
    if (!rawFrame) {
        throw runtime_error("Cannot allocate an AVPacket for encoded video");
    }

    scaledFrame = av_frame_alloc();
    if (!scaledFrame) {
        throw runtime_error("Cannot allocate an AVPacket for encoded video");
    }

    outPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
    if (!outPacket) {
        throw runtime_error("Cannot allocate an AVPacket for encoded video");
    }
    //init the resampler
    SwrContext *swrContext = nullptr;
    swrContext = swr_alloc_set_opts(swrContext,
                                    av_get_default_channel_layout(AudioCodecContextOut->channels),
                                    AudioCodecContextOut->sample_fmt,
                                    AudioCodecContextOut->sample_rate,
                                    av_get_default_channel_layout(AudioCodecContextIn->channels),
                                    AudioCodecContextIn->sample_fmt,
                                    AudioCodecContextIn->sample_rate,
                                    0,
                                    nullptr);

    if (!swrContext) {
        throw runtime_error("Cannot allocate the resample context");
    }
    if ((swr_init(swrContext)) < 0) {
        throw runtime_error("Could not open resample context");
        swr_free(&swrContext);
    }

    //audio_stop_mutex.lock();
    bool firstBuffer = true;
    while (true) {
        unique_lock<mutex> ul(mu);
        if (status == RecordingStatus::stopped) {
            ul.unlock();
            break;
        }
        if (status == RecordingStatus::paused) {
            cout << "Audio Pause" << endl;
#if defined _WIN32
            avformat_close_input(&FormatContextAudio);
            if (FormatContextAudio != nullptr) {
                cerr << "Error: unable to close the inAudioFormatContext" << endl;
                exit(-1);
                //throw error("Error: unable to close the inAudioFormatContext (before pause)");
            }
#endif
        }
        cv.wait(ul, [this]() { return status != RecordingStatus::paused; });
        ul.unlock();
        //audio_stop_mutex.unlock();
        if (av_read_frame(FormatContextAudio, inPacket) >= 0 && inPacket->stream_index == audioIndex) {
            //decode audio routing
            av_packet_rescale_ts(outPacket, FormatContextAudio->streams[audioIndex]->time_base, AudioCodecContextIn->time_base);

            if ((ret = avcodec_send_packet(AudioCodecContextIn, inPacket)) < 0) {
                throw runtime_error("Cannot decode current audio packet ");
                continue;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(AudioCodecContextIn, rawFrame);

                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    throw runtime_error("Error during decoding");
                }
                if (avFmtCtxOut->streams[audioIndexOut]->start_time <= 0) {
                    avFmtCtxOut->streams[audioIndexOut]->start_time = rawFrame->pts;
                }
                initConvertedSamples(&resampledData, AudioCodecContextOut, rawFrame->nb_samples);

                swr_convert(swrContext,
                            resampledData, rawFrame->nb_samples,
                            (const uint8_t **)rawFrame->extended_data, rawFrame->nb_samples);
                add_samples_to_fifo(resampledData, rawFrame->nb_samples);

                //raw frame ready
                av_init_packet(outPacket);
                outPacket->data = nullptr;
                outPacket->size = 0;

                scaledFrame = av_frame_alloc();
                if (!scaledFrame) {
                    throw runtime_error("Cannot allocate an AVPacket for encoded video");
                }

                scaledFrame->nb_samples = AudioCodecContextOut->frame_size;
                scaledFrame->channel_layout = AudioCodecContextOut->channel_layout;
                scaledFrame->format = AudioCodecContextOut->sample_fmt;
                scaledFrame->sample_rate = AudioCodecContextOut->sample_rate;
                av_frame_get_buffer(scaledFrame, 0);
                while (av_audio_fifo_size(AudioFifoBuff) >= AudioCodecContextOut->frame_size) {
                    ret = av_audio_fifo_read(AudioFifoBuff, (void **)(scaledFrame->data), AudioCodecContextOut->frame_size);
                    scaledFrame->pts = pts;
                    pts += scaledFrame->nb_samples;

                    if (avcodec_send_frame(AudioCodecContextOut, scaledFrame) < 0) {
                        throw runtime_error("Cannot encode current audio packet ");
                    }

                    while (ret >= 0) {
                        ret = avcodec_receive_packet(AudioCodecContextOut, outPacket);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                            break;
                        else if (ret < 0) {
                            throw runtime_error("Error during encoding");
                        }
                        av_packet_rescale_ts(outPacket, AudioCodecContextOut->time_base, avFmtCtxOut->streams[audioIndexOut]->time_base);
                        outPacket->stream_index = audioIndexOut;

                        write_lock.lock();
                        if (gotFirstValidVideoPacket) {
                            if (!firstBuffer) {
                                if (av_write_frame(avFmtCtxOut, outPacket) != 0) {
                                    throw runtime_error("Error in writing audio frame");
                                }
                            } else {
                                firstBuffer = false;
                            }
                        }

                        write_lock.unlock();
                        av_packet_unref(outPacket);
                    }
                    ret = 0;
                }

                av_frame_free(&scaledFrame);
                av_packet_unref(outPacket);
            }
        }
        //audio_stop_mutex.lock();
    }
    //audio_stop_mutex.unlock();
}

void ScreenRecorder::init_fifo() {
    /* Create the FIFO buffer based on the specified output sample format. */
    if (!(AudioFifoBuff = av_audio_fifo_alloc(AudioCodecContextOut->sample_fmt, AudioCodecContextOut->channels, 1))) {
        throw runtime_error("Could not allocate FIFO");
    }
}

void ScreenRecorder::add_samples_to_fifo(uint8_t **converted_input_samples, const int frame_size) {
    int error;
    /* Make the FIFO as large as it needs to be to hold both,
    * the old and the new samples. */
    if ((error = av_audio_fifo_realloc(AudioFifoBuff, av_audio_fifo_size(AudioFifoBuff) + frame_size)) < 0) {
        throw runtime_error("Could not reallocate FIFO");
    }
    /* Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(AudioFifoBuff, (void **)converted_input_samples, frame_size) < frame_size) {
        throw runtime_error("Could not write data to FIFO");
    }
}

void ScreenRecorder::initConvertedSamples(uint8_t ***converted_input_samples, AVCodecContext *output_codec_context, int frame_size) {
    if (!(*converted_input_samples = (uint8_t **)calloc(output_codec_context->channels, sizeof(**converted_input_samples)))) {
        throw runtime_error("Could not allocate converted input sample pointers");
    }
    /* Allocate memory for the samples of all channels in one consecutive
  * block for convenience. */
    if (av_samples_alloc(*converted_input_samples, nullptr, output_codec_context->channels, frame_size, output_codec_context->sample_fmt, 0) < 0) {
        throw runtime_error("could not allocate memeory for samples in all channels (audio)");
    }
}
