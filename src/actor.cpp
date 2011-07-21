/*
** actor.cpp
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

#include "actor.h"
#include "gamemap.h"
#include "id_ca.h"
#include "thinker.h"
#include "thingdef/thingdef.h"

Frame::~Frame()
{
	if(freeActionArgs)
	{
		// We need to delete CallArguments objects. Since we don't default to
		// NULL on these we need to check if a pointer has been set to know if
		// we created an object.
		if(action.pointer)
			delete action.args;
		if(thinker.pointer)
			delete thinker.args;
	}
}

void Frame::ActionCall::operator() (AActor *self) const
{
	if(pointer)
	{
		args->Evaluate(self);
		pointer(self, *args);
	}
}

////////////////////////////////////////////////////////////////////////////////

// We can't make AActor a thinker since we create non-thinking objects.
class AActorProxy : public Thinker
{
	DECLARE_THINKER(AActorProxy)
	public:
		AActorProxy(AActor *parent) : parent(parent)
		{
		}

		~AActorProxy()
		{
			delete parent;
		}

		void Tick()
		{
			parent->Tick();
		}
	private:
		AActor * const parent;
};
IMPLEMENT_THINKER(AActorProxy)

LinkedList<AActor *> AActor::actors;
const ClassDef *AActor::__StaticClass = ClassDef::DeclareNativeClass<AActor>("Actor", NULL);

AActor::AActor(const ClassDef *type) : classType(type), flags(0), distance(0),
	dir(nodir), soundZone(NULL), SpawnState(NULL)
{
}

AActor::~AActor()
{
	if(actorRef)
		actors.Remove(actorRef);
}

void AActor::Destroy()
{
	assert(thinker != NULL);
	thinker->Destroy();
}

void AActor::Die()
{
	GivePoints(points);
	if(flags & FL_COUNTKILL)
		gamestate.killcount++;
	flags &= ~FL_SHOOTABLE;

	if(DeathState)
		SetState(DeathState);
	else
		Destroy();
}

void AActor::EnterZone(const MapZone *zone)
{
	if(zone)
		soundZone = zone;
}

const Frame *AActor::FindState(const FName &name) const
{
	return classType->FindState(name);
}

void AActor::Init(bool nothink)
{
	if(!nothink)
	{
		actorRef = actors.Push(this);
		thinker = new AActorProxy(this);

		if(SpawnState)
			SetState(SpawnState, true);
		else
		{
			state = NULL;
			Destroy();
		}
	}
	else
	{
		actorRef = NULL;
		thinker = NULL;
		state = NULL;
	}
}

void AActor::SetState(const Frame *state, bool notic)
{
	if(state == NULL)
		return;

	this->state = state;
	sprite = state->spriteInf;
	ticcount = state->duration;
	state->action(this);
}

void AActor::Tick()
{
	if(state == NULL)
	{
		Destroy();
		return;
	}

	if(ticcount > 0)
		--ticcount;

	while(ticcount == 0)
	{
		if(!state->next)
		{
			Destroy();
			return;
		}
		SetState(state->next);
	}

	state->thinker(this);
}

AActor *AActor::Spawn(const ClassDef *type, fixed x, fixed y, fixed z)
{
	if(type == NULL)
	{
		printf("Tried to spawn classless actor.\n");
		return NULL;
	}

	AActor *actor = type->CreateInstance();
	actor->x = x;
	actor->y = y;
	actor->tilex = x>>FRACBITS;
	actor->tiley = y>>FRACBITS;

	MapSpot spot = map->GetSpot(actor->tilex, actor->tiley, 0);
	actor->EnterZone(spot->zone);

	if(actor->flags & FL_COUNTKILL)
		++gamestate.killtotal;
	return actor;
}

class AWeapon : public AActor
{
	DECLARE_NATIVE_CLASS(Weapon, Actor)
};
IMPLEMENT_CLASS(Weapon, Actor)
