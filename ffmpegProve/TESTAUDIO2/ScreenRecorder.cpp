//
// Created by enrico on 11/08/21.
//

#include "ScreenRecorder.h"

using namespace std;

//atomic<bool> activeMenu{ false };

//Show Dshow Device
void show_dshow_device() {
    AVFormatContext* pFormatCtx = avformat_alloc_context();
    AVDictionary* options = NULL;
    av_dict_set(&options, "list_devices", "true", 0);
    AVInputFormat* iformat = av_find_input_format("dshow");
    printf("========Device Info=============\n");
    avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);
    printf("================================\n");
}

void show_avfoundation_device() {
    AVFormatContext* pFormatCtx = avformat_alloc_context();
    AVDictionary* options = NULL;
    av_dict_set(&options, "list_devices", "true", 0);
    AVInputFormat* iformat = av_find_input_format("avfoundation");
    printf("==AVFoundation Device Info===\n");
    avformat_open_input(&pFormatCtx, "", iformat, &options);
    printf("=============================\n");
}

void getScreenResolution(int& width, int& height) {
#if defined __APPLE__
    //Display* disp = XOpenDisplay(NULL);
    //Screen* scrn = DefaultScreenOfDisplay(disp);
    width = 2880;//scrn->width;
    height = 1800;//scrn->height;
#endif
#if defined _WIN32
    width = (int)GetSystemMetrics(SM_CXSCREEN);
    height = (int)GetSystemMetrics(SM_CYSCREEN);
#endif
#if defined linux
    Display* disp = XOpenDisplay(NULL);
    Screen* scrn = DefaultScreenOfDisplay(disp);
    width = scrn->width;
    height = scrn->height;
#endif
}

ScreenRecorder::ScreenRecorder() : pauseCapture(false), stopCapture(false), started(false), activeMenu(true), recordAudio(false), dir_path(nullptr), disabledMenu(false) {
    avcodec_register_all();
    avdevice_register_all();
#if defined _WIN32
    ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
    deviceName = "";
#endif
    getScreenResolution(width, height);
    screen_height = height;
    screen_width = width;

    cout << width << "x" << height << endl;
    //getScreenResolution(screen_width, screen_height);

    x_offset = 0;
    y_offset = 0;
}

ScreenRecorder::~ScreenRecorder() {

    if (started) {

        t_video.join();
        if (recordAudio)
            t_audio.join();

        value = av_write_trailer(outAVFormatContext);
        if (value < 0) {
            cerr << "Error in writing av trailer" << endl;
            exit(-1);
        }

        avformat_close_input(&inAudioFormatContext);
        if (inAudioFormatContext == nullptr) {
            cout << "inAudioFormatContext close successfully" << endl;
        }
        else {
            cerr << "Error: unable to close the inAudioFormatContext" << endl;
            exit(-1);
        }
        avformat_free_context(inAudioFormatContext);
        if (inAudioFormatContext == nullptr) {
            cout << "AudioFormat freed successfully" << endl;
        }
        else {
            cerr << "Error: unable to free AudioFormatContext" << endl;
            exit(-1);
        }

        avformat_close_input(&pAVFormatContext);
        if (pAVFormatContext == nullptr) {
            cout << "File close successfully" << endl;
        }
        else {
            cerr << "Error: unable to close the file" << endl;
            exit(-1);
        }

        avformat_free_context(pAVFormatContext);
        if (pAVFormatContext == nullptr) {
            cout << "VideoFormat freed successfully" << endl;
        }
        else {
            cerr << "Error: unable to free VideoFormatContext" << endl;
            exit(-1);
        }
    }
}

void ScreenRecorder::stopCommand() {
    unique_lock<mutex> ul(mu);
    stopCapture = true;
    if (pauseCapture)
        pauseCapture = false;
    cv.notify_all();
}

void ScreenRecorder::pauseCommand() {
    unique_lock<mutex> ul(mu);
    if (!pauseCapture)
        pauseCapture = true;
}

void ScreenRecorder::resumeCommand() {
    unique_lock<mutex> ul(mu);
    if (pauseCapture) {
        pauseCapture = false;
        cv.notify_all();
    }
}

void ScreenRecorder::setScreenDimension(int width, int height) {
    this->width = width;
    this->height = height;

    cout << "Screen dimension setted correctly" << endl;
}

void ScreenRecorder::setScreenOffset(int x_offset, int y_offset) {
    this->x_offset = x_offset;
    this->y_offset = y_offset;

    cout << "Screen offset setted correctly" << endl;
}

/*==================================== VIDEO ==============================*/

