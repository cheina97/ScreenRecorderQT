#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "unistd.h"
#include <iostream>

//#include <Windows.h>
#ifdef __cplusplus
};
#endif

//#include "SoundRecordImpl.h"
#include <thread>
#include <fstream>

using namespace std;

AVFormatContext *FormatContext, *FormatContextOut = nullptr;

//AUDIO PARAMETERS

AVCodecContext *AudioCodecContext = nullptr;
AVCodecContext *AudioEncoderContext = nullptr;
AVInputFormat *AudioInputFormat = nullptr;
const AVCodec *AudioCodec = nullptr;
const AVCodec *AudioEncoder = nullptr;
AVStream *AudioStream = nullptr;
AVStream *AudioStreamOut = nullptr;
AVFrame *AudioFrame = nullptr;
SwrContext *swrContext = nullptr;
int64_t NextAudioPts = 0;
int AudioSamplesCount = 0;

string GetSpeakerDeviceName()
{
  char sName[256] = {0};
  string speaker = "";
  bool bRet = false;
  ::CoInitialize(NULL);

  ICreateDevEnum *pCreateDevEnum; //enumrate all speaker devices
  HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_ICreateDevEnum,
                                (void **)&pCreateDevEnum);

  IEnumMoniker *pEm;
  hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioRendererCategory, &pEm, 0);
  if (hr != NOERROR)
  {
    ::CoUninitialize();
    return "";
  }

  pEm->Reset();
  ULONG cFetched;
  IMoniker *pM;
  while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)
  {

    IPropertyBag *pBag = NULL;
    hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
    if (SUCCEEDED(hr))
    {
      VARIANT var;
      var.vt = VT_BSTR;
      hr = pBag->Read(L"FriendlyName", &var, NULL);
      if (hr == NOERROR)
      {
        WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sName, 256, "", NULL);
        speaker = string::fromLocal8Bit(sName);
        SysFreeString(var.bstrVal);
      }
      pBag->Release();
    }
    pM->Release();
    bRet = true;
  }
  pCreateDevEnum = NULL;
  pEm = NULL;
  ::CoUninitialize();
  return speaker;
}

