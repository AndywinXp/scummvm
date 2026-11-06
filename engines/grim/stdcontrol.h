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

#ifndef GRIM_STDCONTROL_H
#define GRIM_STDCONTROL_H

#include "common/scummsys.h"

namespace Grim {

struct StdControlAxis {
	int flags;
	int minVal;
	int maxVal;
	int center;
	int deadZone;
	float invRange;
};

struct StdControlKey {
	int state;
	int frameTime;
	int rawState;
	int changeTime;
	int lastPressTime;
	int pressCount;
	int bDoubleClick;
};

struct StdControlDeviceEvent {
	uint offset;
	int data;
	int timeStamp;
};

struct StdControlMouseState {
	int x;
	int y;
	int z;
	byte buttons[4];
};

struct StdControlJoyCaps {
	int bAvailable;
	int flags;
	int maxButtons;
	int axisMin[6];
	int axisMax[6];
};

struct StdControlJoyState {
	int result;
	int flags;
	int axisPos[6];
	int buttons;
	int pov;
};

struct StdControlInput {
	StdControlDeviceEvent keyboardEvents[32];
	int numKeyboardEvents;
	int bKeyboardOverflow;
	byte keyboardState[256];

	StdControlMouseState mouse;
	StdControlDeviceEvent mouseEvents[32];
	int numMouseEvents;
	int bMouseOverflow;

	int numJoysticks;
	StdControlJoyCaps joyCaps[2];
	StdControlJoyState joyState[2];
};

class StdControl {
public:
	enum {
		kNumAxes = 15,
		kNumKeys = 288
	};

	StdControl();

	StdControlInput *getInput() { return &_input; }
	int isStarted() const { return _bStartup; }
	int isOpen() const { return _bOpen; }

	int startup();
	void shutdown();
	int open();
	int close();
	void flush();
	void reset();
	int pressButton(uint button);
	void readControls();
	float readAxis(int axis);
	int readKey(int key, int *pPressCount, int *pDoubleClick);
	void setActivation(int bActive);

private:
	void initJoysticks();
	void initMouse();
	void setButtonPressed(uint button);
	void readKeyboard();
	void flushKeyboard();
	void readJoysticks();
	void readMouse();
	void flushMouse();
	void enableAxis(int axis, int minVal, int maxVal, float deadZone);
	void updateKey(StdControlKey *pKey, int down, int time);

	int _bStartup;
	int _bOpen;
	int _bControlsActive;
	int _bControlsIdle;
	int _bDisableKeyboard;
	int _bReadMouse;
	int _bReadJoysticks;
	int _abReadJoy[2];
	int _aReadJoyFlags[2];
	int _aJoyPresent[2];
	int _aJoyMaxButtons[2];
	int _aJoyHasPOV[2];
	int _curReadTime;
	int _lastReadTime;
	int _readDelta;
	float _deltaTime;
	float _deltaTimeMs;
	int _prevMouseX;
	int _prevMouseY;

	StdControlAxis _aAxes[kNumAxes];
	int _aAxisPos[kNumAxes];
	StdControlKey _aKeyInfo[kNumKeys];

	StdControlInput _input;
};

} // End of namespace Grim

#endif
