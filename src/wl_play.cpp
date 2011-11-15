// WL_PLAY.C

#include "wl_act.h"
#include "wl_def.h"
#include "wl_menu.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"

#include "wl_cloudsky.h"
#include "wl_shade.h"
#include "language.h"
#include "lumpremap.h"
#include "thinker.h"
#include "actor.h"
#include "textures/textures.h"
#include "wl_agent.h"

/*
=============================================================================

												LOCAL CONSTANTS

=============================================================================
*/

#define sc_Question     0x35

/*
=============================================================================

												GLOBAL VARIABLES

=============================================================================
*/

boolean madenoise;              // true when shooting or screaming

exit_t playstate;

static int DebugOk;

objtype *newobj, *obj, *lastobj, *objfreelist, *killerobj;

boolean noclip, ammocheat;
int godmode, singlestep;
unsigned int extravbls = 0; // to remove flicker (gray stuff at the bottom)

//
// replacing refresh manager
//
bool noadaptive = false;
unsigned tics;

//
// control info
//
ControlScheme controlScheme[] =
{
	{ bt_moveforward,		"Forward",		-1,	sc_UpArrow,		-1 },
	{ bt_movebackward,		"Backward",		-1,	sc_DownArrow,	-1 },
	{ bt_strafeleft,		"Strafe Left",	-1,	sc_Comma,		-1 },
	{ bt_straferight,		"Strafe Right",	-1,	sc_Peroid,		-1 },
	{ bt_turnleft,			"Turn Left",	-1,	sc_LeftArrow,	-1 },
	{ bt_turnright,			"Turn Right",	-1,	sc_RightArrow,	-1 },
	{ bt_attack,			"Attack",		-1,	sc_Control,		-1 },
	{ bt_strafe,			"Strafe",		-1,	sc_Alt,			-1 },
	{ bt_run,				"Run",			-1,	sc_LShift,		-1 },
	{ bt_use,				"Use",			-1,	sc_Space,		-1 },
	{ bt_readyknife,		"Slot 1",		-1,	sc_1,			-1 },
	{ bt_readypistol,		"Slot 2", 		-1,	sc_2,			-1 },
	{ bt_readymachinegun,	"Slot 3",		-1,	sc_3,			-1 },
	{ bt_readychaingun,		"Slot 4",		-1,	sc_4,			-1 },

	// End of List
	{ bt_nobutton,			NULL,			-1,	-1,				-1 }
};

void ControlScheme::setKeyboard(ControlScheme* scheme, Button button, int value)
{
	for(int i = 0;scheme[i].button != bt_nobutton;i++)
	{
		if(scheme[i].keyboard == value)
			scheme[i].keyboard = -1;
		if(scheme[i].button == button)
			scheme[i].keyboard = value;
	}
}

void ControlScheme::setJoystick(ControlScheme* scheme, Button button, int value)
{
	for(int i = 0;scheme[i].button != bt_nobutton;i++)
	{
		if(scheme[i].joystick == value)
			scheme[i].joystick = -1;
		if(scheme[i].button == button)
			scheme[i].joystick = value;
	}
}

void ControlScheme::setMouse(ControlScheme* scheme, Button button, int value)
{
	for(int i = 0;scheme[i].button != bt_nobutton;i++)
	{
		if(scheme[i].mouse == value)
			scheme[i].mouse = -1;
		if(scheme[i].button == button)
			scheme[i].mouse = value;
	}
}

boolean alwaysrun;
boolean mouseenabled, mouseyaxisdisabled, joystickenabled;
int dirscan[4] = { sc_UpArrow, sc_RightArrow, sc_DownArrow, sc_LeftArrow };
int buttonscan[NUMBUTTONS] = { sc_Control, sc_Alt, sc_LShift, sc_Space, sc_1, sc_2, sc_3, sc_4 };
int buttonmouse[4] = { bt_attack, bt_strafe, bt_use, bt_nobutton };
int buttonjoy[32] = {
#ifdef _arch_dreamcast
	bt_attack, bt_strafe, bt_use, bt_run, bt_esc, bt_prevweapon, bt_nobutton, bt_nextweapon,
	bt_pause, bt_strafeleft, bt_straferight, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton,
#else
	bt_attack, bt_strafe, bt_use, bt_run, bt_strafeleft, bt_straferight, bt_esc, bt_pause,
	bt_prevweapon, bt_nextweapon, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton,
#endif
	bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton,
	bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton, bt_nobutton
};

