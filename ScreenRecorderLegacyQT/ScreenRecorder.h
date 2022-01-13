#ifndef SCREENRECORDER_H
#define SCREENRECORDER_H
#include <time.h>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

extern "C" {
#if defined _WIN32
#include <windows.h>

#include "io.h"
#elif defined __linux__
#include <X11/Xlib.h>

#include "alsa/asoundlib.h"
#include "unistd.h"
#elif defined __APPLE__

#endif

#include <stdlib.h>

#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

using namespace std;

//error messages

const string err_msg_baddevice_audio =
    "Couldn't use this audio device to record, maybe it is busy or cannot be used to record.\n"
    "Check if it is used by another program, then try again.\n"
    "If it still not work try with another device";
const string err_msg_baddevice_video =
    "Couldn't use this video device to record, maybe it is busy or cannot be used to record.\n"
    "Check if it is used by another program, then try again.\n"
    "If it still not work try with another device";
const string err_msg_badpath = "Failed to create output file.\nThis path is invalid or doesn't exist.";
typedef struct
{
    int width;
    int height;
    int offset_x;
    int offset_y;
    int screen_number;
} RecordingRegionSettings;

typedef struct
{
    int fps;
    float quality;    //value between 0.1 and 1
    int compression;  // value between 1 and 8
    bool audioOn;
} VideoSettings;

enum class RecordingStatus {
    recording,
    paused,
    stopped
};

class ScreenRecorder {
   public:
    ScreenRecorder(RecordingRegionSettings rrs, VideoSettings vs, string outFilePath, string audioDevice = "noDevice");
    ~ScreenRecorder();
    void record();
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    RecordingStatus getStatus();

   private:
    //errors handling
    queue<string> error_queue;
    mutex error_queue_m;
    int terminated_threads = 0;
    condition_variable error_queue_cv;
    function<void(void)> make_error_handler(function<void(void)> f);

    //settings variables
    RecordingRegionSettings rrs;
    VideoSettings vs;
    RecordingStatus status;
    string outFilePath;
    string audioDevice;
    mutex write_lock;
    mutex status_lock;
    mutex video_lock;
    mutex audio_lock;
    bool audio_ready = false;
    bool video_ready = false;
    bool audio_end = false;
    bool end = false;

    //common variables
    unique_ptr<thread> captureVideo_thread;
    unique_ptr<thread> captureAudio_thread;
    unique_ptr<thread> elaborate_thread;
    bool gotFirstValidVideoPacket;

    //video variables
    AVFormatContext *avFmtCtx, *avFmtCtxOut;
    AVCodecContext *avRawCodecCtx;
    AVCodecContext *avEncoderCtx;
    AVDictionary *avRawOptions;
    AVCodec *avDecodec;
    AVCodec *avEncodec;
    struct SwsContext *swsCtx;
    queue<AVPacket *> avRawPkt_queue;
    mutex avRawPkt_queue_mutex;
    int videoIndex;
    AVFrame *avYUVFrame;
    AVOutputFormat *fmt;
    AVStream *video_st;
    int64_t pts_offset;

    //audio variables
    AVDictionary *AudioOptions;
    AVFormatContext *FormatContextAudio;
    AVCodecContext *AudioCodecContextIn;
    AVCodecContext *AudioCodecContextOut;
    AVInputFormat *AudioInputFormat;
    const AVCodec *AudioCodecIn;
    const AVCodec *AudioCodecOut;
    AVAudioFifo *AudioFifoBuff;
    AVStream *AudioStream;
    mutex audio_stop_mutex;
    bool audio_stop;

    int audioIndex;  // AUDIO STREAM INDEX
    int audioIndexOut;

    int64_t NextAudioPts = 0;
    int AudioSamplesCount = 0;
    int AudioSamples = 0;
    int targetSamplerate = 48000;

    int EncodeFrameCnt = 0;
    int64_t pts = 0;

    // HANDLER PAUSE/RESUME/STOP
    condition_variable cv_audio;
    condition_variable cv_video;
    condition_variable cv;
    int getlatestFramesValue();
    bool audioReady();
    bool videoReady();
    void audioEnd();
    void handler();
    void linuxVideoResume();
    void windowsResumeAudio();
    string statusToString();

    //functions
    void initCommon();
    void initVideoSource();
    void initVideoVariables();
    void initAudioSource();
    void initAudioVariables();
    void initOutputFile();
    void getRawPackets();
    void decodeAndEncode();

    void acquireAudio();
    void init_fifo();
    void add_samples_to_fifo(uint8_t **, const int);
    void initConvertedSamples(uint8_t ***, AVCodecContext *, int);
};

#endif  // SCREENRECORDER_H
