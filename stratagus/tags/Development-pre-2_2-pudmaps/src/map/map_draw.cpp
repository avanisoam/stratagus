//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name map_draw.c - The map drawing. */
//
//      (c) Copyright 1999-2004 by Lutz Sammer and Jimmy Salmon
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//      $Id$

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "tileset.h"
#include "video.h"
#include "map.h"
#include "player.h"
#include "pathfinder.h"
#include "ui.h"

#if defined(DEBUG) && defined(TIMEIT)
#include "rdtsc.h"
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

#ifdef DEBUG
#define noTIMEIT  /// defined time function
#endif

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Global functions
----------------------------------------------------------------------------*/

/**
**  Denote wether area in map is overlapping with the viewport.
**
**  @param vp  Viewport pointer.
**  @param sx  X map tile position of area in map to be checked.
**  @param sy  Y map tile position of area in map to be checked.
**  @param ex  X map tile position of area in map to be checked.
**  @param ey  Y map tile position of area in map to be checked.
**
**  @return    True if overlapping, false otherwise.
*/
int MapAreaVisibleInViewport(const Viewport* vp, int sx, int sy,
	int ex, int ey)
{
	return sx >= vp->MapX && sy >= vp->MapY &&
		ex < vp->MapX + vp->MapWidth && ey < vp->MapY + vp->MapHeight;
}

/**
**  Check if a point is visible (inside) a viewport.
**
**  @param vp  Viewport pointer.
**  @param x   X map tile position of point in map to be checked.
**  @param y   Y map tile position of point in map to be checked.
**
**  @return    True if point is in the visible map, false otherwise
*/
static inline int PointInViewport(const Viewport* vp, int x, int y)
{
	return vp->MapX <= x && x < vp->MapX + vp->MapWidth &&
		vp->MapY <= y && y < vp->MapY + vp->MapHeight;
}

/**
**  Check if any part of an area is visible in a viewport.
**
**  @param vp  Viewport pointer.
**  @param sx  X map tile position of area in map to be checked.
**  @param sy  Y map tile position of area in map to be checked.
**  @param ex  X map tile position of area in map to be checked.
**  @param ey  Y map tile position of area in map to be checked.
**
**  @return    True if any part of area is visible, false otherwise
**
**  @todo Doesn't work if all points lay outside and the area covers
**        the complete viewport.
*/
int AnyMapAreaVisibleInViewport(const Viewport* vp, int sx, int sy,
	int ex, int ey)
{
	// FIXME: Can be written faster
	return PointInViewport(vp, sx, sy) || PointInViewport(vp, sx, ey) ||
		PointInViewport(vp, ex, sy) || PointInViewport(vp, ex, ey);
}

/**
**  Convert viewport x coordinate to map tile x coordinate.
**
**  @param vp  Viewport pointer.
**  @param x   X coordinate into this viewport (in pixels, relative
**             to origin of Stratagus's window - not the viewport
**             itself!).
**
**  @return    X map tile coordinate.
*/
int Viewport2MapX(const Viewport* vp, int x)
{
	int r;

	r = (x - vp->X + vp->MapX * TileSizeX + vp->OffsetX) / TileSizeX;
	return r < TheMap.Info.MapWidth ? r : TheMap.Info.MapWidth - 1;
}

/**
**  Convert viewport y coordinate to map tile y coordinate.
**
**  @param vp  Viewport pointer.
**  @param y   Y coordinate into this viewport (in pixels, relative
**             to origin of Stratagus's window - not the viewport
**             itself!).
**
**  @return    Y map tile coordinate.
*/
int Viewport2MapY(const Viewport* vp, int y)
{
	int r;

	r = (y - vp->Y + vp->MapY * TileSizeY + vp->OffsetY) / TileSizeY;
	return r < TheMap.Info.MapHeight ? r : TheMap.Info.MapHeight - 1;
}

/**
**  Convert a map tile X coordinate into a viewport x pixel coordinate.
**
**  @param vp  Viewport pointer.
**  @param x   The map tile's X coordinate.
**
**  @return    X screen coordinate in pixels (relative
**             to origin of Stratagus's window).
*/
int Map2ViewportX(const Viewport* vp, int x)
{
	return vp->X + (x - vp->MapX) * TileSizeX - vp->OffsetX;
}

/**
**  Convert a map tile Y coordinate into a viewport y pixel coordinate.
**
**  @param vp  Viewport pointer.
**  @param y   The map tile's Y coordinate.
**
**  @return    Y screen coordinate in pixels (relative
**             to origin of Stratagus's window).
*/
int Map2ViewportY(const Viewport* vp, int y)
{
	return vp->Y + (y - vp->MapY) * TileSizeY - vp->OffsetY;
}