int viewsize;

boolean buttonheld[NUMBUTTONS];

boolean demorecord, demoplayback;
int8_t *demoptr, *lastdemoptr;
memptr demobuffer;

//
// current user input
//
int controlx, controly;         // range from -100 to 100 per tic
boolean buttonstate[NUMBUTTONS];

int lastgamemusicoffset = 0;


//===========================================================================


void CenterWindow (word w, word h);
void InitObjList (void);
void PollControls (void);
int StopMusic (void);
void StartMusic (void);
void ContinueMusic (int offs);
void PlayLoop (void);

/*
=============================================================================

												LOCAL VARIABLES

=============================================================================
*/

//
// LIST OF SONGS FOR EACH VERSION
//
const char* songs[] = {
#ifndef SPEAR
	//
	// Episode One
	//
	"GETTHEM",	"SEARCHN",	"POW",		"SUSPENSE",	"GETTHEM",
	"SEARCHN",	"POW",		"SUSPENSE",	"WARMARCH",	"CORNER",

	//
	// Episode Two
	//
	"NAZI_OMI",	"PREGNANT",	"GOINGAFT",	"HEADACHE",	"NAZI_OMI",
	"PREGNANT",	"HEADACHE",	"GOINGAFT",	"WARMARCH",	"DUNGEON",

	//
	// Episode Three
	//
	"INTROCW3",	"NAZI_RAP",	"TWELFTH",	"ZEROHOUR",	"INTROCW3",
	"NAZI_RAP",	"TWELFTH",	"ZEROHOUR",	"ULTIMATE",	"PACMAN",

	//
	// Episode Four
	//
	"GETTHEM",	"SEARCHN",	"POW",		"SUSPENSE",	"GETTHEM",
	"SEARCHN",	"POW",		"SUSPENSE",	"WARMARCH",	"CORNER",

	//
	// Episode Five
	//
	"NAZI_OMI",	"PREGNANT",	"GOINGAFT",	"HEADACHE",	"NAZI_OMI",
	"PREGNANT",	"HEADACHE",	"GOINGAFT",	"WARMARCH",	"DUNGEON",

	//
	// Episode Six
	//
	"INTROCW3",	"NAZI_RAP",	"TWELFTH",	"ZEROHOUR",	"INTROCW3",
	"NAZI_RAP",	"TWELFTH",	"ZEROHOUR",	"ULTIMATE",	"FUNKYOU"
#else

	//////////////////////////////////////////////////////////////
	//
	// SPEAR OF DESTINY TRACKS
	//
	//////////////////////////////////////////////////////////////
	"XTIPTOE",	"XFUNKIE",	"XDEATH",	"XGETYOU",	"ULTIMATE",	// Trans Gr�sse

	"DUNGEON",	"GOINGAFT",	"POW",		"TWELFTH",	"ULTIMATE",	// Barnacle Wilhelm BOSS

	"NAZI_OMI",	"GETTHEM",	"SUSPENSE",	"SEARCHN",	"ZEROHOUR",	"ULTIMATE",	// Super Mutant BOSS

	"XPUTIT",	"ULTIMATE",	// Death Knight BOSS

	"XJAZNAZI",	"XFUNKIE",	// Secret level

	"XEVIL"	// Angel of Death BOSS
#endif
};


/*
=============================================================================

							USER CONTROL

=============================================================================
*/

/*
===================
=
= PollKeyboardButtons
=
===================
*/

void PollKeyboardButtons (void)
{
	for(int i = 0;controlScheme[i].button != bt_nobutton;i++)
	{
		if(controlScheme[i].keyboard != -1 && Keyboard[controlScheme[i].keyboard])
			buttonstate[controlScheme[i].button] = true;
	}
}


/*
===================
=
= PollMouseButtons
=
===================
*/

