#include <time.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

extern "C"
{
#if defined _WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include "alsa/asoundlib.h"
#endif

#include <stdlib.h>
#include "libavformat/avio.h"
#include "libavutil/audio_fifo.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "unistd.h"
}

using namespace std;

typedef struct
{
    int width;
    int height;
    int offset_x;
    int offset_y;
    int screen_number;
    bool fullscreen;
} RecordingRegionSettings;

//NOTE: audio e video settings, riempiti con i parametri settati
typedef struct
{
    int fps;
    int capturetime_seconds;
    float quality; //value between 0.1 and 1
} VideoSettings;

enum class RecordingStatus
{
    recording,
    paused,
    stopped
};

class ScreenRecorder
{
public:
    ScreenRecorder(RecordingRegionSettings rrs, VideoSettings vs, bool audioOn);
    ~ScreenRecorder();
    int record();

private:
    //settings variables
    RecordingRegionSettings rrs;
    VideoSettings vs;
    bool audioOn;
    mutex write_lock;
    //status variables
    RecordingStatus status;

    //NOTE: vedere se serve

    //common variables
    unique_ptr<thread> captureVideo_thread;
    unique_ptr<thread> captureAudio_thread;
    unique_ptr<thread> elaborate_thread;
    bool stop;

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

    //char* output.mp4?
    string out_file = "out.mp4";
    //TODO: cambiare nome
    FILE *debug;

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

    int audioIndex; // AUDIO STREAM INDEX
    int audioIndexOut;

    ///????????????????
        int64_t NextAudioPts = 0;
        int AudioSamplesCount = 0;
        int AudioSamples = 0;
        int targetSamplerate = 48000;

        int EncodeFrameCnt = 0;
        int64_t pts = 0;
    //???????????????????????

    //functions
    void initCommon();
    void initVideoSource();
    void initVideoVariables();
    void initAudioSource();
    void initAudioVariables();
    void getRawPackets();
    void decodeAndEncode();
    
    void acquireAudio();
    int init_fifo();
    int add_samples_to_fifo(uint8_t **, const int);
    int initConvertedSamples(uint8_t ***, AVCodecContext *, int);

};
