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

using namespace std;

// FORMAT CONTEXT

AVFormatContext *FormatContextAudio = NULL;
AVFormatContext *FormatContextVideo = NULL;
AVFormatContext *FormatContextOut = NULL;

//AUDIO PARAMETERS

AVCodecContext *AudioCodecContext = NULL;
AVCodecContext *AudioEncoderContext = NULL;
AVInputFormat *AudioInputFormat = NULL;
const AVCodec *AudioCodec = NULL;
const AVCodec *AudioEncoder = NULL;
AVStream *AudioStream = NULL;
//AVStream *AudioStreamOut = NULL;
AVFrame *AudioFrame = NULL;
AVAudioFifo *AudioFifoBuff = NULL;
SwrContext *swrContext = NULL;
int64_t NextAudioPts = 0;
int AudioSamplesCount = 0;
int AudioSamples = 0;
int targetSamplerate = 48000;

int audioIndex = -1;
int audioIndexOut;

int EncodeFrameCnt = 0;

int decodeAudio()
{
  AudioInputFormat = av_find_input_format("alsa");
  if (AudioInputFormat == nullptr)
  {
    cout << "av_find_input_format not found......" << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] DECODE: Find input format: OK" << endl;

  if (avformat_open_input(&FormatContextAudio, "hw:0,0", AudioInputFormat, NULL) < 0)
  {
    cout << "Couldn't open audio input stream." << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] DECODE: Open input: OK" << endl;

  if (avformat_find_stream_info(FormatContextAudio, NULL) < 0)
  {
    cout << "Couldn't find audio stream information." << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] DECODE: Find audio stream info: OK" << endl;

  for (int i = 0; i < (int)FormatContextAudio->nb_streams; ++i)
  {
    if (FormatContextAudio->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      audioIndex = i;
      AudioStream = FormatContextAudio->streams[i];
      break;
    }
  }

  if (audioIndex == -1 || audioIndex >= (int)FormatContextAudio->nb_streams)
  {
    cout << "Didn't find a audio stream." << endl;
    exit(-1);
  }

  AudioCodec = avcodec_find_decoder(AudioStream->codecpar->codec_id);
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
  cout << "[ DEBUG ] DECODE: Decoder open: OK" << endl;

  return 1;
}

int encodeAudio()
{
  /* if (AudioStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
  { */
  AVStream *AudioStreamOut = NULL;
  const char *outputFilename = "test.mp4";

  avformat_alloc_output_context2(&FormatContextOut, NULL, NULL, outputFilename);

  //PARTE VIDEO

  //PARTE AUDIO
  if (AudioStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
  {
    AVCodecContext *CodecContextOut;
    AudioStreamOut = avformat_new_stream(FormatContextOut, NULL);
    AVCodec *AudioCodecOut = avcodec_find_encoder(FormatContextOut->oformat->audio_codec);
    CodecContextOut = avcodec_alloc_context3(AudioCodecOut);
    //AudioStreamOut = FormatContextOut->streams[0];
    //AudioStreamOut->codec->codec = avcodec_find_encoder(FormatContextOut->oformat->audio_codec);
    //CodecContextOut = AudioStreamOut->codec;

    CodecContextOut->sample_rate = 48000; //FormatContextAudio->streams[0]->codec->sample_rate;
    CodecContextOut->channel_layout = AV_CH_LAYOUT_STEREO; // modificato
    CodecContextOut->channels = av_get_channel_layout_nb_channels(CodecContextOut->channel_layout/*AudioStreamOut->codec->channel_layout*/);
    if (CodecContextOut->channel_layout == 0)
    {
      CodecContextOut->channel_layout = AV_CH_LAYOUT_STEREO;
      CodecContextOut->channels = av_get_channel_layout_nb_channels(CodecContextOut->channel_layout);
    }
    CodecContextOut->sample_fmt = AudioCodecOut->sample_fmts[0]; //modificato
    AudioStreamOut->time_base = av_make_q(1, CodecContextOut->sample_rate);

    CodecContextOut->codec_tag = 0;
    if (FormatContextOut->oformat->flags & AVFMT_GLOBALHEADER)
      CodecContextOut->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    cout << "[ DEBUG ] ENCODE: Context Flags: " << CodecContextOut->flags << endl;

    if (avcodec_open2(CodecContextOut, CodecContextOut->codec, 0) < 0)
    {
      cout << "errore audio" << endl;
      exit(-1);
    }
    cout << "  --------------- " << endl;
  }
  if (!(FormatContextOut->oformat->flags & AVFMT_NOFILE))
  {
    if (avio_open(&FormatContextOut->pb, outputFilename, AVIO_FLAG_WRITE) < 0)
    {
      printf("can not open output file handle!\n");
      exit(-1);
    }
  }
  cout << "open file" <<endl;

  if (avformat_write_header(FormatContextOut, NULL) < 0)
  {
    printf("can not write the header of the output file!\n");
    exit(-1);
  }

  cout << "write file" << endl;
  return 1;
};

int initAudio()
{
  avdevice_register_all();
  return 1;
}

int acquireAudio(){

  
  AVPacket pkt;
	AVFrame *frame;
	frame = av_frame_alloc();

  while(true){
    cout << "[ ACQUIRE ]: STARTED" << endl;
    if (av_read_frame(FormatContextAudio, &pkt) < 0)
    {
      cout << "audio av_read_frame < 0" << endl;
      ;
      continue;
    }

    //deprecata
    /* if (avcodec_decode_audio4(AudioCodecContext, frame, &gotframe, &pkt) < 0)
		{
			av_frame_free(&frame);
			printf("can not decoder a frame");
			break;
		} */

    if (avcodec_send_packet(AudioCodecContext, &pkt) != 0)
    {
      av_packet_unref(&pkt);
      continue;
    }

    if (avcodec_receive_frame(AudioCodecContext, frame) != 0)
    {
      av_packet_unref(&pkt);
      continue;
    }

    av_packet_unref(&pkt);

    if (NULL == AudioFifoBuff)
		{
			AudioFifoBuff = av_audio_fifo_alloc(AudioCodecContext->sample_fmt/* FormatContextAudio->streams[0]->codec->sample_fmt*/,
				AudioCodecContext->channels, 30 * frame->nb_samples);
		}

    //int bufferSpace = av_audio_fifo_space(AudioFifoBuff);
		if (av_audio_fifo_space(AudioFifoBuff) >= frame->nb_samples)
		{
			av_audio_fifo_write(AudioFifoBuff, (void **)frame->data, frame->nb_samples);
		}
    av_frame_free(&frame);
  }

  return 1;
}

int recordAudio(){

  acquireAudio();

  while(1){

  }
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