void PollMouseButtons (void)
{
	int buttons = IN_MouseButtons();
	for(int i = 0;controlScheme[i].button != bt_nobutton;i++)
	{
		if(controlScheme[i].mouse != -1 && (buttons & (1<<controlScheme[i].mouse)))
			buttonstate[controlScheme[i].button] = true;
	}
}



/*
===================
=
= PollJoystickButtons
=
===================
*/

void PollJoystickButtons (void)
{
	int buttons = IN_JoyButtons();
	for(int i = 0;controlScheme[i].button != bt_nobutton;i++)
	{
		if(controlScheme[i].joystick != -1 && (buttons & (1<<controlScheme[i].joystick)))
			buttonstate[controlScheme[i].button] = true;
	}
}


/*
===================
=
= PollKeyboardMove
=
===================
*/

void PollKeyboardMove (void)
{
	int delta = (!alwaysrun && buttonstate[bt_run]) || (alwaysrun && !buttonstate[bt_run]) ? RUNMOVE * tics : BASEMOVE * tics;

	if(buttonstate[bt_moveforward])
		controly -= delta;
	if(buttonstate[bt_movebackward])
		controly += delta;
	if(buttonstate[bt_turnleft])
		controlx -= delta;
	if(buttonstate[bt_turnright])
		controlx += delta;
}


/*
===================
=
= PollMouseMove
=
===================
*/

void PollMouseMove (void)
{
	int mousexmove, mouseymove;

	SDL_GetMouseState(&mousexmove, &mouseymove);
	if(IN_IsInputGrabbed())
		IN_CenterMouse();

	mousexmove -= screenWidth / 2;
	mouseymove -= screenHeight / 2;

	controlx += mousexmove * 20 / (21 - mouseadjustment);
	if(!mouseyaxisdisabled)
		controly += mouseymove * 40 / (21 - mouseadjustment);
}


/*
===================
=
= PollJoystickMove
=
===================
*/

void PollJoystickMove (void)
{
	int joyx, joyy;

	IN_GetJoyDelta (&joyx, &joyy);

	int delta = buttonstate[bt_run] ? RUNMOVE * tics : BASEMOVE * tics;

	if (joyx > 64 || buttonstate[bt_turnright])
		controlx += delta;
	else if (joyx < -64  || buttonstate[bt_turnleft])
		controlx -= delta;
	if (joyy > 64 || buttonstate[bt_movebackward])
		controly += delta;
	else if (joyy < -64 || buttonstate[bt_moveforward])
		controly -= delta;
}

/*
===================
=
= PollControls
=
= Gets user or demo input, call once each frame
=
= controlx              set between -100 and 100 per tic
= controly
= buttonheld[]  the state of the buttons LAST frame
= buttonstate[] the state of the buttons THIS frame
=
===================
*/

