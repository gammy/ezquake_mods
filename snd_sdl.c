/* 
Contains parts of snd_sdl.c from sdlquake quakespasm and pieces from 
ezquake's other snd_*.c-files.

Cobbled together by gammy & will.

Com_DPrintf is printed if developer is set to 1 in common.c.

(C) 2005 Contributors of the ZQuake Project
(C) 2010 Contributors of the quakespasm Project
(C) 2010 Contributors of the ezquake Project

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <stdio.h>
#include "SDL/SDL.h"
#include "SDL/SDL_audio.h"
#include "SDL/SDL_byteorder.h"
#include "quakedef.h"
#include "qsound.h"

static int sdl_snd_initialized;

// Callback 
static void paint_audio(void *unused, Uint8 *stream, int len)
{
	if(! shm || ! sdl_snd_initialized)
		return;

	shm->buffer = stream;
	//Com_Printf("len = %d, samples = %d\n", len, shm->samples);
	shm->samplepos += len / 4; //shm->samples / (shm->format.channels * len);
	//shm->samplepos += shm->samples * (shm->format.channels ;

	// Render audio
	S_PaintChannels (shm->samplepos);
}

qbool SNDDMA_Init_SDL(void)
{
	SDL_AudioSpec desired, obtained;

	sdl_snd_initialized = 0;

	char *audio_driver = Cvar_String("s_device");
	int i;

	Com_DPrintf("Selected driver: \"%s\"\n", audio_driver);

	/* We manually call this internal SDL function since we want
	 * control of the audio driver via the quake configuration.
	 * Looking at the SDL 1.2-source, this actually cannot return !0 - 
	 * they allow it to pass through and die on SDL_OpenAudio. */
	if(SDL_AudioInit(audio_driver) < 0) {
		Com_DPrintf("SDL_AudioInit(\"%s\") failure: %s\n",
			   audio_driver,
			   SDL_GetError());
		return(0);
	}

	/* Set up the desired format */

	Com_DPrintf("khz returns %d\n", s_khz.value);
	Com_DPrintf("s_khz returns %d\n", Cvar_Value("s_khz"));

	unsigned int desired_bits = Cvar_Value("s_bits");
	desired.format            = desired_bits == 8 ? AUDIO_U8 : AUDIO_S16SYS;
	desired.channels          = Cvar_Value("s_stereo") == 0 ? 1 : 2;
	desired.freq              = SND_Rate((int) s_khz.value);
	desired.samples           = 0; // Let SDL handle it (see SDL_audio.c)
	desired.callback          = paint_audio;

	Com_DPrintf("Requesting %s driver with %d-bit %s %dHz with %d samples.\n",
		   audio_driver,
		   desired_bits, 
		   desired.channels == 1 ? "mono" : "stereo",
		   desired.freq,
		   desired.samples);
	
	/* Open the audio device */
	if ( SDL_OpenAudio(&desired, &obtained) < 0 ) {
        	Com_Printf("Couldn't open SDL audio: %s\n", SDL_GetError());
		return(0);
	}
	
	/* Try to force obtained.format if we didn't get it */
	if(obtained.format != AUDIO_U8 &&
	   obtained.format != AUDIO_S16SYS) {

		Com_Printf("Obtained format isn't desired - trying to force it.\n");

		SDL_CloseAudio();

		if ( SDL_OpenAudio(&desired, NULL) < 0 ) {
			Com_Printf("Couldn't open SDL audio: %s\n",
				   SDL_GetError());
			return(0);
		}

		memcpy(&obtained, &desired, sizeof(desired));

	}
	
	SDL_PauseAudio(0);
	
	Com_Printf("SDL_Audio [%s] initialized.\n", 
		   audio_driver);
	
	Com_DPrintf("Got fmt 0x%x (%s)%s %dHz with %d samples.\n",
		   obtained.format,
		   obtained.format == desired.format ?  "desired" : "not desired",
		   obtained.channels == 1 ? "mono" : "stereo",
		   obtained.freq,
		   obtained.samples);

	memset((void *) shm, 0, sizeof(shm));

	shm->format.width    = (obtained.format & 0xFF) / 8;
	shm->format.speed    = obtained.freq;
	shm->format.channels = obtained.channels;
	shm->samples         = obtained.samples * shm->format.channels;
	shm->sampleframes    = obtained.samples; // / shm->format.channels;
	shm->samplepos       = 0;

	sdl_snd_initialized = 1;

	return(1);
}

int SNDDMA_GetDMAPos_SDL(void)
{
	return(shm->samplepos);
}

void SNDDMA_Shutdown_SDL(void)
{

	if(! sdl_snd_initialized)
		return;

	Com_DPrintf("SDL_Audio shutdown\n");

	SDL_PauseAudio(1);	
	SDL_CloseAudio();
	SDL_AudioQuit();

	sdl_snd_initialized = 0;
}

void SNDDMA_LockBuffer_SDL (void)
{
	SDL_LockAudio();
}

void SNDDMA_Submit_SDL (void)
{
	SDL_UnlockAudio();
}

void SNDDMA_BlockSound_SDL (void)
{
	SDL_PauseAudio(1);
}

void SNDDMA_UnblockSound_SDL (void)
{
	SDL_PauseAudio(0);
}
