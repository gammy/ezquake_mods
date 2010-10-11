
#include <stdio.h>
#include "SDL/SDL.h"
#include "SDL/SDL_audio.h"
#include "SDL/SDL_byteorder.h"
#include "quakedef.h"
#include "qsound.h"

static dma_t the_shm;
static int snd_inited;

static void paint_audio(void *unused, Uint8 *stream, int len)
{
	if ( shm ) {
		shm->buffer = stream;
		//shm->samplepos += len/(shm->samplebits/8)/2;
		shm->samplepos += len/(shm->samples)/2;
		// Check for samplepos overflow?
		S_PaintChannels (shm->samplepos);
	}
}

qbool SNDDMA_Init_SDL(void)
{
	SDL_AudioSpec desired, obtained;

	snd_inited = 0;

	/* Set up the desired format */
	desired.freq = SND_Rate((int) s_khz.value);
	unsigned int desired_bits = Cvar_Value("s_bits");

	switch (desired_bits) {
		case 8:
			desired.format = AUDIO_U8;
			break;
		case 16:
			if ( SDL_BYTEORDER == SDL_BIG_ENDIAN )
				desired.format = AUDIO_S16MSB;
			else
				desired.format = AUDIO_S16LSB;
			break;
		default:
        		Con_Printf("Unknown number of audio bits: %d\n",
								desired_bits);
			return 0;
	}

	desired.channels = Cvar_Value("s_stereo") == 0 ? 1 : 2;
	desired.samples = 512;
	desired.callback = paint_audio;
	
	/* Open the audio device */
	if ( SDL_OpenAudio(&desired, &obtained) < 0 ) {
        	Con_Printf("A:Couldn't open SDL audio: %s\n", SDL_GetError());
		return 0;
	}

	/* Make sure we can support the audio format */
	switch (obtained.format) {
		case AUDIO_U8:
			/* Supported */
			break;
		case AUDIO_S16LSB:
		case AUDIO_S16MSB:
			if ( ((obtained.format == AUDIO_S16LSB) &&
			     (SDL_BYTEORDER == SDL_LIL_ENDIAN)) ||
			     ((obtained.format == AUDIO_S16MSB) &&
			     (SDL_BYTEORDER == SDL_BIG_ENDIAN)) ) {
				/* Supported */
				break;
			}
			/* Unsupported, fall through */;
		default:
			/* Not supported -- force SDL to do our bidding */
			SDL_CloseAudio();
			if ( SDL_OpenAudio(&desired, NULL) < 0 ) {
        			Con_Printf("B: Couldn't open SDL audio: %s\n",
							SDL_GetError());
				return 0;
			}
			memcpy(&obtained, &desired, sizeof(desired));
			break;
	}
	SDL_PauseAudio(0);

	/* Fill the audio DMA information block */
	shm = &the_shm;
	//shm->splitbuffer = 0;
	//shm->samplebits = (obtained.format & 0xFF);
	shm->format.width    = (obtained.format & 0xFF) / 8;
	shm->format.speed    = obtained.freq;
	shm->format.channels = obtained.channels;
	shm->samples         = obtained.samples*shm->format.channels;
	shm->sampleframes    = shm->samples;
	shm->samplepos       = 0;
	//shm->submission_chunk = 1;
	shm->buffer          = NULL;

	snd_inited = 1;
	return 1;
}

int SNDDMA_GetDMAPos_SDL(void)
{
	return shm->samplepos;
}

void SNDDMA_Shutdown_SDL(void)
{
	if (snd_inited)
	{
		SDL_CloseAudio();
		snd_inited = 0;
	}
}

