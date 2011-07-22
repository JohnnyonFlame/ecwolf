/*
** actor.h
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

#ifndef __ACTOR_H__
#define __ACTOR_H__

#include "wl_def.h"
#include "gamemap.h"
#include "linkedlist.h"
#include "name.h"

#define DECLARE_NATIVE_CLASS(name, parent) \
	friend class ClassDef; \
	protected: \
		A##name(const ClassDef *classType) : A##parent(classType) {} \
		virtual AActor *__NewNativeInstance(const ClassDef *classType) { return new A##name(classType); } \
	public: \
		static const ClassDef *__StaticClass;
#define IMPLEMENT_CLASS(name, parent) \
	const ClassDef *A##name::__StaticClass = ClassDef::DeclareNativeClass<A##name>(#name, A##parent::__StaticClass);
#define NATIVE_CLASS(name) A##name::__StaticClass

class CallArguments;
class ExpressionNode;

typedef uint32_t flagstype_t;
typedef void (*ActionPtr)(AActor *, const CallArguments &args);

class Frame
{
	public:
		~Frame();

		union
		{
			char	sprite[4];
			uint32_t isprite;
		};
		uint8_t		frame;
		int			duration;
		class ActionCall
		{
			public:
				ActionPtr		pointer;
				CallArguments	*args;

				void operator() (AActor *self) const;
		} action, thinker;
		const Frame	*next;

		unsigned int	spriteInf;

		bool	freeActionArgs;
};

class AActorProxy;
class ClassDef;
class AActor
{
	public:
		~AActor();

		void			Destroy();
		void			Die();
		void			EnterZone(const MapZone *zone);
		const Frame		*FindState(const FName &name) const;
		const MapZone	*GetZone() const { return soundZone; }
		void			SetState(const Frame *state, bool notic=false);
		static AActor	*Spawn(const ClassDef *type, fixed x, fixed y, fixed z);
		void			Tick();

		const AActor	*defaults;

		// Basic properties from objtype
		flagstype_t flags;

		int32_t	distance; // if negative, wait for that door to open
		dirtype	dir;

		fixed	x, y;
		word	tilex, tiley;

		short	angle;
		int32_t	health;
		short	defaultHealth[9];
		int32_t	speed, runspeed;
		int		points;
		fixed	radius;

		activetype  active;
		short       ticcount;
		classtype   obclass;
		const Frame	*state;
		unsigned int sprite;

		short       viewx;
		word        viewheight;
		fixed       transx,transy;      // in global coord

		uint16_t	sighttime;
		uint8_t		sightrandom;
		FName		attacksound, deathsound, seesound;

		const Frame *SpawnState, *SeeState, *PathState, *PainState, *MeleeState, *MissileState, *DeathState;
		short       temp1,hidden;

		typedef LinkedList<AActor *>::Node Iterator;
		static LinkedList<AActor *>	actors;
		LinkedList<AActor *>::Node	*actorRef;
		static Iterator *GetIterator() { return actors.Head(); }
	protected:
		friend class AActorProxy;
		void			Init(bool nothink=false);

		const MapZone	*soundZone;
		AActorProxy		*thinker;

		const ClassDef	*classType;

	// ClassDef stuff
	friend class ClassDef;
	public:
		static const ClassDef *__StaticClass;
	protected:
		AActor(const ClassDef *type);
		virtual AActor *__NewNativeInstance(const ClassDef *classType) { return new AActor(classType); }
};

#endif
