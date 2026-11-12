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

#include "common/textconsole.h"

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/eventgenerator.h"
#include "engines/grim/emi/joystick.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/profilertimer.h"
#include "engines/grim/emi/sputcontrol_v2.h"

namespace Grim {

const KeyBinding SputControl_v2::kDefaultBinding = { 0, 3, 0 };

const ControlEntry SputControl_v2::kPcControlTable[SputControl_v2::kNumControls] = {
	{ 4, 0x1B, 0, 0.0f, "KEY_ESCAPE" },
	{ 4, 0x31, 0, 0.0f, "KEY_1" },
	{ 4, 0x32, 0, 0.0f, "KEY_2" },
	{ 4, 0x33, 0, 0.0f, "KEY_3" },
	{ 4, 0x34, 0, 0.0f, "KEY_4" },
	{ 4, 0x35, 0, 0.0f, "KEY_5" },
	{ 4, 0x36, 0, 0.0f, "KEY_6" },
	{ 4, 0x37, 0, 0.0f, "KEY_7" },
	{ 4, 0x38, 0, 0.0f, "KEY_8" },
	{ 4, 0x39, 0, 0.0f, "KEY_9" },
	{ 4, 0x30, 0, 0.0f, "KEY_0" },
	{ 4, 0xBD, 0, 0.0f, "KEY_MINUS" },
	{ 4, 0xBB, 0, 0.0f, "KEY_EQUAL" },
	{ 4, 0x08, 0, 0.0f, "KEY_BACKSPACE" },
	{ 4, 0x09, 0, 0.0f, "KEY_TAB" },
	{ 4, 0xC0, 0, 0.0f, "KEY_TILDE" },
	{ 4, 0x51, 0, 0.0f, "KEY_Q" },
	{ 4, 0x57, 0, 0.0f, "KEY_W" },
	{ 4, 0x45, 0, 0.0f, "KEY_E" },
	{ 4, 0x52, 0, 0.0f, "KEY_R" },
	{ 4, 0x54, 0, 0.0f, "KEY_T" },
	{ 4, 0x59, 0, 0.0f, "KEY_Y" },
	{ 4, 0x55, 0, 0.0f, "KEY_U" },
	{ 4, 0x49, 0, 0.0f, "KEY_I" },
	{ 4, 0x4F, 0, 0.0f, "KEY_O" },
	{ 4, 0x50, 0, 0.0f, "KEY_P" },
	{ 4, 0xDB, 0, 0.0f, "KEY_LEFTBRACKET" },
	{ 4, 0xDD, 0, 0.0f, "KEY_RIGHTBRACKET" },
	{ 4, 0xDC, 0, 0.0f, "KEY_BACKSLASH" },
	{ 4, 0x41, 0, 0.0f, "KEY_A" },
	{ 4, 0x53, 0, 0.0f, "KEY_S" },
	{ 4, 0x44, 0, 0.0f, "KEY_D" },
	{ 4, 0x46, 0, 0.0f, "KEY_F" },
	{ 4, 0x47, 0, 0.0f, "KEY_G" },
	{ 4, 0x48, 0, 0.0f, "KEY_H" },
	{ 4, 0x4A, 0, 0.0f, "KEY_J" },
	{ 4, 0x4B, 0, 0.0f, "KEY_K" },
	{ 4, 0x4C, 0, 0.0f, "KEY_L" },
	{ 4, 0xBA, 0, 0.0f, "KEY_COLON" },
	{ 4, 0xDE, 0, 0.0f, "KEY_QUOTE" },
	{ 4, 0x0D, 0, 0.0f, "KEY_RETURN" },
	{ 4, 0x5A, 0, 0.0f, "KEY_Z" },
	{ 4, 0x58, 0, 0.0f, "KEY_X" },
	{ 4, 0x43, 0, 0.0f, "KEY_C" },
	{ 4, 0x56, 0, 0.0f, "KEY_V" },
	{ 4, 0x42, 0, 0.0f, "KEY_B" },
	{ 4, 0x4E, 0, 0.0f, "KEY_N" },
	{ 4, 0x4D, 0, 0.0f, "KEY_M" },
	{ 4, 0xBC, 0, 0.0f, "KEY_COMMA" },
	{ 4, 0xBE, 0, 0.0f, "KEY_PERIOD" },
	{ 4, 0xBF, 0, 0.0f, "KEY_SLASH" },
	{ 4, 0x10, 0, 0.0f, "KEY_SHIFT" },
	{ 4, 0x11, 0, 0.0f, "KEY_CONTROL" },
	{ 4, 0x12, 0, 0.0f, "KEY_ALT" },
	{ 4, 0x20, 0, 0.0f, "KEY_SPACE" },
	{ 4, 0x70, 0, 0.0f, "KEY_F1" },
	{ 4, 0x71, 0, 0.0f, "KEY_F2" },
	{ 4, 0x72, 0, 0.0f, "KEY_F3" },
	{ 4, 0x73, 0, 0.0f, "KEY_F4" },
	{ 4, 0x74, 0, 0.0f, "KEY_F5" },
	{ 4, 0x75, 0, 0.0f, "KEY_F6" },
	{ 4, 0x76, 0, 0.0f, "KEY_F7" },
	{ 4, 0x77, 0, 0.0f, "KEY_F8" },
	{ 4, 0x78, 0, 0.0f, "KEY_F9" },
	{ 4, 0x79, 0, 0.0f, "KEY_F10" },
	{ 4, 0x7A, 0, 0.0f, "KEY_F11" },
	{ 4, 0x7B, 0, 0.0f, "KEY_F12" },
	{ 4, 0x14, 0, 0.0f, "KEY_CAPSLOCK" },
	{ 4, 0xA0, 0, 0.0f, "KEY_NUMLOCK" },
	{ 4, 0xA1, 0, 0.0f, "KEY_SCROLLLOCK" },
	{ 4, 0x13, 0, 0.0f, "KEY_PAUSE" },
	{ 4, 0x60, 0, 0.0f, "KEY_NP0" },
	{ 4, 0x61, 0, 0.0f, "KEY_NP1" },
	{ 4, 0x62, 0, 0.0f, "KEY_NP2" },
	{ 4, 0x63, 0, 0.0f, "KEY_NP3" },
	{ 4, 0x64, 0, 0.0f, "KEY_NP4" },
	{ 4, 0x65, 0, 0.0f, "KEY_NP5" },
	{ 4, 0x66, 0, 0.0f, "KEY_NP6" },
	{ 4, 0x67, 0, 0.0f, "KEY_NP7" },
	{ 4, 0x68, 0, 0.0f, "KEY_NP8" },
	{ 4, 0x69, 0, 0.0f, "KEY_NP9" },
	{ 4, 0x6E, 0, 0.0f, "KEY_DECIMAL" },
	{ 4, 0x100, 0, 0.0f, "KEY_NPENTER" },
	{ 4, 0x6B, 0, 0.0f, "KEY_ADD" },
	{ 4, 0x6D, 0, 0.0f, "KEY_SUBTRACT" },
	{ 4, 0x6A, 0, 0.0f, "KEY_MULTIPLY" },
	{ 4, 0x6F, 0, 0.0f, "KEY_DIVIDE" },
	{ 4, 0x26, 0, 0.0f, "KEY_UP" },
	{ 4, 0x25, 0, 0.0f, "KEY_LEFT" },
	{ 4, 0x27, 0, 0.0f, "KEY_RIGHT" },
	{ 4, 0x28, 0, 0.0f, "KEY_DOWN" },
	{ 4, 0x0C, 0, 0.0f, "KEY_CLEAR" },
	{ 4, 0x2D, 0, 0.0f, "KEY_INSERT" },
	{ 4, 0x24, 0, 0.0f, "KEY_HOME" },
	{ 4, 0x21, 0, 0.0f, "KEY_PAGEUP" },
	{ 4, 0x22, 0, 0.0f, "KEY_PAGEDOWN" },
	{ 4, 0x23, 0, 0.0f, "KEY_END" },
	{ 4, 0x2E, 0, 0.0f, "KEY_DELETE" },
	{ 4, 0x5B, 0, 0.0f, "KEY_LWIN" },
	{ 4, 0x5C, 0, 0.0f, "KEY_RWIN" },
	{ 4, 0x5D, 0, 0.0f, "KEY_APPS" },
	{ 8, 0x80, 0, 0.0f, "KEY_JOY1_B1" },
	{ 8, 0x81, 0, 0.0f, "KEY_JOY1_B2" },
	{ 8, 0x82, 0, 0.0f, "KEY_JOY1_B3" },
	{ 8, 0x83, 0, 0.0f, "KEY_JOY1_B4" },
	{ 8, 0x84, 0, 0.0f, "KEY_JOY1_B5" },
	{ 8, 0x85, 0, 0.0f, "KEY_JOY1_B6" },
	{ 8, 0x86, 0, 0.0f, "KEY_JOY1_B7" },
	{ 8, 0x87, 0, 0.0f, "KEY_JOY1_B8" },
	{ 8, 0x88, 0, 0.0f, "KEY_JOY1_B9" },
	{ 8, 0x89, 0, 0.0f, "KEY_JOY1_B10" },
	{ 8, 0x8A, 0, 0.0f, "KEY_JOY1_B11" },
	{ 8, 0x8B, 0, 0.0f, "KEY_JOY1_B12" },
	{ 8, 0x8C, 0, 0.0f, "KEY_JOY1_B13" },
	{ 8, 0x8D, 0, 0.0f, "KEY_JOY1_B14" },
	{ 8, 0x8E, 0, 0.0f, "KEY_JOY1_B15" },
	{ 8, 0x8F, 0, 0.0f, "KEY_JOY1_B16" },
	{ 8, 0x90, 0, 0.0f, "KEY_JOY1_B17" },
	{ 8, 0x91, 0, 0.0f, "KEY_JOY1_B18" },
	{ 8, 0x92, 0, 0.0f, "KEY_JOY1_B19" },
	{ 8, 0x93, 0, 0.0f, "KEY_JOY1_B20" },
	{ 8, 0x95, 0, 0.0f, "KEY_JOY1_HUP" },
	{ 8, 0x97, 0, 0.0f, "KEY_JOY1_HDOWN" },
	{ 8, 0x94, 0, 0.0f, "KEY_JOY1_HLEFT" },
	{ 8, 0x96, 0, 0.0f, "KEY_JOY1_HRIGHT" },
	{ 2, 0x0, 0, 0.0f, "AXIS_JOY1_X" },
	{ 2, 0x1, 0, 0.0f, "AXIS_JOY1_Y" },
	{ 2, 0x2, 0, 0.0f, "AXIS_JOY1_Z" },
	{ 2, 0x3, 0, 0.0f, "AXIS_JOY1_R" },
	{ 2, 0x4, 0, 0.0f, "AXIS_JOY1_U" },
	{ 2, 0x5, 0, 0.0f, "AXIS_JOY1_V" },
	{ 2, 0x6, 0, 0.0f, "AXIS_JOY2_X" },
	{ 2, 0x7, 0, 0.0f, "AXIS_JOY2_Y" },
	{ 2, 0x8, 0, 0.0f, "AXIS_JOY2_Z" },
	{ 2, 0x9, 0, 0.0f, "AXIS_JOY2_R" },
	{ 2, 0xA, 0, 0.0f, "AXIS_JOY2_U" },
	{ 2, 0xB, 0, 0.0f, "AXIS_JOY2_V" }
};

const ControlEntry SputControl_v2::kPs2ControlTable[SputControl_v2::kNumControls] = {
	{ 4, 0x1B, 0, 0.0f, "KEY_ESCAPE" },
	{ 4, 0x31, 0, 0.0f, "KEY_1" },
	{ 4, 0x32, 0, 0.0f, "KEY_2" },
	{ 4, 0x33, 0, 0.0f, "KEY_3" },
	{ 4, 0x34, 0, 0.0f, "KEY_4" },
	{ 4, 0x35, 0, 0.0f, "KEY_5" },
	{ 4, 0x36, 0, 0.0f, "KEY_6" },
	{ 4, 0x37, 0, 0.0f, "KEY_7" },
	{ 4, 0x38, 0, 0.0f, "KEY_8" },
	{ 4, 0x39, 0, 0.0f, "KEY_9" },
	{ 4, 0x30, 0, 0.0f, "KEY_0" },
	{ 4, 0xBD, 0, 0.0f, "KEY_MINUS" },
	{ 4, 0xBB, 0, 0.0f, "KEY_EQUAL" },
	{ 4, 0x8, 0, 0.0f, "KEY_BACKSPACE" },
	{ 4, 0x9, 0, 0.0f, "KEY_TAB" },
	{ 4, 0xC0, 0, 0.0f, "KEY_TILDE" },
	{ 4, 0x51, 0, 0.0f, "KEY_Q" },
	{ 4, 0x57, 0, 0.0f, "KEY_W" },
	{ 4, 0x45, 0, 0.0f, "KEY_E" },
	{ 4, 0x52, 0, 0.0f, "KEY_R" },
	{ 4, 0x54, 0, 0.0f, "KEY_T" },
	{ 4, 0x59, 0, 0.0f, "KEY_Y" },
	{ 4, 0x55, 0, 0.0f, "KEY_U" },
	{ 4, 0x49, 0, 0.0f, "KEY_I" },
	{ 4, 0x4F, 0, 0.0f, "KEY_O" },
	{ 4, 0x50, 0, 0.0f, "KEY_P" },
	{ 4, 0xDB, 0, 0.0f, "KEY_LEFTBRACKET" },
	{ 4, 0xDD, 0, 0.0f, "KEY_RIGHTBRACKET" },
	{ 4, 0xDC, 0, 0.0f, "KEY_BACKSLASH" },
	{ 4, 0x41, 0, 0.0f, "KEY_A" },
	{ 4, 0x53, 0, 0.0f, "KEY_S" },
	{ 4, 0x44, 0, 0.0f, "KEY_D" },
	{ 4, 0x46, 0, 0.0f, "KEY_F" },
	{ 4, 0x47, 0, 0.0f, "KEY_G" },
	{ 4, 0x48, 0, 0.0f, "KEY_H" },
	{ 4, 0x4A, 0, 0.0f, "KEY_J" },
	{ 4, 0x4B, 0, 0.0f, "KEY_K" },
	{ 4, 0x4C, 0, 0.0f, "KEY_L" },
	{ 4, 0xBA, 0, 0.0f, "KEY_COLON" },
	{ 4, 0xDE, 0, 0.0f, "KEY_QUOTE" },
	{ 4, 0xD, 0, 0.0f, "KEY_RETURN" },
	{ 4, 0x5A, 0, 0.0f, "KEY_Z" },
	{ 4, 0x58, 0, 0.0f, "KEY_X" },
	{ 4, 0x43, 0, 0.0f, "KEY_C" },
	{ 4, 0x56, 0, 0.0f, "KEY_V" },
	{ 4, 0x42, 0, 0.0f, "KEY_B" },
	{ 4, 0x4E, 0, 0.0f, "KEY_N" },
	{ 4, 0x4D, 0, 0.0f, "KEY_M" },
	{ 4, 0xBC, 0, 0.0f, "KEY_COMMA" },
	{ 4, 0xBE, 0, 0.0f, "KEY_PERIOD" },
	{ 4, 0xBF, 0, 0.0f, "KEY_SLASH" },
	{ 4, 0x10, 0, 0.0f, "KEY_SHIFT" },
	{ 4, 0x11, 0, 0.0f, "KEY_CONTROL" },
	{ 4, 0x12, 0, 0.0f, "KEY_ALT" },
	{ 4, 0x20, 0, 0.0f, "KEY_SPACE" },
	{ 4, 0x70, 0, 0.0f, "KEY_F1" },
	{ 4, 0x71, 0, 0.0f, "KEY_F2" },
	{ 4, 0x72, 0, 0.0f, "KEY_F3" },
	{ 4, 0x73, 0, 0.0f, "KEY_F4" },
	{ 4, 0x74, 0, 0.0f, "KEY_F5" },
	{ 4, 0x75, 0, 0.0f, "KEY_F6" },
	{ 4, 0x76, 0, 0.0f, "KEY_F7" },
	{ 4, 0x77, 0, 0.0f, "KEY_F8" },
	{ 4, 0x78, 0, 0.0f, "KEY_F9" },
	{ 4, 0x79, 0, 0.0f, "KEY_F10" },
	{ 4, 0x7A, 0, 0.0f, "KEY_F11" },
	{ 4, 0x7B, 0, 0.0f, "KEY_F12" },
	{ 4, 0x14, 0, 0.0f, "KEY_CAPSLOCK" },
	{ 4, 0xA0, 0, 0.0f, "KEY_NUMLOCK" },
	{ 4, 0xA1, 0, 0.0f, "KEY_SCROLLLOCK" },
	{ 4, 0x13, 0, 0.0f, "KEY_PAUSE" },
	{ 4, 0x60, 0, 0.0f, "KEY_NP0" },
	{ 4, 0x61, 0, 0.0f, "KEY_NP1" },
	{ 4, 0x62, 0, 0.0f, "KEY_NP2" },
	{ 4, 0x63, 0, 0.0f, "KEY_NP3" },
	{ 4, 0x64, 0, 0.0f, "KEY_NP4" },
	{ 4, 0x65, 0, 0.0f, "KEY_NP5" },
	{ 4, 0x66, 0, 0.0f, "KEY_NP6" },
	{ 4, 0x67, 0, 0.0f, "KEY_NP7" },
	{ 4, 0x68, 0, 0.0f, "KEY_NP8" },
	{ 4, 0x69, 0, 0.0f, "KEY_NP9" },
	{ 4, 0x6E, 0, 0.0f, "KEY_DECIMAL" },
	{ 4, 0x100, 0, 0.0f, "KEY_NPENTER" },
	{ 4, 0x6B, 0, 0.0f, "KEY_ADD" },
	{ 4, 0x6D, 0, 0.0f, "KEY_SUBTRACT" },
	{ 4, 0x6A, 0, 0.0f, "KEY_MULTIPLY" },
	{ 4, 0x6F, 0, 0.0f, "KEY_DIVIDE" },
	{ 4, 0x26, 0, 0.0f, "KEY_UP" },
	{ 4, 0x25, 0, 0.0f, "KEY_LEFT" },
	{ 4, 0x27, 0, 0.0f, "KEY_RIGHT" },
	{ 4, 0x28, 0, 0.0f, "KEY_DOWN" },
	{ 4, 0x2D, 0, 0.0f, "KEY_INSERT" },
	{ 4, 0x24, 0, 0.0f, "KEY_HOME" },
	{ 4, 0x21, 0, 0.0f, "KEY_PAGEUP" },
	{ 4, 0x22, 0, 0.0f, "KEY_PAGEDOWN" },
	{ 4, 0x23, 0, 0.0f, "KEY_END" },
	{ 4, 0x2E, 0, 0.0f, "KEY_DELETE" },
	{ 8, 0x80, 0, 0.0f, "KEY_JOY1_SQUARE" },
	{ 8, 0x81, 0, 0.0f, "KEY_JOY1_TRIANGLE" },
	{ 8, 0x82, 0, 0.0f, "KEY_JOY1_CIRCLE" },
	{ 8, 0x83, 0, 0.0f, "KEY_JOY1_X" },
	{ 8, 0x84, 0, 0.0f, "KEY_JOY1_R1" },
	{ 8, 0x85, 0, 0.0f, "KEY_JOY1_L1" },
	{ 8, 0x86, 0, 0.0f, "KEY_JOY1_R2" },
	{ 8, 0x87, 0, 0.0f, "KEY_JOY1_L2" },
	{ 8, 0x88, 0, 0.0f, "KEY_JOY1_START" },
	{ 8, 0x89, 0, 0.0f, "KEY_JOY1_SELECT" },
	{ 8, 0x8A, 0, 0.0f, "KEY_JOY1_DPAD_U" },
	{ 8, 0x8B, 0, 0.0f, "KEY_JOY1_DPAD_D" },
	{ 8, 0x8C, 0, 0.0f, "KEY_JOY1_DPAD_L" },
	{ 8, 0x8D, 0, 0.0f, "KEY_JOY1_DPAD_R" },
	{ 8, 0x8E, 0, 0.0f, "KEY_JOY1_LMUSHROOM" },
	{ 8, 0x8F, 0, 0.0f, "KEY_JOY1_RMUSHROOM" },
	{ 8, 0x90, 0, 0.0f, "KEY_JOY2_SQUARE" },
	{ 8, 0x91, 0, 0.0f, "KEY_JOY2_TRIANGLE" },
	{ 8, 0x92, 0, 0.0f, "KEY_JOY2_CIRCLE" },
	{ 8, 0x93, 0, 0.0f, "KEY_JOY2_X" },
	{ 8, 0x94, 0, 0.0f, "KEY_JOY2_R1" },
	{ 8, 0x95, 0, 0.0f, "KEY_JOY2_L1" },
	{ 8, 0x96, 0, 0.0f, "KEY_JOY2_R2" },
	{ 8, 0x97, 0, 0.0f, "KEY_JOY2_L2" },
	{ 8, 0x98, 0, 0.0f, "KEY_JOY2_START" },
	{ 8, 0x99, 0, 0.0f, "KEY_JOY2_SELECT" },
	{ 8, 0x9A, 0, 0.0f, "KEY_JOY2_DPAD_U" },
	{ 8, 0x9B, 0, 0.0f, "KEY_JOY2_DPAD_D" },
	{ 8, 0x9C, 0, 0.0f, "KEY_JOY2_DPAD_L" },
	{ 8, 0x9D, 0, 0.0f, "KEY_JOY2_DPAD_R" },
	{ 8, 0x9E, 0, 0.0f, "KEY_JOY2_LMUSHROOM" },
	{ 8, 0x9F, 0, 0.0f, "KEY_JOY2_RMUSHROOM" },
	{ 2, 0x0, 0, 0.0f, "AXIS_JOY1_X" },
	{ 2, 0x1, 0, 0.0f, "AXIS_JOY1_Y" },
	{ 2, 0x2, 0, 0.0f, "AXIS_JOY1_Z" },
	{ 2, 0x3, 0, 0.0f, "AXIS_JOY1_R" },
	{ 2, 0x4, 0, 0.0f, "AXIS_JOY2_X" },
	{ 2, 0x5, 0, 0.0f, "AXIS_JOY2_Y" },
	{ 2, 0x6, 0, 0.0f, "AXIS_JOY2_Z" },
	{ 2, 0x7, 0, 0.0f, "AXIS_JOY2_R" }
};

SputControl_v2::SputControl_v2(EmiEngine *engine) : _engine(engine), _keyEventMapper(nullptr), _started(false), _opened(false) {
	memset(controlTable, 0, sizeof(controlTable));
}

SputControl_v2::~SputControl_v2() {
	delete _keyEventMapper;
	_keyEventMapper = nullptr;
}

int SputControl_v2::startup() {
	if (_started)
		return 0;

	if (_engine->getGamePlatform() == Common::kPlatformPS2)
		memcpy(controlTable, kPs2ControlTable, sizeof(controlTable));
	else
		memcpy(controlTable, kPcControlTable, sizeof(controlTable));

	clearAll();

	_keyEventMapper = new KeyEventMapper(_engine);

	_engine->getJoystick()->startup();

	_started = true;

	return 1;
}

void SputControl_v2::shutdown() {
	_engine->getJoystick()->shutdown();

	delete _keyEventMapper;
	_keyEventMapper = nullptr;
	_started = false;
}

void SputControl_v2::clearAll() {
	for (int i = 0; i < kNumControls; i++) {
		controlTable[i].flags &= ~(uint)ControlEntry::kEnabled;

		if (controlTable[i].flags & ControlEntry::kAxis)
			controlTable[i].axisValue = 0.0f;
		else
			controlTable[i].state = 0;
	}

	_engine->getJoystick()->resetControls();
}

void SputControl_v2::enableControl(uint index) {
	if (index >= (uint)kNumControls) {
		if (_engine->getConsoleServices())
			_engine->getConsoleServices()->logError("Invalid control %d; will not be enabled\n", (int)index);
		else
			warning("Invalid control %d; will not be enabled", (int)index);

		return;
	}

	ControlEntry *control = &controlTable[index];

	if (control->flags & ControlEntry::kEnabled)
		return;

	control->flags |= ControlEntry::kEnabled;

	if (control->flags & ControlEntry::kAxis) {
		_engine->getJoystick()->enableControl(control->code);
	} else if (control->flags & ControlEntry::kKey) {
		if (_keyEventMapper)
			_keyEventMapper->bindKey(control->code, &kDefaultBinding);
		else
			warning("SputControl_v2::enableControl(): the key mapper does not exist");
	}
}

void SputControl_v2::disableControl(uint index) {
	if (index >= (uint)kNumControls) {
		if (_engine->getConsoleServices())
			_engine->getConsoleServices()->logError("Not a valid control: %d\n", (int)index);
		else
			warning("Not a valid control: %d", (int)index);

		return;
	}

	ControlEntry *control = &controlTable[index];

	if (control->flags & ControlEntry::kEnabled)
		control->flags &= ~(uint)ControlEntry::kEnabled;
}

bool SputControl_v2::open() {
	bool result = _engine->getJoystick()->open();

	if (result)
		_opened = true;

	return result;
}

bool SputControl_v2::close() {
	if (!_opened)
		return false;

	if (!_engine->getJoystick()->close()) {
		if (_engine->getConsoleServices())
			_engine->getConsoleServices()->logError("std control system close failed.\n");
		else
			warning("std control system close failed.");

		return false;
	}

	_opened = false;

	return true;
}

void SputControl_v2::update() {
	_engine->_controlUpdateTimer.start();
	EventGenerator::dispatchEvents(_engine);
	_engine->getJoystick()->update();
	_engine->getLua()->axisHandler();
	_engine->_controlUpdateTimer.stop();
}

void SputControl_v2::clearKeyStates() {
	for (int i = 0; i < kNumControls; i++) {
		if (controlTable[i].flags & ControlEntry::kKey)
			controlTable[i].state = 0;
	}
}

bool SputControl_v2::isModifierKey(int vk) {
	return vk == 16 || vk == 17 || vk == 18;
}

} // End of namespace Grim