void initAudioSource()
{

  int audioFrameIndex = 0;
  int audioIndex = -1;
  int audioIndexOut = -1;

  char *displayName = getenv("AUDIO");

  string audioSpeaker = "audio=" + GetSpeakerDeviceName();

  avdevice_register_all();
  FormatContext = avformat_alloc_context();
  AudioInputFormat = av_find_input_format("alsa");
  //AudioCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);

  //DECODER
  AudioCodecContext = avcodec_alloc_context3(AudioCodec);

  if (avformat_open_input(&FormatContext, "TODO", AudioInputFormat, nullptr) != 0)
    ;
  {
    cout << "Couldn't open audio input stream." << endl;
    exit(-1);
  }

  if (avformat_find_stream_info(FormatContext, nullptr) < 0)
  {
    cout << "Couldn't find audio stream information." << endl;
    exit(-1);
  }

  for (int i = 0; i < FormatContext->nb_streams; ++i)
  {
    AudioStream = FormatContext->streams[i];
    if (AudioStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      /* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
			if (decoder == nullptr)
			{
				printf("Codec not found.��û���ҵ���������\n");
				return -1;
			}
			//����Ƶ���п���������codecCtx
			m_aDecodeCtx = avcodec_alloc_context3(decoder);
			if ((ret = avcodec_parameters_to_context(m_aDecodeCtx, stream->codecpar)) < 0)
			{
				qDebug() << "Audio avcodec_parameters_to_context failed,error code: " << ret;
				return -1;
			} */
      audioIndex = i;
      break;
    }

    if (audioIndex == -1 || audioIndex >= (int)FormatContext->nb_streams)
    {
      cout << "Didn't find a audio stream." << endl;
      exit(-1);
    }
  }

  AudioCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
  AudioCodecContext = avcodec_alloc_context3(AudioCodec);
  if (avcodec_parameters_to_context(AudioCodecContext, AudioStream->codecpar) < 0)
  {
    cout << "Audio avcodec_parameters_to_context failed" << endl;
    exit(-1);
  }

  if (avcodec_open2(AudioCodecContext, AudioCodec, nullptr) < 0)
  {
    cout << "Could not open decodec . " << endl;
    exit(-1);
  }

  //ENCODER
  FormatContextOut = avformat_alloc_context();
  //AudioStreamOut = FormatContextOut->streams[i];
  if (AudioStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
  {
    /* AudioStreamOut = avformat_new_stream(FormatContextOut, NULL);
    if (!AudioStreamOut)
    {
      printf("can not new audio stream for output!\n");
      exit(-1);
    }
    audioIndexOut = AudioStreamOut->index; */
    //AudioEncoder = avcodec_find_encoder(FormatContextOut->oformat->audio_codec);
    AudioEncoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!AudioEncoder)
    {
      cout << "Can not find audio encoder" << endl;
      exit(-1);
    }
    AudioStreamOut = avformat_new_stream(FormatContextOut, AudioEncoder);
    if (!AudioStreamOut)
    {
      printf("can not new audio stream for output!\n");
      exit(-1);
    }
    audioIndexOut = AudioStreamOut->index;

    AudioEncoderContext = avcodec_alloc_context3(AudioEncoder);
    if (AudioEncoderContext = nullptr)
    {
      cout << "audio avcodec_alloc_context3 failed" << endl;
      exit(-1);
    }
    AudioEncoderContext->sample_fmt = AudioEncoder->sample_fmts ? AudioEncoder->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    AudioEncoderContext->bit_rate = 128000;
    AudioEncoderContext->sample_rate = 48000;

    int targetSamplerate = 48000;
    if (AudioEncoder->supported_samplerates)
    {
      AudioEncoderContext->sample_rate = AudioEncoder->supported_samplerates[0];
      for (int i = 0; AudioEncoder->supported_samplerates[i]; i++)
      {
        if (AudioEncoder->supported_samplerates[i] == targetSamplerate)
          AudioEncoderContext->sample_rate = AudioEncoder->supported_samplerates[i];
      }
    }

    AudioEncoderContext->channels = av_get_channel_layout_nb_channels(AudioEncoderContext->channel_layout);
    AudioEncoderContext->channel_layout = AV_CH_LAYOUT_STEREO;
    if (AudioEncoder->channel_layouts)
    {
      AudioEncoderContext->channel_layout = AudioEncoder->channel_layouts[0];
      for (int i = 0; AudioEncoder->channel_layouts[i]; i++)
      {
        if (AudioEncoder->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
          AudioEncoderContext->channel_layout = AV_CH_LAYOUT_STEREO;
      }
    }

    //AudioEncoderContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    AudioEncoderContext->time_base = av_make_q(1, AudioEncoderContext->sample_rate);
    AudioStreamOut->time_base = av_make_q(1, AudioEncoderContext->sample_rate);
    if ((FormatContextOut->oformat->flags & AVFMT_GLOBALHEADER) != 0)
      AudioEncoderContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    AudioEncoderContext->codec_tag = 0;

    if (!check_sample_fmt(AudioEncoder, AudioEncoderContext->sample_fmt))
    {
      cout << "Encoder does not support sample format " << endl;
      exit(-1);
    }

    if (avcodec_open2(AudioEncoderContext, AudioEncoder, nullptr) < 0)
    {
      cout << "errore audio" << endl;
      exit(-1);
    }

    if (avcodec_parameters_from_context(AudioStreamOut->codecpar, AudioEncoderContext) < 0)
    {
      cout << "Output audio avcodec_parameters_from_context" << endl;
      exit(-1);
    }
    swrContext = swr_alloc();
    if (!swrContext)
    {
      cout << "swr_alloc failed" << endl;
      exit(-1);
    }

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
  }

  if (!(FormatContextOut->oformat->flags & AVFMT_NOFILE))
  {
    if (avio_open(&FormatContextOut->pb, "PATH", AVIO_FLAG_WRITE) < 0)
    {
      printf("can not open output file handle!\n");
      exit(-1);
    }
  }

  if (avformat_write_header(FormatContextOut, nullptr) < 0)
  {
    printf("can not write the header of the output file!\n");
    exit(-1);
  }

  return;
}

int main(int argc, char const *argv[])
{

  return 0;
}