void PollControls (void)
{
	int max, min, i;
	byte buttonbits;

	IN_ProcessEvents();

//
// get timing info for last frame
//
	if (demoplayback || demorecord)   // demo recording and playback needs to be constant
	{
		// wait up to DEMOTICS Wolf tics
		uint32_t curtime = SDL_GetTicks();
		lasttimecount += DEMOTICS;
		int32_t timediff = (lasttimecount * 100) / 7 - curtime;
		if(timediff > 0)
			SDL_Delay(timediff);

		if(timediff < -2 * DEMOTICS)       // more than 2-times DEMOTICS behind?
			lasttimecount = (curtime * 7) / 100;    // yes, set to current timecount

		tics = DEMOTICS;
	}
	else
		CalcTics ();

	controlx = 0;
	controly = 0;
	memcpy (buttonheld, buttonstate, sizeof (buttonstate));
	memset (buttonstate, 0, sizeof (buttonstate));

	if (demoplayback)
	{
		//
		// read commands from demo buffer
		//
		buttonbits = *demoptr++;
		for (i = 0; i < NUMBUTTONS; i++)
		{
			buttonstate[i] = buttonbits & 1;
			buttonbits >>= 1;
		}

		controlx = *demoptr++;
		controly = *demoptr++;

		if (demoptr == lastdemoptr)
			playstate = ex_completed;   // demo is done

		controlx *= (int) tics;
		controly *= (int) tics;

		return;
	}


//
// get button states
//
	PollKeyboardButtons ();

	if (mouseenabled && IN_IsInputGrabbed())
		PollMouseButtons ();

	if (joystickenabled)
		PollJoystickButtons ();

//
// get movements
//
	PollKeyboardMove ();

	if (mouseenabled && IN_IsInputGrabbed())
		PollMouseMove ();

	if (joystickenabled)
		PollJoystickMove ();

//
// bound movement to a maximum
//
	max = 100 * tics;
	min = -max;
	//if (controlx > max)
	//    controlx = max;
	//else if (controlx < min)
	//    controlx = min;

	//if (controly > max)
	//    controly = max;
	//else if (controly < min)
	//    controly = min;

	if (demorecord)
	{
		//
		// save info out to demo buffer
		//
		controlx /= (int) tics;
		controly /= (int) tics;

		buttonbits = 0;

		// TODO: Support 32-bit buttonbits
		for (i = NUMBUTTONS - 1; i >= 0; i--)
		{
			buttonbits <<= 1;
			if (buttonstate[i])
				buttonbits |= 1;
		}

		*demoptr++ = buttonbits;
		*demoptr++ = controlx;
		*demoptr++ = controly;

		if (demoptr >= lastdemoptr - 8)
			playstate = ex_completed;
		else
		{
			controlx *= (int) tics;
			controly *= (int) tics;
		}
	}
}



//==========================================================================



///////////////////////////////////////////////////////////////////////////
//
//      CenterWindow() - Generates a window of a given width & height in the
//              middle of the screen
//
///////////////////////////////////////////////////////////////////////////
#define MAXX    320
#define MAXY    160

void CenterWindow (word w, word h)
{
	US_DrawWindow (((MAXX / 8) - w) / 2, ((MAXY / 8) - h) / 2, w, h);
}

//===========================================================================


/*
=====================
=
= CheckKeys
=
=====================
*/

