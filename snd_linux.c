/*
(C) 2005 Contributors of the ZQuake Project

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

    $Id: snd_linux.c,v 1.13 2007-09-26 13:53:42 tonik Exp $
*/

#include "quakedef.h"

#ifndef __FreeBSD__
// Note: The functions here keep track of if the sound system is inited.
// They perform checks so that the real functions are only called if appropriate.

// Prototypes

qbool SNDDMA_Init_SDL(void);
int SNDDMA_GetDMAPos_SDL(void);
void SNDDMA_Shutdown_SDL(void);
//void SNDDMA_Submit_SDL(void);
#endif

qbool SNDDMA_Init_OSS(void);
int SNDDMA_GetDMAPos_OSS(void);
void SNDDMA_Shutdown_OSS(void);

extern cvar_t s_device;

// Main functions
qbool SNDDMA_Init(void)
{
	int retval = 0;

#ifdef __FreeBSD__
	Com_Printf("\nsound: Initializing OSS...\n");
	retval = SNDDMA_Init_OSS();
#else
	Com_Printf("\nsound: Initializing SDL_Audio...\n");

	retval = SNDDMA_Init_SDL();
	if(retval == 0) {
		Com_Printf("SDL_Audio init failed.\n\n");
	} else {
		Com_Printf("SDL_Audio init ok.\n\n");
	}

#endif
	return retval;
}

int SNDDMA_GetDMAPos(void)
{
#ifndef __FreeBSD__
	return SNDDMA_GetDMAPos_SDL();
#else
	return SNDDMA_GetDMAPos_OSS();
#endif
}

void SNDDMA_Shutdown(void)
{
#ifndef __FreeBSD__
	SNDDMA_Shutdown_SDL();
#else
	SNDDMA_Shutdown_OSS();
#endif
}

//Send sound to device if buffer isn't really the dma buffer
void SNDDMA_Submit(void)
{
#ifndef __FreeBSD__
	// SNDDMA_Submit_SDL();
#endif
	// OSS doesn't use this so no need to call it.
}

