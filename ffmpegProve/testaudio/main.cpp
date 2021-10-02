
#include <thread>
//#include <fstream>
#include <iostream>

extern "C"
{
#include <libavutil/samplefmt.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "unistd.h"
#include "alsa/asoundlib.h"
};

//#include "SoundRecordImpl.h"

using namespace std;

AVFormatContext *FormatContext = nullptr;
AVFormatContext *FormatContextOut = nullptr;

//AUDIO PARAMETERS

AVCodecContext *AudioCodecContext = nullptr;
AVCodecContext *AudioEncoderContext = nullptr;
AVInputFormat *AudioInputFormat = nullptr;
const AVCodec *AudioCodec = nullptr;
const AVCodec *AudioEncoder = nullptr;
AVStream *AudioStream = nullptr;
AVStream *AudioStreamOut = nullptr;
AVFrame *AudioFrame = nullptr;
AVAudioFifo *AudioFifoBuff = nullptr;
SwrContext *swrContext = nullptr;
int64_t NextAudioPts = 0;
int AudioSamplesCount = 0;
int AudioSamples = 0;
int targetSamplerate = 48000;

int audioIndex = -1;
int audioIndexOut;

int EncodeFrameCnt = 0;

int initAudio()
{

  //string audioSpeaker = "audio=" + GetSpeakerDeviceName();
  avdevice_register_all();
  FormatContext = avformat_alloc_context();
  AVOutputFormat *fmt = av_guess_format("mpeg1video", "NULL", "NULL");
  FormatContext->oformat = fmt;
  cout << fmt->flags << endl;
  cout << FormatContext->oformat->flags << endl;
  //exit(5);
  return 1;
}

static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
  const enum AVSampleFormat *p = codec->sample_fmts;

  while (*p != AV_SAMPLE_FMT_NONE)
  {
    if (*p == sample_fmt)
      return 1;
    p++;
  }
  return 0;
}

int decodeAudio()
{

  AudioInputFormat = av_find_input_format("alsa");
  if (AudioInputFormat == nullptr)
  {
    cout << "av_find_input_format not found......" << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] DECODE: Find input format: OK" << endl;

  //command aplay -l -> hw:X,Y where X is card number and Y is device number
  if (avformat_open_input(&FormatContext, "hw:0,0", AudioInputFormat, NULL) != 0)
  {
    cout << "Couldn't open audio input stream." << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] DECODE: Open input: OK" << endl;

//////addedd
  	AVInputFormat *ifmt=av_find_input_format("x11grab");
if(avformat_open_input(&FormatContext,":0.0+10,20",ifmt,nullptr)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}
  cout << "[ DEBUG ] NSTREAMS: "<<FormatContext->nb_streams << endl;
  cout << "[ DEBUG ] NSTREAMS: "<<FormatContext->streams[0]->id << endl;
  cout << "[ DEBUG ] NSTREAMS: "<<FormatContext->streams[1]->id << endl;
///////


  if (avformat_find_stream_info(FormatContext, NULL) < 0)
  {
    cout << "Couldn't find audio stream information." << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] DECODE: Find audio stream info: OK" << endl;

  for (int i = 0; i < (int)FormatContext->nb_streams; ++i)
  {
    if (FormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      audioIndex = i;
      AudioStream = FormatContext->streams[i];
      break;
    }
  }

  if (audioIndex == -1 || audioIndex >= (int)FormatContext->nb_streams)
  {
    cout << "Didn't find a audio stream." << endl;
    exit(-1);
  }

  cout << "[ DEBUG ] DECODE: Find audio stream index: OK - INDEX: " << audioIndex << endl;

  AudioCodec = avcodec_find_decoder(AudioStream->codecpar->codec_id);
  cout << "[ DEBUG ] DECODE: Find decoder: OK - CODEC ID: " << AudioCodec << endl;
  AudioCodecContext = avcodec_alloc_context3(AudioCodec);
  if (avcodec_parameters_to_context(AudioCodecContext, FormatContext->streams[audioIndex]->codecpar) < 0)
  {
    cout << "Audio avcodec_parameters_to_context failed" << endl;
    exit(-1);
  }

  cout << "[ DEBUG ] DECODE: Parameters to context: OK" << endl;
  if (avcodec_open2(AudioCodecContext, AudioCodec, nullptr) < 0)
  {
    cout << "Could not open decodec . " << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] DECODE: Decoder open: OK" << endl;

  return 1;
}

