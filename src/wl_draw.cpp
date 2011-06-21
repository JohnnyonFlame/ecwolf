// WL_DRAW.C

#include "wl_act.h"
#include "wl_def.h"
#include "id_pm.h"
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "textures/textures.h"
#include "c_cvars.h"

#include "wl_cloudsky.h"
#include "wl_atmos.h"
#include "wl_shade.h"
#include "thingdef.h"
#include "id_ca.h"
#include "gamemap.h"

/*
=============================================================================

							LOCAL CONSTANTS

=============================================================================
*/

// the door is the last picture before the sprites
#define DOORWALL        (PMSpriteStart-8)

#define ACTORSIZE       0x4000

/*
=============================================================================

							GLOBAL VARIABLES

=============================================================================
*/

const RatioInformation AspectCorrection[] =
{
	/* UNC */	{960,	600,	0x10000,	48},
	/* 16:9 */	{1280,	450,	0x15555,	48*3/4},
	/* 16:10 */	{1152,	500,	0x13333,	48*5/6},
	/* 4:3 */	{960,	720,	0x10000,	48*5/6},
	/* 5:4 */	{960,	640,	0x10000,	48*15/16}
};

static byte *vbuf = NULL;
unsigned vbufPitch = 0;

int32_t    lasttimecount;
int32_t    frameon;
boolean fpscounter;

int fps_frames=0, fps_time=0, fps=0;

int *wallheight;
int min_wallheight;

//
// math tables
//
short *pixelangle;
int32_t finetangent[FINEANGLES/4];
fixed sintable[ANGLES+ANGLES/4];
fixed finesine[FINEANGLES];
fixed *costable = sintable+(ANGLES/4);

//
// refresh variables
//
fixed   viewx,viewy;                    // the focal point
short   viewangle;
fixed   viewsin,viewcos;

void    TransformActor (objtype *ob);
void    BuildTables (void);
void    ClearScreen (void);
int     CalcRotate (objtype *ob);
void    DrawScaleds (void);
void    CalcTics (void);
void    ThreeDRefresh (void);



//
// wall optimization variables
//
int     lastside;               // true for vertical
int32_t    lastintercept;
MapSpot lasttilehit;
int     lasttexture;

//
// ray tracing variables
//
short    focaltx,focalty,viewtx,viewty;
longword xpartialup,xpartialdown,ypartialup,ypartialdown;

float   midangle;
short   angle;

MapTile::Side hitdir;
MapSpot tilehit;
int     pixx;

short   xtile,ytile;
short   xtilestep,ytilestep;
int32_t    xintercept,yintercept;
word    xstep,ystep;
word    xspot,yspot;
int     texdelta;

word horizwall[MAXWALLTILES],vertwall[MAXWALLTILES];


/*
============================================================================

						3 - D  DEFINITIONS

============================================================================
*/

/*
========================
=
= TransformActor
=
= Takes paramaters:
=   gx,gy               : globalx/globaly of point
=
= globals:
=   viewx,viewy         : point of view
=   viewcos,viewsin     : sin/cos of viewangle
=   scale               : conversion from global value to screen value
=
= sets:
=   screenx,transx,transy,screenheight: projected edge location and size
=
========================
*/


//
// transform actor
//
void TransformActor (objtype *ob)
{
	fixed gx,gy,gxt,gyt,nx,ny;

//
// translate point to view centered coordinates
//
	gx = ob->x-viewx;
	gy = ob->y-viewy;

//
// calculate newx
//
	gxt = FixedMul(gx,viewcos);
	gyt = FixedMul(gy,viewsin);
	nx = gxt-gyt-ACTORSIZE;         // fudge the shape forward a bit, because
									// the midpoint could put parts of the shape
									// into an adjacent wall

//
// calculate newy
//
	gxt = FixedMul(gx,viewsin);
	gyt = FixedMul(gy,viewcos);
	ny = gyt+gxt;

//
// calculate perspective ratio
//
	ob->transx = nx;
	ob->transy = ny;

	if (nx<MINDIST)                 // too close, don't overflow the divide
	{
		ob->viewheight = 0;
		return;
	}

	ob->viewx = (word)(centerx + ny*scale/nx);

//
// calculate height (heightnumerator/(nx>>8))
//
	ob->viewheight = (word)(heightnumerator/(nx>>8));
}

//==========================================================================

/*
========================
=
= TransformTile
=
= Takes paramaters:
=   tx,ty               : tile the object is centered in
=
= globals:
=   viewx,viewy         : point of view
=   viewcos,viewsin     : sin/cos of viewangle
=   scale               : conversion from global value to screen value
=
= sets:
=   screenx,transx,transy,screenheight: projected edge location and size
=
= Returns true if the tile is withing getting distance
=
========================
*/

boolean TransformTile (int tx, int ty, short *dispx, short *dispheight)
{
	fixed gx,gy,gxt,gyt,nx,ny;

//
// translate point to view centered coordinates
//
	gx = ((int32_t)tx<<TILESHIFT)+0x8000-viewx;
	gy = ((int32_t)ty<<TILESHIFT)+0x8000-viewy;

//
// calculate newx
//
	gxt = FixedMul(gx,viewcos);
	gyt = FixedMul(gy,viewsin);
	nx = gxt-gyt-0x2000;            // 0x2000 is size of object

//
// calculate newy
//
	gxt = FixedMul(gx,viewsin);
	gyt = FixedMul(gy,viewcos);
	ny = gyt+gxt;


//
// calculate height / perspective ratio
//
	if (nx<MINDIST)                 // too close, don't overflow the divide
		*dispheight = 0;
	else
	{
		*dispx = (short)(centerx + ny*scale/nx);
		*dispheight = (short)(heightnumerator/(nx>>8));
	}

//
// see if it should be grabbed
//
	if (nx<TILEGLOBAL && ny>-TILEGLOBAL/2 && ny<TILEGLOBAL/2)
		return true;
	else
		return false;
}

//==========================================================================

