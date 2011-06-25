/*
** thingdef_properties.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "thingdef.h"

#define INT_PARAM(var, no) int64_t var = params[no].i
#define FLOAT_PARAM(var, no) double var = params[no].f;
#define STRING_PARAM(var, no) const char* var = params[no].s;

HANDLE_PROPERTY(health)
{
	INT_PARAM(health, 0);

	defaults->defaultHealth[0] = defaults->health = health;

	int currentSlope = 0;
	unsigned int lastValue = health;
	for(unsigned int i = 1;i < PARAM_COUNT;i++)
	{
		INT_PARAM(skillHealth, i);

		defaults->defaultHealth[i] = skillHealth;
		currentSlope = skillHealth - lastValue;
	}

	for(unsigned int i = PARAM_COUNT;i < 9;i++)
	{
		defaults->defaultHealth[i] = defaults->defaultHealth[i-1] + currentSlope;
	}
}

HANDLE_PROPERTY(seesound)
{
	STRING_PARAM(snd, 0);
	defaults->seesound = snd;
}

HANDLE_PROPERTY(sighttime)
{
	INT_PARAM(time, 0);
	defaults->sighttime = time;

	if(PARAM_COUNT == 2)
	{
		INT_PARAM(rnd, 1);
		defaults->sightrandom = rnd <= 255 ? rnd : 0;
	}
	else
		defaults->sightrandom = 0;
}

HANDLE_PROPERTY(speed)
{
	// Speed = units per 2 tics (1/35 of second)
	FLOAT_PARAM(speed, 0);
	defaults->speed = int32_t(speed*FRACUNIT)/128;

	if(PARAM_COUNT == 2)
	{
		FLOAT_PARAM(runspeed, 1);
		defaults->runspeed = int32_t(runspeed*FRACUNIT)/128;
	}
	else
		defaults->runspeed = defaults->speed;
}

#define DEFINE_PROP(name, class, params) { class::__StaticClass, #name, #params, __Handler_##name }
extern const PropDef properties[NUM_PROPERTIES] =
{
	DEFINE_PROP(health, AActor, I_IIIIIIII),
	DEFINE_PROP(seesound, AActor, S),
	DEFINE_PROP(sighttime, AActor, I_I),
	DEFINE_PROP(speed, AActor, F_F)
};
