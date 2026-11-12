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
#include "engines/grim/movie/sputsmush.h"

namespace Grim {

class GrimEngine;
class SmushDemoDriver;

// The movies of the demo: a 8-bit buffer filled by the player is converted to the colors
// of the target with a palette table when a frame is shown
class SputSmushDemo : public SputSmush, public SmushDemoVideo {
public:
	SputSmushDemo(Res *res, StdDisplay *stdDisplay, SputRender *sputRender, StdVBuffer *mainGfxBuffer, WuRegistry *wuRegistry);
	~SputSmushDemo() override;

	void setGrimEngine(GrimEngine *grim) { _grim = grim; }

	int getMovieTime() override;
	int initialize(SysStruct *hs) override;
	int startup() override;
	int shutdown() override;
	int terminate() override;
	int periodic() override;
	int setTarget(StdVBuffer *buffer) override;
	int startMovie(const char *name, int looping, int x, int y) override;
	int stopMovie() override;
	int getMovieLength(const char *name) override;
	int moviePlaying() override;
	int pause(int pause) override;
	void blitFrame() override;

	int videoStart(int width, int height, int depth) override;
	void videoStop() override;
	void videoLock(SmushDemoTarget *target, SmushDemoRect *rect) override;
	void videoUnlock() override;
	void videoSetPalette(const uint32 *palette, int start, int count) override;

private:
	GrimEngine *_grim;
	SmushDemoDriver *_driver;
	SmushDemoPlayer *_player;
	StdVBuffer *_movieBuffer;
	uint16 _paletteTable[256];
	byte _moviePalette[768];
	int _demoPlaying;
	int _demoPaused;
	int _finished;
};

} // End of namespace Grim

#endif