int encodeAudio()
{
  /* if (AudioStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
  { */
  AudioEncoder = avcodec_find_encoder(AV_CODEC_ID_AC3);
  if (!AudioEncoder)
  {
    cout << "Can not find audio encoder" << endl;
    exit(-1);
  }

  cout << "[ DEBUG ] ENCODE: Find Encoder: OK" << endl;

  //FormatContextOut = avformat_alloc_context();
  AudioStreamOut = avformat_new_stream(FormatContext, AudioEncoder);
  if (!AudioStreamOut)
  {
    printf("can not new audio stream for output!\n");
    exit(-1);
  }
  cout << "[ DEBUG ] ENCODE: Encoder new stream: OK" << endl;

  audioIndexOut = AudioStreamOut->index;

  AudioEncoderContext = avcodec_alloc_context3(AudioEncoder);
  if (AudioEncoderContext == nullptr)
  {
    cout << "audio avcodec_alloc_context3 failed" << endl;
    exit(-1);
  }

  cout << "[ DEBUG ] ENCODE: Alloc context: OK" << endl;

  AudioEncoderContext->sample_fmt = AudioEncoder->sample_fmts ? AudioEncoder->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
  cout << "[ DEBUG ] ENCODE: Context sample fmt: " << AudioEncoderContext->sample_fmt << endl;
  AudioEncoderContext->bit_rate = 128000;
  cout << "[ DEBUG ] ENCODE: Context Bit Rate: " << AudioEncoderContext->bit_rate << endl;
  AudioEncoderContext->sample_rate = 48000;
  cout << "[ DEBUG ] ENCODE: Context Sample Rate: " << AudioEncoderContext->sample_rate << endl;

  //cout << "[ DEBUG ] ENCODE: Context sample fmt: " << AudioEncoderContext->sample_fmt << "\n[ DEBUG ] ENCODE: Bit Rate: "<< AudioEncoderContext->bit_rate << "\n[ DEBUG ] ENCODE: Sample Rate: " << AudioEncoderContext->sample_rate << endl;

  /* if (AudioEncoder->supported_samplerates)
  {
    AudioEncoderContext->sample_rate = AudioEncoder->supported_samplerates[0];
    cout << "[ DEBUG ] ENCODE: audio encoder context sample rate: " << AudioEncoderContext->sample_rate << endl;
    for (int i = 0; sizeof(AudioEncoder->supported_samplerates); i++)
    {
      if (AudioEncoder->supported_samplerates[i] == targetSamplerate){
        AudioEncoderContext->sample_rate = AudioEncoder->supported_samplerates[i];
        cout << "[ DEBUG ] ENCODE: Supported sample rate found: OK - Index: " << i << endl;
      }
    }
  } */

  AudioEncoderContext->channel_layout = AV_CH_LAYOUT_STEREO;
  cout << "[ DEBUG ] ENCODE: Context channel layout: " << AudioEncoderContext->channel_layout << endl;
  AudioEncoderContext->channels = av_get_channel_layout_nb_channels(AudioEncoderContext->channel_layout);
  cout << "[ DEBUG ] ENCODE: Context channels number: " << AudioEncoderContext->channels << endl;
  /* if (AudioEncoder->channel_layouts)
  {
    AudioEncoderContext->channel_layout = AudioEncoder->channel_layouts[0];
    cout << "[ DEBUG ] ENCODE: audio encoder context channel layout: " << AudioEncoderContext->channel_layout << endl;
    for (int i = 0; AudioEncoder->channel_layouts[i]; i++)
    {
      if (AudioEncoder->channel_layouts[i] == AV_CH_LAYOUT_STEREO){
        AudioEncoderContext->channel_layout = AV_CH_LAYOUT_STEREO;
        cout << "[ DEBUG ] ENCODE: channel layout found: OK - Index: " << i << endl;
        }
    }
  } */

  //AudioEncoderContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
  AudioEncoderContext->time_base = av_make_q(1, AudioEncoderContext->sample_rate);
  cout << "[ DEBUG ] ENCODE: Context time base: OK" << endl;
  AudioStreamOut->time_base = av_make_q(1, AudioEncoderContext->sample_rate);
  cout << "[ DEBUG ] ENCODE: Stream time base: OK" << endl;
  AudioEncoderContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  cout << "[ DEBUG ] ENCODE: Context Flags: " << AudioEncoderContext->flags << endl;
  AudioEncoderContext->codec_tag = 0;

  if (!check_sample_fmt(AudioEncoder, AudioEncoderContext->sample_fmt))
  {
    cout << "Encoder does not support sample format " << endl;
    exit(-1);
  }

  cout << "[ DEBUG ] ENCODE: Sample format check: OK " << endl;

  if (avcodec_open2(AudioEncoderContext, AudioEncoder, nullptr) < 0)
  {
    cout << "errore audio" << endl;
    exit(-1);
  }

  cout << "[ DEBUG ] ENCODE: Encoder open: OK " << endl;

  if (avcodec_parameters_from_context(AudioStreamOut->codecpar, AudioEncoderContext) < 0)
  {
    cout << "Output audio avcodec_parameters_from_context" << endl;
    exit(-1);
  }

  cout << "[ DEBUG ] ENCODE: Parameters to context: OK " << endl;

  swrContext = swr_alloc();
  if (!swrContext)
  {
    cout << "swr_alloc failed" << endl;
    exit(-1);
  }

  cout << "[ DEBUG ] ENCODE: SWR Alloc: OK " << endl;

  av_opt_set_int(swrContext, "in_channel_count", AudioCodecContext->channels, 0);          //2
  av_opt_set_int(swrContext, "in_sample_rate", AudioCodecContext->sample_rate, 0);         //44100
  av_opt_set_sample_fmt(swrContext, "in_sample_fmt", AudioCodecContext->sample_fmt, 0);    //AV_SAMPLE_FMT_S16
  av_opt_set_int(swrContext, "out_channel_count", AudioEncoderContext->channels, 0);       //2
  av_opt_set_int(swrContext, "out_sample_rate", AudioEncoderContext->sample_rate, 0);      //44100
  av_opt_set_sample_fmt(swrContext, "out_sample_fmt", AudioEncoderContext->sample_fmt, 0); //AV_SAMPLE_FMT_FLTP
  if ((swr_init(swrContext)) < 0)
  {
    cout << "swr_init failed" << endl;
    exit(-1);
  }
  //}
  cout << "[ DEBUG ] ENCODE: SWR Init: OK " << endl;

  //av_dump_format(FormatContext, 0, "out.mp4", 1);

  /* if (!(FormatContext->oformat->flags & AVFMT_NOFILE))
  
    
    
  } */
  if (avio_open(&FormatContext->pb, "out.mp4", AVIO_FLAG_WRITE) < 0)
  {
    printf("can not open output file handle!\n");
    exit(-1);
  }

  cout << "[ DEBUG ] ENCODE: IO Open: OK " << endl;

  if (avformat_write_header(FormatContext, nullptr) < 0)
  {
    printf("can not write the header of the output file!\n");
    exit(-1);
  }

  cout << "[ DEBUG ] ENCODE: OUT File Header write: OK " << endl;

  return 1;
}

