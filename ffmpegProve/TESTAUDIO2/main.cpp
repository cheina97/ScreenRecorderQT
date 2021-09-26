#include <iostream>
#include <chrono>

extern "C"
{
#include <libavutil/samplefmt.h>

#include "alsa/asoundlib.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "unistd.h"
};

//OUTPUT
AVFormatContext *FormatContextOut = NULL;
AVOutputFormat *AVOutFormat = NULL;
AVDictionary *AudioOptions = NULL;

//AUDIO
AVFormatContext *FormatContextAudio = NULL;
AVCodecContext *AudioCodecContextIn = NULL;
AVCodecContext *AudioCodecContextOut = NULL;
AVInputFormat *AudioInputFormat = NULL;
const AVCodec *AudioCodecIn = NULL;
const AVCodec *AudioCodecOut = NULL;
AVAudioFifo *AudioFifoBuff = NULL;
AVStream *AudioStream = NULL;

int audioIndex = -1; // AUDIO STREAM INDEX
int audioIndexOut = -1;

char *outputFile;

//VIDEO
AVFormatContext *FormatContextVideo = NULL;
AVFrame *AudioFrame = NULL;

SwrContext *swrContext = NULL;
int64_t NextAudioPts = 0;
int AudioSamplesCount = 0;
int AudioSamples = 0;
int targetSamplerate = 48000;

int EncodeFrameCnt = 0;
int64_t pts = 0;

int counter = 0;

using namespace std;

//NOTE: generateAudioStream
int decodeAudio()
{
  // FIND DECODER PARAMS
  AVCodecParameters *AudioParams = AudioStream->codecpar;

  // FIND DECODER CODEC
  AudioCodecIn = avcodec_find_decoder(AudioParams->codec_id);
  if (AudioCodecIn == NULL)
  {
    cout << "[ ERROR ] Didn't find a codec audio." << endl;
    exit(-1);
  }
  // cout << "[ DEBUG ] INPUT AUDIO: Find decoder: OK - CODEC ID: " << AudioCodecIn << endl;

  // ALLOC AUDIO CODEC CONTEXT BY AUDIO CODEC
  AudioCodecContextIn = avcodec_alloc_context3(AudioCodecIn);
  if (avcodec_parameters_to_context(AudioCodecContextIn, AudioParams) < 0)
  {
    cout << "[ ERROR ] Audio avcodec_parameters_to_context failed" << endl;
    exit(-1);
  }

  // cout << "[ DEBUG ] INPUT AUDIO: Audio context parameters from codec: OK " << endl;

  // OPEN CODEC
  if (avcodec_open2(AudioCodecContextIn, AudioCodecIn, NULL) < 0)
  {
    cout << "[ ERROR ] Could not open decodec . " << endl;
    exit(-1);
  }
  // cout << "[ DEBUG ] INPUT AUDIO: Decoder open: OK" << endl;

  return 1;
}

