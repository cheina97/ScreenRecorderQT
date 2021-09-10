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

//OUTPUT
AVFormatContext *FormatContextOut = NULL;
AVCodecContext *CodecContextOut = NULL;
AVStream *AudioStreamOut = NULL;
AVStream *StreamOut = NULL;

int audioIndexOut = 1;

//AUDIO
AVFormatContext *FormatContextAudio = NULL;
AVCodecContext *AudioCodecContext = NULL;
AVInputFormat *AudioInputFormat = NULL;
const AVCodec *AudioCodec = NULL;
AVAudioFifo *AudioFifoBuff = NULL;
AVStream *AudioStream = NULL;

int audioIndex = -1; // AUDIO STREAM INDEX

//VIDEO
AVFormatContext *FormatContextVideo = NULL;

AVFrame *AudioFrame = NULL;

SwrContext *swrContext = NULL;
int64_t NextAudioPts = 0;
int AudioSamplesCount = 0;
int AudioSamples = 0;
int targetSamplerate = 48000;

int EncodeFrameCnt = 0;

int decodeAudio()
{
  int StreamsNumber;
  // GET INPUT FORMAT ALSA
  AudioInputFormat = av_find_input_format("alsa");
  if (AudioInputFormat == NULL)
  {
    cout << "av_find_input_format not found......" << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] INPUT AUDIO: Find input format: OK" << endl;

  // OPEN INPUT FORMAT FROM AUDIO INPUT FORMAT
  if (avformat_open_input(&FormatContextAudio, "hw:0,0", AudioInputFormat, NULL) < 0)
  {
    cout << "Couldn't open audio input stream." << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] INPUT AUDIO: Open input: OK" << endl;

  // CHECK STREAM INFO
  if (avformat_find_stream_info(FormatContextAudio, NULL) < 0)
  {
    cout << "Couldn't find audio stream information." << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] INPUT AUDIO: Find audio stream info: OK" << endl;

  // FIND AUDIO STREAM INDEX
  StreamsNumber = (int)FormatContextAudio->nb_streams;
  for (int i = 0; i < StreamsNumber; ++i)
  {
    if (FormatContextAudio->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      audioIndex = i;
      // SAVE AUDIO STREAM FROM AUDIO CONTEXT
      AudioStream = FormatContextAudio->streams[i];
      break;
    }
  }

  // CHECK AUDIO STREAM INDEX
  if (audioIndex == -1 || audioIndex >= StreamsNumber)
  {
    cout << "Didn't find a audio stream." << endl;
    exit(-1);
  }

  cout << "[ DEBUG ] INPUT AUDIO: Find audio stream index: OK - INDEX: " << audioIndex << endl;

  // FIND DECODER CODEC
  AudioCodec = avcodec_find_decoder(AudioStream->codecpar->codec_id);
  if (AudioCodec == NULL)
  {
    cout << "Didn't find a codec audio." << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] INPUT AUDIO: Find decoder: OK - CODEC ID: " << AudioCodec << endl;

  // ALLOC AUDIO CODEC CONTEXT BY AUDIO CODEC
  AudioCodecContext = avcodec_alloc_context3(AudioCodec);
  if (avcodec_parameters_to_context(AudioCodecContext, AudioStream->codecpar) < 0)
  {
    cout << "Audio avcodec_parameters_to_context failed" << endl;
    exit(-1);
  }

  // OPEN CODEC
  if (avcodec_open2(AudioCodecContext, AudioCodec, NULL) < 0)
  {
    cout << "Could not open decodec . " << endl;
    exit(-1);
  }
  cout << "[ DEBUG ] INPUT AUDIO: Decoder open: OK" << endl;

  return 1;
}