AVFrame *AllocAudioFrame(AVCodecContext *c, int samples)
{

  AVFrame *frame = av_frame_alloc();

  frame->format = c->sample_fmt;
  frame->channel_layout = c->channel_layout ? c->channel_layout : AV_CH_LAYOUT_STEREO;
  frame->sample_rate = c->sample_rate;
  frame->nb_samples = samples;

  if (samples)
  {
    if (av_frame_get_buffer(frame, 0) < 0)
    {
      cout << "av_frame_get_buffer failed" << endl;
      return nullptr;
    }
  }

  return frame;
}

void FlushEncoder()
{
  int ret = -1;
  int Flush = 0;
  AVPacket pkt = {0};
  av_init_packet(&pkt);
  ret = avcodec_send_frame(AudioEncoderContext, nullptr);
  cout << "flush audio avcodec_send_frame ret: " << ret << endl;
  while (ret >= 0)
  {
    ret = avcodec_receive_packet(AudioEncoderContext, &pkt);
    if (ret < 0)
    {
      av_packet_unref(&pkt);
      if (ret == AVERROR(EAGAIN))
      {
        cout << "flush audio EAGAIN avcodec_receive_packet" << endl;
        ret = 1;
        continue;
      }
      else if (ret == AVERROR_EOF)
      {
        cout << "flush audio encoder finished" << endl;
        break;
      }
      cout << "flush audio avcodec_receive_packet failed, ret: " << ret << endl;
      return;
    }
    ++Flush;
    pkt.stream_index = audioIndex;
    ret = av_interleaved_write_frame(FormatContext, &pkt);
    if (ret == 0)
      cout << "flush write audio packet id: " << ++EncodeFrameCnt << endl;
    else
      cout << "flush audio av_interleaved_write_frame failed, ret: " << ret << endl;
    av_packet_unref(&pkt);
  }
  cout << "flush times: " << Flush;
}