/*
====================
=
= CalcHeight
=
= Calculates the height of xintercept,yintercept from viewx,viewy
=
====================
*/

int CalcHeight()
{
	fixed z = FixedMul(xintercept - viewx, viewcos)
		- FixedMul(yintercept - viewy, viewsin);
	if(z < MINDIST) z = MINDIST;
	int height = heightnumerator / (z >> 8);
	if(height < min_wallheight) min_wallheight = height;
	return height;
}

//==========================================================================

/*
===================
=
= ScalePost
=
===================
*/

const byte *postsource;
int postx;

void ScalePost()
{
	if(postsource == NULL)
		return;

	int ywcount, yoffs, yw, yd, yendoffs;
	byte col;

	byte *curshades = shadetable[GetShade(wallheight[postx])];

	ywcount = yd = wallheight[postx] >> 3;
	if(yd <= 0) yd = 100;

	yoffs = (viewheight / 2 - ywcount) * vbufPitch;
	if(yoffs < 0) yoffs = 0;
	yoffs += postx;

	yendoffs = viewheight / 2 + ywcount - 1;
	yw=TEXTURESIZE-1;

	while(yendoffs >= viewheight)
	{
		ywcount -= TEXTURESIZE/2;
		while(ywcount <= 0)
		{
			ywcount += yd;
			yw--;
		}
		yendoffs--;
	}
	if(yw < 0) return;

	if(r_depthfog)
		col = curshades[postsource[yw]];
	else
		col = postsource[yw];
	yendoffs = yendoffs * vbufPitch + postx;
	while(yoffs <= yendoffs)
	{
		vbuf[yendoffs] = col;
		ywcount -= TEXTURESIZE/2;
		if(ywcount <= 0)
		{
			do
			{
				ywcount += yd;
				yw--;
			}
			while(ywcount <= 0);
			if(yw < 0) break;
			if(r_depthfog)
				col = curshades[postsource[yw]];
			else
				col = postsource[yw];
		}
		yendoffs -= vbufPitch;
	}
}

void GlobalScalePost(byte *vidbuf, unsigned pitch)
{
	vbuf = vidbuf;
	vbufPitch = pitch;
	ScalePost();
}

static void DetermineHitDir(bool vertical)
{
	if(vertical)
	{
		if(xtilestep==-1 && (xintercept>>16)<=xtile)
			hitdir = MapTile::East;
		else
			hitdir = MapTile::West;
	}
	else
	{
		if(ytilestep==-1 && (yintercept>>16)<=ytile)
			hitdir = MapTile::South;
		else
			hitdir = MapTile::North;
	}
}

/*
====================
=
= HitVertWall
=
= tilehit bit 7 is 0, because it's not a door tile
= if bit 6 is 1 and the adjacent tile is a door tile, use door side pic
=
====================
*/

void HitVertWall (void)
{
	int wallpic;
	int texture;

	DetermineHitDir(true);

	texture = ((yintercept+texdelta-tilehit->slideAmount[hitdir])>>TEXTUREFROMFIXEDSHIFT)&TEXTUREMASK;
	if (xtilestep == -1 && !tilehit->tile->offsetVertical)
	{
		texture = TEXTUREMASK-texture;
		xintercept += TILEGLOBAL;
	}

	if(lastside==1 && lastintercept==xtile && lasttilehit==tilehit && !(lasttilehit->tile->offsetVertical))
	{
		if((pixx&3) && texture == lasttexture)
		{
			ScalePost();
			postx = pixx;
			wallheight[pixx] = wallheight[pixx-1];
			return;
		}
		ScalePost();
		wallheight[pixx] = CalcHeight();
		postsource+=texture-lasttexture;
		postx=pixx;
		lasttexture=texture;
		return;
	}

	if(lastside!=-1) ScalePost();

	lastside=1;
	lastintercept=xtile;
	lasttilehit=tilehit;
	lasttexture=texture;
	wallheight[pixx] = CalcHeight();
	postx = pixx;
	FTexture *source = NULL;

	MapSpot adj = tilehit->GetAdjacent(hitdir);
	if (adj && adj->tile && adj->tile->offsetHorizontal && !adj->tile->offsetVertical) // check for adjacent doors
		source = TexMan(adj->tile->texture[hitdir]);
	else
		source = TexMan(tilehit->tile->texture[hitdir]);

	postsource = source->GetColumn(texture/64, NULL);
}


/*
====================
=
= HitHorizWall
=
= tilehit bit 7 is 0, because it's not a door tile
= if bit 6 is 1 and the adjacent tile is a door tile, use door side pic
=
====================
*/

void HitHorizWall (void)
{
	int wallpic;
	int texture;

	DetermineHitDir(false);

	texture = ((xintercept+texdelta-tilehit->slideAmount[hitdir])>>TEXTUREFROMFIXEDSHIFT)&TEXTUREMASK;
	if(!tilehit->tile->offsetHorizontal)
	{
		if (ytilestep == -1)
			yintercept += TILEGLOBAL;
		else
			texture = TEXTUREMASK-texture;
	}

	if(lastside==0 && lastintercept==ytile && lasttilehit==tilehit && !(lasttilehit->tile->offsetHorizontal))
	{
		if((pixx&3) && texture == lasttexture)
		{
			ScalePost();
			postx=pixx;
			wallheight[pixx] = wallheight[pixx-1];
			return;
		}
		ScalePost();
		wallheight[pixx] = CalcHeight();
		postsource+=texture-lasttexture;
		postx=pixx;
		lasttexture=texture;
		return;
	}

	if(lastside!=-1) ScalePost();

	lastside=0;
	lastintercept=ytile;
	lasttilehit=tilehit;
	lasttexture=texture;
	wallheight[pixx] = CalcHeight();
	postx = pixx;
	FTexture *source = NULL;

	MapSpot adj = tilehit->GetAdjacent(hitdir);
	if (adj && adj->tile && adj->tile->offsetVertical && !adj->tile->offsetHorizontal) // check for adjacent doors
		source = TexMan(adj->tile->texture[hitdir]);
	else
		source = TexMan(tilehit->tile->texture[hitdir]);

	postsource = source->GetColumn(texture/64, NULL);
}