int ScreenRecorder::openVideoDevice() throw() {
    value = 0;
    videoOptions = nullptr;
    pAVFormatContext = nullptr;

    pAVFormatContext = avformat_alloc_context();


    string dimension = to_string(width) + "x" + to_string(height);
    av_dict_set(&videoOptions, "video_size", dimension.c_str(), 0);   //option to set the dimension of the screen section to record
    value = av_dict_set(&videoOptions, "framerate", "25", 0);
    if (value < 0) {
        cerr << "Error in setting dictionary value (setting framerate)" << endl;
        exit(-1);
    }

    value = av_dict_set(&videoOptions, "preset", "ultrafast", 0);
    if (value < 0) {
        cerr << "Error in setting dictionary value (setting preset value)" << endl;
        exit(-1);
    }
#ifdef _WIN32
    //AVDictionary* opt = nullptr;
    pAVInputFormat = av_find_input_format("gdigrab");
    //Set some options
    //grabbing frame rate
    //The distance from the left edge of the screen or desktop
    av_dict_set(&videoOptions, "offset_x", to_string(x_offset).c_str(), 0);
    //The distance from the top edge of the screen or desktop
    av_dict_set(&videoOptions, "offset_y", to_string(y_offset).c_str(), 0);
    //Video frame size. The default is to capture the full screen
    //av_dict_set(&options,"video_size","640x480",0);

    if (avformat_open_input(&pAVFormatContext, "desktop", pAVInputFormat, &videoOptions) != 0) {
        cerr << "Couldn't open input stream" << endl;
        exit(-1);
    }

#elif defined linux
    //permits to set the capturing from screen
    //Set some options
    //grabbing frame rate
    //av_dict_set(&options,"framerate","5",0);
    //Make the grabbed area follow the mouse
    //av_dict_set(&options,"follow_mouse","centered",0);
    //Video frame size. The default is to capture the full screen
    //av_dict_set(&opt, "offset_x", "20", 0);
    //av_dict_set(&opt, "offset_y", "20", 0);
    //AVDictionary* opt = nullptr;
    //int offset_x = 0, offset_y = 0;
    string url = ":0.0+" + to_string(x_offset) + "," + to_string(y_offset);  //custom string to set the start point of the screen section
    //string dimension = to_string(width) + "x" + to_string(height);
    //av_dict_set(&opt,"video_size",dimension.c_str(),0);   //option to set the dimension of the screen section to record
    pAVInputFormat = av_find_input_format("x11grab");
    value = avformat_open_input(&pAVFormatContext, url.c_str(), pAVInputFormat, &videoOptions);

    if (value != 0) {
        cerr << "Error in opening input device (video)" << endl;
        exit(-1);
        //throw error("Error in opening input device");
    }
    //get video stream infos from context
    /*value = avformat_find_stream_info(pAVFormatContext, nullptr);
    if (value < 0) {
        cout << "\nCannot find the stream information";
        exit(1);
    }*/
#else

    show_avfoundation_device();

    //The distance from the left edge of the screen or desktop
    value = av_dict_set(&videoOptions, "vf", ("crop=" + to_string(width) + ":" + to_string(height) + ":" + to_string(x_offset) + ":" +
        to_string(y_offset)).c_str(), 0);

    if (value < 0) {
        cerr << "Error in setting crop" << endl;
        exit(-1);
    }

    /*value = av_dict_set(&videoOptions,"offset_x",to_string(x_offset).c_str(), 0);
    if(value < 0){
        cerr << "Error in setting offset_x" <<endl;
        exit(-1);
    }
    //The distance from the top edge of the screen or desktop
    value = av_dict_set(&videoOptions,"offset_y",to_string(y_offset).c_str(), 0);
    if(value < 0){
        cerr << "Error in setting offset_y" <<endl;
        exit(-1);
    }
     */
    value = av_dict_set(&videoOptions, "pixel_format", "yuv420p", 0);
    if (value < 0) {
        cerr << "Error in setting pixel format" << endl;
        exit(-1);
    }


    //value = av_dict_set(&options, "video_device_index", "1", 0);
    /*
    if(value < 0){
        cerr << "Error in setting video device index" <<endl;
        exit(-1);
    }
    */
    pAVInputFormat = av_find_input_format("avfoundation");

    if (avformat_open_input(&pAVFormatContext, "1:none", pAVInputFormat, &videoOptions) != 0) {
        cerr << "Error in opening input device" << endl;
        exit(-1);
    }




#endif
    //set frame per second


    /*
    value = av_dict_set(&options, "vsync", "1", 0);
    if(value < 0){
        cerr << "Error in setting dictionary value (setting vsync value)" << endl;
        exit(-1);
    }
    */
    /*
    value = av_dict_set(&options, "probesize", "60M", 0);
    if (value < 0) {
        cerr << "Error in setting probesize value" << endl;
        exit(-1);
    }
    */
    //get video stream infos from context
    value = avformat_find_stream_info(pAVFormatContext, nullptr);
    if (value < 0) {
        cerr << "Error in retrieving the stream info" << endl;
        exit(-1);
    }

    VideoStreamIndx = -1;
    for (int i = 0; i < pAVFormatContext->nb_streams; i++) {
        if (pAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            VideoStreamIndx = i;
            break;
        }
    }
    if (VideoStreamIndx == -1) {
        cerr << "Error: unable to find video stream index" << endl;
        exit(-2);
    }

    pAVCodecContext = pAVFormatContext->streams[VideoStreamIndx]->codec;
    //AVCodecParameters *params = pAVFormatContext->streams[VideoStreamIndx]->codecpar;
    pAVCodec = avcodec_find_decoder(pAVCodecContext->codec_id/*params->codec_id*/);
    if (pAVCodec == nullptr) {
        cerr << "Error: unable to find decoder video" << endl;
        exit(-1);
    }


    //pAVCodecContext = avcodec_alloc_context3(pAVCodec);
    //avcodec_parameters_to_context(pAVCodecContext, params);

    /*int h, w;
    cout << "Insert height and width [h w]: ";   //custom screen dimension to record
    cin >> h >> w;*/



    return 0;
}

/*==========================================  AUDIO  ============================*/

