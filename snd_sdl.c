/* Contains parts of snd_sdl.c from sdlquake and bits and pieces from ezquake's other
   snd_*.c-files.

   Cobbled together by gammy & will.
*/
#include <stdio.h>
#include "SDL/SDL.h"
#include "SDL/SDL_audio.h"
#include "SDL/SDL_byteorder.h"
#include "quakedef.h"
#include "qsound.h"

static int sdl_snd_initialized;
static int stfu;



// Callback 
static void paint_audio(void *unused, Uint8 *stream, int len)
{
	if(! shm)
		return;
	if(! sdl_snd_initialized)
		return;

	shm->buffer = stream;
	//Con_Printf("len = %d, samples = %d\n", len, shm->samples);
	shm->samplepos += len / 4 ; // (shm->samples / 2);

	// Check for samplepos overflow?
	S_PaintChannels (shm->samplepos);
}

qbool SNDDMA_Init_SDL(void)
{
	SDL_AudioSpec desired, obtained;

	stfu = 0;
	sdl_snd_initialized = 0;

	char *audio_driver = Cvar_String("s_device");
	int i;

	Com_Printf("Selected driver: \"%s\"\n", audio_driver);

	/* We manually call this internal SDL function since we want
	 * control of the audio driver via the quake configuration.
	 * Looking at the SDL source, this actually cannot return !0 - 
	 * they allow it to pass through and die on SDL_OpenAudio. */
	if(SDL_AudioInit(audio_driver) < 0) {
		Com_Printf("SDL_AudioInit(\"%s\") failure: %s\n",
			   audio_driver,
			   SDL_GetError());
		return(0);
	}

	/* Set up the desired format */
	desired.freq              = SND_Rate((int) s_khz.value);
	desired.channels          = Cvar_Value("s_stereo") == 0 ? 1 : 2;
	desired.samples           = 2048;
	desired.callback          = paint_audio;
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
			desired.format = AUDIO_S16SYS;
	}

	Com_Printf("Requesting %s driver with %d-bit %s %dHz with %d samples.\n",
		   audio_driver,
		   desired_bits, 
		   desired.channels == 1 ? "mono" : "stereo",
		   desired.freq,
		   desired.samples);
	
	/* Open the audio device */
	if ( SDL_OpenAudio(&desired, &obtained) < 0 ) {
        	Con_Printf("A:Couldn't open SDL audio: %s\n", SDL_GetError());
		return 0;
	}
	
	Com_Printf("SDL_Audio [%s] initialized.\n", 
		   audio_driver);
	
	Com_Printf("Got fmt 0x%x(%s)%s %dHz with %d samples.\n",
		   obtained.format,
		   obtained.format == desired.format ?  "desired" : "not desired",
		   obtained.channels == 1 ? "mono" : "stereo",
		   obtained.freq,
		   obtained.samples);

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

	memset((void *) shm, 0, sizeof(shm));


	shm->format.width    = (obtained.format & 0xFF) / 8;
	shm->format.speed    = obtained.freq;
	shm->format.channels = obtained.channels;
	shm->samples         = obtained.samples * shm->format.channels;
	//shm->samples         = obtained.samples * 2;
	shm->sampleframes    = obtained.samples / shm->format.channels;
	shm->samplepos       = 0;
//	shm->buffer = NULL;
//	shm->buffer = calloc(shm->sampleframes, sizeof(char));
//	if(shm->buffer == NULL)
//		abort();

	sdl_snd_initialized = 1;

	return 1;
}

int SNDDMA_GetDMAPos_SDL(void)
{
	if(stfu == 1) {
		SDL_audiostatus status = SDL_GetAudioStatus();

		switch(status) {
			case SDL_AUDIO_STOPPED:
				Com_Printf("Audio (pos %d): STOPPED\n", shm->samplepos);
				break;
			case SDL_AUDIO_PLAYING:
				Com_Printf("Audio (pos %d): PLAYING\n", shm->samplepos);
				break;
			case SDL_AUDIO_PAUSED:
				Com_Printf("Audio (pos %d): PAUSED\n", shm->samplepos);
				break;
		}
	}

	stfu = (stfu + 1) % 512;


	return shm->samplepos;
}

void SNDDMA_Shutdown_SDL(void)
{
	if (sdl_snd_initialized)
	{
		SDL_CloseAudio();
		SDL_AudioQuit();
		sdl_snd_initialized = 0;
	}
}