//NOTE: parte finale di generateAudioStream
int encodeAudio()
{
  // NEW AUDIOSTREAM OUTPUT
  AVStream *AudioStreamOut = avformat_new_stream(FormatContextOut, NULL);
  if (!AudioStreamOut)
  {
    printf("[ ERROR ] cannnot create new audio stream for output!\n");
    exit(-1);
  }

  // cout << "[ DEBUG ] OUTPUT AUDIO: Encoder new stream: OK" << endl;

  // FIND CODEC OUTPUT
  AudioCodecOut = avcodec_find_encoder(AV_CODEC_ID_AAC);
  if (!AudioCodecOut)
  {
    cout << "[ ERROR ] Can not find audio encoder" << endl;
    exit(-1);
  }

  // cout << "[ DEBUG ] OUTPUT AUDIO: Find Encoder: OK - CODEC ID" << AudioCodecOut->id << endl;

  AudioCodecContextOut = avcodec_alloc_context3(AudioCodecOut);
  if (!AudioCodecContextOut)
  {
    cout << "[ ERROR ] Can not perform alloc for CodecContextOut" << endl;
    exit(-1);
  }

  if ((AudioCodecOut)->supported_samplerates)
  {
    AudioCodecContextOut->sample_rate = (AudioCodecOut)->supported_samplerates[0];
    for (int i = 0; (AudioCodecOut)->supported_samplerates[i]; i++)
    {
      if ((AudioCodecOut)->supported_samplerates[i] == AudioCodecContextIn->sample_rate)
        AudioCodecContextOut->sample_rate = AudioCodecContextIn->sample_rate;
    }
  }
  //AudioStreamOut = FormatContextOut->streams[0];
  //AudioStreamOut->codec->codec = avcodec_find_encoder(FormatContextOut->oformat->audio_codec);
  //CodecContextOut = AudioStreamOut->codec;

  // cout << "[ DEBUG ] OUTPUT AUDIO: Parameters init..." << endl;
  // INIT CODEC CONTEXT OUTPUT
  AudioCodecContextOut->codec_id = AV_CODEC_ID_AAC;
  AudioCodecContextOut->bit_rate = 128000; // ; 96000
  // cout << "[ DEBUG ] OUTPUT AUDIO: --- Context Bit Rate: " << AudioCodecContextOut->bit_rate << endl;
  //AudioCodecContextOut->sample_rate = AudioCodecOut->supported_samplerates ? AudioCodecOut->supported_samplerates[0] : 48000; //FormatContextAudio->streams[0]->codec->sample_rate;
  //// cout << "[ DEBUG ] OUTPUT AUDIO: --- Context Sample Rate: " << AudioCodecContextOut->sample_rate << endl;
  AudioCodecContextOut->channels = AudioCodecContextIn->channels;
  // cout << "[ DEBUG ] OUTPUT AUDIO: --- Context channels number: " << AudioCodecContextOut->channels << endl;
  AudioCodecContextOut->channel_layout = av_get_default_channel_layout(AudioCodecContextOut->channels);
  // cout << "[ DEBUG ] OUTPUT AUDIO: --- Context channel layout: " << AudioCodecContextOut->channel_layout << endl;
  AudioCodecContextOut->sample_fmt = AudioCodecOut->sample_fmts ? AudioCodecOut->sample_fmts[0] : AV_SAMPLE_FMT_FLTP; //modificato
  // cout << "[ DEBUG ] OUTPUT AUDIO: --- Context sample fmt: " << AudioCodecContextOut->sample_fmt << endl;
  AudioCodecContextOut->time_base = {1, AudioCodecContextIn->sample_rate}; //av_make_q(1, AudioCodecContextIn->sample_rate);
  // cout << "[ DEBUG ] OUTPUT AUDIO: --- Stream time base: OK" << endl;
  //CodecContextOut->time_base = av_make_q(1, CodecContextOut->sample_rate);
  AudioCodecContextOut->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
  //CodecContextOut->codec_tag = 0;
  if (FormatContextOut->oformat->flags & AVFMT_GLOBALHEADER)
    AudioCodecContextOut->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  // cout << "[ DEBUG ] OUTPUT AUDIO: --- Context Flags: " << AudioCodecContextOut->flags << endl;

  if (avcodec_open2(AudioCodecContextOut, AudioCodecOut, NULL) < 0)
  {
    cout << "[ ERROR ] errore open audio" << endl;
    exit(-1);
  }
  // cout << "[ DEBUG ] OUTPUT AUDIO: Encoder open: OK " << endl;

  //find a free stream index
  for (int i = 0; i < (int)FormatContextOut->nb_streams; i++)
  {
    if (FormatContextOut->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_UNKNOWN)
    {
      audioIndexOut = i;
      //AudioStreamOut = FormatContextOut->streams[i];
    }
  }
  if (audioIndexOut < 0)
  {
    cout << "[ ERROR ] cannot find a free stream for audio on the output" << endl;
    exit(1);
  }

  avcodec_parameters_from_context(FormatContextOut->streams[audioIndexOut]->codecpar, AudioCodecContextOut);

  // create an empty video file
  if (!(FormatContextOut->flags & AVFMT_NOFILE))
  {
    if (avio_open2(&FormatContextOut->pb, outputFile, AVIO_FLAG_WRITE, NULL, NULL) < 0)
    {
      cerr << "Error in creating the video file" << endl;
      exit(-1);
    }
  }

  if (FormatContextOut->nb_streams == 0)
  {
    cerr << "Output file does not contain any stream" << endl;
    exit(-1);
  }

  if (avformat_write_header(FormatContextOut, NULL) < 0)
  {
    cerr << "Error in writing the header context" << endl;
    exit(-1);
  }
  return 1;
};

