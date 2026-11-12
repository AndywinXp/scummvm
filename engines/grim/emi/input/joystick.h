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

#ifndef GRIM_EMI_JOYSTICK_H
#define GRIM_EMI_JOYSTICK_H

#include "common/scummsys.h"

namespace Grim {

class EmiEngine;

class Joystick {
public:
	enum {
		kMaxJoysticks = 24,
		kAxesPerJoystick = 6,
		kNumAxes = 144,
		kButtonBase = 0x80,
		kNumControlStates = 0x99
	};

	enum {
		kAxisHasRange = 0x1,
		kAxisEnabled = 0x2,
		kAxisNoDeadZone = 0x8
	};

	enum {
		kReturnX = 0x1,
		kReturnY = 0x2,
		kReturnZ = 0x4,
		kReturnR = 0x8,
		kReturnU = 0x10,
		kReturnV = 0x20,
		kReturnButtons = 0x80,
		kReturnPovContinuous = 0x200,
		kDefaultReturnFlags = 0x680
	};

	enum {
		kNumDevices = 1,
		kDeviceMaxButtons = 24,
		kDeviceAxisMin = 0,
		kDeviceAxisMax = 65535,
		kDeviceCaps = 0xF,
		kDeviceAxisCenter = 32768,
		kPovCentered = 0xFFFF
	};

	struct AxisRange {
		uint flags;
		int min;
		int max;
		int center;
		int deadZone;
		float scale;
	};

	struct DeviceState {
		int axisPosition[kAxesPerJoystick];
		uint buttons;
		uint pov;
	};

	explicit Joystick(EmiEngine *engine);

	bool startup();
	void shutdown();
	bool open();
	bool close();
	void resetControls();
	void update();
	void detect();
	void poll();
	void setAxisRange(int axis, int min, int max, float deadZone);
	bool enableControl(uint code);
	void requestAxis(uint axis);
	float getAxis(int axis);
	bool getControlState(int code);

	void setDeviceAxis(int axis, int position);
	void setDeviceButton(int button, bool pressed);

	uint joyId;

private:
	EmiEngine *_engine;
	bool _started;
	bool _opened;
	bool _anyRequested;
	uint _lastPollTime;
	bool _present[kMaxJoysticks];
	bool _hasPov[kMaxJoysticks];
	bool _requested[kMaxJoysticks];
	uint _numButtons[kMaxJoysticks];
	uint _returnFlags[kMaxJoysticks];
	AxisRange _axisRanges[kNumAxes];
	int _axisValues[kNumAxes];
	byte _controlStates[kNumControlStates];
	DeviceState _deviceState;
};

} // End of namespace Grim

#endif
