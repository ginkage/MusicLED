#include "audio_data.h"

#include <stdio.h>
#include <stdlib.h>

audio_data::audio_data()
{
    const char* audio_source = "hw:CARD=audioinjectorpi,DEV=0";

    // alsa: open device to capture audio
    int err = snd_pcm_open(&handle, audio_source, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        fprintf(stderr, "error opening stream: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params); // assembling params
    snd_pcm_hw_params_any(handle, params); // setting defaults or something

    // interleaved mode right left right left
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    // trying to set 16bit
    snd_pcm_format_t pcm_format = SND_PCM_FORMAT_S16_LE;
    snd_pcm_hw_params_set_format(handle, params, pcm_format);

    // assuming stereo
    unsigned int pcm_channels = 2;
    snd_pcm_hw_params_set_channels(handle, params, pcm_channels);

    // trying our rate
    unsigned int pcm_rate = 44100;
    snd_pcm_hw_params_set_rate_near(handle, params, &pcm_rate, NULL);

    // number of frames per read
    snd_pcm_uframes_t pcm_frames = 256;
    snd_pcm_hw_params_set_period_size_near(handle, params, &pcm_frames, NULL);

    err = snd_pcm_hw_params(handle, params); // attempting to set params
    if (err < 0) {
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    err = snd_pcm_prepare(handle);
    if (err < 0) {
        fprintf(stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    // getting actual format
    snd_pcm_hw_params_get_format(params, &pcm_format);

    // converting result to number of bits
    switch (pcm_format) {
    case SND_PCM_FORMAT_S8:
    case SND_PCM_FORMAT_U8:
        format = 8;
        break;
    case SND_PCM_FORMAT_S16_LE:
    case SND_PCM_FORMAT_S16_BE:
    case SND_PCM_FORMAT_U16_LE:
    case SND_PCM_FORMAT_U16_BE:
        format = 16;
        break;
    case SND_PCM_FORMAT_S24_LE:
    case SND_PCM_FORMAT_S24_BE:
    case SND_PCM_FORMAT_U24_LE:
    case SND_PCM_FORMAT_U24_BE:
        format = 24;
        break;
    case SND_PCM_FORMAT_S32_LE:
    case SND_PCM_FORMAT_S32_BE:
    case SND_PCM_FORMAT_U32_LE:
    case SND_PCM_FORMAT_U32_BE:
        format = 32;
        break;
    default:
        break;
    }

    snd_pcm_hw_params_get_rate(params, &rate, NULL);
    snd_pcm_hw_params_get_period_size(params, &frames, NULL);
    snd_pcm_hw_params_get_channels(params, &channels);

    if (format == -1 || rate == 0) {
        fprintf(stderr, "Could not get rate and/or format, quiting...\n");
        exit(EXIT_FAILURE);
    }
}

audio_data::~audio_data() { snd_pcm_close(handle); }
