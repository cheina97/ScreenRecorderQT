#ifndef CORE
#define CORE

//#include <windows.h>
#include <stdint.h>
extern "C"
{
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libavutil/time.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}
//#include "PixelFormat.h"
#include "VideoCodec.h"
#include "AudioCodec.h"
//#include "SampleFormat.h"


#pragma comment(lib, "ffmpeg_shared_lib/lib/avformat.lib")
#pragma comment(lib, "ffmpeg_shared_lib/lib/avutil.lib")
#pragma comment(lib, "ffmpeg_shared_lib/lib/avcodec.lib" )
#pragma comment(lib, "ffmpeg_shared_lib/lib/swscale.lib" )
#pragma comment(lib, "ffmpeg_shared_lib/lib/swresample.lib" )
#pragma comment(lib, "ffmpeg_shared_lib/lib/avfilter.lib" )
#pragma comment(lib, "ffmpeg_shared_lib/lib/avdevice.lib" )

#pragma warning(default:4635)
#pragma warning(default:4244)

#endif //CORE