void Release()
{
  if (FormatContext)
  {
    avio_close(FormatContext->pb);
    avformat_free_context(FormatContext);
    FormatContext = nullptr;
  }
  if (AudioCodecContext)
  {
    avcodec_free_context(&AudioCodecContext);
    AudioCodecContext = nullptr;
  }
  if (AudioEncoderContext)
  {
    avcodec_free_context(&AudioEncoderContext);
    AudioEncoderContext = nullptr;
  }
  if (AudioFifoBuff)
  {
    av_audio_fifo_free(AudioFifoBuff);
    AudioFifoBuff = nullptr;
  }
  if (FormatContext)
  {
    avformat_close_input(&FormatContext);
    FormatContext = nullptr;
  }
}

void AcquireAudio()
{
  AVPacket pkg = {0};
  av_init_packet(&pkg);

  int Samples = AudioSamples;
  int TargetSamples, MaxTargetSamples;

  AVFrame *rawFrame = av_frame_alloc();
  AVFrame *newFrame = AllocAudioFrame(AudioEncoderContext, Samples);

  MaxTargetSamples = TargetSamples = av_rescale_rnd(Samples, AudioEncoderContext->sample_rate, AudioCodecContext->sample_rate, AV_ROUND_UP);

  while (1)
  {
    if (av_read_frame(FormatContext, &pkg) < 0)
    {
      cout << "audio av_read_frame < 0" << endl;
      ;
      continue;
    }

    if (pkg.stream_index != audioIndex)
    {
      av_packet_unref(&pkg);
      continue;
    }

    if (avcodec_send_packet(AudioCodecContext, &pkg) != 0)
    {
      av_packet_unref(&pkg);
      continue;
    }

    if (avcodec_receive_frame(AudioCodecContext, rawFrame) != 0)
    {
      av_packet_unref(&pkg);
      continue;
    }

    TargetSamples = av_rescale_rnd(swr_get_delay(swrContext, AudioCodecContext->sample_rate) + rawFrame->nb_samples, AudioEncoderContext->sample_rate, AudioCodecContext->sample_rate, AV_ROUND_UP);

    if (TargetSamples > MaxTargetSamples)
    {
      cout << "newFrame realloc" << endl;
      av_freep(&newFrame->data[0]);
      //nb_samples*nb_channels*Bytes_sample_fmt
      if (av_samples_alloc(newFrame->data, newFrame->linesize, AudioEncoderContext->channels, TargetSamples, AudioEncoderContext->sample_fmt, 1) < 0)
      {
        cout << "av_samples_alloc failed" << endl;
        exit(-1);
      }
      MaxTargetSamples = TargetSamples;
      AudioEncoderContext->frame_size = TargetSamples;
      AudioSamples = newFrame->nb_samples;
    }

    newFrame->nb_samples = swr_convert(swrContext, newFrame->data, TargetSamples, (const uint8_t **)rawFrame->data, rawFrame->nb_samples);
    if (newFrame->nb_samples < 0)
    {
      cout << "swr_convert failed" << endl;
      exit(-1);
    }

    if (av_audio_fifo_write(AudioFifoBuff, (void **)newFrame->data, newFrame->nb_samples) < newFrame->nb_samples)
    {
      cout << "FIFO Write" << endl;
      return;
    }
  }

  av_frame_free(&rawFrame);
  av_frame_free(&newFrame);
  cout << "sound record exit" << endl;
}