//==========================================================================

#define HitHorizBorder HitHorizWall
#define HitVertBorder HitVertWall

//==========================================================================

byte vgaCeiling[]=
{
#ifndef SPEAR
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0xbf,
0x4e,0x4e,0x4e,0x1d,0x8d,0x4e,0x1d,0x2d,0x1d,0x8d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x2d,0xdd,0x1d,0x1d,0x98,

0x1d,0x9d,0x2d,0xdd,0xdd,0x9d,0x2d,0x4d,0x1d,0xdd,
0x7d,0x1d,0x2d,0x2d,0xdd,0xd7,0x1d,0x1d,0x1d,0x2d,
0x1d,0x1d,0x1d,0x1d,0xdd,0xdd,0x7d,0xdd,0xdd,0xdd
#else
0x6f,0x4f,0x1d,0xde,0xdf,0x2e,0x7f,0x9e,0xae,0x7f,
0x1d,0xde,0xdf,0xde,0xdf,0xde,0xe1,0xdc,0x2e,0x1d,0xdc
#endif
};

/*
=====================
=
= VGAClearScreen
=
=====================
*/

void VGAClearScreen (void)
{
	byte ceiling=vgaCeiling[gamestate.episode*10+mapon];

	int y;
	byte *ptr = vbuf;
	if(r_depthfog)
	{
		for(y = 0; y < viewheight / 2; y++, ptr += vbufPitch)
			memset(ptr, shadetable[GetShade((viewheight / 2 - y) << 3)][ceiling], viewwidth);
		for(; y < viewheight; y++, ptr += vbufPitch)
			memset(ptr, shadetable[GetShade((y - viewheight / 2) << 3)][0x19], viewwidth);
	}
	else
	{
		for(y = 0; y < viewheight / 2; y++, ptr += vbufPitch)
			memset(ptr, ceiling, viewwidth);
		for(; y < viewheight; y++, ptr += vbufPitch)
			memset(ptr, 0x19, viewwidth);
	}
}

//==========================================================================

/*
=====================
=
= CalcRotate
=
=====================
*/

int CalcRotate (objtype *ob)
{
	int angle, viewangle;

	// this isn't exactly correct, as it should vary by a trig value,
	// but it is close enough with only eight rotations

	viewangle = player->angle + (centerx - ob->viewx)/8;

	if (ob->obclass == rocketobj || ob->obclass == hrocketobj)
		angle = (viewangle-180) - ob->angle;
	else
		angle = (viewangle-180) - dirangle[ob->dir];

	angle+=ANGLES/16;
	while (angle>=ANGLES)
		angle-=ANGLES;
	while (angle<0)
		angle+=ANGLES;

	if (ob->state->rotate == 2)             // 2 rotation pain frame
		return 0;               // pain with shooting frame bugfix

	return angle/(ANGLES/8);
}

void ScaleShape (int xcenter, int shapenum, unsigned height, uint32_t flags)
{
	t_compshape *shape;
	unsigned scale,pixheight,pixwidth;
	unsigned starty,endy;
	word *cmdptr;
	byte *cline;
	byte *line;
	byte *vmem;
	int actx,i,upperedge;
	short newstart;
	int scrstarty,screndy,lpix,rpix,pixcnt,ycnt;
	unsigned j;
	byte col;

	byte *curshades;
	if(flags & FL_FULLBRIGHT)
		curshades = shadetable[0];
	else
		curshades = shadetable[GetShade(height)];

	shape = (t_compshape *) PM_GetSprite(shapenum);

	scale=height>>3;                 // low three bits are fractional
	if(!scale) return;   // too close or far away

	pixwidth=CorrectWidthFactor(scale*SPRITESCALEFACTOR);
	pixheight=scale*SPRITESCALEFACTOR;
	actx=xcenter-CorrectWidthFactor(scale);
	upperedge=(viewheight/2)-scale;

	cmdptr=(word *) shape->dataofs;

	for(i=shape->leftpix,pixcnt=i*pixwidth,rpix=(pixcnt>>6)+actx;i<=shape->rightpix;i++,cmdptr++)
	{
		lpix=rpix;
		if(lpix>=viewwidth) break;
		pixcnt+=pixwidth;
		rpix=(pixcnt>>6)+actx;
		if(lpix!=rpix && rpix>0)
		{
			if(lpix<0) lpix=0;
			if(rpix>viewwidth) rpix=viewwidth,i=shape->rightpix+1;
			cline=(byte *)shape + *cmdptr;
			while(lpix<rpix)
			{
				if(wallheight[lpix]<=(int)height)
				{
					line=cline;
					while((endy = READWORD(line)) != 0)
					{
						endy >>= 1;
						newstart = READWORD(line);
						starty = READWORD(line) >> 1;
						j=starty;
						ycnt=j*pixheight;
						screndy=(ycnt>>6)+upperedge;
						if(screndy<0) vmem=vbuf+lpix;
						else vmem=vbuf+screndy*vbufPitch+lpix;
						for(;j<endy;j++)
						{
							scrstarty=screndy;
							ycnt+=pixheight;
							screndy=(ycnt>>6)+upperedge;
							if(scrstarty!=screndy && screndy>0)
							{
								if(r_depthfog)
									col=curshades[((byte *)shape)[newstart+j]];
								else
									col=((byte *)shape)[newstart+j];
								if(scrstarty<0) scrstarty=0;
								if(screndy>viewheight) screndy=viewheight,j=endy;

								while(scrstarty<screndy)
								{
									*vmem=col;
									vmem+=vbufPitch;
									scrstarty++;
								}
							}
						}
					}
				}
				lpix++;
			}
		}
	}
}