bool changeSize = true;
void CheckKeys (void)
{
	ScanCode scan;


	if (screenfaded || demoplayback)    // don't do anything with a faded screen
		return;

	scan = LastScan;

	// [BL] Allow changing the screen size with the -/= keys a la Doom.
	if(changeSize)
	{
		if(Keyboard[sc_Equals] && !Keyboard[sc_Minus])
			NewViewSize(viewsize+1);
		else if(!Keyboard[sc_Equals] && Keyboard[sc_Minus])
			NewViewSize(viewsize-1);
		if(Keyboard[sc_Equals] || Keyboard[sc_Minus])
		{
			SD_PlaySound("world/hitwall");
			DrawPlayScreen();
			changeSize = false;
		}
	}
	else if(!Keyboard[sc_Equals] && !Keyboard[sc_Minus])
		changeSize = true;

#ifdef SPEAR
	//
	// SECRET CHEAT CODE: TAB-G-F10
	//
	if (Keyboard[sc_Tab] && Keyboard[sc_G] && Keyboard[sc_F10])
	{
		WindowH = 160;
		if (godmode)
		{
			Message ("God mode OFF");
			SD_PlaySound ("misc/no_bonus");
		}
		else
		{
			Message ("God mode ON");
			SD_PlaySound ("misc/end_bonus2");
		}

		IN_Ack ();
		godmode ^= 1;
		DrawPlayBorderSides ();
		IN_ClearKeysDown ();
		return;
	}
#endif


	//
	// SECRET CHEAT CODE: 'MLI'
	//
	if (Keyboard[sc_M] && Keyboard[sc_L] && Keyboard[sc_I])
	{
		players[0].health = 100;
		gamestate.ammo = 99;
		gamestate.keys = 3;
		players[0].score = 0;
		gamestate.TimeCount += 42000L;
		GiveWeapon (wp_chaingun);
		DrawWeapon ();
		DrawHealth ();
		DrawKeys ();
		DrawAmmo ();
		DrawScore ();

		ClearMemory ();
		ClearSplitVWB ();

		Message (language["STR_CHEATER"]);

		IN_ClearKeysDown ();
		IN_Ack ();

		if (viewsize < 17)
			DrawPlayBorder ();
	}

	//
	// OPEN UP DEBUG KEYS
	//
	if (Keyboard[sc_BackSpace] && Keyboard[sc_LShift] && Keyboard[sc_Alt])
	{
		ClearMemory ();
		ClearSplitVWB ();

		Message ("Debugging keys are\nnow available!");
		IN_ClearKeysDown ();
		IN_Ack ();

		DrawPlayBorderSides ();
		DebugOk = 1;
	}

	//
	// TRYING THE KEEN CHEAT CODE!
	//
	if (Keyboard[sc_B] && Keyboard[sc_A] && Keyboard[sc_T])
	{
		ClearMemory ();
		ClearSplitVWB ();

		Message ("Commander Keen is also\n"
				"available from Apogee, but\n"
				"then, you already know\n" "that - right, Cheatmeister?!");

		IN_ClearKeysDown ();
		IN_Ack ();

		if (viewsize < 18)
			DrawPlayBorder ();
	}

//
// pause key weirdness can't be checked as a scan code
//
	if(buttonstate[bt_pause]) Paused = true;
	if(Paused)
	{
		int lastoffs = StopMusic();
//        LatchDrawPic (20 - 4, 80 - 2 * 8, PAUSEDPIC);
		VWB_DrawPic((20 - 4)*8, 80 - 2*8, "PAUSED");
		VH_UpdateScreen();
		IN_Ack ();
		Paused = false;
		ContinueMusic(lastoffs);
		if (MousePresent && IN_IsInputGrabbed())
			IN_CenterMouse();     // Clear accumulated mouse movement
		lasttimecount = GetTimeCount();
		return;
	}

//
// F1-F7/ESC to enter control panel
//
	if (
#ifndef DEBCHECK
		scan == sc_F10 ||
#endif
		scan == sc_F9 || scan == sc_F7 || scan == sc_F8)     // pop up quit dialog
	{
		short oldmapon = gamestate.mapon;
		short oldepisode = gamestate.episode;
		ClearMemory ();
		ClearSplitVWB ();
		US_ControlPanel (scan);

		DrawPlayBorderSides ();

		SETFONTCOLOR (0, 15);
		IN_ClearKeysDown ();
		return;
	}

	if ((scan >= sc_F1 && scan <= sc_F9) || scan == sc_Escape || buttonstate[bt_esc])
	{
		int lastoffs = StopMusic ();
		ClearMemory ();
		VW_FadeOut ();

		US_ControlPanel (buttonstate[bt_esc] ? sc_Escape : scan);

		SETFONTCOLOR (0, 15);
		IN_ClearKeysDown ();
		VW_FadeOut();
		if(viewsize != 21)
			DrawPlayScreen ();
		if (!startgame && !loadedgame)
			ContinueMusic (lastoffs);
		if (loadedgame)
			playstate = ex_abort;
		lasttimecount = GetTimeCount();
		if (MousePresent && IN_IsInputGrabbed())
			IN_CenterMouse();     // Clear accumulated mouse movement
		return;
	}

//
// TAB-? debug keys
//
	if (Keyboard[sc_Tab] && DebugOk)
	{
		fontnumber = 0;
		SETFONTCOLOR (0, 15);
		if (DebugKeys () && viewsize < 20)
			DrawPlayBorder ();       // dont let the blue borders flash

		if (MousePresent && IN_IsInputGrabbed())
			IN_CenterMouse();     // Clear accumulated mouse movement

		lasttimecount = GetTimeCount();
		return;
	}
}


//===========================================================================

/*
=============================================================================

												MUSIC STUFF

=============================================================================
*/


/*
=================
=
= StopMusic
=
=================
*/
int StopMusic (void)
{
	int lastoffs = SD_MusicOff ();

	return lastoffs;
}

//==========================================================================


/*
=================
=
= StartMusic
=
=================
*/

void StartMusic ()
{
	SD_MusicOff ();
	SD_StartMusic(songs[gamestate.mapon + gamestate.episode * 10]);
}