int ScreenRecorder::openAudioDevice() {
    audioOptions = nullptr;
    inAudioFormatContext = nullptr;

    inAudioFormatContext = avformat_alloc_context();
    value = av_dict_set(&audioOptions, "sample_rate", "44100", 0);
    if (value < 0) {
        cerr << "Error: cannot set audio sample rate" << endl;
        exit(-1);
    }
    value = av_dict_set(&audioOptions, "async", "25", 0);
    if (value < 0) {
        cerr << "Error: cannot set audio sample rate" << endl;
        exit(-1);
    }

#if defined linux
    audioInputFormat = av_find_input_format("alsa");
    value = avformat_open_input(&inAudioFormatContext, "hw:0", audioInputFormat, &audioOptions);
    //audioInputFormat = av_find_input_format("pulse");
    //value = avformat_open_input(&inAudioFormatContext, "default", audioInputFormat, &audioOptions);
    if (value != 0) {
        cerr << "Error in opening input device (audio)" << endl;
        exit(-1);
        //throw error("Error in opening input device");
    }
#endif

#if defined _WIN32
    show_dshow_device();
    cout << "\nPlease select the audio device among those listed before: ";
    getchar();
    getline(cin, deviceName);
    deviceName = "audio=" + deviceName;

    audioInputFormat = av_find_input_format("dshow");
    value = avformat_open_input(&inAudioFormatContext, deviceName.c_str(), audioInputFormat, &audioOptions);
    //audioInputFormat = av_find_input_format("pulse");
    //value = avformat_open_input(&inAudioFormatContext, "default", audioInputFormat, &audioOptions);
    if (value != 0) {
        cerr << "Error in opening input device (audio)" << endl;
        exit(-1);
        //throw error("Error in opening input device");
    }
#endif

#if defined __APPLE__
    audioInputFormat = av_find_input_format("avfoundation");
    value = avformat_open_input(&inAudioFormatContext, "none:0", audioInputFormat, &audioOptions);
#endif

    value = avformat_find_stream_info(inAudioFormatContext, nullptr);
    if (value != 0) {
        cerr << "Error: cannot find the audio stream information" << endl;
        exit(-1);
    }

    audioStreamIndx = -1;
    for (int i = 0; i < inAudioFormatContext->nb_streams; i++) {
        if (inAudioFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndx = i;
            break;
        }
    }
    if (audioStreamIndx == -1) {
        cerr << "Error: unable to find audio stream index" << endl;
        exit(-2);
    }
}

int ScreenRecorder::initOutputFile() {
    value = 0;
    outputFile = const_cast<char*>("output.mp4");
#if defined _WIN32
    string completePath = string(dir_path) + string("\\") + string(outputFile);
#else
    string completePath = string(dir_path) + string("/") + string(outputFile);
#endif

    outAVFormatContext = nullptr;
    outputAVFormat = av_guess_format(nullptr, "output.mp4", nullptr);
    if (outputAVFormat == nullptr) {
        cerr << "Error in guessing the video format, try with correct format" << endl;
        exit(-5);
    }

    //avformat_alloc_output_context2(&outAVFormatContext, outputAVFormat, nullptr, outputFile);  //for just video, we used this
    avformat_alloc_output_context2(&outAVFormatContext, outputAVFormat, outputAVFormat->name, const_cast<char*>(completePath.c_str()));

    if (outAVFormatContext == nullptr) {
        cerr << "Error in allocating outAVFormatContext" << endl;
        exit(-4);
    }

    /*===========================================================================*/
    this->generateVideoStream();
    if (recordAudio) this->generateAudioStream();

    //create an empty video file
    if (!(outAVFormatContext->flags & AVFMT_NOFILE)) {
        if (avio_open2(&outAVFormatContext->pb, const_cast<char*>(completePath.c_str()), AVIO_FLAG_WRITE, nullptr, &videoOptions) < 0) {
            cerr << "Error in creating the video file" << endl;
            exit(-10);
        }
    }

    if (outAVFormatContext->nb_streams == 0) {
        cerr << "Output file does not contain any stream" << endl;
        exit(-11);
    }

    value = avformat_write_header(outAVFormatContext, &videoOptions);
    if (value < 0) {
        cerr << "Error in writing the header context" << endl;
        exit(-12);
    }

    return 0;
}

//ANCHOR: STEP 1
void ScreenRecorder::startRecording() {
    if (dir_path == nullptr) {
        cout << "Output direcotry not set. Set it before start recording." << endl;
        setActiveMenu(true);
        setStarted(false);
        return;
    }

    if ((width + x_offset) > screen_width || (height + y_offset) > screen_height) {
        cout << "Diemensions of screen section setted are too large for the screen.";
        setActiveMenu(true);
        setStarted(false);
        return;
    }

    if (recordAudio) openAudioDevice();
    //ANCHOR: STEP 2
    openVideoDevice();
    //ANCHOR: STEP 3
    initOutputFile();
    t_video = std::move(std::thread{ [this]() {
        //ANCHOR: STEP 4
        this->captureVideoFrames();
    }
        });
    if (recordAudio) {
        t_audio = std::move(std::thread{ [this]() {
            this->captureAudio();
        }
            });
    }
}

/*===================================  VIDEO  ==================================*/

