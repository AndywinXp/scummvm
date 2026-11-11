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

	static bool startup();
	static void shutdown();
	static bool open();
	static bool close();
	static void resetControls();
	static void update();
	static void detect();
	static void poll();
	static void setAxisRange(int axis, int min, int max, float deadZone);
	static bool enableControl(uint code);
	static void requestAxis(uint axis);
	static float getAxis(int axis);
	static bool getControlState(int code);

	static void setDeviceAxis(int axis, int position);
	static void setDeviceButton(int button, bool pressed);

	static uint joyId;

private:
	static bool _started;
	static bool _opened;
	static bool _anyRequested;
	static uint _lastPollTime;
	static bool _present[kMaxJoysticks];
	static bool _hasPov[kMaxJoysticks];
	static bool _requested[kMaxJoysticks];
	static uint _numButtons[kMaxJoysticks];
	static uint _returnFlags[kMaxJoysticks];
	static AxisRange _axisRanges[kNumAxes];
	static int _axisValues[kNumAxes];
	static byte _controlStates[kNumControlStates];
	static DeviceState _deviceState;
};

} // End of namespace Grim

#endif