int encodeAudio()
{

  const char *outputFilename = "test.mp4";

  // ALLOC FORMAT CONTEXT OUTPUT
  avformat_alloc_output_context2(&FormatContextOut, NULL, NULL, outputFilename);

  //PARTE VIDEO TODO

  //PARTE AUDIO
  if (AudioStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
  {
    // FIND CODEC OUTPUT
    AVCodec *AudioCodecOut = avcodec_find_encoder(/* AV_CODEC_ID_AC3 */FormatContextOut->oformat->audio_codec);
    if (!AudioCodecOut)
    {
      cout << "Can not find audio encoder" << endl;
      exit(-1);
    }

    cout << "[ DEBUG ] OUTPUT AUDIO: Find Encoder: OK" << endl;

    // NEW AUDIOSTREAM OUTPUT
    AudioStreamOut = avformat_new_stream(FormatContextOut, AudioCodecOut);
    if (!AudioStreamOut)
    {
      printf("can not new audio stream for output!\n");
      exit(-1);
    }

    cout << "[ DEBUG ] OUTPUT AUDIO: Encoder new stream: OK" << endl;

    CodecContextOut = avcodec_alloc_context3(AudioCodecOut);
    //AudioStreamOut = FormatContextOut->streams[0];
    //AudioStreamOut->codec->codec = avcodec_find_encoder(FormatContextOut->oformat->audio_codec);
    //CodecContextOut = AudioStreamOut->codec;

    cout << "[ DEBUG ] OUTPUT AUDIO: Parameters init..." << endl;
    // INIT CODEC CONTEXT OUTPUT
    CodecContextOut->bit_rate = 128000;
    cout << "[ DEBUG ] OUTPUT AUDIO: --- Context Bit Rate: " << CodecContextOut->bit_rate << endl;
    CodecContextOut->sample_rate = AudioCodecOut->supported_samplerates ? AudioCodecOut->supported_samplerates[0] : 48000; //FormatContextAudio->streams[0]->codec->sample_rate;
    cout << "[ DEBUG ] OUTPUT AUDIO: --- Context Sample Rate: " << CodecContextOut->sample_rate << endl;
    CodecContextOut->channel_layout = AudioCodecOut->channel_layouts ? AudioCodecOut->channel_layouts[0] : AV_CH_LAYOUT_STEREO; // modificato
    cout << "[ DEBUG ] OUTPUT AUDIO: --- Context channel layout: " << CodecContextOut->channel_layout << endl;
    CodecContextOut->channels = av_get_channel_layout_nb_channels(CodecContextOut->channel_layout /*AudioStreamOut->codec->channel_layout*/);
    cout << "[ DEBUG ] OUTPUT AUDIO: --- Context channels number: " << CodecContextOut->channels << endl;
    CodecContextOut->sample_fmt = AudioCodecOut->sample_fmts ? AudioCodecOut->sample_fmts[0] : AV_SAMPLE_FMT_FLTP; //modificato
    cout << "[ DEBUG ] OUTPUT AUDIO: --- Context sample fmt: " << CodecContextOut->sample_fmt << endl;
    AudioStreamOut->time_base = av_make_q(1, CodecContextOut->sample_rate);
    cout << "[ DEBUG ] OUTPUT AUDIO: --- Stream time base: OK" << endl;
    //CodecContextOut->time_base = av_make_q(1, CodecContextOut->sample_rate);
    CodecContextOut->codec_tag = 0;
    if (FormatContextOut->oformat->flags & AVFMT_GLOBALHEADER)
      CodecContextOut->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    cout << "[ DEBUG ] OUTPUT AUDIO: --- Context Flags: " << CodecContextOut->flags << endl;

    if (avcodec_open2(CodecContextOut, AudioCodecOut, NULL) < 0)
    {
      cout << "errore audio" << endl;
      exit(-1);
    }
    cout << "[ DEBUG ] OUTPUT AUDIO: Encoder open: OK " << endl;
  }

  avcodec_parameters_from_context(AudioStreamOut->codecpar, CodecContextOut);
  if (!(FormatContextOut->oformat->flags & AVFMT_NOFILE))
  {
    if (avio_open(&FormatContextOut->pb, outputFilename, AVIO_FLAG_WRITE) < 0)
    {
      printf("can not open output file handle!\n");
      exit(-1);
    }
  }
  cout << "[ DEBUG ] OUTPUT AUDIO: Opening File: OK " << endl;

  if (avformat_write_header(FormatContextOut, NULL) < 0)
  {
    printf("can not write the header of the output file!\n");
    exit(-1);
  }

  cout << "[ DEBUG ] OUTPUT AUDIO: File header write: OK " << endl;
  return 1;
};

int initAudio()
{
  avdevice_register_all();
  return 1;
}

int acquireAudio()
{

  AVPacket pkt;
  AVFrame *frame;
  frame = av_frame_alloc();

  while (true)
  {
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
      AudioFifoBuff = av_audio_fifo_alloc(AudioCodecContext->sample_fmt /* FormatContextAudio->streams[0]->codec->sample_fmt*/,
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

int recordAudio()
{

  acquireAudio();

  while (1)
  {
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
  //recordAudio();

  return 0;
}