/**
**  Change viewpoint of map viewport v to x,y.
**
**  @param vp       Viewport pointer.
**  @param x        X map tile position.
**  @param y        Y map tile position.
**  @param offsetx  X offset in tile.
**  @param offsety  Y offset in tile.
*/
void ViewportSetViewpoint(Viewport* vp, int x, int y, int offsetx, int offsety)
{
	Assert(vp);

	x = x * TileSizeX + offsetx;
	y = y * TileSizeY + offsety;
	if (x < 0) {
		x = 0;
	}
	if (y < 0) {
		y = 0;
	}
	if (x > TheMap.Info.MapWidth * TileSizeX - (vp->EndX - vp->X) - 1) {
		x = TheMap.Info.MapWidth * TileSizeX - (vp->EndX - vp->X) - 1;
	}
	if (y > TheMap.Info.MapHeight * TileSizeY - (vp->EndY - vp->Y) - 1) {
		y = TheMap.Info.MapHeight * TileSizeY - (vp->EndY - vp->Y) - 1;
	}
	vp->MapX = x / TileSizeX;
	vp->MapY = y / TileSizeY;
	vp->OffsetX = x % TileSizeX;
	vp->OffsetY = y % TileSizeY;
	vp->MapWidth = ((vp->EndX - vp->X) + vp->OffsetX - 1) / TileSizeX + 1;
	vp->MapHeight = ((vp->EndY - vp->Y) + vp->OffsetY - 1) / TileSizeY + 1;
}

/**
**  Center map viewport v on map tile (x,y).
**
**  @param vp  Viewport pointer.
**  @param x   X map tile position.
**  @param y   Y map tile position.
**  @param offsetx  X offset in tile.
**  @param offsety  Y offset in tile.
*/
void ViewportCenterViewpoint(Viewport* vp, int x, int y, int offsetx, int offsety)
{
	x = x * TileSizeX + offsetx - (vp->EndX - vp->X) / 2;
	y = y * TileSizeY + offsety - (vp->EndY - vp->Y) / 2;
	ViewportSetViewpoint(vp, x / TileSizeX, y / TileSizeY, x % TileSizeX, y % TileSizeY);
}

/**
**  Draw the map backgrounds.
**
**  @param vp  Viewport pointer.
**  @param x   Map viewpoint x position.
**  @param y   Map viewpoint y position.
**
** StephanR: variables explained below for screen:<PRE>
** *---------------------------------------*
** |                                       |
** |        *-----------------------*      |<-TheUi.MapY,dy (in pixels)
** |        |   |   |   |   |   |   |      |        |
** |        |   |   |   |   |   |   |      |        |
** |        |---+---+---+---+---+---|      |        |
** |        |   |   |   |   |   |   |      |        |MapHeight (in tiles)
** |        |   |   |   |   |   |   |      |        |
** |        |---+---+---+---+---+---|      |        |
** |        |   |   |   |   |   |   |      |        |
** |        |   |   |   |   |   |   |      |        |
** |        *-----------------------*      |<-ey,TheUI.MapEndY (in pixels)
** |                                       |
** |                                       |
** *---------------------------------------*
**          ^                       ^
**        dx|-----------------------|ex,TheUI.MapEndX (in pixels)
**            TheUI.MapX MapWidth (in tiles)
** (in pixels)
** </PRE>
*/
void DrawMapBackgroundInViewport(const Viewport* vp, int x, int y)
{
	int sx;
	int sy;
	int dx;
	int ex;
	int dy;
	int ey;
#ifdef TIMEIT
	u_int64_t sv = rdtsc();
	u_int64_t ev;
	static long mv = 9999999;
#endif

	ex = vp->EndX;
	sy = y * TheMap.Info.MapWidth;
	dy = vp->Y - vp->OffsetY;
	ey = vp->EndY;

	while (dy <= ey) {
		sx = x + sy;
		dx = vp->X - vp->OffsetX;
		while (dx <= ex) {
			if (ReplayRevealMap) {
				VideoDrawClip(TheMap.TileGraphic, TheMap.Fields[sx].Tile, dx, dy);
			} else {
				VideoDrawClip(TheMap.TileGraphic, TheMap.Fields[sx].SeenTile, dx, dy);
			}

			++sx;
			dx += TileSizeX;
		}
		sy += TheMap.Info.MapWidth;
		dy += TileSizeY;
	}

#ifdef TIMEIT
	ev = rdtsc();
	sx = (ev - sv);
	if (sx < mv) {
		mv = sx;
	}

	DebugPrint("%ld %ld %3ld\n" _C_ (long)sx _C_ mv _C_ (sx * 100) / mv);
#endif
}

/**
**  Initialize the fog of war.
**  Build tables, setup functions.
*/
void InitMap(void)
{
}

//@}