void SimpleScaleShape (int xcenter, int shapenum, unsigned height)
{
	t_compshape   *shape;
	unsigned scale,pixheight,pixwidth;
	unsigned starty,endy;
	word *cmdptr;
	byte *cline;
	byte *line;
	int actx,i,upperedge;
	short newstart;
	int scrstarty,screndy,lpix,rpix,pixcnt,ycnt;
	unsigned j;
	byte col;
	byte *vmem;

	shape = (t_compshape *) PM_GetSprite(shapenum);

	scale=height>>1;
	pixwidth=CorrectWidthFactor(scale*SPRITESCALEFACTOR);
	pixheight=scale*SPRITESCALEFACTOR;
	actx=xcenter-CorrectWidthFactor(scale);
	upperedge=(viewheight/2)-scale;

	cmdptr=shape->dataofs;

	for(i=shape->leftpix,pixcnt=i*pixwidth,rpix=(pixcnt>>6)+actx;i<=shape->rightpix;i++,cmdptr++)
	{
		lpix=rpix;
		if(lpix>=viewwidth) break;
		pixcnt+=pixwidth;
		rpix=(pixcnt>>6)+actx;
		if(lpix!=rpix && rpix>0)
		{
			if(lpix<0) lpix=0;
			if(rpix>viewwidth) rpix=viewwidth,i=shape->rightpix+1;
			cline = (byte *)shape + *cmdptr;
			while(lpix<rpix)
			{
				line=cline;
				while((endy = READWORD(line)) != 0)
				{
					endy >>= 1;
					newstart = READWORD(line);
					starty = READWORD(line) >> 1;
					j=starty;
					ycnt=j*pixheight;
					screndy=(ycnt>>6)+upperedge;
					if(screndy<0) vmem=vbuf+lpix;
					else vmem=vbuf+screndy*vbufPitch+lpix;
					for(;j<endy;j++)
					{
						scrstarty=screndy;
						ycnt+=pixheight;
						screndy=(ycnt>>6)+upperedge;
						if(scrstarty!=screndy && screndy>0)
						{
							col=((byte *)shape)[newstart+j];
							if(scrstarty<0) scrstarty=0;
							if(screndy>viewheight) screndy=viewheight,j=endy;

							while(scrstarty<screndy)
							{
								*vmem=col;
								vmem+=vbufPitch;
								scrstarty++;
							}
						}
					}
				}
				lpix++;
			}
		}
	}
}

/*
=====================
=
= DrawScaleds
=
= Draws all objects that are visable
=
=====================
*/

#define MAXVISABLE 250

typedef struct
{
	short      viewx,
			viewheight,
			shapenum;
	short      flags;          // this must be changed to uint32_t, when you
							// you need more than 16-flags for drawing
#ifdef USE_DIR3DSPR
	statobj_t *transsprite;
#endif
} visobj_t;

visobj_t vislist[MAXVISABLE];
visobj_t *visptr,*visstep,*farthest;

void DrawScaleds (void)
{
	int      i,least,numvisable,height;
	byte     *visspot;
	unsigned spotloc;

	statobj_t *statptr;
	objtype   *obj;

	visptr = &vislist[0];

//
// place static objects
//
	for (statptr = &statobjlist[0] ; statptr !=laststatobj ; statptr++)
	{
		if ((visptr->shapenum = statptr->shapenum) == -1)
			continue;                                               // object has been deleted

		if (!*statptr->visspot)
			continue;                                               // not visable

		if (TransformTile (statptr->tilex,statptr->tiley,
			&visptr->viewx,&visptr->viewheight) && statptr->flags & FL_BONUS)
		{
			GetBonus (statptr);
			if(statptr->shapenum == -1)
				continue;                                           // object has been taken
		}

		if (!visptr->viewheight)
			continue;                                               // to close to the object

#ifdef USE_DIR3DSPR
		if(statptr->flags & FL_DIR_MASK)
			visptr->transsprite=statptr;
		else
			visptr->transsprite=NULL;
#endif

		if (visptr < &vislist[MAXVISABLE-1])    // don't let it overflow
		{
			visptr->flags = (short) statptr->flags;
			visptr++;
		}
	}

//
// place active objects
//
	for (obj = player->next;obj;obj=obj->next)
	{
		if ((visptr->shapenum = obj->state->shapenum)==0)
			continue;                                               // no shape

		spotloc = (obj->tilex<<mapshift)+obj->tiley;   // optimize: keep in struct?
		visspot = &spotvis[0][0]+spotloc;
		MapSpot spot = map->GetSpot(spotloc%64, spotloc/64, 0);
		MapSpot spots[8];
		spots[0] = spot->GetAdjacent(MapTile::East);
		spots[1] = spots[0]->GetAdjacent(MapTile::North);
		spots[2] = spot->GetAdjacent(MapTile::North);
		spots[3] = spots[2]->GetAdjacent(MapTile::West);
		spots[4] = spot->GetAdjacent(MapTile::West);
		spots[5] = spots[4]->GetAdjacent(MapTile::South);
		spots[6] = spot->GetAdjacent(MapTile::South);
		spots[7] = spots[6]->GetAdjacent(MapTile::East);

		//
		// could be in any of the nine surrounding tiles
		//
		if (*visspot
			|| ( *(visspot-1) && !(spots[4] && spots[4]->tile) )
			|| ( *(visspot+1) && !(spots[0] && spots[0]->tile) )
			|| ( *(visspot-65) && !(spots[3] && spots[3]->tile) )
			|| ( *(visspot-64) && !(spots[2] && spots[2]->tile) )
			|| ( *(visspot-63) && !(spots[1] && spots[1]->tile) )
			|| ( *(visspot+65) && !(spots[7] && spots[7]->tile) )
			|| ( *(visspot+64) && !(spots[6] && spots[6]->tile) )
			|| ( *(visspot+63) && !(spots[5] && spots[5]->tile) ) )
		{
			obj->active = ac_yes;
			TransformActor (obj);
			if (!obj->viewheight)
				continue;                                               // too close or far away

			visptr->viewx = obj->viewx;
			visptr->viewheight = obj->viewheight;
			if (visptr->shapenum == -1)
				visptr->shapenum = obj->temp1;  // special shape

			if (obj->state->rotate)
				visptr->shapenum += CalcRotate (obj);

			if (visptr < &vislist[MAXVISABLE-1])    // don't let it overflow
			{
				visptr->flags = (short) obj->flags;
#ifdef USE_DIR3DSPR
				visptr->transsprite = NULL;
#endif
				visptr++;
			}
			obj->flags |= FL_VISABLE;
		}
		else
			obj->flags &= ~FL_VISABLE;
	}

//
// draw from back to front
//
	numvisable = (int) (visptr-&vislist[0]);

	if (!numvisable)
		return;                                                                 // no visable objects

	for (i = 0; i<numvisable; i++)
	{
		least = 32000;
		for (visstep=&vislist[0] ; visstep<visptr ; visstep++)
		{
			height = visstep->viewheight;
			if (height < least)
			{
				least = height;
				farthest = visstep;
			}
		}
		//
		// draw farthest
		//
#ifdef USE_DIR3DSPR
		if(farthest->transsprite)
			Scale3DShape(vbuf, vbufPitch, farthest->transsprite);
		else
#endif
			ScaleShape(farthest->viewx, farthest->shapenum, farthest->viewheight, farthest->flags);

		farthest->viewheight = 32000;
	}
}

