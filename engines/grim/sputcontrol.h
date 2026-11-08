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

#ifndef GRIM_SPUTCONTROL_H
#define GRIM_SPUTCONTROL_H

#include "common/scummsys.h"

#include "engines/grim/std/std_sys.h"

namespace Grim {

class GrimEngine;
class RenderDroid;
class SputRoom;
class StdControl;

enum {
	kNumControlSymbols = 167,
	kMaxControlCallbacks = 8
};

struct ControlSymbol {
	uint flags;
	int code;
	int lastValue;
	const char *pName;
};

class SputControlCallback {
public:
	virtual ~SputControlCallback() {}

	virtual int controlCallback(float frameTime) = 0;
};

class SputControl : public SputControlCallback {
public:
	SputControl(RenderDroid *pRd, SputRoom *pSputRoom, StdControl *pStdControl);

	void setGrimEngine(GrimEngine *pGrim) { _pGrim = pGrim; }

	int controlCallback(float frameTime) override;
	void resetSymbols();
	void enableControl(uint index);
	void disableControl(uint index);
	int initialize(SysStruct *pHS);
	int open();
	int close();
	int addCallback(SputControlCallback *pCallback);
	void update(float frameTime);
	int terminate();
	int startup();
	int shutdown();

	ControlSymbol *getSymbols() { return _aControlSymbols; }
	int getNumSymbols() const { return kNumControlSymbols; }

private:
	int standardHandler();
	int changeFOV(double step);

	RenderDroid *_pRd;
	SputRoom *_pSputRoom;
	StdControl *_pStdControl;
	GrimEngine *_pGrim;
	SysStruct *_pHS;
	int _bInitialized;
	int _bStarted;
	int _bControlOpen;
	int _numCallbacks;
	SputControlCallback *_apCallbacks[kMaxControlCallbacks];
	ControlSymbol _aControlSymbols[kNumControlSymbols];
};

} // End of namespace Grim

#endif
