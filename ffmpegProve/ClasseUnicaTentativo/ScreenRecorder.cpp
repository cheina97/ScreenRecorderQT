#include "ScreenRecorder.h"

#include <exception>
#include <iostream>

#include "GetAudioDevices.h"
#include "MemoryCheckLinux.h"

using namespace std;

ScreenRecorder::ScreenRecorder(RecordingRegionSettings rrs, VideoSettings vs, bool audioOn, string outFilePath, string audioDevice) : rrs(rrs), vs(vs), audioOn(audioOn), status(RecordingStatus::stopped), outFilePath(outFilePath), audioDevice(audioDevice) {
    initCommon();
    cout << "-> Finita initCommon" << endl;
    initVideoSource();
    cout << "-> Finita initVideoSource" << endl;
    initVideoVariables();
    cout << "-> Finita initiVideoVariables" << endl;
    if (audioOn) {
        initAudioSource();
        initAudioVariables();
        cout << "-> Finita initAudioSource" << endl;
    }
    initOutputFile();
    memoryCheck_init(4000);
}

ScreenRecorder::~ScreenRecorder() {
    captureVideo_thread.get()->join();
    elaborate_thread.get()->join();
    if (audioOn)
        captureAudio_thread.get()->join();

    av_write_trailer(avFmtCtxOut);
    avformat_close_input(&avFmtCtx);
    avio_close(avFmtCtxOut->pb);
    avformat_free_context(avFmtCtx);

    cout << "Distruttore Screen Recorder" << endl;
}

void ScreenRecorder::record() {
    stop = false;
    audio_stop = false;
    captureVideo_thread = make_unique<thread>([this]() { this->getRawPackets(); });
    elaborate_thread = make_unique<thread>([this]() { this->decodeAndEncode(); });
    if (audioOn)
        captureAudio_thread = make_unique<thread>([this]() { this->acquireAudio(); });
}

void ScreenRecorder::initCommon() {
    avdevice_register_all();
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
        Display *display = XOpenDisplay(displayName);
        int screenNum = DefaultScreen(display);
        rrs.width = DisplayWidth(display, screenNum);
        rrs.height = DisplayHeight(display, screenNum);
        XCloseDisplay(display);
    }

    rrs.width = rrs.width / 32 * 32;
    rrs.height = rrs.height / 2 * 2;

    avRawOptions = nullptr;
    av_dict_set(&avRawOptions, "video_size", (to_string(rrs.width) + "*" + to_string(rrs.height)).c_str(), 0);
    av_dict_set(&avRawOptions, "framerate", to_string(vs.fps).c_str(), 0);
    av_dict_set(&avRawOptions, "show_region", "1", 0);
    av_dict_set(&avRawOptions, "probesize", "30M", 0);

    AVInputFormat *avInputFmt = av_find_input_format("x11grab");

    if (avInputFmt == nullptr) {
        throw logic_error{"av_find_input_format not found......"};
    }

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
}

void ScreenRecorder::initVideoVariables() {
    video_st = avformat_new_stream(avFmtCtxOut, avEncodec);
    avEncoderCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(avEncoderCtx, video_st->codecpar);

    avEncoderCtx->codec_id = AV_CODEC_ID_H264;
    avEncoderCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    avEncoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    avEncoderCtx->bit_rate = 80000;
    avEncoderCtx->width = (int)(rrs.width * vs.quality) / 32 * 32;
    avEncoderCtx->height = (int)(rrs.height * vs.quality) / 2 * 2;
    avEncoderCtx->time_base.num = 1;
    avEncoderCtx->time_base.den = vs.fps;
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
    // WINDOWS OPEN INPUT
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
    int frameNumber = vs.fps * vs.capturetime_seconds;
    AVPacket *avRawPkt;
    for (int i = 0; i < frameNumber; i++) {
        avRawPkt = av_packet_alloc();
        if (av_read_frame(avFmtCtx, avRawPkt) < 0) {
            throw runtime_error("Error in getting RawPacket from x11");
        }
        avRawPkt_queue_mutex.lock();
        avRawPkt_queue.push(avRawPkt);
        avRawPkt_queue_mutex.unlock();

        //da togliere da qui e mettere nell'app con chiamata a stop();
        try {
            memoryCheck_limitSurpassed();
        } catch (const runtime_error &e) {
            cout << "ERROR: MEMORY LIMIT SURPASSED" << endl;
            i = frameNumber;
        }
    }

    avRawPkt_queue_mutex.lock();
    stop = true;
    avRawPkt_queue_mutex.unlock();
    audio_stop_mutex.lock();
    audio_stop = true;
    audio_stop_mutex.unlock();
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
    int i = 0;

    avRawPkt_queue_mutex.lock();
    while (!stop || !avRawPkt_queue.empty()) {

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
                    avYUVFrame->pts = (int64_t)i * (int64_t)30 * (int64_t)30 * (int64_t)100 / (int64_t)vs.fps;
                    flag = avcodec_send_frame(avEncoderCtx, avYUVFrame);
                    got_picture = avcodec_receive_packet(avEncoderCtx, &pkt);
                    //Fine ENCODING

                    if (flag >= 0) {
                        if (got_picture == 0) {
                            pkt.pts = (int64_t)i * (int64_t)30 * (int64_t)30 * (int64_t)100 / (int64_t)vs.fps;
                            pkt.dts = (int64_t)i * (int64_t)30 * (int64_t)30 * (int64_t)100 / (int64_t)vs.fps;
                            write_lock.lock();
                            if (av_write_frame(avFmtCtxOut, &pkt) < 0) {
                                throw runtime_error("Error in writing file");
                            }
                            write_lock.unlock();
                        }
                    }
                } else {
                    throw runtime_error("Error Decoding: receiving packet");
                }
            }
            i++;
        } else {
            avRawPkt_queue_mutex.unlock();
        }
        avRawPkt_queue_mutex.lock();
    }
    avRawPkt_queue_mutex.unlock();
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

    int contatoreDaEliminare = 0;
    audio_stop_mutex.lock();
    while (!audio_stop) {
        audio_stop_mutex.unlock();
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
                    //throw error("Error during decoding");
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
                    contatoreDaEliminare++;
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
                        //outPacket ready
                        av_packet_rescale_ts(outPacket, AudioCodecContextOut->time_base, avFmtCtxOut->streams[audioIndexOut]->time_base);
                        outPacket->stream_index = audioIndexOut;

                        write_lock.lock();

                        if (av_write_frame(avFmtCtxOut, outPacket) != 0) {
                            throw runtime_error("Error in writing audio frame");
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
        audio_stop_mutex.lock();
    }
    audio_stop_mutex.unlock();
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
