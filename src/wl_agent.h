#ifndef __WL_AGENT__
#define __WL_AGENT__

#include "wl_def.h"
#include "a_playerpawn.h"

/*
=============================================================================

							WL_AGENT DEFINITIONS

=============================================================================
*/

extern  short    anglefrac;
extern  int      facecount, facetimes;
extern  word     plux,pluy;         // player coordinates scaled to unsigned
extern  int32_t  thrustspeed;
extern  objtype  *LastAttacker;

void    Cmd_Use ();
void    Thrust (int angle, int32_t speed);
void    SpawnPlayer (int tilex, int tiley, int dir);
void    TakeDamage (int points,objtype *attacker);
void    GivePoints (int32_t points);
void    GetBonus (statobj_t *check);
void    GiveWeapon (int weapon);
void    GiveAmmo (int ammo);
void    GiveKey (int key);
void    VictorySpin ();

//
// player state info
//

void    StatusDrawFace(unsigned picnum);
void    DrawFace (void);
void    DrawHealth (void);
void    DrawLevel (void);
void    DrawLives (void);
void    GiveExtraMan (void);
void    DrawScore (void);
void    DrawWeapon (void);
void    DrawKeys (void);
void    DrawAmmo (void);
void    UpdateFace ();
void    CheckWeaponChange ();
void    ControlMovement (AActor *self);

////////////////////////////////////////////////////////////////////////////////

class AWeapon;

#define WP_NOCHANGE ((AWeapon*)~0)

extern class player_t
{
	public:
		void	Reborn();

		enum State
		{
			PST_ENTER,
			PST_DEAD,
			PST_REBORN,
			PST_LIVE
		};

		APlayerPawn	*mo;

		int32_t		oldscore,score,nextextra;
		short		lives;
		int32_t		health;

		AWeapon		*ReadyWeapon;
		AWeapon		*PendingWeapon;

		State		state;
} players[];

#endif
