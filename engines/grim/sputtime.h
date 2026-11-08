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

#ifndef GRIM_SPUTTIME_H
#define GRIM_SPUTTIME_H

#include "common/scummsys.h"

#include "engines/grim/std/std_sys.h"

namespace Grim {

class SputTime {
public:
	explicit SputTime(bool isDemo);

	int getGameTime() const;
	int getFrameTime() const;
	float getFrameTimeFlex() const;
	uint advance();
	uint advanceManual(uint ticks);
	uint reset();
	int initialize(SysStruct *pHS);
	int terminate();
	int startup();
	void shutdown();

private:
	// The demo limits a frame to 500 ms instead of 300
	uint _maxFrameTime;
	SysStruct *_pHS;
	int _bInitialized;
	int _bStarted;
	uint _lastRealTime;
	int _gameTime;
	int _frameTime;
	float _frameTimeFlex;
};

} // End of namespace Grim

#endif