// NOTE : START RECORDING: OPEN AUDIO DEVICE
int initAudio()
{
  avdevice_register_all();
  FormatContextAudio = avformat_alloc_context();

  //DICTIONARY INIT
  if (av_dict_set(&AudioOptions, "sample_rate", "44100", 0) < 0)
  {
    cout << "[ ERROR ] impossibile settare il sample rate";
    exit(-1);
  }
  if (av_dict_set(&AudioOptions, "async", "25", 0) < 0)
  {
    cout << "[ ERROR ] Impossibile settare async" << endl;
    exit(-1);
  }

  //#if defined linux
  // GET INPUT FORMAT ALSA
  AudioInputFormat = av_find_input_format("alsa");
  if (AudioInputFormat == NULL)
  {
    cout << "[ ERROR ] av_find_input_format not found......" << endl;
    exit(-1);
  }
  // cout << "[ DEBUG ] INPUT AUDIO: Find input format: OK" << endl;

  // OPEN INPUT FORMAT FROM AUDIO INPUT FORMAT

  //TODO: capire come prendere il device! da ale viene hw:0,0
  //AVDeviceInfoList *devices = (AVDeviceInfoList *)malloc(sizeof(AVDeviceInfoList));
  //avdevice_list_input_sources(NULL, "", NULL, &devices);
  //cout << "Sono qui" << endl;
  // cout << "DEVICES: " << devices.nb_devices << endl;
  // int res = avdevice_list_devices(FormatContextAudio, devices);
  // cout << "tentativo 2: numero di dispositivi " << res << endl;
  // devices = FormatContextAudio->oformat->get_device_list();
  // cout << "tentativo 3: numero di dispositivi" << devices.nb_devices << endl;
  //for (size_t i = 0; i < devices->nb_devices; i++) {
  //cout << devices->devices[i]->device_name << endl;
  //cout << devices->devices[i]->device_description << endl;
  //cout<<endl;
  //}
  //cout << "default: "<<devices->default_device <<endl;
  // exit(-1);
  //////////////////////////////////////

  if (avformat_open_input(&FormatContextAudio, "hw:0", AudioInputFormat, NULL) < 0)
  {
    cout << "[ ERROR ] Couldn't open audio input stream." << endl;
    exit(-1);
  }
  // cout << "[ DEBUG ] INPUT AUDIO: Open input: OK" << endl;

  //#endif

  // CHECK STREAM INFO
  if (avformat_find_stream_info(FormatContextAudio, NULL) < 0)
  {
    cout << "[ ERROR ] Couldn't find audio stream information." << endl;
    exit(-1);
  }
  // cout << "[ DEBUG ] INPUT AUDIO: Find audio stream info: OK" << endl;

  // FIND AUDIO STREAM INDEX
  int StreamsNumber = (int)FormatContextAudio->nb_streams;
  for (int i = 0; i < StreamsNumber; i++)
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
    cout << "[ ERROR ] Didn't find a audio stream." << endl;
    exit(-1);
  }

  // cout << "[ DEBUG ] INPUT AUDIO: Find audio stream index: OK - INDEX: " << audioIndex << endl;

  //avdevice_register_all();
  return 1;
}

int initOutputFile()
{
  outputFile = const_cast<char *>("output.mp4");

  AVOutFormat = av_guess_format(nullptr, outputFile, nullptr);
  if (AVOutFormat == NULL)
  {
    cout << "[ ERROR ] Impossible to guess the format" << endl;
    exit(-1);
  }
  // cout << "[ DEBUG ] OPEN FILE: Guessed Format: OK" << endl;
  // ALLOC FORMAT CONTEXT OUTPUT
  avformat_alloc_output_context2(&FormatContextOut, AVOutFormat, AVOutFormat->name, outputFile);
  if (FormatContextOut == NULL)
  {
    cout << "[ ERROR ] impossible to allocate FormatContextOut" << endl;
    exit(-1);
  }
  // cout << "[ DEBUG ] OPEN FILE: alloc output context: OK" << endl;

  return 1;
}

int init_fifo()
{
  /* Create the FIFO buffer based on the specified output sample format. */
  if (!(AudioFifoBuff = av_audio_fifo_alloc(AudioCodecContextOut->sample_fmt, AudioCodecContextOut->channels, 1)))
  {
    cout << "[ ERROR ] Could not allocate FIFO" << endl;
    exit(-1);
  }
  // cout << "[ DEBUG ] Init fifo: OK" << endl;
  return 0;
}