void ScreenRecorder::generateVideoStream() {
    outVideoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);  //AV_CODEC_ID_MPEG4
    if (outVideoCodec == nullptr) {
        cerr << "Error in finding the AVCodec, try again with the correct codec" << endl;
        exit(-8);
    }

    //Generate video stream
    videoSt = avformat_new_stream(outAVFormatContext, outVideoCodec);
    if (videoSt == nullptr) {
        cerr << "Error in creating AVFormatStream" << endl;
        exit(-6);
    }

    //outVideoCodec = nullptr;   //avoid segmentation fault on call avcodec_alloc_context3(outAVCodec)
    /*outVideoCodecContext = avcodec_alloc_context3(outVideoCodec);
    if (outVideoCodecContext == nullptr) {
        cerr << "Error in allocating the codec context" << endl;
        exit(-7);
    }*/

    //set properties of the video file (stream)
    outVideoCodecContext = videoSt->codec;
    outVideoCodecContext->codec_id = AV_CODEC_ID_H264;// AV_CODEC_ID_MPEG4; // AV_CODEC_ID_H264 // AV_CODEC_ID_MPEG1VIDEO
    outVideoCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    outVideoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    outVideoCodecContext->bit_rate = /*10000000;*/ 2500000;
    outVideoCodecContext->width = width;   //dimension of the output video file
    outVideoCodecContext->height = height;
    outVideoCodecContext->gop_size = 15;     // 3
    //outVideoCodecContext->global_quality = 500;   //for AV_CODEC_ID_MPEG4
    outVideoCodecContext->max_b_frames = 2;
    outVideoCodecContext->time_base.num = 1;
    outVideoCodecContext->time_base.den = 25; // 15fps
    outVideoCodecContext->bit_rate_tolerance = 400000;

    if (outVideoCodecContext->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(outVideoCodecContext, "preset", "ultrafast", 0);
        av_opt_set(outVideoCodecContext, "cabac", "1", 0);
        av_opt_set(outVideoCodecContext, "ref", "3", 0);
        av_opt_set(outVideoCodecContext, "deblock", "1:0:0", 0);
        av_opt_set(outVideoCodecContext, "analyse", "0x3:0x113", 0);
        av_opt_set(outVideoCodecContext, "subme", "7", 0);
        av_opt_set(outVideoCodecContext, "chroma_qp_offset", "4", 0);
        av_opt_set(outVideoCodecContext, "rc", "crf", 0);
        av_opt_set(outVideoCodecContext, "rc_lookahead", "40", 0);
        av_opt_set(outVideoCodecContext, "crf", "10.0", 0);
    }

    if (outAVFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        outVideoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    value = avcodec_open2(outVideoCodecContext, outVideoCodec, nullptr);
    if (value < 0) {
        cerr << "Error in opening the AVCodec" << endl;
        exit(-9);
    }

    outVideoStreamIndex = -1;
    for (int i = 0; i < outAVFormatContext->nb_streams; i++) {
        if (outAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            outVideoStreamIndex = i;
        }
    }
    if (outVideoStreamIndex < 0) {
        cerr << "Error: cannot find a free stream index for video output" << endl;
        exit(-1);
    }
    avcodec_parameters_from_context(outAVFormatContext->streams[outVideoStreamIndex]->codecpar, outVideoCodecContext);
}

/*===============================  AUDIO  ==================================*/