void recordAudio()
{
  int AudioFrameIndex = 0;

  AudioSamples = AudioEncoderContext->frame_size;
  if (!AudioSamples)
  {
    cout << "AudioSamples == 0" << endl;
    AudioSamples = 1024;
  }

  AudioFifoBuff = av_audio_fifo_alloc(AudioEncoderContext->sample_fmt, AudioEncoderContext->channels, 30 * AudioSamples);
  if (!AudioFifoBuff)
  {
    cout << "av_audio_fifo_alloc failed" << endl;
    exit(-1);
  }

  AcquireAudio();

  while (1)
  {
    if (av_audio_fifo_size(AudioFifoBuff) < AudioSamples)
      break;

    AVFrame *aFrame = av_frame_alloc();
    aFrame->nb_samples = AudioSamples;
    aFrame->channel_layout = AudioEncoderContext->channel_layout;
    aFrame->format = AudioEncoderContext->sample_fmt;
    aFrame->sample_rate = AudioEncoderContext->sample_rate;
    aFrame->pts = AudioFrameIndex * AudioSamples;
    ++AudioFrameIndex;

    av_frame_get_buffer(aFrame, 0);
    av_audio_fifo_read(AudioFifoBuff, (void **)aFrame->data, AudioSamples);

    AVPacket pkt = {0};
    av_init_packet(&pkt);

    if (avcodec_send_frame(AudioEncoderContext, aFrame) != 0)
    {
      cout << "audio avcodec_send_frame failed" << endl;
      av_frame_free(&aFrame);
      av_packet_unref(&pkt);
      continue;
    }

    if (avcodec_receive_packet(AudioEncoderContext, &pkt) != 0)
    {
      cout << "audio avcodec_receive_packet failed" << endl;
      av_frame_free(&aFrame);
      av_packet_unref(&pkt);
      continue;
    }

    pkt.stream_index = audioIndex;

    if (av_interleaved_write_frame(FormatContext, &pkt) == 0)
      cout << "Write audio packet id: " << ++EncodeFrameCnt << endl;
    else
      cout << "audio av_interleaved_write_frame failed" << endl;

    av_frame_free(&aFrame);
    av_packet_unref(&pkt);
  }

  FlushEncoder();
  av_write_trailer(FormatContext);
  Release();
  cout << "parent thread exit" << endl;
  ;
}

int main(int argc, char const *argv[])
{
  cout << "[DEBUG] Init devices" << endl;
  initAudio();
  cout << "------------------------------" << endl;
  cout << "[DEBUG] DECODING..." << endl;
  cout << "------------------------------" << endl;
  decodeAudio();
  cout << "------------------------------" << endl;
  cout << "[DEBUG] ENCODING..." << endl;
  cout << "------------------------------" << endl;
  encodeAudio();
  cout << "------------------------------" << endl;
  cout << "[DEBUG] RECORDING..." << endl;
  cout << "------------------------------" << endl;
  recordAudio();

  return 0;
}