int add_samples_to_fifo(uint8_t **converted_input_samples, const int frame_size)
{
  int error;
  /* Make the FIFO as large as it needs to be to hold both,
    * the old and the new samples. */
  if ((error = av_audio_fifo_realloc(AudioFifoBuff, av_audio_fifo_size(AudioFifoBuff) + frame_size)) < 0)
  {
    cerr << "[ ERROR ] Could not reallocate FIFO" << endl;
    return error;
  }
  /* Store the new samples in the FIFO buffer. */
  if (av_audio_fifo_write(AudioFifoBuff, (void **)converted_input_samples, frame_size) < frame_size)
  {
    cerr << "[ ERROR ] Could not write data to FIFO" << endl;
    return AVERROR_EXIT;
  }
  return 0;
}

int initConvertedSamples(uint8_t ***converted_input_samples, AVCodecContext *output_codec_context, int frame_size)
{
  if (!(*converted_input_samples = (uint8_t **)calloc(output_codec_context->channels, sizeof(**converted_input_samples))))
  {
    cerr << "[ ERROR ] Could not allocate converted input sample pointers" << endl;
    return AVERROR(ENOMEM);
  }
  /* Allocate memory for the samples of all channels in one consecutive
  * block for convenience. */
  if (av_samples_alloc(*converted_input_samples, nullptr, output_codec_context->channels, frame_size, output_codec_context->sample_fmt, 0) < 0)
  {
    cout << "[ ERROR ] could not allocate memeory for samples in all channels (audio)" << endl;
    exit(1);
  }
  return 0;
}