void ScreenRecorder::generateAudioStream() {
    //inAudioCodecContext = inAudioFormatContext->streams[audioStreamIndx]->codec;
    AVCodecParameters* params = inAudioFormatContext->streams[audioStreamIndx]->codecpar;
    inAudioCodec = avcodec_find_decoder(params->codec_id);
    if (inAudioCodec == nullptr) {
        cerr << "Error: cannot find the audio decoder" << endl;
        exit(-1);
    }

    inAudioCodecContext = avcodec_alloc_context3(inAudioCodec);
    if (avcodec_parameters_to_context(inAudioCodecContext, params) < 0) {
        cout << "Cannot create codec context for audio input" << endl;
    }

    value = avcodec_open2(inAudioCodecContext, inAudioCodec, nullptr);
    if (value < 0) {
        cerr << "Error: cannot open the input audio codec" << endl;
        exit(-1);
    }

    //Generate audio stream
    outAudioCodecContext = nullptr;
    outAudioCodec = nullptr;
    int i;

    AVStream* audio_st = avformat_new_stream(outAVFormatContext, nullptr);
    if (audio_st == nullptr) {
        cerr << "Error: cannot create audio stream" << endl;
        exit(1);
    }

    outAudioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (outAudioCodec == nullptr) {
        cerr << "Error: cannot find requested encoder" << endl;
        exit(1);
    }

    outAudioCodecContext = avcodec_alloc_context3(outAudioCodec);
    if (outAudioCodecContext == nullptr) {
        cerr << "Error: cannot create related VideoCodecContext" << endl;
        exit(1);
    }

    if ((outAudioCodec)->supported_samplerates) {
        outAudioCodecContext->sample_rate = (outAudioCodec)->supported_samplerates[0];
        for (i = 0; (outAudioCodec)->supported_samplerates[i]; i++) {
            if ((outAudioCodec)->supported_samplerates[i] == inAudioCodecContext->sample_rate)
                outAudioCodecContext->sample_rate = inAudioCodecContext->sample_rate;
        }
    }
    outAudioCodecContext->codec_id = AV_CODEC_ID_AAC;
    outAudioCodecContext->sample_fmt = (outAudioCodec)->sample_fmts ? (outAudioCodec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    outAudioCodecContext->channels = inAudioCodecContext->channels;
    outAudioCodecContext->channel_layout = av_get_default_channel_layout(outAudioCodecContext->channels);
    outAudioCodecContext->bit_rate = 96000;
    outAudioCodecContext->time_base = { 1, inAudioCodecContext->sample_rate };

    outAudioCodecContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    if ((outAVFormatContext)->oformat->flags & AVFMT_GLOBALHEADER) {
        outAudioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(outAudioCodecContext, outAudioCodec, nullptr) < 0) {
        cerr << "error in opening the avcodec" << endl;
        exit(1);
    }

    //find a free stream index
    outAudioStreamIndex = -1;
    for (i = 0; i < outAVFormatContext->nb_streams; i++) {
        if (outAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            outAudioStreamIndex = i;
        }
    }
    if (outAudioStreamIndex < 0) {
        cerr << "Error: cannot find a free stream for audio on the output" << endl;
        exit(1);
    }

    avcodec_parameters_from_context(outAVFormatContext->streams[outAudioStreamIndex]->codecpar, outAudioCodecContext);
}

int ScreenRecorder::init_fifo()
{
    /* Create the FIFO buffer based on the specified output sample format. */
    if (!(fifo = av_audio_fifo_alloc(outAudioCodecContext->sample_fmt,
        outAudioCodecContext->channels, 1))) {
        fprintf(stderr, "Could not allocate FIFO\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}

int ScreenRecorder::add_samples_to_fifo(uint8_t** converted_input_samples, const int frame_size) {
    int error;
    /* Make the FIFO as large as it needs to be to hold both,
     * the old and the new samples. */
    if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size)) < 0) {
        fprintf(stderr, "Could not reallocate FIFO\n");
        return error;
    }
    /* Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(fifo, (void**)converted_input_samples, frame_size) < frame_size) {
        fprintf(stderr, "Could not write data to FIFO\n");
        return AVERROR_EXIT;
    }
    return 0;
}

int ScreenRecorder::initConvertedSamples(uint8_t*** converted_input_samples,
    AVCodecContext* output_codec_context,
    int frame_size) {
    int error;
    /* Allocate as many pointers as there are audio channels.
     * Each pointer will later point to the audio samples of the corresponding
     * channels (although it may be NULL for interleaved formats).
     */
    if (!(*converted_input_samples = (uint8_t**)calloc(output_codec_context->channels,
        sizeof(**converted_input_samples)))) {
        fprintf(stderr, "Could not allocate converted input sample pointers\n");
        return AVERROR(ENOMEM);
    }
    /* Allocate memory for the samples of all channels in one consecutive
     * block for convenience. */
    if (av_samples_alloc(*converted_input_samples, nullptr,
        output_codec_context->channels,
        frame_size,
        output_codec_context->sample_fmt, 0) < 0) {

        exit(1);
    }
    return 0;
}

static int64_t pts = 0;
void ScreenRecorder::captureAudio() {
    int ret;
    AVPacket* inPacket, * outPacket;
    AVFrame* rawFrame, * scaledFrame;
    uint8_t** resampledData;

    bool endPause = false;

    init_fifo();

    //allocate space for a packet
    inPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    if (!inPacket) {
        cerr << "Cannot allocate an AVPacket for encoded video" << endl;
        exit(1);
    }
    av_init_packet(inPacket);

    //allocate space for a packet
    rawFrame = av_frame_alloc();
    if (!rawFrame) {
        cerr << "Cannot allocate an AVPacket for encoded video" << endl;
        exit(1);
    }

    scaledFrame = av_frame_alloc();
    if (!scaledFrame) {
        cerr << "Cannot allocate an AVPacket for encoded video" << endl;
        exit(1);
    }

    outPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    if (!outPacket) {
        cerr << "Cannot allocate an AVPacket for encoded video" << endl;
        exit(1);
    }

    //init the resampler
    SwrContext* resampleContext = nullptr;
    resampleContext = swr_alloc_set_opts(resampleContext,
        av_get_default_channel_layout(outAudioCodecContext->channels),
        outAudioCodecContext->sample_fmt,
        outAudioCodecContext->sample_rate,
        av_get_default_channel_layout(inAudioCodecContext->channels),
        inAudioCodecContext->sample_fmt,
        inAudioCodecContext->sample_rate,
        0,
        nullptr);
    if (!resampleContext) {
        cerr << "Cannot allocate the resample context" << endl;
        exit(1);
    }
    if ((swr_init(resampleContext)) < 0) {
        fprintf(stderr, "Could not open resample context\n");
        swr_free(&resampleContext);
        exit(1);
    }

    //int frameFinished, gotFrame;

    //cout << "[AudioThread] thread started!" << endl;

    while (true) {
        unique_lock<mutex> ul(mu);
        //ul.unlock();
        //if(ii++ == noFrames)
        //  break;

        //ul.lock();
        if (pauseCapture) {
            //cout << "Pause audio" << endl;
            endPause = true;
#if defined _WIN32
            avformat_close_input(&inAudioFormatContext);
            if (inAudioFormatContext == nullptr) {
                cout << "inAudioFormatContext close successfully" << endl;
            }
            else {
                cerr << "Error: unable to close the inAudioFormatContext" << endl;
                exit(-1);
            }
#endif
        }
        cv.wait(ul, [this]() { return !pauseCapture; });   //pause capture (not busy waiting)

        if (endPause) {
            endPause = false;
#if defined _WIN32
            value = avformat_open_input(&inAudioFormatContext, deviceName.c_str(), audioInputFormat, &audioOptions);
            //audioInputFormat = av_find_input_format("pulse");
            //value = avformat_open_input(&inAudioFormatContext, "default", audioInputFormat, &audioOptions);
            if (value != 0) {
                cerr << "Error in opening input device (audio)" << endl;
                exit(-1);
                //throw error("Error in opening input device");
            }

            value = avformat_find_stream_info(inAudioFormatContext, nullptr);
            if (value != 0) {
                cerr << "Error: cannot find the audio stream information" << endl;
                exit(-1);
            }

            audioStreamIndx = -1;
            for (int i = 0; i < inAudioFormatContext->nb_streams; i++) {
                if (inAudioFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    audioStreamIndx = i;
                    break;
                }
            }
            if (audioStreamIndx == -1) {
                cerr << "Error: unable to find audio stream index" << endl;
                exit(-2);
            }
#endif
        }
        if (stopCapture) { //check if the capture has to stop
            break;
        }

        ul.unlock();

        if (av_read_frame(inAudioFormatContext, inPacket) >= 0 && inPacket->stream_index == audioStreamIndx) {
            //decode audio routing
            av_packet_rescale_ts(outPacket, inAudioFormatContext->streams[audioStreamIndx]->time_base, inAudioCodecContext->time_base);
            if ((ret = avcodec_send_packet(inAudioCodecContext, inPacket)) < 0) {
                cout << "Cannot decode current audio packet " << ret << endl;
                continue;
            }
            /* ret = avcodec_decode_audio4(inAudioCodecContext, rawFrame, &frameFinished, inPacket);
                if(ret < 0){
                    cout << "Error: unable to decode audio" << endl;
                }
                if(frameFinished){
                    //av_packet_rescale_ts(outPacket,  inAudioFormatContext->streams[audioStreamIndx]->time_base, inAudioCodecContext->time_base);
                    av_init_packet(outPacket);
                    outPacket->data = nullptr;
                    outPacket->size = 0;
                    scaledFrame->nb_samples     = outAudioCodecContext->frame_size;
                    scaledFrame->channel_layout = outAudioCodecContext->channel_layout;
                    scaledFrame->format         = outAudioCodecContext->sample_fmt;
                    scaledFrame->sample_rate    = outAudioCodecContext->sample_rate;
                    avcodec_encode_audio2(outAVCodecContext, outPacket, scaledFrame, &gotFrame);
                    if(gotFrame){
                        if(outPacket->pts != AV_NOPTS_VALUE){
                            outPacket->pts = av_rescale_q(outPacket->pts, audioSt->codec->time_base, audioSt->time_base);
                        }
                        if(outPacket->dts != AV_NOPTS_VALUE){
                            outPacket->dts = av_rescale_q(outPacket->dts, audioSt->codec->time_base, audioSt->time_base);
                        }
                        if(av_write_frame(outAVFormatContext, outPacket) != 0){
                            cerr << "Error in writing audio frame" <<endl;
                        }
                        av_packet_unref(outPacket);
                    }
                    av_packet_unref(outPacket);
                    av_free_packet(inPacket);
                    }
                */
            while (ret >= 0) {
                ret = avcodec_receive_frame(inAudioCodecContext, rawFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    cerr << "Error during decoding" << endl;
                    exit(1);
                }
                if (outAVFormatContext->streams[outAudioStreamIndex]->start_time <= 0) {
                    outAVFormatContext->streams[outAudioStreamIndex]->start_time = rawFrame->pts;
                }
                initConvertedSamples(&resampledData, outAudioCodecContext, rawFrame->nb_samples);

                swr_convert(resampleContext,
                    resampledData, rawFrame->nb_samples,
                    (const uint8_t**)rawFrame->extended_data, rawFrame->nb_samples);

                /*if(endPause){
                    cout << "end pause" << endl;
                    cout << av_audio_fifo_size(fifo) << endl;
                    pts += outAudioCodecContext->frame_size;
                    endPause = false;
                    break;
                }*/

                add_samples_to_fifo(resampledData, rawFrame->nb_samples);

                //raw frame ready
                av_init_packet(outPacket);
                outPacket->data = nullptr;    // packet data will be allocated by the encoder
                outPacket->size = 0;

                const int frame_size = FFMAX(av_audio_fifo_size(fifo), outAudioCodecContext->frame_size);

                scaledFrame = av_frame_alloc();
                if (!scaledFrame) {
                    cerr << "Cannot allocate an AVPacket for encoded video" << endl;
                    exit(1);
                }

                scaledFrame->nb_samples = outAudioCodecContext->frame_size;
                scaledFrame->channel_layout = outAudioCodecContext->channel_layout;
                scaledFrame->format = outAudioCodecContext->sample_fmt;
                scaledFrame->sample_rate = outAudioCodecContext->sample_rate;
                //scaledFrame->best_effort_timestamp = rawFrame->best_effort_timestamp;
                //scaledFrame->pts = rawFrame->pts;
                av_frame_get_buffer(scaledFrame, 0);

                while (av_audio_fifo_size(fifo) >= outAudioCodecContext->frame_size) {

                    ret = av_audio_fifo_read(fifo, (void**)(scaledFrame->data), outAudioCodecContext->frame_size);
                    scaledFrame->pts = pts;
                    pts += scaledFrame->nb_samples;
                    if (avcodec_send_frame(outAudioCodecContext, scaledFrame) < 0) {
                        cout << "Cannot encode current audio packet " << endl;
                        exit(1);
                    }
                    while (ret >= 0) {
                        ret = avcodec_receive_packet(outAudioCodecContext, outPacket);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                            break;
                        else if (ret < 0) {
                            cerr << "Error during encoding" << endl;
                            exit(1);
                        }
                        //outPacket ready
                        av_packet_rescale_ts(outPacket, outAudioCodecContext->time_base, outAVFormatContext->streams[outAudioStreamIndex]->time_base);

                        outPacket->stream_index = outAudioStreamIndex;

                        write_lock.lock();

                        if (av_write_frame(outAVFormatContext, outPacket) != 0)
                        {
                            cerr << "Error in writing audio frame" << endl;
                        }
                        write_lock.unlock();
                        av_packet_unref(outPacket);
                    }
                    ret = 0;
                }// got_picture
                av_frame_free(&scaledFrame);
                av_packet_unref(outPacket);
                //av_freep(&resampledData[0]);
                // free(resampledData);
            }
        }
    }
    /*value = av_write_trailer(outAVFormatContext);
    if(value != 0){
        cerr << "Error in writing audio trailer" << endl;
        exit(-1);
    }
    av_free(audioOutBuff);*/
}

static int64_t ptsVideo = 0;

int ScreenRecorder::captureVideoFrames() {
    int64_t pts = 0;
    int flag;
    int frameFinished = 0;
    bool endPause = false;
    int numPause = 0;

    int64_t numFrame = 0;
#if defined _WIN32
    ofstream outFile{ "..\\media\\log.txt", ios::out };
#else
    ofstream outFile{ "media/log.txt", ios::out };
#endif
    int frameIndex = 0;
    value = 0;

    pAVPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    if (pAVPacket == nullptr) {
        cerr << "Error in allocating AVPacket" << endl;
        exit(-1);
    }

    pAVFrame = av_frame_alloc();
    if (pAVFrame == nullptr) {
        cerr << "Error: unable to alloc the AVFrame resources" << endl;
        exit(-1);
    }
    //pAVFrame->height = 1080;
    //pAVFrame->width = 1920;

    outFrame = av_frame_alloc();
    if (outFrame == nullptr) {
        cerr << "Error: unable to alloc the AVFrame resources for out frame" << endl;
        exit(-1);
    }

    int videoOutBuffSize;
    int nBytes = av_image_get_buffer_size(outVideoCodecContext->pix_fmt, outVideoCodecContext->width, outVideoCodecContext->height, 32);
    uint8_t* videoOutBuff = (uint8_t*)av_malloc(nBytes);

    if (videoOutBuff == nullptr) {
        cerr << "Error: unable to allocate memory" << endl;
        exit(-1);
    }

    value = av_image_fill_arrays(outFrame->data, outFrame->linesize, videoOutBuff, AV_PIX_FMT_YUV420P, outVideoCodecContext->width, outVideoCodecContext->height, 1);
    if (value < 0) {
        cerr << "Error in filling image array" << endl;
    }

    SwsContext* swsCtx_;

    if (avcodec_open2(pAVCodecContext, pAVCodec, nullptr) < 0) {
        cerr << "Could not open codec" << endl;
        exit(-1);
    }



    swsCtx_ = sws_getContext(pAVCodecContext->width, pAVCodecContext->height, pAVCodecContext->pix_fmt, outVideoCodecContext->width, outVideoCodecContext->height, outVideoCodecContext->pix_fmt, SWS_BICUBIC,
        nullptr, nullptr, nullptr);


    cout << "pVCodec Context width: height " << pAVCodecContext->width << " " << pAVCodecContext->height << endl;
    cout << "outVideoCodecContext width: height " << outVideoCodecContext->width << " " << outVideoCodecContext->height << endl;
    AVPacket outPacket;
    int gotPicture;

    double pauseTime, startSeconds, precSeconds=0;
    time_t startPause, stopPause;
    time_t startTime;
    time(&startTime);

    startSeconds = difftime(startTime, 0);

    cout << endl;

    while (true) {
        //ul.unlock();
        //if(ii++ == noFrames)
        //  break;

        //ul.lock();
        unique_lock<mutex> ul(mu);
        if (pauseCapture) {



            endPause = true;
            time(&startPause);
            numPause++;
            /*avformat_close_input(&pAVFormatContext);
            if(pAVFormatContext == nullptr){
                cout << "File close successfully" << endl;
            }
            else{
                cerr << "Error: unable to close the file" << endl;
                exit(-1);
                //throw "Error: unable to close the file";
            }
            avformat_free_context(pAVFormatContext);
            if(pAVFormatContext == nullptr){
                cout << "VideoFormat freed successfully" << endl;
            }
            else{
                cerr << "Error: unable to free VideoFormatContext" << endl;
                exit(-1);
            }*/
            ////////////////////////////////////////////////////////////////////
        }
        cv.wait(ul, [this]() { return !pauseCapture; });   //pause capture (not busy waiting)
        if (endPause) {
            endPause = false;
            time(&stopPause);
            pauseTime = difftime(stopPause, startPause);
            startSeconds += pauseTime;
            ///////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////
        }

        if (stopCapture) { //check if the capture has to stop
            break;
        }
        ul.unlock();

        if (av_read_frame(pAVFormatContext, pAVPacket) >= 0 && pAVPacket->stream_index == VideoStreamIndx) {
            av_packet_rescale_ts(pAVPacket, pAVFormatContext->streams[VideoStreamIndx]->time_base, pAVCodecContext->time_base);
            /////////////////////////////////////////////////////////////////////
            /*if ((value = avcodec_send_packet(pAVCodecContext, pAVPacket)) < 0) {
                cout << "Cannot decode current video packet" << endl;
                continue;
            }
            while(value >= 0){
                value = avcodec_receive_frame(pAVCodecContext, pAVFrame);
                if(value == AVERROR(EAGAIN) || value == AVERROR_EOF){
                    break;
                }
                else if(value < 0){
                    cerr << "Error during decoding" << endl;
                    exit(-1);
                }
                if(outAVFormatContext->streams[outVideoStreamIndex]->start_time <= 0){
                    outAVFormatContext->streams[outVideoStreamIndex]->start_time = pAVFrame->pts;
                }
                av_init_packet(&outPacket);
                outPacket.data = nullptr;
                outPacket.size = 0;
                outFrame->width = outVideoCodecContext->width;
                outFrame->height = outVideoCodecContext->height;
                outFrame->format = outVideoCodecContext->pix_fmt;
                outFrame->pts = pAVFrame->pts;
                outFrame->pkt_dts = pAVFrame->pkt_dts;
                outFrame->best_effort_timestamp = pAVFrame->best_effort_timestamp;
                sws_scale(swsCtx_, pAVFrame->data, pAVFrame->linesize, 0, pAVCodecContext->height, outFrame->data, outFrame->linesize);
                if(avcodec_send_frame(outVideoCodecContext, outFrame) < 0){
                    cerr << "cannot encode current video packet" << endl;
                    exit(-1);
                }
                while(value >= 0){
                    value = avcodec_receive_packet(outVideoCodecContext, &outPacket);
                    if(value == AVERROR(EAGAIN) || value == AVERROR_EOF){
                        break;
                    }
                    else if(value < 0){
                        cerr << "Error during encoding" << endl;
                        exit(-1);
                    }
                    if(outPacket.pts != AV_NOPTS_VALUE){
                        outPacket.pts = av_rescale_q(outPacket.pts, outVideoCodecContext->time_base, outAVFormatContext->streams[outVideoStreamIndex]->time_base);
                    }
                    if(outPacket.dts != AV_NOPTS_VALUE){
                        outPacket.dts = av_rescale_q(outPacket.dts, outVideoCodecContext->time_base, outAVFormatContext->streams[outVideoStreamIndex]->time_base);
                    }
                    outPacket.stream_index = outVideoStreamIndex;
                    write_lock.lock();
                    if(av_write_frame(outAVFormatContext, &outPacket) != 0){
                        cerr << "Error in writing video frame" << endl;
                    }
                    write_lock.unlock();
                    av_packet_unref(&outPacket);
                }
                av_packet_unref(&outPacket);
                av_free_packet(pAVPacket);
            }*/
            //////////////////////////////////////////////////////////////////////
            value = avcodec_decode_video2(pAVCodecContext, pAVFrame, &frameFinished, pAVPacket);
            if (value < 0) {
                cout << "Unable to decode video" << endl;
            }

            pAVFrame->pts = numFrame;

            if (frameFinished) { //frame successfully decoded

                numFrame++;

                //sws_scale(swsCtx_, pAVFrame->data, pAVFrame->linesize, 0, pAVCodecContext->height, outFrame->data, outFrame->linesize);
                av_init_packet(&outPacket);
                outPacket.data = nullptr;
                outPacket.size = 0;

                if (outAVFormatContext->streams[outVideoStreamIndex]->start_time <= 0) {
                    outAVFormatContext->streams[outVideoStreamIndex]->start_time = pAVFrame->pts;
                }

                //disable warning on the console
                outFrame->width = outVideoCodecContext->width;
                outFrame->height = outVideoCodecContext->height;
                outFrame->format = outVideoCodecContext->pix_fmt;

                outFrame->pts = pAVFrame->pts;
                sws_scale(swsCtx_, pAVFrame->data, pAVFrame->linesize, 0, pAVCodecContext->height, outFrame->data, outFrame->linesize);

                avcodec_encode_video2(outVideoCodecContext, &outPacket, outFrame, &gotPicture);

                if (gotPicture) {
                    /*
                    if (outPacket.pts != AV_NOPTS_VALUE) {
                        outPacket.pts = av_rescale_q(outPacket.pts, outVideoCodecContext->time_base, videoSt->time_base);
                    }
                    if (outPacket.dts != AV_NOPTS_VALUE) {
                        outPacket.dts = av_rescale_q(outPacket.dts, outVideoCodecContext->time_base, videoSt->time_base);
                    }
                    */

                    //cout << "Write frame " << j++ << " (size = " << outPacket.size / 1000 << ")" << endl;
                    //cout << "(size = " << outPacket.size << ")" << endl;

                    av_packet_rescale_ts(&outPacket, outVideoCodecContext->time_base, outAVFormatContext->streams[outVideoStreamIndex]->time_base);
                    //outPacket.stream_index = outVideoStreamIndex;

                    outFile << "outPacket->duration: " << outPacket.duration << ", " << "pAVPacket->duration: " << pAVPacket->duration << endl;
                    outFile << "outPacket->pts: " << outPacket.pts << ", " << "pAVPacket->pts: " << pAVPacket->pts << endl;
                    outFile << "outPacket.dts: " << outPacket.dts << ", " << "pAVPacket->dts: " << pAVPacket->dts << endl;

                    time_t timer;
                    double seconds;

                    mu.lock();
                    if (!activeMenu) {
                        disabledMenu = false;
                        time(&timer);
                        seconds = difftime(timer, 0) - startSeconds;
                        int h = (int)(seconds / 3600);
                        int m = (int)(seconds / 60) % 60;
                        int s = (int)(seconds) % 60;
                        if ((seconds - precSeconds) >= 1) {
                            std::cout << "\r" << std::setw(2) << std::setfill('0') << h << ':'
                                << std::setw(2) << std::setfill('0') << m << ':'
                                << std::setw(2) << std::setfill('0') << s << std::flush;
                            precSeconds = seconds;
                        }

                    }
                    else {
                        disabledMenu = true;
                        //std::cout << "Fine dell'attesa" << std::endl;
                    }
                    mu.unlock();

                    write_lock.lock();






                    if (av_write_frame(outAVFormatContext, &outPacket) != 0) {
                        cerr << "Error in writing video frame" << endl;
                    }
                    write_lock.unlock();
                    av_packet_unref(&outPacket);
                }
                gotPicture = 0;
                av_packet_unref(&outPacket);
                av_free_packet(pAVPacket);  //avoid memory saturation
            }
            frameFinished = 0;
        }
    }

    outFile.close();

    av_free(videoOutBuff);

    return 0;
}