//==========================================================================

/*
==============
=
= DrawPlayerWeapon
=
= Draw the player's hands
=
==============
*/

int weaponscale[NUMWEAPONS] = {SPR_KNIFEREADY, SPR_PISTOLREADY,
	SPR_MACHINEGUNREADY, SPR_CHAINREADY};

void DrawPlayerWeapon (void)
{
	int shapenum;

#ifndef SPEAR
	if (gamestate.victoryflag)
	{
#ifndef APOGEE_1_0
		if (player->state == &s_deathcam && (GetTimeCount()&32) )
			SimpleScaleShape(viewwidth/2,SPR_DEATHCAM,viewheight+1);
#endif
		return;
	}
#endif

	if (gamestate.weapon != -1)
	{
		shapenum = weaponscale[gamestate.weapon]+gamestate.weaponframe;
		SimpleScaleShape(viewwidth/2,shapenum,viewheight+1);
	}

	if (demorecord || demoplayback)
		SimpleScaleShape(viewwidth/2,SPR_DEMO,viewheight+1);
}


//==========================================================================


/*
=====================
=
= CalcTics
=
=====================
*/

void CalcTics (void)
{
//
// calculate tics since last refresh for adaptive timing
//
	if (lasttimecount > (int32_t) GetTimeCount())
		lasttimecount = GetTimeCount();    // if the game was paused a LONG time

	uint32_t curtime = SDL_GetTicks();
	tics = (curtime * 7) / 100 - lasttimecount;
	if(!tics)
	{
		// wait until end of current tic
		SDL_Delay(((lasttimecount + 1) * 100) / 7 - curtime);
		tics = 1;
	}

	lasttimecount += tics;

	if (tics>MAXTICS)
		tics = MAXTICS;
}


//==========================================================================