//NOTE: capture audio
int acquireAudio()
{
  int ret;
  AVPacket *inPacket, *outPacket;
  AVFrame *rawFrame, *scaledFrame;
  uint8_t **resampledData;

  // bool endPause = false;
  init_fifo();

  //allocate space for a packet
  inPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
  if (!inPacket)
  {
    cerr << "[ ERROR ] Cannot allocate an AVPacket for encoded video" << endl;
    exit(-1);
    //throw error("Cannot allocate an AVPacket for encoded video");
  }
  // cout << "[ DEBUG ] inPacket alloc : OK" << endl;
  av_init_packet(inPacket);

  //allocate space for a packet
  rawFrame = av_frame_alloc();
  if (!rawFrame)
  {
    cerr << "[ ERROR ] Cannot allocate an AVPacket for encoded video" << endl;
    exit(-1);
    //throw error("Cannot allocate an AVPacket for encoded video");
  }

  // cout << "[ DEBUG ] Frame alloc: OK" << endl;

  scaledFrame = av_frame_alloc();
  if (!scaledFrame)
  {
    cerr << "[ ERROR ] Cannot allocate an AVPacket for encoded video" << endl;
    exit(-1);
    //throw error("Cannot allocate an AVPacket for encoded video");
  }

  // cout << "[ DEBUG ] Scaled Frame alloc: OK" << endl;

  outPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
  if (!outPacket)
  {
    cerr << "Cannot allocate an AVPacket for encoded video" << endl;
    exit(1);
    //throw error("Cannot allocate an AVPacket for encoded video");
  }
  // cout << "[ DEBUG ] outPacket alloc: OK" << endl;

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
  if (!swrContext)
  {
    cerr << "[ ERROR ] Cannot allocate the resample context" << endl;
    exit(-1);
    //throw error("Cannot allocate the resample context");
  }
  if ((swr_init(swrContext)) < 0)
  {
    cerr << "[ ERROR ] Could not open resample context" << endl;
    swr_free(&swrContext);
    exit(-1);
    //throw error("Could not open resample context");
  }

  // cout << "[ DEBUG ] Resampler: OK" << endl;
  //ANCHOR: qui il while
  while (counter < 30304) //10 secondi
  {
    if (av_read_frame(FormatContextAudio, inPacket) >= 0 && inPacket->stream_index == audioIndex)
    {
      //decode audio routing
      av_packet_rescale_ts(outPacket, FormatContextAudio->streams[audioIndex]->time_base, AudioCodecContextIn->time_base);
      if ((ret = avcodec_send_packet(AudioCodecContextIn, inPacket)) < 0)
      {
        cout << "Cannot decode current audio packet " << ret << endl;
        continue;
      }
      while (ret >= 0)
      {
        ret = avcodec_receive_frame(AudioCodecContextIn, rawFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
          break;
        else if (ret < 0)
        {
          cerr << "Error during decoding" << endl;
          exit(1);
          //throw error("Error during decoding");
        }
        if (FormatContextOut->streams[audioIndexOut]->start_time <= 0)
        {
          FormatContextOut->streams[audioIndexOut]->start_time = rawFrame->pts;
        }
        initConvertedSamples(&resampledData, AudioCodecContextOut, rawFrame->nb_samples);
        swr_convert(swrContext,
                    resampledData, rawFrame->nb_samples,
                    (const uint8_t **)rawFrame->extended_data, rawFrame->nb_samples);
        add_samples_to_fifo(resampledData, rawFrame->nb_samples);

        //raw frame ready
        av_init_packet(outPacket);
        outPacket->data = nullptr; // packet data will be allocated by the encoder
        outPacket->size = 0;
        //const int frame_size = FFMAX(av_audio_fifo_size(AudioFifoBuff), AudioCodecContextOut->frame_size);
        //cout << "frame_size : " << frame_size << endl;

        scaledFrame = av_frame_alloc();
        if (!scaledFrame)
        {
          cerr << "[ ERROR ] Cannot allocate an AVPacket for encoded video" << endl;
          exit(1);
          //throw error("Cannot allocate an AVPacket for encoded video");
        }

        scaledFrame->nb_samples = AudioCodecContextOut->frame_size;
        scaledFrame->channel_layout = AudioCodecContextOut->channel_layout;
        scaledFrame->format = AudioCodecContextOut->sample_fmt;
        scaledFrame->sample_rate = AudioCodecContextOut->sample_rate;
        //scaledFrame->best_effort_timestamp = rawFrame->best_effort_timestamp;
        //scaledFrame->pts = rawFrame->pts;
        av_frame_get_buffer(scaledFrame, 0);

        while (av_audio_fifo_size(AudioFifoBuff) >= AudioCodecContextOut->frame_size)
        {
          ret = av_audio_fifo_read(AudioFifoBuff, (void **)(scaledFrame->data), AudioCodecContextOut->frame_size);
          scaledFrame->pts = pts;
          pts += scaledFrame->nb_samples;

          if (avcodec_send_frame(AudioCodecContextOut, scaledFrame) < 0)
          {
            cerr << "Cannot encode current audio packet " << endl;
            exit(1);
            //throw error("Cannot encode current audio packet");
          }

          while (ret >= 0)
          {
            ret = avcodec_receive_packet(AudioCodecContextOut, outPacket);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
              break;
            else if (ret < 0)
            {
              cerr << "Error during encoding" << endl;
              exit(1);
              //throw error("Error during encoding");
            }
            //outPacket ready
            av_packet_rescale_ts(outPacket, AudioCodecContextOut->time_base, FormatContextOut->streams[audioIndexOut]->time_base);
            outPacket->stream_index = audioIndexOut;

            // write_lock.lock();
            if (av_write_frame(FormatContextOut, outPacket) != 0)
            {
              cerr << "Error in writing audio frame" << endl;
            }
            // write_lock.unlock();
            av_packet_unref(outPacket);
          }
          ret = 0;

        } // got_picture
        av_frame_free(&scaledFrame);
        av_packet_unref(outPacket);
        //av_freep(&resampledData[0]);
        // free(resampledData);
      }
    }
    //cout << "[ DEBUG ] WHILE: CONTATORE: " << counter << endl;
    counter++;
  }
  if (av_write_trailer(FormatContextOut) != 0)
  {
    cerr << "[ ERROR ] impossible to write audio trailer" << endl;
    exit(-1);
  }
  av_free(AudioFifoBuff);
  return 1;
}

int main(int argc, char const *argv[])
{
  //auto startmain = chrono::steady_clock::now();
  cout << "[  DEBUG  ] Init devices" << endl;
  initAudio();
  cout << "------------------------------" << endl;
  cout << "[  DEBUG  ] Init Output File" << endl;
  cout << "------------------------------" << endl;
  initOutputFile();
  cout << "------------------------------" << endl;
  cout << "[  DEBUG  ] DECODING..." << endl;
  cout << "------------------------------" << endl;
  decodeAudio();
  cout << "------------------------------" << endl;
  cout << "[  DEBUG  ] ENCODING..." << endl;
  cout << "------------------------------" << endl;
  encodeAudio();
  cout << "------------------------------" << endl;
  cout << "[  DEBUG  ] RECORDING..." << endl;
  cout << "------------------------------" << endl;
  auto startrec = chrono::steady_clock::now();
  acquireAudio();
  cout << "------------------------------" << endl;
  auto endrec = chrono::steady_clock::now();
  auto intervalrec = endrec - startrec;
  //auto intervalmain = endrec -startmain;
  cout << "We've recorded for " << chrono::duration_cast<chrono::seconds>(intervalrec).count() << " seconds" << endl;
  //cout<<"Total time "<<chrono::duration_cast<chrono::seconds>(intervalmain).count() << endl;

  return 0;
}
