/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GRIM_SPUTSMUSH_DEMO_H
#define GRIM_SPUTSMUSH_DEMO_H

#include "engines/grim/movie/smush_anim.h"
#include "engines/grim/sputsmush.h"

namespace Grim {

class GrimEngine;
class SmushDemoDriver;

// The movies of the demo: a 8-bit buffer filled by the player is converted to the colors
// of the target with a palette table when a frame is shown
class SputSmushDemo : public SputSmush, public SmushDemoVideo {
public:
	SputSmushDemo(Res *pRes, StdDisplay *pStdDisplay, SputRender *pSputRender, StdVBuffer *pMainGfxBuffer, WuRegistry *pWuRegistry);
	~SputSmushDemo() override;

	void setGrimEngine(GrimEngine *pGrim) { _pGrim = pGrim; }

	int getMovieTime() override;
	int initialize(SysStruct *pHS) override;
	int startup() override;
	int shutdown() override;
	int terminate() override;
	int periodic() override;
	int setTarget(StdVBuffer *pBuffer) override;
	int startMovie(const char *name, int bLooping, int x, int y) override;
	int stopMovie() override;
	int getMovieLength(const char *name) override;
	int moviePlaying() override;
	int pause(int bPause) override;

	int videoStart(int width, int height, int depth) override;
	void videoStop() override;
	void videoLock(SmushDemoTarget *target, SmushDemoRect *rect) override;
	void videoUnlock() override;
	void videoSetPalette(const uint32 *palette, int start, int count) override;

private:
	void blitFrame();

	GrimEngine *_pGrim;
	SmushDemoDriver *_pDriver;
	SmushDemoPlayer *_pPlayer;
	StdVBuffer *_pMovieBuffer;
	uint16 _aPaletteTable[256];
	byte _aMoviePalette[768];
	int _bDemoPlaying;
	int _bDemoPaused;
	int _bFinished;
};

} // End of namespace Grim

#endif
