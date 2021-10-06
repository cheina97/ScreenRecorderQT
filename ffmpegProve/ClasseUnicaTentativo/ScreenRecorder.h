#include <time.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

extern "C" {
#if defined _WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#endif

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

//TODO: CAMBIARE NOME
typedef struct {
    int width;
    int height;
    int offset_x;
    int offset_y;
    int screen_number;
    bool fullscreen;
} RecordingRegionSettings;

//NOTE: audio e video settings, riempiti con i parametri settati
typedef struct {
    int fps;
    int capturetime_seconds;
    float quality;  //value between 0.1 and 1
} VideoSettings;

typedef struct {
    //TODO
} AudioSettings;

enum class RecordingStatus { recording,
                    paused,
                    stopped };

class ScreenRecorder {
   public:
    ScreenRecorder(RecordingRegionSettings rrs, VideoSettings vs, AudioSettings as);
    ~ScreenRecorder();
    int record();

   private:
    //settings variables
    RecordingRegionSettings rrs;
    VideoSettings vs;
    AudioSettings as;
    //status variables
    RecordingStatus status;
    
    //NOTE: vedere se serve
    bool stop;

    //common variables
    unique_ptr<thread> capture_thread;
    unique_ptr<thread> elaborate_thread;
    
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

    //functions
    void initCommon();
    void initVideoSource();
    void initAudioSource();
    void initVideoVariables();
    void getRawPackets();
    void decodeAndEncode();
   

   protected:
};
