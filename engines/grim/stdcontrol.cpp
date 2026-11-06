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

#include "common/system.h"
#include "common/textconsole.h"
#include "graphics/cursorman.h"

#include "engines/grim/stdcontrol.h"

namespace Grim {

StdControl::StdControl() : _bStartup(0), _bOpen(0), _bControlsActive(0), _bControlsIdle(0), _bDisableKeyboard(0),
		_bReadMouse(1), _bReadJoysticks(0), _curReadTime(0), _lastReadTime(0), _readDelta(0), _deltaTime(1.0f),
		_deltaTimeMs(1.0f), _prevMouseX(0), _prevMouseY(0) {
	memset(_abReadJoy, 0, sizeof(_abReadJoy));
	memset(_aReadJoyFlags, 0, sizeof(_aReadJoyFlags));
	memset(_aJoyPresent, 0, sizeof(_aJoyPresent));
	memset(_aJoyMaxButtons, 0, sizeof(_aJoyMaxButtons));
	memset(_aJoyHasPOV, 0, sizeof(_aJoyHasPOV));
	memset(_aAxes, 0, sizeof(_aAxes));
	memset(_aAxisPos, 0, sizeof(_aAxisPos));
	memset(_aKeyInfo, 0, sizeof(_aKeyInfo));
	memset(&_input, 0, sizeof(_input));
}

int StdControl::startup() {
	memset(_aAxes, 0, sizeof(_aAxes));
	memset(_aAxisPos, 0, sizeof(_aAxisPos));

	initJoysticks();
	initMouse();
	reset();

	_bStartup = 1;

	return 1;
}

void StdControl::shutdown() {
	_bStartup = 0;
}

int StdControl::open() {
	_bOpen = 1;
	setActivation(1);

	return 1;
}

int StdControl::close() {
	if (!_bOpen)
		return 0;

	setActivation(0);
	_bOpen = 0;

	return 1;
}

void StdControl::flush() {
	_curReadTime = g_system->getMillis();

	flushKeyboard();
	flushMouse();
}

void StdControl::reset() {
	_bReadMouse = 0;
	_bReadJoysticks = 0;
	_abReadJoy[0] = 0;
	_aReadJoyFlags[0] = 1664;
	_abReadJoy[1] = 0;
	_aReadJoyFlags[1] = 1664;

	for (int i = 0; i < kNumAxes; i++)
		_aAxes[i].flags &= ~2;
}

int StdControl::pressButton(uint button) {
	if (button >= kNumAxes)
		return 0;

	if ((_aAxes[button].flags & 1) == 0)
		return 0;

	_aAxes[button].flags |= 2;
	setButtonPressed(button);

	return 1;
}

void StdControl::readControls() {
	if (!_bControlsActive)
		return;

	_bControlsIdle = 1;
	_curReadTime = g_system->getMillis();

	if (_curReadTime != _lastReadTime) {
		uint delta = (uint)_curReadTime - (uint)_lastReadTime;

		_readDelta = delta;

		double invDelta = 1.0 / (double)delta;

		memset(_aAxisPos, 0, sizeof(_aAxisPos));
		_deltaTime = (float)invDelta;
		_deltaTimeMs = (float)(invDelta * 1000.0);

		readKeyboard();

		if (_bReadJoysticks)
			readJoysticks();

		readMouse();

		_lastReadTime = _curReadTime;
	}
}

float StdControl::readAxis(int axis) {
	if (!_bControlsActive)
		return 0.0f;

	if (axis < 0 || axis >= kNumAxes) {
		warning("StdControl::readAxis(): axis %d out of range", axis);
		return 0.0f;
	}

	const StdControlAxis *pAxis = &_aAxes[axis];

	if ((pAxis->flags & 2) == 0)
		return 0.0f;

	int pos = _aAxisPos[axis] - pAxis->center;

	if (pos == 0)
		return 0.0f;

	if ((pAxis->flags & 8) == 0 && pAxis->deadZone) {
		int magnitude = pos;

		if (magnitude < 0)
			magnitude = -magnitude;

		if (magnitude < pAxis->deadZone)
			return 0.0f;
	}

	double result = (double)pAxis->invRange * (double)pos;

	if (fabs(result) <= (double)0.00001f)
		result = 0.0;

	if (_bControlsIdle && result != 0.0)
		_bControlsIdle = 0;

	return (float)result;
}

int StdControl::readKey(int key, int *pPressCount, int *pDoubleClick) {
	if (!_bControlsActive || _bDisableKeyboard || key < 0 || key >= kNumKeys) {
		if (key < 0 || key >= kNumKeys)
			warning("StdControl::readKey(): key %d out of range", key);

		if (pPressCount)
			*pPressCount = 0;

		if (pDoubleClick)
			*pDoubleClick = 0;

		return 0;
	}

	const StdControlKey *pKey = &_aKeyInfo[key];

	if (_bControlsIdle && pKey->state)
		_bControlsIdle = 0;

	if (pPressCount)
		*pPressCount = pKey->pressCount;

	if (pDoubleClick)
		*pDoubleClick = pKey->bDoubleClick;

	return pKey->state;
}

void StdControl::setActivation(int bActive) {
	if (!_bOpen)
		return;

	if (bActive) {
		if (_bReadMouse)
			CursorMan.showMouse(false);

		_bControlsActive = 1;
	} else {
		CursorMan.showMouse(true);
		_bControlsActive = 0;
	}
}

void StdControl::initJoysticks() {
	for (int i = 0; i < 2; i++) {
		_aJoyPresent[i] = 0;
		_aJoyMaxButtons[i] = 0;
		_aJoyHasPOV[i] = 0;
	}

	int numDevs = _input.numJoysticks;

	if (numDevs >= 2)
		numDevs = 2;

	for (int joy = 0; joy < numDevs; joy++) {
		const StdControlJoyCaps *pCaps = &_input.joyCaps[joy];
		int axisBase = 6 * joy;

		if (!pCaps->bAvailable)
			continue;

		_aJoyPresent[joy] = 1;

		int maxButtons = pCaps->maxButtons;

		if (maxButtons >= 14)
			maxButtons = 14;

		_aJoyMaxButtons[joy] = maxButtons;

		enableAxis(axisBase, pCaps->axisMin[0], pCaps->axisMax[0], 0.1f);
		enableAxis(axisBase + 1, pCaps->axisMin[1], pCaps->axisMax[1], 0.1f);

		if (pCaps->flags & 1)
			enableAxis(axisBase + 2, pCaps->axisMin[2], pCaps->axisMax[2], 0.1f);

		if (pCaps->flags & 2)
			enableAxis(axisBase + 3, pCaps->axisMin[3], pCaps->axisMax[3], 0.1f);

		if (pCaps->flags & 4)
			enableAxis(axisBase + 4, pCaps->axisMin[4], pCaps->axisMax[4], 0.1f);

		if (pCaps->flags & 8)
			enableAxis(axisBase + 5, pCaps->axisMin[5], pCaps->axisMax[5], 0.1f);

		if (pCaps->flags & 0x10)
			_aJoyHasPOV[joy] = 1;
	}
}

void StdControl::initMouse() {
	enableAxis(12, -250, 250, 0.0f);
	enableAxis(13, -200, 200, 0.0f);
	enableAxis(14, -20, 20, 0.0f);
}

void StdControl::setButtonPressed(uint button) {
	if (button >= 12 && button <= 14) {
		_bReadMouse = 1;
	} else if (button <= 11) {
		_bReadJoysticks = 1;

		int joy = button > 5 ? 1 : 0;

		_abReadJoy[joy] = 1;

		switch (button - 6 * joy) {
		case 0:
			_aReadJoyFlags[joy] |= 1;
			break;
		case 1:
			_aReadJoyFlags[joy] |= 2;
			break;
		case 2:
			_aReadJoyFlags[joy] |= 4;
			break;
		case 3:
			_aReadJoyFlags[joy] |= 8;
			break;
		case 4:
			_aReadJoyFlags[joy] |= 0x10;
			break;
		case 5:
			_aReadJoyFlags[joy] |= 0x20;
			break;
		default:
			break;
		}
	}
}

void StdControl::readKeyboard() {
	if (_bDisableKeyboard)
		return;

	int now = _curReadTime;

	for (int i = 0; i < kNumKeys; i++) {
		StdControlKey *pKey = &_aKeyInfo[i];

		pKey->frameTime = now;
		pKey->state = pKey->rawState;
		pKey->pressCount = 0;
		pKey->bDoubleClick = 0;
	}

	int numEvents = _input.numKeyboardEvents;

	if (numEvents > 32) {
		warning("StdControl::readKeyboard(): too many keyboard events");
		numEvents = 32;
	}

	for (int i = 0; i < numEvents; i++) {
		const StdControlDeviceEvent *pEvent = &_input.keyboardEvents[i];

		if (pEvent->offset >= 256) {
			warning("StdControl::readKeyboard(): key %u out of range", pEvent->offset);
			continue;
		}

		updateKey(&_aKeyInfo[pEvent->offset], pEvent->data, pEvent->timeStamp);
	}

	int bOverflow = _input.bKeyboardOverflow;

	_input.numKeyboardEvents = 0;
	_input.bKeyboardOverflow = 0;

	if (bOverflow) {
		now = _curReadTime;

		for (int i = 0; i < 256; i++)
			updateKey(&_aKeyInfo[i], _input.keyboardState[i], now);
	}
}

void StdControl::flushKeyboard() {
	_input.numKeyboardEvents = 0;
	_input.bKeyboardOverflow = 0;

	int now = _curReadTime;

	for (int i = 0; i < 256; i++) {
		StdControlKey *pKey = &_aKeyInfo[i];

		if (pKey->rawState) {
			pKey->rawState = 0;
			pKey->changeTime = now;
		}
	}
}

void StdControl::readJoysticks() {
	int buttonMask = 1;

	for (int joy = 0; joy < 2; joy++) {
		int keyBase = 256 + 14 * joy;

		if (!_abReadJoy[joy] || !_aJoyPresent[joy])
			continue;

		const StdControlJoyState *pState = &_input.joyState[joy];

		if (pState->result)
			break;

		int *pAxisPos = &_aAxisPos[6 * joy];
		int gotFlags = pState->flags;

		pAxisPos[0] = pState->axisPos[0];
		pAxisPos[1] = pState->axisPos[1];

		if (gotFlags & 4)
			pAxisPos[2] = pState->axisPos[2];

		if (gotFlags & 8)
			pAxisPos[3] = pState->axisPos[3];

		if (gotFlags & 0x10)
			pAxisPos[4] = pState->axisPos[4];

		if (gotFlags & 0x20)
			pAxisPos[5] = pState->axisPos[5];

		int now = _curReadTime;

		for (int button = 0; button < 10; button++) {
			updateKey(&_aKeyInfo[keyBase + button], buttonMask & pState->buttons, now);
			buttonMask *= 2;
		}

		if (pState->flags & 0x200) {
			int pov = pState->pov;
			int povA = pov >= 22500 && pov <= 31500;
			int povB = (pov >= 31500 || pov <= 4500) && pov >= 0 && pov != 0xFFFF;
			int povC = pov >= 4500 && pov <= 13500;
			int povD = pov >= 13500 && pov <= 22500;

			updateKey(&_aKeyInfo[keyBase + 10], povA, now);
			updateKey(&_aKeyInfo[keyBase + 11], povB, now);
			updateKey(&_aKeyInfo[keyBase + 12], povC, now);
			updateKey(&_aKeyInfo[keyBase + 13], povD, now);
		}
	}
}

void StdControl::readMouse() {
	if (!_bReadMouse)
		return;

	_aAxisPos[14] = _input.mouse.z;
	_aAxisPos[12] = _input.mouse.x;
	_aAxisPos[13] = _input.mouse.y;

	if ((uint)_readDelta < 0x19) {
		_aAxisPos[12] = (_input.mouse.x + _prevMouseX) >> 1;
		_aAxisPos[13] = (_input.mouse.y + _prevMouseY) >> 1;
	}

	_prevMouseX = _input.mouse.x;
	_prevMouseY = _input.mouse.y;
	_input.mouse.x = 0;
	_input.mouse.y = 0;
	_input.mouse.z = 0;

	int numEvents = _input.numMouseEvents;

	if (numEvents > 32) {
		warning("StdControl::readMouse(): too many mouse events");
		numEvents = 32;
	}

	for (int i = 0; i < numEvents; i++) {
		const StdControlDeviceEvent *pEvent = &_input.mouseEvents[i];

		if (pEvent->offset < 12 || pEvent->offset > 15)
			continue;

		updateKey(&_aKeyInfo[pEvent->offset + 272], pEvent->data, pEvent->timeStamp);
	}

	int bOverflow = _input.bMouseOverflow;

	_input.numMouseEvents = 0;
	_input.bMouseOverflow = 0;

	if (bOverflow) {
		int now = _curReadTime;

		for (int i = 0; i < 4; i++)
			updateKey(&_aKeyInfo[284 + i], _input.mouse.buttons[i], now);
	}
}

void StdControl::flushMouse() {
	if (!_bReadMouse)
		return;

	_input.mouse.x = 0;
	_input.mouse.y = 0;
	_input.mouse.z = 0;
	_input.numMouseEvents = 0;
	_input.bMouseOverflow = 0;

	int now = _curReadTime;

	for (int i = 0; i < 4; i++)
		updateKey(&_aKeyInfo[284 + i], _input.mouse.buttons[i], now);
}

void StdControl::enableAxis(int axis, int minVal, int maxVal, float deadZone) {
	int center = minVal + (maxVal - minVal + 1) / 2;
	StdControlAxis *pAxis = &_aAxes[axis];

	pAxis->flags |= 1;
	pAxis->minVal = minVal;
	pAxis->maxVal = maxVal;
	pAxis->center = center;

	double halfRange = (double)(maxVal - center);

	pAxis->invRange = (float)(1.0 / halfRange);

	if (deadZone == 0.0f)
		pAxis->deadZone = 0;
	else
		pAxis->deadZone = (int)((double)deadZone * halfRange);
}

void StdControl::updateKey(StdControlKey *pKey, int down, int time) {
	if (down != pKey->rawState) {
		if (down && (uint)time - (uint)pKey->lastPressTime < 200)
			pKey->bDoubleClick = 1;

		pKey->rawState = down;
		pKey->changeTime = time;

		if (!down)
			return;

		pKey->pressCount++;
		pKey->state = down;
	} else if (!down) {
		return;
	}

	pKey->lastPressTime = time;
}

} // End of namespace Grim