void AsmRefresh()
{
	int32_t xstep,ystep;
	longword xpartial,ypartial;
	MapSpot focalspot = map->GetSpot(focaltx, focalty, 0);
	bool playerInPushwallBackTile = focalspot->pushAmount != 0 || focalspot->pushReceptor;

	for(pixx=0;pixx<viewwidth;pixx++)
	{
		short angl=midangle+pixelangle[pixx];
		if(angl<0) angl+=FINEANGLES;
		if(angl>=ANG360) angl-=FINEANGLES;
		if(angl<ANG90)
		{
			xtilestep=1;
			ytilestep=-1;
			xstep=finetangent[ANG90-1-angl];
			ystep=-finetangent[angl];
			xpartial=xpartialup;
			ypartial=ypartialdown;
		}
		else if(angl<ANG180)
		{
			xtilestep=-1;
			ytilestep=-1;
			xstep=-finetangent[angl-ANG90];
			ystep=-finetangent[ANG180-1-angl];
			xpartial=xpartialdown;
			ypartial=ypartialdown;
		}
		else if(angl<ANG270)
		{
			xtilestep=-1;
			ytilestep=1;
			xstep=-finetangent[ANG270-1-angl];
			ystep=finetangent[angl-ANG180];
			xpartial=xpartialdown;
			ypartial=ypartialup;
		}
		else if(angl<ANG360)
		{
			xtilestep=1;
			ytilestep=1;
			xstep=finetangent[angl-ANG270];
			ystep=finetangent[ANG360-1-angl];
			xpartial=xpartialup;
			ypartial=ypartialup;
		}
		yintercept=FixedMul(ystep,xpartial)+viewy;
		xtile=focaltx+xtilestep;
		xspot=(word)((xtile<<mapshift)+((uint32_t)yintercept>>16));
		xintercept=FixedMul(xstep,ypartial)+viewx;
		ytile=focalty+ytilestep;
		yspot=(word)((((uint32_t)xintercept>>16)<<mapshift)+ytile);
		texdelta=0;

		// Special treatment when player is in back tile of pushwall
		if(playerInPushwallBackTile)
		{
			if(focalspot->pushReceptor)
				focalspot = focalspot->pushReceptor;

			if((focalspot->pushDirection == MapTile::East && xtilestep == 1) ||
				(focalspot->pushDirection == MapTile::West && xtilestep == -1))
			{
				int32_t yintbuf = yintercept - ((ystep * (64 - focalspot->pushAmount)) >> 6);
				if((yintbuf >> 16) == focalty)   // ray hits pushwall back?
				{
					if(focalspot->pushDirection == MapTile::East)
						xintercept = (focaltx << TILESHIFT) + (focalspot->pushAmount << 10);
					else
						xintercept = (focaltx << TILESHIFT) - TILEGLOBAL + ((64 - focalspot->pushAmount) << 10);
					yintercept = yintbuf;
					ytile = (short) (yintercept >> TILESHIFT);
					tilehit = focalspot;
					HitVertWall();
					continue;
				}
			}
			else if((focalspot->pushDirection == MapTile::South && ytilestep == 1) ||
				(focalspot->pushDirection == MapTile::North && ytilestep == -1))
			{
				int32_t xintbuf = xintercept - ((xstep * (64 - focalspot->pushAmount)) >> 6);
				if((xintbuf >> 16) == focaltx)   // ray hits pushwall back?
				{
					xintercept = xintbuf;
					if(focalspot->pushDirection == MapTile::South)
						yintercept = (focalty << TILESHIFT) + (focalspot->pushAmount << 10);
					else
						yintercept = (focalty << TILESHIFT) - TILEGLOBAL + ((64 - focalspot->pushAmount) << 10);
					xtile = (short) (xintercept >> TILESHIFT);
					tilehit = focalspot;
					HitHorizWall();
					continue;
				}
			}
		}

		do
		{
			if(ytilestep==-1 && (yintercept>>16)<=ytile) goto horizentry;
			if(ytilestep==1 && (yintercept>>16)>=ytile) goto horizentry;
vertentry:

			if((uint32_t)yintercept>mapheight*65536-1 || (word)xtile>=mapwidth)
			{
				if(xtile<0) xintercept=0, xtile=0;
				else if(xtile>=mapwidth) xintercept=mapwidth<<TILESHIFT, xtile=mapwidth-1;
				else xtile=(short) (xintercept >> TILESHIFT);
				if(yintercept<0) yintercept=0, ytile=0;
				else if(yintercept>=(mapheight<<TILESHIFT)) yintercept=mapheight<<TILESHIFT, ytile=mapheight-1;
				yspot=0xffff;
				tilehit=0;
				HitHorizBorder();
				break;
			}
			if(xspot>=maparea) break;
			tilehit=map->GetSpot(xspot/MAPSIZE, xspot%MAPSIZE, 0);
			if(tilehit && tilehit->tile)
			{
				if(tilehit->tile->offsetVertical)
				{
					DetermineHitDir(true);
					int32_t yintbuf=yintercept+(ystep>>1);
					if((yintbuf>>16)!=(yintercept>>16))
						goto passvert;
					if((word)yintbuf<tilehit->slideAmount[hitdir])
						goto passvert;
					yintercept=yintbuf;
					xintercept=(xtile<<TILESHIFT)|0x8000;
					ytile = (short) (yintercept >> TILESHIFT);
					HitVertWall();
				}
				else
				{
					bool isPushwall = tilehit->pushAmount != 0 || tilehit->pushReceptor;
					if(tilehit->pushReceptor)
						tilehit = tilehit->pushReceptor;

					if(isPushwall)
					{
						if(tilehit->pushDirection==MapTile::West || tilehit->pushDirection==MapTile::East)
						{
							int32_t yintbuf;
							int pwallposnorm;
							int pwallposinv;
							if(tilehit->pushDirection==MapTile::West)
							{
								pwallposnorm = 64-tilehit->pushAmount;
								pwallposinv = tilehit->pushAmount;
							}
							else
							{
								pwallposnorm = tilehit->pushAmount;
								pwallposinv = 64-tilehit->pushAmount;
							}
							if(tilehit->pushDirection==MapTile::East && xtile==tilehit->GetX() && ((uint32_t)yintercept>>16)==tilehit->GetY()
								|| tilehit->pushDirection==MapTile::West && !(xtile==tilehit->GetX() && ((uint32_t)yintercept>>16)==tilehit->GetY()))
							{
								yintbuf=yintercept+((ystep*pwallposnorm)>>6);
								if((yintbuf>>16)!=(yintercept>>16))
									goto passvert;

								xintercept=(xtile<<TILESHIFT)+TILEGLOBAL-(pwallposinv<<10);
								yintercept=yintbuf;
								ytile = (short) (yintercept >> TILESHIFT);
								HitVertWall();
							}
							else
							{
								yintbuf=yintercept+((ystep*pwallposinv)>>6);
								if((yintbuf>>16)!=(yintercept>>16))
									goto passvert;

								xintercept=(xtile<<TILESHIFT)-(pwallposinv<<10);
								yintercept=yintbuf;
								ytile = (short) (yintercept >> TILESHIFT);
								HitVertWall();
							}
						}
						else
						{
							int pwallposi = tilehit->pushAmount;
							if(tilehit->pushDirection==MapTile::North) pwallposi = 64-tilehit->pushAmount;
							if(tilehit->pushDirection==MapTile::South && (word)yintercept<(pwallposi<<10)
								|| tilehit->pushDirection==MapTile::North && (word)yintercept>(pwallposi<<10))
							{
								if(((uint32_t)yintercept>>16)==tilehit->GetY() && xtile==tilehit->GetX())
								{
									if(tilehit->pushDirection==MapTile::South && (int32_t)((word)yintercept)+ystep<(pwallposi<<10)
											|| tilehit->pushDirection==MapTile::North && (int32_t)((word)yintercept)+ystep>(pwallposi<<10))
										goto passvert;

									if(tilehit->pushDirection==MapTile::South)
									{
										yintercept=(yintercept&0xffff0000)+(pwallposi<<10);
										xintercept=xintercept-((xstep*(64-pwallposi))>>6);
									}
									else
									{
										yintercept=(yintercept&0xffff0000)-TILEGLOBAL+(pwallposi<<10);
										xintercept=xintercept-((xstep*pwallposi)>>6);
									}
									xtile = (short) (xintercept >> TILESHIFT);
									HitHorizWall();
								}
								else
								{
									texdelta = -(pwallposi<<10);
									xintercept=xtile<<TILESHIFT;
									ytile = (short) (yintercept >> TILESHIFT);
									HitVertWall();
								}
							}
							else
							{
								if(((uint32_t)yintercept>>16)==tilehit->GetY() && xtile==tilehit->GetX())
								{
									texdelta = -(pwallposi<<10);
									xintercept=xtile<<TILESHIFT;
									ytile = (short) (yintercept >> TILESHIFT);
									HitVertWall();
								}
								else
								{
									if(tilehit->pushDirection==MapTile::South && (int32_t)((word)yintercept)+ystep>(pwallposi<<10)
											|| tilehit->pushDirection==MapTile::North && (int32_t)((word)yintercept)+ystep<(pwallposi<<10))
										goto passvert;

									if(tilehit->pushDirection==MapTile::South)
									{
										yintercept=(yintercept&0xffff0000)-TILEGLOBAL+(pwallposi<<10);
										xintercept=xintercept-((xstep*pwallposi)>>6);
									}
									else
									{
										yintercept=(yintercept&0xffff0000)+(pwallposi<<10);
										xintercept=xintercept-((xstep*(64-pwallposi))>>6);
									}
									xtile = (short) (xintercept >> TILESHIFT);
									HitHorizWall();
								}
							}
						}
					}
					else
					{
						xintercept=xtile<<TILESHIFT;
						ytile = (short) (yintercept >> TILESHIFT);
						HitVertWall();
					}
				}
				break;
			}
passvert:
			*((byte *)spotvis+xspot)=1;
			xtile+=xtilestep;
			yintercept+=ystep;
			xspot=(word)((xtile<<mapshift)+((uint32_t)yintercept>>16));
		}
		while(1);
		continue;

		do
		{
			if(xtilestep==-1 && (xintercept>>16)<=xtile) goto vertentry;
			if(xtilestep==1 && (xintercept>>16)>=xtile) goto vertentry;
horizentry:

			if((uint32_t)xintercept>mapwidth*65536-1 || (word)ytile>=mapheight)
			{
				if(ytile<0) yintercept=0, ytile=0;
				else if(ytile>=mapheight) yintercept=mapheight<<TILESHIFT, ytile=mapheight-1;
				else ytile=(short) (yintercept >> TILESHIFT);
				if(xintercept<0) xintercept=0, xtile=0;
				else if(xintercept>=(mapwidth<<TILESHIFT)) xintercept=mapwidth<<TILESHIFT, xtile=mapwidth-1;
				xspot=0xffff;
				tilehit=0;
				HitVertBorder();
				break;
			}
			if(yspot>=maparea) break;
			tilehit=map->GetSpot(yspot/MAPSIZE, yspot%MAPSIZE, 0);
			if(tilehit && tilehit->tile)
			{
				if(tilehit->tile->offsetHorizontal)
				{
					DetermineHitDir(false);
					int32_t xintbuf=xintercept+(xstep>>1);
					if((xintbuf>>16)!=(xintercept>>16))
						goto passhoriz;
					if((word)xintbuf<tilehit->slideAmount[hitdir])
						goto passhoriz;
					xintercept=xintbuf;
					yintercept=(ytile<<TILESHIFT)+0x8000;
					xtile = (short) (xintercept >> TILESHIFT);
					HitHorizWall();
				}
				else
				{
					bool isPushwall = tilehit->pushAmount != 0 || tilehit->pushReceptor;
					if(tilehit->pushReceptor)
						tilehit = tilehit->pushReceptor;

					if(isPushwall)
					{
						if(tilehit->pushDirection==MapTile::North || tilehit->pushDirection==MapTile::South)
						{
							int32_t xintbuf;
							int pwallposnorm;
							int pwallposinv;
							if(tilehit->pushDirection==MapTile::North)
							{
								pwallposnorm = 64-tilehit->pushAmount;
								pwallposinv = tilehit->pushAmount;
							}
							else
							{
								pwallposnorm = tilehit->pushAmount;
								pwallposinv = 64-tilehit->pushAmount;
							}
							if(tilehit->pushDirection == MapTile::South && ytile==tilehit->GetY() && ((uint32_t)xintercept>>16)==tilehit->GetX()
								|| tilehit->pushDirection == MapTile::North && !(ytile==tilehit->GetY() && ((uint32_t)xintercept>>16)==tilehit->GetX()))
							{
								xintbuf=xintercept+((xstep*pwallposnorm)>>6);
								if((xintbuf>>16)!=(xintercept>>16))
									goto passhoriz;

								yintercept=(ytile<<TILESHIFT)+TILEGLOBAL-(pwallposinv<<10);
								xintercept=xintbuf;
								xtile = (short) (xintercept >> TILESHIFT);
								HitHorizWall();
							}
							else
							{
								xintbuf=xintercept+((xstep*pwallposinv)>>6);
								if((xintbuf>>16)!=(xintercept>>16))
									goto passhoriz;

								yintercept=(ytile<<TILESHIFT)-(pwallposinv<<10);
								xintercept=xintbuf;
								xtile = (short) (xintercept >> TILESHIFT);
								HitHorizWall();
							}
						}
						else
						{
							int pwallposi = tilehit->pushAmount;
							if(tilehit->pushDirection==MapTile::West) pwallposi = 64-tilehit->pushAmount;
							if(tilehit->pushDirection==MapTile::East && (word)xintercept<(pwallposi<<10)
									|| tilehit->pushDirection==MapTile::West && (word)xintercept>(pwallposi<<10))
							{
								if(((uint32_t)xintercept>>16)==tilehit->GetX() && ytile==tilehit->GetY())
								{
									if(tilehit->pushDirection==MapTile::East && (int32_t)((word)xintercept)+xstep<(pwallposi<<10)
											|| tilehit->pushDirection==MapTile::West && (int32_t)((word)xintercept)+xstep>(pwallposi<<10))
										goto passhoriz;

									if(tilehit->pushDirection==MapTile::East)
									{
										xintercept=(xintercept&0xffff0000)+(pwallposi<<10);
										yintercept=yintercept-((ystep*(64-pwallposi))>>6);
									}
									else
									{
										xintercept=(xintercept&0xffff0000)-TILEGLOBAL+(pwallposi<<10);
										yintercept=yintercept-((ystep*pwallposi)>>6);
									}
									ytile = (short) (yintercept >> TILESHIFT);
									HitVertWall();
								}
								else
								{
									texdelta = -(pwallposi<<10);
									yintercept=ytile<<TILESHIFT;
									xtile = (short) (xintercept >> TILESHIFT);
									HitHorizWall();
								}
							}
							else
							{
								if(((uint32_t)xintercept>>16)==tilehit->GetX() && ytile==tilehit->GetY())
								{
									texdelta = -(pwallposi<<10);
									yintercept=ytile<<TILESHIFT;
									xtile = (short) (xintercept >> TILESHIFT);
									HitHorizWall();
								}
								else
								{
									if(tilehit->pushDirection==MapTile::East && (int32_t)((word)xintercept)+xstep>(pwallposi<<10)
											|| tilehit->pushDirection==MapTile::West && (int32_t)((word)xintercept)+xstep<(pwallposi<<10))
										goto passhoriz;

									if(tilehit->pushDirection==MapTile::East)
									{
										xintercept=(xintercept&0xffff0000)-TILEGLOBAL+(pwallposi<<10);
										yintercept=yintercept-((ystep*pwallposi)>>6);
									}
									else
									{
										xintercept=(xintercept&0xffff0000)+(pwallposi<<10);
										yintercept=yintercept-((ystep*(64-pwallposi))>>6);
									}
									ytile = (short) (yintercept >> TILESHIFT);
									HitVertWall();
								}
							}
						}
					}
					else
					{
						yintercept=ytile<<TILESHIFT;
						xtile = (short) (xintercept >> TILESHIFT);
						HitHorizWall();
					}
				}
				break;
			}
passhoriz:
			*((byte *)spotvis+yspot)=1;
			ytile+=ytilestep;
			xintercept+=xstep;
			yspot=(word)((((uint32_t)xintercept>>16)<<mapshift)+ytile);
		}
		while(1);
	}
}