void ContinueMusic (int offs)
{
	SD_MusicOff ();
	SD_ContinueMusic(songs[gamestate.mapon + gamestate.episode * 10], offs);
}

/*
=============================================================================

										PALETTE SHIFTING STUFF

=============================================================================
*/

#define NUMREDSHIFTS    6
#define REDSTEPS        8

#define NUMWHITESHIFTS  3
#define WHITESTEPS      20
#define WHITETICS       6


SDL_Color redshifts[NUMREDSHIFTS][256];
SDL_Color whiteshifts[NUMWHITESHIFTS][256];

int damagecount, bonuscount;
boolean palshifted;

/*
=====================
=
= InitRedShifts
=
=====================
*/

void InitRedShifts (void)
{
	SDL_Color *workptr, *baseptr;
	int i, j, delta;


//
// fade through intermediate frames
//
	for (i = 1; i <= NUMREDSHIFTS; i++)
	{
		workptr = redshifts[i - 1];
		baseptr = gamepal;

		for (j = 0; j <= 255; j++)
		{
			delta = 256 - baseptr->r;
			workptr->r = baseptr->r + delta * i / REDSTEPS;
			delta = -baseptr->g;
			workptr->g = baseptr->g + delta * i / REDSTEPS;
			delta = -baseptr->b;
			workptr->b = baseptr->b + delta * i / REDSTEPS;
			baseptr++;
			workptr++;
		}
	}

	for (i = 1; i <= NUMWHITESHIFTS; i++)
	{
		workptr = whiteshifts[i - 1];
		baseptr = gamepal;

		for (j = 0; j <= 255; j++)
		{
			delta = 256 - baseptr->r;
			workptr->r = baseptr->r + delta * i / WHITESTEPS;
			delta = 248 - baseptr->g;
			workptr->g = baseptr->g + delta * i / WHITESTEPS;
			delta = 0-baseptr->b;
			workptr->b = baseptr->b + delta * i / WHITESTEPS;
			baseptr++;
			workptr++;
		}
	}
}


/*
=====================
=
= ClearPaletteShifts
=
=====================
*/

void ClearPaletteShifts (void)
{
	bonuscount = damagecount = 0;
	palshifted = false;
}


/*
=====================
=
= StartBonusFlash
=
=====================
*/

void StartBonusFlash (void)
{
	bonuscount = NUMWHITESHIFTS * WHITETICS;    // white shift palette
}


/*
=====================
=
= StartDamageFlash
=
=====================
*/

void StartDamageFlash (int damage)
{
	damagecount += damage;
}


/*
=====================
=
= UpdatePaletteShifts
=
=====================
*/

void UpdatePaletteShifts (void)
{
	int red, white;

	if (bonuscount)
	{
		white = bonuscount / WHITETICS + 1;
		if (white > NUMWHITESHIFTS)
			white = NUMWHITESHIFTS;
		bonuscount -= tics;
		if (bonuscount < 0)
			bonuscount = 0;
	}
	else
		white = 0;


	if (damagecount)
	{
		red = damagecount / 10 + 1;
		if (red > NUMREDSHIFTS)
			red = NUMREDSHIFTS;

		damagecount -= tics;
		if (damagecount < 0)
			damagecount = 0;
	}
	else
		red = 0;

	if (red)
	{
		VL_SetBlend(0xFF, 0x00, 0x00, red*(174/NUMREDSHIFTS));
		palshifted = true;
	}
	else if (white)
	{
		// [BL] More of a yellow if you ask me.
		VL_SetBlend(0xFF, 0xF8, 0x00, white*(38/NUMWHITESHIFTS));
		palshifted = true;
	}
	else if (palshifted)
	{
		VL_SetBlend(0, 0, 0, 0);
		palshifted = false;
	}
}


/*
=====================
=
= FinishPaletteShifts
=
= Resets palette to normal if needed
=
=====================
*/

void FinishPaletteShifts (void)
{
	if (palshifted)
	{
		palshifted = false;
		VL_SetBlend(0, 0, 0, 0);
	}
}


/*
=============================================================================

												CORE PLAYLOOP

=============================================================================
*/


