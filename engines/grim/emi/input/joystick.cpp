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

#include "engines/grim/emi/input/joystick.h"

namespace Grim {

static const float kMinAxisValue = 0.0000099999997f;

Joystick::Joystick(EmiEngine *engine) : joyId(0xFFFFFFFF), _engine(engine), _started(false), _opened(false), _anyRequested(false), _lastPollTime(0) {
	memset(_present, 0, sizeof(_present));
	memset(_hasPov, 0, sizeof(_hasPov));
	memset(_requested, 0, sizeof(_requested));
	memset(_numButtons, 0, sizeof(_numButtons));
	memset(_returnFlags, 0, sizeof(_returnFlags));
	memset(_axisRanges, 0, sizeof(_axisRanges));
	memset(_axisValues, 0, sizeof(_axisValues));
	memset(_controlStates, 0, sizeof(_controlStates));

	for (int i = 0; i < kAxesPerJoystick; i++)
		_deviceState.axisPosition[i] = kDeviceAxisCenter;

	_deviceState.buttons = 0;
	_deviceState.pov = kPovCentered;
}

bool Joystick::startup() {
	memset(_axisRanges, 0, sizeof(_axisRanges));
	memset(_axisValues, 0, sizeof(_axisValues));
	detect();
	resetControls();

	_started = true;

	return true;
}

void Joystick::shutdown() {
	_started = false;
}

bool Joystick::open() {
	_opened = true;

	return true;
}

bool Joystick::close() {
	if (!_opened)
		return false;

	_opened = false;

	return true;
}

void Joystick::resetControls() {
	for (int i = 0; i < kMaxJoysticks; i++)
		_returnFlags[i] = kDefaultReturnFlags;

	_anyRequested = false;

	memset(_requested, 0, sizeof(_requested));

	for (int i = 0; i < kNumAxes; i++)
		_axisRanges[i].flags &= ~(uint)kAxisEnabled;
}

void Joystick::update() {
	if (!_opened)
		return;

	uint now = g_system->getMillis();

	if (now == _lastPollTime)
		return;

	memset(_axisValues, 0, sizeof(_axisValues));

	if (_anyRequested)
		poll();

	_lastPollTime = now;
}

void Joystick::detect() {
	memset(_present, 0, sizeof(_present));
	memset(_numButtons, 0, sizeof(_numButtons));
	memset(_hasPov, 0, sizeof(_hasPov));

	uint numDevices = kNumDevices;

	if (numDevices > (uint)kMaxJoysticks)
		numDevices = kMaxJoysticks;

	int axisBase = 0;

	for (uint id = 0; id < numDevices; id++) {
		int base = axisBase;

		if ((int)joyId < 0)
			joyId = id;

		axisBase += kAxesPerJoystick;
		_present[id] = true;

		uint maxButtons = kDeviceMaxButtons;

		if (maxButtons >= 24)
			maxButtons = 24;

		_numButtons[id] = maxButtons;

		setAxisRange(base, kDeviceAxisMin, kDeviceAxisMax, 0.1f);
		setAxisRange(base + 1, kDeviceAxisMin, kDeviceAxisMax, 0.1f);

		if (kDeviceCaps & 0x1)
			setAxisRange(base + 2, kDeviceAxisMin, kDeviceAxisMax, 0.1f);

		if (kDeviceCaps & 0x2)
			setAxisRange(base + 3, kDeviceAxisMin, kDeviceAxisMax, 0.1f);

		if (kDeviceCaps & 0x4)
			setAxisRange(base + 4, kDeviceAxisMin, kDeviceAxisMax, 0.1f);

		if (kDeviceCaps & 0x8)
			setAxisRange(base + 5, kDeviceAxisMin, kDeviceAxisMax, 0.1f);

		if (kDeviceCaps & 0x10)
			_hasPov[id] = true;
	}
}

void Joystick::poll() {
	if ((int)joyId < 0)
		return;

	if (joyId >= (uint)kMaxJoysticks) {
		warning("Joystick::poll(): invalid joystick %u", joyId);

		return;
	}

	if (!_requested[joyId] || !_present[joyId])
		return;

	uint flags = _returnFlags[joyId];

	_axisValues[0] = (flags & kReturnX) ? _deviceState.axisPosition[0] : 0;
	_axisValues[1] = (flags & kReturnY) ? _deviceState.axisPosition[1] : 0;

	if (flags & kReturnZ)
		_axisValues[2] = _deviceState.axisPosition[2];

	if (flags & kReturnR)
		_axisValues[3] = _deviceState.axisPosition[3];

	if (flags & kReturnU)
		_axisValues[4] = _deviceState.axisPosition[4];

	if (flags & kReturnV)
		_axisValues[5] = _deviceState.axisPosition[5];

	uint buttons = 0;

	if (flags & kReturnButtons)
		buttons = _deviceState.buttons;

	uint numButtons = _numButtons[joyId];
	uint mask = 1;

	// The disasm reads one button more than the device has
	for (uint i = 0; i <= numButtons; i++) {
		_controlStates[kButtonBase + i] = (buttons & mask) != 0;
		mask <<= 1;
	}

	if (flags & kReturnPovContinuous) {
		uint pov = _deviceState.pov;
		int signedPov = (int)pov;

		_controlStates[kButtonBase + 0x14] = signedPov >= 22500 && signedPov <= 31500;
		_controlStates[kButtonBase + 0x15] = (signedPov >= 31500 || signedPov <= 4500) && (pov & 0x80000000) == 0 && pov != (uint)kPovCentered;
		_controlStates[kButtonBase + 0x16] = signedPov >= 4500 && signedPov <= 13500;
		_controlStates[kButtonBase + 0x17] = signedPov >= 13500 && signedPov <= 22500;
	}
}

void Joystick::setAxisRange(int axis, int min, int max, float deadZone) {
	int center = min + (max - min + 1) / 2;
	AxisRange *range = &_axisRanges[axis];

	range->flags |= kAxisHasRange;
	range->min = min;
	range->max = max;
	range->center = center;

	double halfRange = (double)(max - center);

	// The original could divide by zero here, let's avoid that...
	if (halfRange == 0.0) {
		warning("Joystick::setAxisRange(): the range of the axis %d is empty", axis);
		range->scale = 0.0f;
	} else {
		range->scale = (float)(1.0 / halfRange);
	}

	if (deadZone == 0.0f)
		range->deadZone = 0;
	else
		range->deadZone = (int)(halfRange * (double)deadZone);
}

bool Joystick::enableControl(uint code) {
	if (code >= (uint)kNumAxes)
		return false;

	uint flags = _axisRanges[code].flags;

	if (!(flags & kAxisHasRange))
		return false;

	_axisRanges[code].flags = flags | kAxisEnabled;

	requestAxis(code);

	return true;
}

void Joystick::requestAxis(uint axis) {
	if (axis > 0xB)
		return;

	if (joyId >= (uint)kMaxJoysticks) {
		warning("Joystick::requestAxis(): invalid joystick %u", joyId);

		return;
	}

	_anyRequested = true;
	_requested[joyId] = true;

	switch (axis) {
	case 0:
		_returnFlags[joyId] |= kReturnX;
		break;
	case 1:
		_returnFlags[joyId] |= kReturnY;
		break;
	case 2:
		_returnFlags[joyId] |= kReturnZ;
		break;
	case 3:
		_returnFlags[joyId] |= kReturnR;
		break;
	case 4:
		_returnFlags[joyId] |= kReturnU;
		break;
	case 5:
		_returnFlags[joyId] |= kReturnV;
		break;
	default:
		break;
	}
}

float Joystick::getAxis(int axis) {
	if (!_opened)
		return 0.0f;

	uint flags = _axisRanges[axis].flags;

	if (!(flags & kAxisEnabled))
		return 0.0f;

	int difference = _axisValues[axis] - _axisRanges[axis].center;

	if (difference == 0)
		return 0.0f;

	if (!(flags & kAxisNoDeadZone)) {
		int deadZone = _axisRanges[axis].deadZone;

		if (deadZone != 0) {
			int magnitude = difference < 0 ? -difference : difference;

			if (magnitude < deadZone)
				return 0.0f;
		}
	}

	double result = (double)difference * (double)_axisRanges[axis].scale;
	double absoluteResult = result < 0.0 ? -result : result;

	if (!(absoluteResult > (double)kMinAxisValue))
		return 0.0f;

	return (float)result;
}

bool Joystick::getControlState(int code) {
	if (!_opened)
		return false;

	return _controlStates[code] != 0;
}

void Joystick::setDeviceAxis(int axis, int position) {
	if (axis < 0 || axis >= kAxesPerJoystick)
		return;

	_deviceState.axisPosition[axis] = position + kDeviceAxisCenter;
}

void Joystick::setDeviceButton(int button, bool pressed) {
	if (button < 0 || button >= 32)
		return;

	if (pressed)
		_deviceState.buttons |= (1u << button);
	else
		_deviceState.buttons &= ~(1u << button);
}

} // End of namespace Grim