/*
====================
=
= WallRefresh
=
====================
*/

void WallRefresh (void)
{
	xpartialdown = viewx&(TILEGLOBAL-1);
	xpartialup = TILEGLOBAL-xpartialdown;
	ypartialdown = viewy&(TILEGLOBAL-1);
	ypartialup = TILEGLOBAL-ypartialdown;

	min_wallheight = viewheight;
	lastside = -1;                  // the first pixel is on a new wall
	AsmRefresh ();
	ScalePost ();                   // no more optimization on last post
}

void CalcViewVariables()
{
	viewangle = player->angle;
	midangle = float(viewangle)*(float(FINEANGLES)/float(ANGLES));
	viewsin = sintable[viewangle];
	viewcos = costable[viewangle];
	viewx = player->x - FixedMul(focallength,viewcos);
	viewy = player->y + FixedMul(focallength,viewsin);

	focaltx = (short)(viewx>>TILESHIFT);
	focalty = (short)(viewy>>TILESHIFT);

	viewtx = (short)(player->x >> TILESHIFT);
	viewty = (short)(player->y >> TILESHIFT);
}

//==========================================================================

/*
========================
=
= ThreeDRefresh
=
========================
*/

void    ThreeDRefresh (void)
{
//
// clear out the traced array
//
	memset(spotvis,0,maparea);
	spotvis[player->tilex][player->tiley] = 1;       // Detect all sprites over player fix

	vbuf = VL_LockSurface(screenBuffer);
	vbuf+=screenofs;
	vbufPitch = bufferPitch;

	CalcViewVariables();

//
// follow the walls from there to the right, drawing as we go
//
	VGAClearScreen ();
#if defined(USE_FEATUREFLAGS) && defined(USE_STARSKY)
	if(GetFeatureFlags() & FF_STARSKY)
		DrawStarSky(vbuf, vbufPitch);
#endif

	WallRefresh ();

#if defined(USE_FEATUREFLAGS) && defined(USE_PARALLAX)
	if(GetFeatureFlags() & FF_PARALLAXSKY)
		DrawParallax(vbuf, vbufPitch);
#endif
#if defined(USE_FEATUREFLAGS) && defined(USE_CLOUDSKY)
	if(GetFeatureFlags() & FF_CLOUDSKY)
		DrawClouds(vbuf, vbufPitch, min_wallheight);
#endif
#ifdef USE_FLOORCEILINGTEX
	DrawFloorAndCeiling(vbuf, vbufPitch, min_wallheight);
#endif

//
// draw all the scaled images
//
	DrawScaleds();                  // draw scaled stuff

#if defined(USE_FEATUREFLAGS) && defined(USE_RAIN)
	if(GetFeatureFlags() & FF_RAIN)
		DrawRain(vbuf, vbufPitch);
#endif
#if defined(USE_FEATUREFLAGS) && defined(USE_SNOW)
	if(GetFeatureFlags() & FF_SNOW)
		DrawSnow(vbuf, vbufPitch);
#endif

	DrawPlayerWeapon ();    // draw player's hands

	if(Keyboard[sc_Tab] && viewsize == 21 && gamestate.weapon != -1)
		ShowActStatus();

	VL_UnlockSurface(screenBuffer);
	vbuf = NULL;

//
// show screen and time last cycle
//

	if (fizzlein)
	{
		FizzleFade(screenBuffer, 0, 0, screenWidth, screenHeight, 20, false);
		fizzlein = false;

		lasttimecount = GetTimeCount();          // don't make a big tic count
	}
	else
	{
#ifndef REMDEBUG
		if (fpscounter)
		{
			fontnumber = 0;
			SETFONTCOLOR(7,127);
			PrintX=4; PrintY=1;
			VWB_Bar(0,0,50,10,bordercol);
			US_PrintSigned(fps);
			US_Print(" fps");
		}
#endif
		SDL_BlitSurface(screenBuffer, NULL, screen, NULL);
		SDL_Flip(screen);
	}

#ifndef REMDEBUG
	if (fpscounter)
	{
		fps_frames++;
		fps_time+=tics;

		if(fps_time>35)
		{
			fps_time-=35;
			fps=fps_frames<<1;
			fps_frames=0;
		}
	}
#endif
}