/*
=====================
=
= DoActor
=
=====================
*/
#if 0
void DoActor (objtype * ob)
{
	void (*think) (objtype *);

	if (!ob->active && !map->CheckLink(ob->GetZone(), player->GetZone(), true))
		return;

	if (!(ob->flags & (FL_NONMARK | FL_NEVERMARK)))
		actorat[ob->tilex][ob->tiley] = NULL;

//
// non transitional object
//

	if (!ob->ticcount)
	{
		think = (void (*)(objtype *)) ob->state->think;
		if (think)
		{
			think (ob);
			if (!ob->state)
			{
				RemoveObj (ob);
				return;
			}
		}

		if (ob->flags & FL_NEVERMARK)
			return;

		if ((ob->flags & FL_NONMARK) && actorat[ob->tilex][ob->tiley])
			return;

		actorat[ob->tilex][ob->tiley] = ob;
		return;
	}

//
// transitional object
//
	ob->ticcount -= (short) tics;
	while (ob->ticcount <= 0)
	{
		think = (void (*)(objtype *)) ob->state->action;        // end of state action
		if (think)
		{
			think (ob);
			if (!ob->state)
			{
				RemoveObj (ob);
				return;
			}
		}

		ob->state = ob->state->next;

		if (!ob->state)
		{
			RemoveObj (ob);
			return;
		}

		if (!ob->state->tictime)
		{
			ob->ticcount = 0;
			goto think;
		}

		ob->ticcount += ob->state->tictime;
	}

think:
	//
	// think
	//
	think = (void (*)(objtype *)) ob->state->think;
	if (think)
	{
		think (ob);
		if (!ob->state)
		{
			RemoveObj (ob);
			return;
		}
	}

	if (ob->flags & FL_NEVERMARK)
		return;

	if ((ob->flags & FL_NONMARK) && actorat[ob->tilex][ob->tiley])
		return;

	actorat[ob->tilex][ob->tiley] = ob;
}
#endif

//==========================================================================


/*
===================
=
= PlayLoop
=
===================
*/
int32_t funnyticount;


void PlayLoop (void)
{
#if defined(USE_FEATUREFLAGS) && defined(USE_CLOUDSKY)
	if(GetFeatureFlags() & FF_CLOUDSKY)
		InitSky();
#endif

	playstate = ex_stillplaying;
	lasttimecount = GetTimeCount();
	frameon = 0;
	anglefrac = 0;
	facecount = 0;
	funnyticount = 0;
	memset (buttonstate, 0, sizeof (buttonstate));
	ClearPaletteShifts ();

	if (MousePresent && IN_IsInputGrabbed())
		IN_CenterMouse();         // Clear accumulated mouse movement

	if (demoplayback)
		IN_StartAck ();

	do
	{
		PollControls ();

//
// actor thinking
//
		madenoise = false;

		for (unsigned int i = 0;i < tics;++i)
			thinkerList->Tick();

		UpdatePaletteShifts ();

		ThreeDRefresh ();

		//
		// MAKE FUNNY FACE IF BJ DOESN'T MOVE FOR AWHILE
		//
#ifdef SPEAR
		funnyticount += tics;
		if (funnyticount > 30l * 70)
		{
			funnyticount = 0;
			if(viewsize != 21)
				StatusDrawFace(BJWAITING1PIC + (US_RndT () & 1));
			facecount = 0;
		}
#endif

		gamestate.TimeCount += tics;
		TexMan.UpdateAnimations(gamestate.TimeCount*14);

		UpdateSoundLoc ();      // JAB
		if (screenfaded)
			VW_FadeIn ();

		CheckKeys ();

//
// debug aids
//
		if (singlestep)
		{
			VW_WaitVBL (singlestep);
			lasttimecount = GetTimeCount();
		}
		if (extravbls)
			VW_WaitVBL (extravbls);

		if (demoplayback)
		{
			if (IN_CheckAck ())
			{
				IN_ClearKeysDown ();
				playstate = ex_abort;
			}
		}
	}
	while (!playstate && !startgame);

	if (playstate != ex_died)
		FinishPaletteShifts ();
}
