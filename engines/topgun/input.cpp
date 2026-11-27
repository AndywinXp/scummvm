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

#include "common/array.h"
#include "common/config-manager.h"
#include "common/endian.h"
#include "common/events.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "topgun/display.h"
#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

bool TopgunEngine_v1_0::peekInputMessage(Message &message) {
	Common::EventManager *eventManager = g_system->getEventManager();
	Common::Event event;

	while (true) {
		if (!eventManager->pollEvent(event))
			return false;

		if (translate(event, message))
			return true;
	}
}

void TopgunEngine_v1_0::discardPendingInput(uint16 id) {
	Common::EventManager *eventManager = g_system->getEventManager();
	Common::Array<Common::Event> kept;
	Common::Event event;
	Message message;

	while (eventManager->pollEvent(event)) {
		if (id == 0)
			continue;

		if (translate(event, message) && message.id == id)
			continue;

		kept.push_back(event);
	}

	for (uint i = 0; i < kept.size(); i++)
		eventManager->pushEvent(kept[i]);
}

bool TopgunEngine_v1_0::translate(const Common::Event &event, Message &message) {
	message.param = 0;
	message.data = 0;
	message.shift = false;
	message.ctrl = false;

	switch (event.type) {
	case Common::EVENT_LBUTTONDOWN:
	case Common::EVENT_RBUTTONDOWN:
		message.id = (event.type == Common::EVENT_LBUTTONDOWN) ? kMsgLButtonDown : kMsgRButtonDown;
		message.data = ((uint32)(uint16)event.mouse.y << 16) | (uint16)event.mouse.x;
		return true;

	case Common::EVENT_KEYDOWN:
		if ((event.kbd.flags & Common::KBD_ALT) != 0 || event.kbd.keycode == Common::KEYCODE_F10)
			return false;

		message.id = kMsgKeyDown;
		message.param = (uint16)event.kbd.keycode;
		message.shift = (event.kbd.flags & Common::KBD_SHIFT) != 0;
		message.ctrl = (event.kbd.flags & Common::KBD_CTRL) != 0;
		return true;

	case Common::EVENT_SCREEN_CHANGED:
		invalidateScreen();
		return false;

	default:
		return false;
	}
}

uint16 TopgunEngine_v1_0::scriptKeyToKeyCodes(uint16 scriptKey, Common::KeyCode *keyCodes) const {
	if (scriptKey >= 0x41 && scriptKey <= 0x5A) {
		keyCodes[0] = (Common::KeyCode)(Common::KEYCODE_a + (scriptKey - 0x41));
		return 1;
	}

	if (scriptKey >= 0x30 && scriptKey <= 0x39) {
		// TODO: On layouts where the top-row digits need Shift (e.g. AZERTY)
		// we report punctuation keycodes instead of KEYCODE_0..9, while the original
		// gets the digit key code; a script bound to a digit may not fire there...
		// Keeping this TODO until every game is implemented and tested...
		keyCodes[0] = (Common::KeyCode)(Common::KEYCODE_0 + (scriptKey - 0x30));
		return 1;
	}

	if (scriptKey >= 0x60 && scriptKey <= 0x69) {
		keyCodes[0] = (Common::KeyCode)(Common::KEYCODE_KP0 + (scriptKey - 0x60));
		return 1;
	}

	if (scriptKey >= 0x70 && scriptKey <= 0x7B) {
		keyCodes[0] = (Common::KeyCode)(Common::KEYCODE_F1 + (scriptKey - 0x70));
		return 1;
	}

	switch (scriptKey) {
	case 0x08:
		keyCodes[0] = Common::KEYCODE_BACKSPACE;
		return 1;
	case 0x09:
		keyCodes[0] = Common::KEYCODE_TAB;
		return 1;
	case 0x0D:
		keyCodes[0] = Common::KEYCODE_RETURN;
		keyCodes[1] = Common::KEYCODE_KP_ENTER;
		return 2;
	case 0x10:
		keyCodes[0] = Common::KEYCODE_LSHIFT;
		keyCodes[1] = Common::KEYCODE_RSHIFT;
		return 2;
	case 0x11:
		keyCodes[0] = Common::KEYCODE_LCTRL;
		keyCodes[1] = Common::KEYCODE_RCTRL;
		return 2;
	case 0x12:
		keyCodes[0] = Common::KEYCODE_LALT;
		keyCodes[1] = Common::KEYCODE_RALT;
		return 2;
	case 0x13:
		keyCodes[0] = Common::KEYCODE_PAUSE;
		return 1;
	case 0x1B:
		keyCodes[0] = Common::KEYCODE_ESCAPE;
		return 1;
	case 0x20:
		keyCodes[0] = Common::KEYCODE_SPACE;
		return 1;
	case 0x21:
		keyCodes[0] = Common::KEYCODE_PAGEUP;
		return 1;
	case 0x22:
		keyCodes[0] = Common::KEYCODE_PAGEDOWN;
		return 1;
	case 0x23:
		keyCodes[0] = Common::KEYCODE_END;
		return 1;
	case 0x24:
		keyCodes[0] = Common::KEYCODE_HOME;
		return 1;
	case 0x25:
		keyCodes[0] = Common::KEYCODE_LEFT;
		return 1;
	case 0x26:
		keyCodes[0] = Common::KEYCODE_UP;
		return 1;
	case 0x27:
		keyCodes[0] = Common::KEYCODE_RIGHT;
		return 1;
	case 0x28:
		keyCodes[0] = Common::KEYCODE_DOWN;
		return 1;
	case 0x2D:
		keyCodes[0] = Common::KEYCODE_INSERT;
		return 1;
	case 0x2E:
		keyCodes[0] = Common::KEYCODE_DELETE;
		return 1;
	case 0x6A:
		keyCodes[0] = Common::KEYCODE_KP_MULTIPLY;
		return 1;
	case 0x6B:
		keyCodes[0] = Common::KEYCODE_KP_PLUS;
		return 1;
	case 0x6D:
		keyCodes[0] = Common::KEYCODE_KP_MINUS;
		return 1;
	case 0x6E:
		keyCodes[0] = Common::KEYCODE_KP_PERIOD;
		return 1;
	case 0x6F:
		keyCodes[0] = Common::KEYCODE_KP_DIVIDE;
		return 1;
	case 0xBB:
		keyCodes[0] = Common::KEYCODE_EQUALS;
		return 1;
	case 0xBD:
		keyCodes[0] = Common::KEYCODE_MINUS;
		return 1;
	default:
		return 0;
	}
}

void TopgunEngine_v1_0::handleKeyDownMessage(Common::KeyCode keycode, bool shift, bool ctrl) {
	if (keycode == Common::KEYCODE_F2) {
		_globalFlags[8] = (_globalFlags[8] == 0) ? 1 : 0;
		ConfMan.setInt("howie", _globalFlags[8]);
		ConfMan.flushToDisk();
	}

	runKeyScript(keycode, shift, ctrl);
}

void TopgunEngine_v1_0::runKeyScript(Common::KeyCode keycode, bool shift, bool ctrl) {
	uint16 script = 0;

	if (keycode == Common::KEYCODE_KP_PLUS || keycode == Common::KEYCODE_KP_MINUS) {
		adjustMasterVolumeByKey(keycode);
	} else {
		KeyScriptRecord record;
		if (_keyScripts.tryGetVal((int)keycode, record)) {
			if (shift)
				script = record.shift;
			else if (ctrl)
				script = record.ctrl;
			else
				script = record.plain;
		}
	}

	if (script != 0)
		runScriptResource(script);
}

void TopgunEngine_v1_0::handleMouseButtonDown(int16 x, int16 y) {
	if (_inputBusy)
		return;

	if (!_cursorEnabled)
		return;

	_globalFlags[3] = 1;

	runClickScriptAt(x, y);
}

void TopgunEngine_v1_0::runClickScriptAt(int16 x, int16 y) {
	uint16 script = 0;

	x -= display()->viewportOrigin().x;
	y -= display()->viewportOrigin().y;

	uint16 sprite = spriteLibrary()->spritePicked(x, y);
	if (sprite != 0) {
		uint16 id = spriteLibrary()->spriteGetId(sprite);
		if (_records && id < _numResources)
			script = _records[id].clickScript;
		else
			warning("TopgunEngine_v1_0::handleMouseButtonDown(): resource id %d is out of range", id);
	}

	if (script == 0) {
		int index = (int)_numHotspots - 1;
		while (index >= 0) {
			const HotspotRecord &hotspot = _hotspots[index];
			if (hotspot.left <= x && x < hotspot.right && hotspot.top <= y && y < hotspot.bottom && hotspot.script != 0)
				break;

			index--;
		}

		if (index >= 0)
			script = _hotspots[index].script;
	}

	if (script != 0)
		runScriptResource(script);
}

void TopgunEngine_v1_0::addOrUpdateClickRegion(const byte *record) {
	int16 left = READ_LE_INT16(record + 6);
	int16 top = READ_LE_INT16(record + 8);
	int16 right = READ_LE_INT16(record + 10);
	int16 bottom = READ_LE_INT16(record + 12);
	uint16 script = READ_LE_UINT16(record + 4);
	uint16 index = 0;

	if (_numHotspots != 0) {
		do {
			const HotspotRecord &hotspot = _hotspots[index];

			if (hotspot.left == left && hotspot.top == top && hotspot.right == right && hotspot.bottom == bottom)
				break;

			index++;
		} while (index < _numHotspots);
	}

	if (index < _numHotspots) {
		_hotspots[index].script = script;
		return;
	}

	if (_numHotspots < TG_HOST_MAX_HOTSPOTS) {
		HotspotRecord &hotspot = _hotspots[_numHotspots];
		hotspot.left = left;
		hotspot.top = top;
		hotspot.right = right;
		hotspot.bottom = bottom;
		hotspot.script = script;
		_numHotspots++;
	}
}

void TopgunEngine_v1_1::handleKeyDownMessage(Common::KeyCode keycode, bool shift, bool ctrl) {
	runKeyScript(keycode, shift, ctrl);
}

void TopgunEngine_v1_1::handleMouseButtonDown(int16 x, int16 y) {
	if (_cursorMode == 3)
		return;

	_globalFlags[0] = 1;

	runClickScriptAt(x, y);
}

void TopgunEngine_v1_0::adjustMasterVolumeByKey(Common::KeyCode keycode) {
	uint32 volume = audioLibrary()->audioCommand(0x82, 0, 0, nullptr);
	uint32 stepped = stepVolumeLevels5Bit(volume, keycode == Common::KEYCODE_KP_PLUS);
	audioLibrary()->audioCommand(0x83, 0, stepped, nullptr);
}

uint32 TopgunEngine_v1_0::stepVolumeLevels5Bit(uint32 volume, bool up) {
	uint16 left = (volume & 0xFFFF) >> 11;
	uint16 right = (volume >> 16) >> 11;

	if (up && left < 31 && right < 31) {
		left++;
		right++;
	} else if (!up && left != 0 && right != 0) {
		left--;
		right--;
	}

	uint32 outLeft = ((uint32)left * 0xFFFF) / 31;
	uint32 outRight = ((uint32)right * 0xFFFF) / 31;
	return (outRight << 16) | outLeft;
}

bool TopgunEngine_v1_2::translate(const Common::Event &event, Message &message) {
	message.param = 0;
	message.data = 0;
	message.shift = false;
	message.ctrl = false;

	switch (event.type) {
	case Common::EVENT_MOUSEMOVE:
	case Common::EVENT_LBUTTONDOWN:
	case Common::EVENT_RBUTTONDOWN:
	case Common::EVENT_LBUTTONUP:
	case Common::EVENT_RBUTTONUP: {
		if (event.type == Common::EVENT_MOUSEMOVE)
			message.id = kMsgV1_2MouseMove;
		else if (event.type == Common::EVENT_LBUTTONDOWN)
			message.id = kMsgLButtonDown;
		else if (event.type == Common::EVENT_RBUTTONDOWN)
			message.id = kMsgRButtonDown;
		else if (event.type == Common::EVENT_LBUTTONUP)
			message.id = kMsgV1_2LButtonUp;
		else
			message.id = kMsgV1_2RButtonUp;

		int modifiers = g_system->getEventManager()->getModifierState();

		message.data = ((uint32)(uint16)event.mouse.y << 16) | (uint16)event.mouse.x;
		message.shift = (modifiers & Common::KBD_SHIFT) != 0;
		message.ctrl = (modifiers & Common::KBD_CTRL) != 0;
		return true;
	}

	case Common::EVENT_KEYDOWN: {
		uint16 key = keyCodeToScriptKey(event.kbd.keycode);
		if (key != TG_HOST_V1_2_NO_KEY)
			_keyStates[key] = true;

		// Of the keys with Alt only the one that switches the application is handled
		if ((event.kbd.flags & Common::KBD_ALT) != 0) {
			if (event.kbd.keycode != Common::KEYCODE_TAB)
				return false;

			message.id = kMsgV1_2SysKeyDown;
			message.param = TG_HOST_V1_2_VK_TAB;
			return true;
		}

		message.shift = (event.kbd.flags & Common::KBD_SHIFT) != 0;
		message.ctrl = (event.kbd.flags & Common::KBD_CTRL) != 0;

		if (event.kbd.keycode == Common::KEYCODE_F10) {
			if (_version < kVersionV1_2_1)
				return false;

			message.id = kMsgV1_2SysKeyDown;
			message.param = (uint16)event.kbd.keycode;
			return true;
		}

		message.id = kMsgKeyDown;
		message.param = (uint16)event.kbd.keycode;
		message.data = event.kbd.ascii;
		return true;
	}

	case Common::EVENT_KEYUP: {
		uint16 key = keyCodeToScriptKey(event.kbd.keycode);
		if (key != TG_HOST_V1_2_NO_KEY)
			_keyStates[key] = false;

		if (_version < kVersionV1_2_2)
			return false;

		if ((event.kbd.flags & Common::KBD_ALT) != 0 || event.kbd.keycode == Common::KEYCODE_F10)
			return false;

		message.id = kMsgV1_2_3KeyUp;
		message.param = (uint16)event.kbd.keycode;
		message.shift = (event.kbd.flags & Common::KBD_SHIFT) != 0;
		message.ctrl = (event.kbd.flags & Common::KBD_CTRL) != 0;
		return true;
	}

	case Common::EVENT_JOYAXIS_MOTION:
		if (_version < kVersionV1_2_1 || !_joystickCaptured)
			return false;

		if (event.joystick.axis == 0)
			_joystickX = (uint16)(event.joystick.position - Common::JOYAXIS_MIN);
		else if (event.joystick.axis == 1)
			_joystickY = (uint16)(event.joystick.position - Common::JOYAXIS_MIN);
		else
			return false;

		message.id = kMsgV1_2_1Joystick1Move;
		message.param = _joystickButtons;
		message.data = ((uint32)_joystickY << 16) | _joystickX;
		return true;

	case Common::EVENT_JOYBUTTON_DOWN:
	case Common::EVENT_JOYBUTTON_UP: {
		if (_version < kVersionV1_2_1 || !_joystickCaptured)
			return false;

		int button = event.joystick.button - Common::JOYSTICK_BUTTON_A;
		if (button < 0 || button > 3)
			return false;

		bool down = (event.type == Common::EVENT_JOYBUTTON_DOWN);

		if (down)
			_joystickButtons |= (byte)(1 << button);
		else
			_joystickButtons &= (byte)~(1 << button);

		message.id = down ? kMsgV1_2_1Joystick1ButtonDown : kMsgV1_2_1Joystick1ButtonUp;
		message.param = (uint16)((0x100 << button) | _joystickButtons);
		message.data = ((uint32)_joystickY << 16) | _joystickX;
		return true;
	}

	case Common::EVENT_SCREEN_CHANGED:
		invalidateScreen();
		return false;

	default:
		return false;
	}
}

bool TopgunEngine_v1_2::peekInputMessage(Message &message) {
	if (!_cursorApplied)
		applyCursorFromRegister();

	if (_hasPendingChar) {
		message = _pendingChar;
		_hasPendingChar = false;
		return true;
	}

	if (!TopgunEngine_v1_0::peekInputMessage(message))
		return false;

	if (message.id == kMsgV1_2MouseMove) {
		Common::EventManager *eventManager = g_system->getEventManager();
		Common::Event event;

		while (eventManager->pollEvent(event)) {
			if (event.type != Common::EVENT_MOUSEMOVE) {
				eventManager->pushEvent(event);
				break;
			}

			translate(event, message);
		}
	}

	// Windows sends no character for Delete, the cursor keys and the function keys
	if (message.id == kMsgKeyDown && message.data != 0 && message.data <= 0xFF && message.param != Common::KEYCODE_DELETE) {
		_pendingChar = Message();
		_pendingChar.id = kMsgV1_2Char;
		_pendingChar.param = (uint16)message.data;
		_hasPendingChar = true;
	}

	return true;
}

uint16 TopgunEngine_v1_2::keyCodeToScriptKey(Common::KeyCode keycode) {
	Common::KeyCode keyCodes[2];
	uint16 keyTableSize = TG_HOST_V1_2_KEY_TABLE_SIZE;

	if (_version >= kVersionV1_2_5)
		keyTableSize = TG_INPUT_V1_2_3_KEY_TABLE_SIZE;

	for (uint16 key = 1; key < keyTableSize; key++) {
		uint16 count = scriptKeyToKeyCodes(key, keyCodes);

		for (uint16 i = 0; i < count; i++) {
			if (keyCodes[i] == keycode)
				return key;
		}
	}

	return TG_HOST_V1_2_NO_KEY;
}

void TopgunEngine_v1_2::adjustMasterVolumeByKey(Common::KeyCode keycode) {
	uint32 volume = audioLib()->audioGetMasterVolume();
	uint32 stepped = stepVolumeLevels5Bit(volume, keycode == Common::KEYCODE_KP_PLUS);

	audioLib()->audioSetMasterVolume(stepped);
}

void TopgunEngine_v1_2::transformPointForDisplayZoom(Common::Point &point) {
	point.x -= display()->viewportOrigin().x;
	point.y -= display()->viewportOrigin().y;

	if (_displayZoomActive && _displayZoomMode != 0) {
		int16 half = (int16)(_displayZoomMode / 2);

		point.x = (int16)((point.x + half) / (int16)_displayZoomMode + (int16)_displayZoomX);
		point.y = (int16)((point.y + half) / (int16)_displayZoomMode + (int16)_displayZoomY);
	}
}

bool TopgunEngine_v1_2::postRunScript(uint16 script, uint16 argument1, uint16 argument2) {
	postMessage(kMsgV1_2RunScript, script, ((uint32)argument2 << 16) | argument1);
	return true;
}

uint16 TopgunEngine_v1_2::postMouseEventScript(uint16 code) {
	byte record[18];

	memset(record, 0, sizeof(record));
	WRITE_LE_UINT16(record + 2, _mouseEventScript);

	if (_version >= kVersionV1_2_2) {
		WRITE_LE_UINT16(record + 6, (uint16)(code - ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS)));
		record[5] = 1;

		uint16 saved = _scriptResult;
		_scriptResult = 0;
		o_callScript(record);

		uint16 result = _scriptResult;
		_scriptResult = saved;
		return result;
	}

	record[4] = 1;
	WRITE_LE_UINT16(record + 5, (uint16)(code - TG_HOST_V1_2_LITERAL_BIAS));

	o_callScript(record);
	return 0;
}

void TopgunEngine_v1_2::handleMouseButtonMessage(const Message &message) {
	bool left = (message.id == kMsgLButtonDown);
	Common::Point point((int16)(message.data & 0xFFFF), (int16)(message.data >> 16));

	if (_version >= kVersionV1_2_3 && _mouseAsStick && _flagActivated && static_cast<TopgunEngine_v1_2_3 *>(this)->mouseAsStickMessage(point, message.id))
		return;

	if (left)
		_leftButtonDown = true;
	else
		_rightButtonDown = true;

	if (_gamePaused)
		return;

	bool wallpaperSelection = true;
	if (_version >= kVersionV1_2_7)
		wallpaperSelection = static_cast<TopgunEngine_v1_2_3 *>(this)->_wallpaperSelectionEnabled;

	if (left && wallpaperSelection && message.shift && message.ctrl) {
		warning("beginRubberBandSelection UNIMPLEMENTED");
	} else if (left && _version >= kVersionV1_2_6_1 && _version <= kVersionV1_2_9 && static_cast<TopgunEngine_v1_2_3 *>(this)->_rubberBandScript != 0) {
		transformPointForDisplayZoom(point);

		_captureDrag = true;
		_rubberBandMode = true;
		static_cast<SpriteLibrary_v1_2_3 *>(spriteLib())->spriteRubberBand(true, point.x, point.y, false);

		_captureDragStart = point;
		_captureDragEnd = point;
		return;
	}

	if (_version >= kVersionV1_2_6_1 && _version <= kVersionV1_2_9 && _rubberBandMode)
		return;

	if (_cursorMode == TG_HOST_V1_2_CURSOR_MODE_BUSY || _cursorMode == TG_HOST_V1_2_CURSOR_MODE_HIDDEN)
		return;

	if (_dragSpriteId != 0)
		return;

	transformPointForDisplayZoom(point);

	if (_version >= kVersionV1_2_2_2) {
		writeVariable(TG_HOST_V1_2_3_VARIABLE_MOUSE_BUTTON, left ? TG_HOST_V1_2_MOUSE_BUTTON_LEFT : TG_HOST_V1_2_MOUSE_BUTTON_RIGHT);
		writeVariable(TG_HOST_V1_2_3_VARIABLE_CLICK_X, (uint16)point.x);
		writeVariable(TG_HOST_V1_2_3_VARIABLE_CLICK_Y, (uint16)point.y);
	} else if (_version >= kVersionV1_2_2) {
		writeVariable(TG_HOST_V1_2_2_VARIABLE_MOUSE_BUTTON, left ? TG_HOST_V1_2_MOUSE_BUTTON_LEFT : TG_HOST_V1_2_MOUSE_BUTTON_RIGHT);
		writeVariable(TG_HOST_V1_2_2_VARIABLE_CLICK_X, (uint16)point.x);
		writeVariable(TG_HOST_V1_2_2_VARIABLE_CLICK_Y, (uint16)point.y);
	} else {
		writeVariable(TG_HOST_V1_2_VARIABLE_MOUSE_BUTTON, left ? TG_HOST_V1_2_MOUSE_BUTTON_LEFT : TG_HOST_V1_2_MOUSE_BUTTON_RIGHT);
		writeVariable(TG_HOST_V1_2_VARIABLE_CLICK_X, (uint16)point.x);
		writeVariable(TG_HOST_V1_2_VARIABLE_CLICK_Y, (uint16)point.y);
	}

	if (_mouseEventScript != 0 && postMouseEventScript(TG_HOST_V1_2_MOUSE_EVENT_DOWN) == 0)
		return;

	uint16 script = 0;
	uint16 argument = 0;
	uint16 clickedId = 0;
	uint16 sprite = spriteLib()->spritePicked(point.x, point.y);

	if (sprite != 0) {
		clickedId = spriteLib()->spriteGetId(sprite);
		uint16 index = operandValue(clickedId);

		if (isRecordIndexValid(index, "handleMouseButtonMessage") && _records[index].data) {
			const byte *object = _records[index].data;
			uint16 draggableOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_DRAGGABLE : TG_HOST_V1_2_SPRITE_DATA_DRAGGABLE;
			uint16 dragScriptOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_DRAG_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_DRAG_SCRIPT;
			uint16 disabledOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_DISABLED : TG_HOST_V1_2_SPRITE_DATA_DISABLED;
			uint16 clickScriptOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_CLICK_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_CLICK_SCRIPT;

			if (_version >= kVersionV1_2_6_1) {
				draggableOffset = TG_INPUT_V1_2_6_1_OBJECT_DRAGGABLE;
				dragScriptOffset = TG_INPUT_V1_2_6_1_OBJECT_DRAG_SCRIPT;
				disabledOffset = TG_INPUT_V1_2_6_1_OBJECT_DISABLED;
			}

			if (object[draggableOffset] != 0) {
				_dragSpriteId = clickedId;
				_dragStartX = point.x;
				_dragStartY = point.y;
				script = READ_LE_UINT16(object + dragScriptOffset);
			} else if (object[disabledOffset] == 0) {
				script = READ_LE_UINT16(object + clickScriptOffset);

				if (_version >= kVersionV1_2_6_1)
					argument = READ_LE_UINT16(object + TG_INPUT_V1_2_6_1_OBJECT_CLICK_ARGUMENT);
			}
		}
	}

	if (script == 0) {
		clickedId = 0;

		if (_version >= kVersionV1_2_6_1)
			clickedId = (uint16)(0 - TG_HOST_V1_2_3_LITERAL_BIAS);
	}

	if (script == 0) {
		int16 region = (int16)_numClickRegions - 1;

		while (region >= 0) {
			const ClickRegion_v1_2 &candidate = _clickRegions[region];

			if (candidate.disabled == 0 && candidate.left <= point.x && candidate.right > point.x && candidate.top <= point.y && candidate.bottom > point.y && candidate.script != 0)
				break;

			region--;
		}

		if (region >= 0) {
			script = _clickRegions[region].script;
			argument = _clickRegions[region].argument;
		}
	}

	if (script != 0)
		postRunScript(script, (_version >= kVersionV1_2_2_1) ? clickedId : 0, argument);
}

void TopgunEngine_v1_2::handleMouseButtonUpMessage(const Message &message) {
	bool left = (message.id == kMsgV1_2LButtonUp);
	Common::Point point((int16)(message.data & 0xFFFF), (int16)(message.data >> 16));

	if (_version >= kVersionV1_2_3 && _mouseAsStick && (_flagActivated || _version >= kVersionV1_2_6_1) && static_cast<TopgunEngine_v1_2_3 *>(this)->mouseAsStickMessage(point, message.id))
		return;

	if (left)
		_leftButtonDown = false;
	else
		_rightButtonDown = false;

	if (_gamePaused)
		return;

	if (_version >= kVersionV1_2_6_1 && _version <= kVersionV1_2_9 && _rubberBandMode) {
		if (left && _captureDrag) {
			_rubberBandMode = false;
			_captureDrag = false;
			static_cast<SpriteLibrary_v1_2_3 *>(spriteLib())->spriteRubberBand(false, 0, 0, false);
			static_cast<TopgunEngine_v1_2_3 *>(this)->callRubberBandScript();
		} else if (left) {
			warning("endRubberBandSelection UNIMPLEMENTED");
		}

		return;
	}

	if (_mouseEventScript != 0 && postMouseEventScript(TG_HOST_V1_2_MOUSE_EVENT_UP) == 0)
		return;

	if (_dragSpriteId != 0) {
		transformPointForDisplayZoom(point);
		dropDraggedSprite(point.x, point.y);
	}
}

void TopgunEngine_v1_2::handleMouseMoveMessage(const Message &message) {
	Common::Point point((int16)(message.data & 0xFFFF), (int16)(message.data >> 16));

	if (_version >= kVersionV1_2_3) {
		if (!_flagActivated)
			return;

		if (_mouseAsStick && static_cast<TopgunEngine_v1_2_3 *>(this)->mouseAsStickMessage(point, message.id))
			return;
	}

	if (_version >= kVersionV1_2_6_1 && _version <= kVersionV1_2_9 && _rubberBandMode) {
		if (_captureDrag) {
			transformPointForDisplayZoom(point);
			static_cast<SpriteLibrary_v1_2_3 *>(spriteLib())->spriteRubberBand(true, point.x, point.y, _rightButtonDown);
			_captureDragEnd = point;
		} else {
			warning("updateRubberBand UNIMPLEMENTED");
		}

		return;
	}

	transformPointForDisplayZoom(point);

	if (_version >= kVersionV1_2_2_2) {
		writeVariable(TG_HOST_V1_2_3_VARIABLE_MOUSE_X, (uint16)point.x);
		writeVariable(TG_HOST_V1_2_3_VARIABLE_MOUSE_Y, (uint16)point.y);
	} else if (_version >= kVersionV1_2_2) {
		writeVariable(TG_HOST_V1_2_2_VARIABLE_MOUSE_X, (uint16)point.x);
		writeVariable(TG_HOST_V1_2_2_VARIABLE_MOUSE_Y, (uint16)point.y);
	} else {
		writeVariable(TG_HOST_V1_2_VARIABLE_MOUSE_X, (uint16)point.x);
		writeVariable(TG_HOST_V1_2_VARIABLE_MOUSE_Y, (uint16)point.y);
	}

	if (_gamePaused)
		return;

	if (_mouseEventScript != 0 && postMouseEventScript(TG_HOST_V1_2_MOUSE_EVENT_MOVE) == 0)
		return;

	if (_version >= kVersionV1_2_7 && _cursorMode == TG_HOST_V1_2_CURSOR_MODE_HIDDEN)
		return;

	if (_dragSpriteId != 0) {
		uint16 index = operandValue(_dragSpriteId);

		if (isRecordIndexValid(index, "handleMouseMoveMessage"))
			spriteLib()->spriteOffset(_records[index].handle, (int16)(point.x - _dragStartX), (int16)(point.y - _dragStartY));

		_dragStartX = point.x;
		_dragStartY = point.y;
		return;
	}

	if (_version >= kVersionV1_2_2_2 && _hoverSpriteScript != 0 && static_cast<TopgunEngine_v1_2_3 *>(this)->updateSpriteHoverFromPoint(point.x, point.y))
		return;

	if (_numHoverRegions != 0)
		updateHoverRegionFromPoint(point.x, point.y);
}

void TopgunEngine_v1_2::dropDraggedSprite(int16 x, int16 y) {
	if (_version >= kVersionV1_2_2_2) {
		writeVariable(TG_HOST_V1_2_3_VARIABLE_DROP_X, (uint16)x);
		writeVariable(TG_HOST_V1_2_3_VARIABLE_DROP_Y, (uint16)y);
	} else if (_version >= kVersionV1_2_2) {
		writeVariable(TG_HOST_V1_2_2_VARIABLE_DROP_X, (uint16)x);
		writeVariable(TG_HOST_V1_2_2_VARIABLE_DROP_Y, (uint16)y);
	} else {
		writeVariable(TG_HOST_V1_2_VARIABLE_DROP_X, (uint16)x);
		writeVariable(TG_HOST_V1_2_VARIABLE_DROP_Y, (uint16)y);
	}

	uint16 index = operandValue(_dragSpriteId);

	if (isRecordIndexValid(index, "dropDraggedSprite") && _records[index].data) {
		uint16 dropScriptOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_DROP_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_DROP_SCRIPT;
		if (_version >= kVersionV1_2_6_1)
			dropScriptOffset = TG_INPUT_V1_2_6_1_OBJECT_DROP_SCRIPT;

		uint16 script = READ_LE_UINT16(_records[index].data + dropScriptOffset);

		if (script != 0)
			postRunScript(script, (_version >= kVersionV1_2_2_1) ? _dragSpriteId : 0, 0);
	}

	_dragSpriteId = 0;
}

void TopgunEngine_v1_2::handleKeyDownMessage(Common::KeyCode keycode, bool shift, bool ctrl) {
	uint16 key = keyCodeToScriptKey(keycode);

	if (_version >= kVersionV1_2_2_1) {
		if (key == TG_HOST_V1_2_NO_KEY)
			return;

		// F10 arrives as a system key, which the filter of the demo does not see
		if ((_version >= kVersionV1_2_2_2 || keycode != Common::KEYCODE_F10) && callMessageFilter(kMsgKeyDown, key, 0, 0))
			return;
	}

	if (_keyboardRouted && !keyboardRoutedKeyDown(key))
		return;

	if (_version >= kVersionV1_2_1) {
		if (key == TG_INPUT_V1_2_1_VK_PAUSE)
			togglePauseState();

		if (_gamePaused)
			return;
	}

	if (_version < kVersionV1_2_2_1 && (key == TG_HOST_V1_2_VK_ADD || key == TG_HOST_V1_2_VK_SUBTRACT)) {
		adjustMasterVolumeByKey(keycode);
		return;
	}

	if (_version >= kVersionV1_2_6_1 && _version <= kVersionV1_2_9 && _rubberBandMode && key == TG_HOST_V1_2_VK_ESCAPE) {
		if (_captureDrag) {
			_rubberBandMode = false;
			_captureDrag = false;
			static_cast<SpriteLibrary_v1_2_3 *>(spriteLib())->spriteRubberBand(false, 0, 0, false);
			static_cast<TopgunEngine_v1_2_3 *>(this)->callRubberBandScript();
		} else {
			warning("endRubberBandSelection UNIMPLEMENTED");
		}

		return;
	}

	if (_version >= kVersionV1_2_2_2 && _keyHookScript != 0 && static_cast<TopgunEngine_v1_2_3 *>(this)->postKeyHookScript(key) == 0)
		return;

	if (_textEditObjectId != 0)
		return;

	uint16 keyTableSize = (_version >= kVersionV1_2_3) ? TG_INPUT_V1_2_3_KEY_TABLE_SIZE : TG_HOST_V1_2_KEY_TABLE_SIZE;

	if (key >= keyTableSize)
		return;

	const KeyEntry_v1_2 &entry = _keyTable[key];
	if (entry.disabled != 0)
		return;

	uint16 script;

	if (shift)
		script = entry.shift;
	else if (ctrl)
		script = entry.ctrl;
	else
		script = entry.plain;

	if (script != 0) {
		if (_version >= kVersionV1_2_10)
			static_cast<TopgunEngine_v1_2_3 *>(this)->runScriptHookWithArg(script, key);
		else
			runScriptResource(script);
	}
}

void TopgunEngine_v1_2::handleCharMessage(uint16 character) {
	if (_gamePaused)
		return;

	uint16 object = getTextObjectFromId(_textEditObjectId);
	if (object == 0)
		return;

	bool changed = false;

	if (character == TG_HOST_V1_2_CHAR_ESCAPE || character == TG_HOST_V1_2_CHAR_RETURN) {
		changed = endTextEdit(object, character == TG_HOST_V1_2_CHAR_ESCAPE);
	} else if (character == TG_HOST_V1_2_CHAR_BACKSPACE) {
		changed = textEditBackspace(object, false);
	} else if (character >= TG_HOST_V1_2_CHAR_FIRST_PRINTABLE || (_version >= kVersionV1_2_1 && character == 0x0A)) {
		if (_version < kVersionV1_2_3 || character != 0x40)
			changed = textEditAppendChar(object, (byte)character);
	}

	if (changed && _textEditObjectId != 0)
		refreshTextObjectBitmap(object);
}

void TopgunEngine_v1_2::handleDeleteKeyMessage() {
	if (_gamePaused)
		return;

	uint16 object = getTextObjectFromId(_textEditObjectId);
	if (object == 0)
		return;

	bool changed = textEditBackspace(object, true);

	if (changed && _textEditObjectId != 0)
		refreshTextObjectBitmap(object);
}

void TopgunEngine_v1_2::handleSysKeyDownAltTab(const Message &message) {
	if (message.param == TG_HOST_V1_2_VK_TAB && !_altTab) {
		_appActive = false;
		_flagActivated = false;

		if (_version >= kVersionV1_2_10)
			static_cast<TopgunEngine_v1_2_3 *>(this)->notifyPluginsFocusChanged(false);

		audioLib()->audioActivate(0);
		_savedCursorMode = _cursorMode;
		_cursorMode = TG_HOST_V1_2_CURSOR_MODE_ARROW;
		applyCursorFromRegister();
	}
}

uint16 TopgunEngine_v1_2::getTextObjectFromId(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "getTextObjectFromId"))
		return 0;

	return _records[index].object;
}

void TopgunEngine_v1_2::o_bindKeyScript(const byte *record) {
	uint16 key = READ_LE_UINT16(record + 2);
	if (_version >= kVersionV1_2_7)
		key = operandValue(key);

	uint16 keyTableSize = (_version >= kVersionV1_2_3) ? TG_INPUT_V1_2_3_KEY_TABLE_SIZE : TG_HOST_V1_2_KEY_TABLE_SIZE;
	uint16 first = 0;
	uint16 last = keyTableSize - 1;

	if (key != 0) {
		if (key >= keyTableSize) {
			warning("TopgunEngine_v1_2::o_bindKeyScript(): key %d is out of range", key);
			return;
		}

		first = key;
		last = key;
	}

	for (uint16 i = first; i <= last; i++) {
		KeyEntry_v1_2 &entry = _keyTable[i];

		if (record[8] != 0) {
			entry.disabled = 0;
		} else if (record[9] != 0) {
			entry.disabled = 1;
		} else {
			if (record[6] != 0)
				entry.ctrl = READ_LE_UINT16(record + 4);
			else if (record[7] != 0)
				entry.shift = READ_LE_UINT16(record + 4);
			else if (_version >= kVersionV1_2_2 && record[0x0A] != 0)
				entry.keyUp = READ_LE_UINT16(record + 4);
			else
				entry.plain = READ_LE_UINT16(record + 4);

			entry.disabled = 0;
		}
	}
}

bool TopgunEngine_v1_2::o_setClickScript(const byte *record) {
	byte allFlag = record[0x0E];
	byte enableFlag = record[0x0F];
	byte disableFlag = record[0x10];

	if (_version >= kVersionV1_2_6_1) {
		allFlag = record[0x10];
		enableFlag = record[0x11];
		disableFlag = record[0x12];
	}

	if (allFlag != 0) {
		if (enableFlag != 0)
			setAllClickRegionsAndSpriteObjectsEnabled(0);
		else if (disableFlag != 0)
			setAllClickRegionsAndSpriteObjectsEnabled(1);
		else
			setAllSpriteObjectsCursor(READ_LE_UINT16(record + 4));

		return true;
	}

	uint16 id = READ_LE_UINT16(record + 2);

	if (id == 0) {
		addOrUpdateClickRegion(record);
		return true;
	}

	ensureResourceDataLoaded(id);

	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "o_setClickScript") || !_records[index].data) {
		warning("TopgunEngine_v1_2::o_setClickScript(): the sprite %d has no data", index);
		return true;
	}

	byte *object = _records[index].data;
	uint16 disabledOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_DISABLED : TG_HOST_V1_2_SPRITE_DATA_DISABLED;
	uint16 clickScriptOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_CLICK_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_CLICK_SCRIPT;

	if (_version >= kVersionV1_2_6_1) {
		disabledOffset = TG_INPUT_V1_2_6_1_OBJECT_DISABLED;

		if (READ_LE_UINT16(record + 0x0E) != 0)
			WRITE_LE_UINT16(object + TG_INPUT_V1_2_6_1_OBJECT_CLICK_ARGUMENT, (uint16)(operandValue(READ_LE_UINT16(record + 0x0E)) - TG_HOST_V1_2_3_LITERAL_BIAS));
		else
			WRITE_LE_UINT16(object + TG_INPUT_V1_2_6_1_OBJECT_CLICK_ARGUMENT, 0);
	}

	object[disabledOffset] = 0;

	if (disableFlag != 0)
		object[disabledOffset] = 1;
	else if (enableFlag != 0)
		object[disabledOffset] = 0;
	else
		WRITE_LE_UINT16(object + clickScriptOffset, READ_LE_UINT16(record + 4));

	return true;
}

void TopgunEngine_v1_2::addOrUpdateClickRegion(const byte *record) {
	int16 left = (int16)operandValue(READ_LE_UINT16(record + 6));
	int16 top = (int16)operandValue(READ_LE_UINT16(record + 8));
	int16 right = (int16)operandValue(READ_LE_UINT16(record + 10));
	int16 bottom = (int16)operandValue(READ_LE_UINT16(record + 12));
	uint16 index = 0;

	if (_numClickRegions != 0) {
		do {
			const ClickRegion_v1_2 &region = _clickRegions[index];

			if (region.left == left && region.top == top && region.right == right && region.bottom == bottom)
				break;

			index++;
		} while (index < _numClickRegions);
	}

	if (_numClickRegions <= index) {
		if (!appendClickRegion())
			return;

		index = _numClickRegions - 1;
	}

	ClickRegion_v1_2 &region = _clickRegions[index];
	byte enableFlag = record[0x0F];
	byte disableFlag = record[0x10];

	if (_version >= kVersionV1_2_6_1) {
		enableFlag = record[0x11];
		disableFlag = record[0x12];
	}

	if (disableFlag != 0) {
		region.disabled = 1;
	} else if (enableFlag != 0) {
		region.disabled = 0;
	} else if (_version >= kVersionV1_2_10 && READ_LE_UINT16(record + 4) == 0) {
		_numClickRegions--;

		for (uint16 i = index; i < _numClickRegions; i++)
			_clickRegions[i] = _clickRegions[i + 1];
	} else {
		region.left = left;
		region.top = top;
		region.right = right;
		region.bottom = bottom;
		region.script = READ_LE_UINT16(record + 4);

		if (_version >= kVersionV1_2_2 && region.script != 0)
			region.script = (uint16)(operandValue(region.script) - ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS));

		if (_version >= kVersionV1_2_6_1) {
			region.argument = READ_LE_UINT16(record + 0x0E);

			if (region.argument != 0)
				region.argument = (uint16)(operandValue(region.argument) - TG_HOST_V1_2_3_LITERAL_BIAS);
		}

		region.disabled = 0;
	}
}

bool TopgunEngine_v1_2::appendClickRegion() {
	uint16 maxRegions = TG_HOST_V1_2_MAX_CLICK_REGIONS;
	if (_version >= kVersionV1_2_6_1)
		maxRegions = TG_INPUT_V1_2_6_1_MAX_CLICK_REGIONS;
	else if (_version >= kVersionV1_2_5)
		maxRegions = TG_INPUT_V1_2_5_MAX_CLICK_REGIONS;
	else if (_version >= kVersionV1_2_3)
		maxRegions = TG_INPUT_V1_2_3_MAX_CLICK_REGIONS;

	if (_numClickRegions >= maxRegions)
		return false;

	if (_numClickRegions >= _clickRegions.size())
		_clickRegions.push_back(ClickRegion_v1_2());
	else if (_version >= kVersionV1_2_3)
		_clickRegions[_numClickRegions] = ClickRegion_v1_2();

	_numClickRegions++;
	return true;
}

void TopgunEngine_v1_2::setAllSpriteObjectsCursor(uint16 value) {
	if (value == 0) {
		_numClickRegions = 0;
	} else {
		for (uint16 i = 0; i < _numClickRegions; i++) {
			_clickRegions[i].script = value;
			_clickRegions[i].disabled = 0;
		}
	}

	if (!_records)
		return;

	uint16 spriteType = kResV1_2Sprite;
	if (_version >= kVersionV1_2_10)
		spriteType = kResV1_2_10Sprite;
	else if (_version >= kVersionV1_2_5)
		spriteType = kResV1_2_5Sprite;
	else if (_version >= kVersionV1_2_2_1)
		spriteType = kResV1_2_3Sprite;
	else if (_version >= kVersionV1_2_2)
		spriteType = kResV1_2_2Sprite;
	uint16 clickScriptOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_CLICK_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_CLICK_SCRIPT;

	for (uint16 i = 0; i < _numResources; i++) {
		if (resourceLib()->resGetType(_file, i) == spriteType && _records[i].data)
			WRITE_LE_UINT16(_records[i].data + clickScriptOffset, value);
	}
}

void TopgunEngine_v1_2::setAllClickRegionsAndSpriteObjectsEnabled(byte disabled) {
	for (uint16 i = 0; i < _numClickRegions; i++)
		_clickRegions[i].disabled = disabled;

	if (!_records)
		return;

	uint16 spriteType = kResV1_2Sprite;
	if (_version >= kVersionV1_2_10)
		spriteType = kResV1_2_10Sprite;
	else if (_version >= kVersionV1_2_5)
		spriteType = kResV1_2_5Sprite;
	else if (_version >= kVersionV1_2_2_1)
		spriteType = kResV1_2_3Sprite;
	else if (_version >= kVersionV1_2_2)
		spriteType = kResV1_2_2Sprite;
	uint16 disabledOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_DISABLED : TG_HOST_V1_2_SPRITE_DATA_DISABLED;
	if (_version >= kVersionV1_2_6_1)
		disabledOffset = TG_INPUT_V1_2_6_1_OBJECT_DISABLED;

	for (uint16 i = 0; i < _numResources; i++) {
		if (resourceLib()->resGetType(_file, i) == spriteType && _records[i].data)
			_records[i].data[disabledOffset] = disabled;
	}
}

bool TopgunEngine_v1_2::o_setSpriteObjectProperties(const byte *record) {
	uint16 id = READ_LE_UINT16(record + 2);

	ensureResourceDataLoaded(id);

	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "o_setSpriteObjectProperties") || !_records[index].data) {
		warning("TopgunEngine_v1_2::o_setSpriteObjectProperties(): the sprite %d has no data", index);
		return true;
	}

	byte *object = _records[index].data;
	uint16 dropScriptOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_DROP_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_DROP_SCRIPT;
	uint16 dragScriptOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_DRAG_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_DRAG_SCRIPT;
	uint16 draggableOffset = (_version >= kVersionV1_2_2_1) ? TG_INPUT_V1_2_3_OBJECT_DRAGGABLE : TG_HOST_V1_2_SPRITE_DATA_DRAGGABLE;

	if (_version >= kVersionV1_2_6_1) {
		dropScriptOffset = TG_INPUT_V1_2_6_1_OBJECT_DROP_SCRIPT;
		dragScriptOffset = TG_INPUT_V1_2_6_1_OBJECT_DRAG_SCRIPT;
		draggableOffset = TG_INPUT_V1_2_6_1_OBJECT_DRAGGABLE;
	}

	WRITE_LE_UINT16(object + dropScriptOffset, READ_LE_UINT16(record + 6));
	WRITE_LE_UINT16(object + dragScriptOffset, READ_LE_UINT16(record + 4));
	object[draggableOffset] = (record[8] == 0) ? 1 : 0;

	if (_dragSpriteId == id && record[8] != 0) {
		Common::Point cursor = g_system->getEventManager()->getMousePos();

		dropDraggedSprite(cursor.x, cursor.y);
	}

	return true;
}

bool TopgunEngine_v1_2::o_setHoverRegion(const byte *record) {
	byte clearFlag = record[0x0E];
	if (_version >= kVersionV1_2_7)
		clearFlag = record[0x14];
	else if (_version >= kVersionV1_2_6_1)
		clearFlag = record[0x12];

	if (_version >= kVersionV1_2_2 && clearFlag != 0) {
		if (_version >= kVersionV1_2_2_2)
			_hoverRegionCurrent = 0;

		_numHoverRegions = 0;
		return true;
	}

	if (READ_LE_UINT16(record + 0x0A) == 0)
		return removeHoverRegion(record);

	return addHoverRegion(record);
}

bool TopgunEngine_v1_2::addHoverRegion(const byte *record) {
	int16 left = (int16)operandValue(READ_LE_UINT16(record + 2));
	int16 top = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 right = (int16)operandValue(READ_LE_UINT16(record + 6));
	int16 bottom = (int16)operandValue(READ_LE_UINT16(record + 8));
	uint16 sprite = 0;
	uint16 index = 0;

	if (_version >= kVersionV1_2_7 && READ_LE_UINT16(record + 0x12) != 0)
		sprite = (uint16)(operandValue(READ_LE_UINT16(record + 0x12)) - TG_HOST_V1_2_3_LITERAL_BIAS);

	if (_numHoverRegions != 0) {
		do {
			const HoverRegion_v1_2 &region = _hoverRegions[index];

			if (sprite != 0) {
				if (region.sprite == sprite)
					break;
			} else if (region.left == left && region.top == top && region.right == right && region.bottom == bottom) {
				break;
			}

			index++;
		} while (index < _numHoverRegions);
	}

	if (_numHoverRegions <= index) {
		uint16 maxRegions = TG_HOST_V1_2_MAX_HOVER_REGIONS;
		if (_version >= kVersionV1_2_10)
			maxRegions = TG_HOST_V1_2_10_MAX_HOVER_REGIONS;

		if (_numHoverRegions >= maxRegions)
			return false;

		index = _numHoverRegions;
		_numHoverRegions++;
	}

	HoverRegion_v1_2 &region = _hoverRegions[index];

	region.left = left;
	region.top = top;
	region.right = right;
	region.bottom = bottom;

	if (_version >= kVersionV1_2_6_1) {
		uint16 bias = TG_HOST_V1_2_3_LITERAL_BIAS;
		uint16 enterScript = READ_LE_UINT16(record + 0x0A);
		uint16 enterArgument = READ_LE_UINT16(record + 0x0C);
		uint16 leaveScript = READ_LE_UINT16(record + 0x0E);
		uint16 leaveArgument = READ_LE_UINT16(record + 0x10);

		region.enterScript = (enterScript != 0) ? (uint16)(operandValue(enterScript) - bias) : 0;
		region.enterArgument = (enterArgument != 0) ? (uint16)(operandValue(enterArgument) - bias) : 0;
		region.leaveScript = (leaveScript != 0) ? (uint16)(operandValue(leaveScript) - bias) : 0;
		region.leaveArgument = (leaveArgument != 0) ? (uint16)(operandValue(leaveArgument) - bias) : 0;
		region.sprite = sprite;
	} else {
		region.enterScript = READ_LE_UINT16(record + 0x0A);
		region.leaveScript = READ_LE_UINT16(record + 0x0C);

		if (_version >= kVersionV1_2_2) {
			uint16 bias = (_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS;

			if (region.enterScript != 0)
				region.enterScript = (uint16)(operandValue(region.enterScript) - bias);

			if (region.leaveScript != 0)
				region.leaveScript = (uint16)(operandValue(region.leaveScript) - bias);
		}
	}

	Common::Point cursor = g_system->getEventManager()->getMousePos();
	transformPointForDisplayZoom(cursor);

	if (_version >= kVersionV1_2_7 || (region.left <= cursor.x && region.right > cursor.x && region.top <= cursor.y && region.bottom > cursor.y))
		updateHoverRegionFromPoint(cursor.x, cursor.y);

	return true;
}

bool TopgunEngine_v1_2::removeHoverRegion(const byte *record) {
	int16 left = (int16)operandValue(READ_LE_UINT16(record + 2));
	int16 top = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 right = (int16)operandValue(READ_LE_UINT16(record + 6));
	int16 bottom = (int16)operandValue(READ_LE_UINT16(record + 8));
	uint16 sprite = 0;
	uint16 index = 0;

	if (_version >= kVersionV1_2_7 && READ_LE_UINT16(record + 0x12) != 0)
		sprite = (uint16)(operandValue(READ_LE_UINT16(record + 0x12)) - TG_HOST_V1_2_3_LITERAL_BIAS);

	if (_numHoverRegions != 0) {
		do {
			const HoverRegion_v1_2 &region = _hoverRegions[index];

			if (sprite != 0) {
				if (region.sprite == sprite)
					break;
			} else if (region.left == left && region.top == top && region.right == right && region.bottom == bottom) {
				break;
			}

			index++;
		} while (index < _numHoverRegions);
	}

	if (_numHoverRegions > index) {
		_numHoverRegions--;

		for (uint16 i = index; i < _numHoverRegions; i++)
			_hoverRegions[i] = _hoverRegions[i + 1];

		if (_hoverRegionCurrent != 0) {
			uint16 current = _hoverRegionCurrent - 1;

			if (current == index)
				_hoverRegionCurrent = 0;
			else if (current > index)
				_hoverRegionCurrent--;
		}
	}

	return true;
}

void TopgunEngine_v1_2::updateHoverRegionFromPoint(int16 x, int16 y) {
	int16 found = (int16)_numHoverRegions - 1;

	while (found >= 0) {
		const HoverRegion_v1_2 &region = _hoverRegions[found];

		if (_version >= kVersionV1_2_7 && region.sprite != 0) {
			uint16 index = operandValue(region.sprite);

			if (isRecordIndexValid(index, "updateHoverRegionFromPoint") && _records[index].handle != 0 && spriteLib()->spriteIsPicked(_records[index].handle, x, y))
				break;
		} else if (region.left <= x && region.right > x && region.top <= y && region.bottom > y) {
			break;
		}

		found--;
	}

	uint16 leaveArgument = 0;
	uint16 enterArgument = 0;

	if (_version >= kVersionV1_2_2_2) {
		leaveArgument = (uint16)(0 - TG_HOST_V1_2_3_LITERAL_BIAS);
		enterArgument = (uint16)(1 - TG_HOST_V1_2_3_LITERAL_BIAS);
	} else if (_version >= kVersionV1_2_2_1) {
		leaveArgument = (uint16)(0 - TG_HOST_V1_2_LITERAL_BIAS);
		enterArgument = (uint16)(1 - TG_HOST_V1_2_LITERAL_BIAS);
	}

	int16 current = (int16)_hoverRegionCurrent - 1;

	if (current != found) {
		if (current >= 0) {
			uint16 script = _hoverRegions[current].leaveScript;

			if (script != 0 && !postRunScript(script, leaveArgument, _hoverRegions[current].leaveArgument))
				return;
		}

		_hoverRegionCurrent = 0;
	}

	if (found < 0 || current == found)
		return;

	uint16 script = _hoverRegions[found].enterScript;

	if (script != 0 && !postRunScript(script, enterArgument, _hoverRegions[found].enterArgument))
		return;

	_hoverRegionCurrent = (uint16)(found + 1);
}

void TopgunEngine_v1_2::handleKeyUpMessage(Common::KeyCode keycode) {
	uint16 key = keyCodeToScriptKey(keycode);

	if (_version >= kVersionV1_2_2_1) {
		if (key == TG_HOST_V1_2_NO_KEY)
			return;

		if (callMessageFilter(kMsgV1_2_3KeyUp, key, 0, 0))
			return;
	}

	if (_keyboardRouted && !keyboardRoutedKeyUp(key))
		return;

	if (_version >= kVersionV1_2_2_1 && _gamePaused)
		return;

	uint16 keyTableSize = (_version >= kVersionV1_2_3) ? TG_INPUT_V1_2_3_KEY_TABLE_SIZE : TG_HOST_V1_2_KEY_TABLE_SIZE;

	if (key >= keyTableSize)
		return;

	uint16 script = _keyTable[key].keyUp;

	if (script != 0) {
		if (_version >= kVersionV1_2_10)
			static_cast<TopgunEngine_v1_2_3 *>(this)->runScriptHookWithArg(script, key);
		else
			runScriptResource(script);
	}
}

void TopgunEngine_v1_2::togglePauseState() {
	if (_version < kVersionV1_2_2)
		audioLib()->audioStopAllWaves(nullptr);
	else if (_version < kVersionV1_2_3)
		audioLib()->audioPause(_gamePaused ? 0 : 1);

	if (_pauseScript != 0) {
		byte record[18];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record + 2, _pauseScript);

		if (_version >= kVersionV1_2_2) {
			record[5] = 1;
			WRITE_LE_UINT16(record + 6, (uint16)((_gamePaused ? 0 : 1) - ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS)));
		} else {
			record[4] = 1;
			WRITE_LE_UINT16(record + 5, (uint16)((_gamePaused ? 0 : 1) - TG_HOST_V1_2_LITERAL_BIAS));
		}

		uint16 savedResult = _scriptResult;

		if (_version >= kVersionV1_2_6)
			_scriptResult = 0;

		o_callScript(record);

		uint16 hookResult = _scriptResult;

		if (_version >= kVersionV1_2_10)
			_scriptResult = savedResult;

		if (_version >= kVersionV1_2_6 && hookResult != 0)
			return;

		spriteLib()->spriteAnimate();
	} else if (_version >= kVersionV1_2_3) {
		audioLib()->audioPause(_gamePaused ? 0 : 1);
	}

	_gamePaused = !_gamePaused;

	if (_version >= kVersionV1_2_10)
		static_cast<TopgunEngine_v1_2_3 *>(this)->notifyServicePause(_gamePaused);

	if (_version >= kVersionV1_2_6)
		static_cast<TopgunEngine_v1_2_3 *>(this)->releaseOrRestoreDeviceGroups(_gamePaused);
}

uint16 TopgunEngine_v1_2_3::postKeyHookScript(uint16 vk) {
	byte record[18];

	memset(record, 0, sizeof(record));
	WRITE_LE_UINT16(record + 2, _keyHookScript);
	WRITE_LE_UINT16(record + 6, (uint16)(vk - TG_HOST_V1_2_3_LITERAL_BIAS));
	record[5] = 1;

	uint16 saved = _scriptResult;
	_scriptResult = 0;
	o_callScript(record);

	uint16 result = _scriptResult;
	_scriptResult = saved;
	return result;
}

uint16 TopgunEngine_v1_2_3::runScriptHookWithArg(uint16 script, uint16 argument) {
	byte record[18];

	memset(record, 0, sizeof(record));
	WRITE_LE_UINT16(record + 2, script);
	WRITE_LE_UINT16(record + 6, (uint16)(argument - TG_HOST_V1_2_3_LITERAL_BIAS));
	record[5] = 1;

	uint16 saved = _scriptResult;
	_scriptResult = 0;
	o_callScript(record);

	uint16 result = _scriptResult;
	_scriptResult = saved;
	return result;
}

bool TopgunEngine_v1_2_3::postRunScript(uint16 script, uint16 argument1, uint16 argument2) {
	return enqueuePostedScriptRun(script, argument1, argument2);
}

void TopgunEngine_v1_2_3::cancelMouseCapture() {
	if (!_captureDrag)
		return;

	_rubberBandMode = false;
	_captureDrag = false;
	spriteLib()->spriteRubberBand(false, 0, 0, false);
}

void TopgunEngine_v1_2_3::callRubberBandScript() {
	byte record[18];

	memset(record, 0, sizeof(record));
	WRITE_LE_UINT16(record + 2, _rubberBandScript);
	WRITE_LE_UINT16(record + 6, (uint16)(_captureDragStart.x - TG_HOST_V1_2_3_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 8, (uint16)(_captureDragStart.y - TG_HOST_V1_2_3_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 10, (uint16)(_captureDragEnd.x - TG_HOST_V1_2_3_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 12, (uint16)(_captureDragEnd.y - TG_HOST_V1_2_3_LITERAL_BIAS));
	record[5] = 4;

	uint16 saved = _scriptResult;
	_scriptResult = 0;
	o_callScript(record);
	_scriptResult = saved;
}

void TopgunEngine_v1_2_3::idleRearm() {
	_idleDeadline = getEngineMillis() + _idleDelay;
}

void TopgunEngine_v1_2_3::idleCheck() {
	if (_idleScript == 0)
		return;

	if (getEngineMillis() < _idleDeadline)
		return;

	enqueuePostedScriptRun(_idleScript, 0, 0);
	_idleScript = 0;
}

bool TopgunEngine_v1_2_3::updateSpriteHoverFromPoint(int16 x, int16 y) {
	uint16 sprite = spriteLib()->spritePicked(x, y);

	if (sprite == 0) {
		if (_hoverSprite != 0)
			postSpriteLeaveScript();

		return false;
	}

	uint16 id = spriteLib()->spriteGetId(sprite);

	if (_hoverSprite == id)
		return true;

	if (_hoverSprite != 0)
		postSpriteLeaveScript();

	if (_version < kVersionV1_2_3) {
		byte record[18];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record + 2, _hoverSpriteScript);
		WRITE_LE_UINT16(record + 6, id);
		WRITE_LE_UINT16(record + 8, (uint16)(1 - TG_HOST_V1_2_3_LITERAL_BIAS));
		record[5] = 2;
		o_callScript(record);
		_hoverSprite = id;
	} else {
		_hoverSprite = id;
		enqueuePostedScriptRun(_hoverSpriteScript, id, (uint16)(1 - TG_HOST_V1_2_3_LITERAL_BIAS));
	}

	return true;
}

void TopgunEngine_v1_2_3::postSpriteLeaveScript() {
	if (_version < kVersionV1_2_3) {
		byte record[18];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record + 2, _hoverSpriteScript);
		WRITE_LE_UINT16(record + 6, _hoverSprite);
		WRITE_LE_UINT16(record + 8, (uint16)(0 - TG_HOST_V1_2_3_LITERAL_BIAS));
		record[5] = 2;
		o_callScript(record);
	} else {
		enqueuePostedScriptRun(_hoverSpriteScript, _hoverSprite, (uint16)(0 - TG_HOST_V1_2_3_LITERAL_BIAS));
	}

	_hoverSprite = 0;
}

void TopgunEngine_v1_2_3::o_getKeyState(const byte *record) {
	uint16 key = READ_LE_UINT16(record + 4);

	if (_version >= kVersionV1_2_7) {
		key = operandValue(key);

		if (key >= TG_HOST_V1_2_KEY_STATE_SIZE)
			return;
	}

	writeVariable(READ_LE_UINT16(record + 2), builtinGetKeyState(key));
}

void TopgunEngine_v1_2_3::o_setMouseEventScript(const byte *record) {
	uint16 script = READ_LE_UINT16(record + 2);

	if (_version >= kVersionV1_2_7 && script != 0)
		script = (uint16)(operandValue(script) - TG_HOST_V1_2_3_LITERAL_BIAS);

	_mouseEventScript = script;
}

void TopgunEngine_v1_2::o_setJoystickScript(const byte *record) {
	uint16 index = READ_LE_UINT16(record + 2);

	if (index >= TG_JOYSTICK_V1_2_1_NUM_SCRIPTS) {
		warning("TopgunEngine_v1_2::o_setJoystickScript(): slot %d is out of range", index);
		return;
	}

	_joystickScripts[index] = READ_LE_UINT16(record + 4);
}

uint16 TopgunEngine_v1_2_3::builtinGetKeyState(uint16 vk) {
	if (vk < TG_HOST_V1_2_KEY_STATE_SIZE && _keyStates[vk])
		return 1;

	return 0;
}

uint16 TopgunEngine_v1_2_3::builtinSetDraggedSpriteId(uint16 id) {
	uint16 raw = (uint16)(id - TG_HOST_V1_2_3_LITERAL_BIAS);
	uint16 spriteType = kResV1_2_3Sprite;
	if (_version >= kVersionV1_2_10)
		spriteType = kResV1_2_10Sprite;
	else if (_version >= kVersionV1_2_5)
		spriteType = kResV1_2_5Sprite;

	if (!isResourceObjectLoaded(raw, spriteType))
		return 0;

	_dragSpriteId = raw;
	return 1;
}

uint16 TopgunEngine_v1_2_3::builtinSetKeyHookScript(uint16 script) {
	uint16 previous = _keyHookScript;

	_keyHookScript = script != 0 ? (uint16)(script - TG_HOST_V1_2_3_LITERAL_BIAS) : 0;
	return operandValue(previous);
}

uint16 TopgunEngine_v1_2_3::builtinSetSpriteHoverScript(uint16 script) {
	uint16 previous = _hoverSpriteScript;
	uint16 scriptType = kResV1_2_3Script;
	if (_version >= kVersionV1_2_10)
		scriptType = kResV1_2_10Script;
	else if (_version >= kVersionV1_2_5)
		scriptType = kResV1_2_5Script;

	if (script == 0 || resourceLib()->resGetType(_file, script) == scriptType) {
		if (_hoverSpriteScript != 0 && _hoverSprite != 0)
			postSpriteLeaveScript();

		_hoverSpriteScript = script != 0 ? (uint16)(script - TG_HOST_V1_2_3_LITERAL_BIAS) : 0;

		if (script != 0) {
			Common::Point cursor = g_system->getEventManager()->getMousePos();

			transformPointForDisplayZoom(cursor);
			updateSpriteHoverFromPoint(cursor.x, cursor.y);
		}
	}

	return operandValue(previous);
}

uint16 TopgunEngine_v1_2_3::builtinGetMouseEventScript() {
	return operandValue(_mouseEventScript);
}

uint16 TopgunEngine_v1_2_3::builtinSetJoystickDistanceMode(uint16 value) {
	_joystickDirectionFlag = (value != 0);
	return 110;
}

uint16 TopgunEngine_v1_2_3::builtinSetJoystickButtonOffsetMode(uint16 value) {
	_joystickButtonFlag = (value != 0);
	return 130;
}

static const byte kDeviceCodes[TG_DEVICE_V1_2_2_CODE_TABLE_SIZE] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x06, 0x08, 0x0A, 0x09, 0x0C, 0x0B, 0x00, 0x00, 0x07,
	0x06, 0x03, 0x0A, 0x09, 0x05, 0x0C, 0x0B, 0xFE, 0x01, 0xFD, 0x02, 0xFB, 0x04, 0xF7, 0x08, 0x00
};

static const byte kDirectionCodes[16] = {
	0x00, 0x03, 0x05, 0x88, 0x07, 0x0A, 0x0C, 0x88, 0x06, 0x09, 0x0B, 0x88, 0x88, 0x88, 0x88, 0x88
};

static const byte kMouseStickCodes[4] = { 5, 7, 3, 6 };

void TopgunEngine_v1_2::clearJoystickScripts() {
	for (uint16 i = 0; i < TG_JOYSTICK_V1_2_1_NUM_SCRIPTS; i++)
		_joystickScripts[i] = 0;
}

void TopgunEngine_v1_2::clearDeviceState() {
	for (uint16 i = 0; i < TG_INPUT_V1_2_3_KEY_TABLE_SIZE; i++)
		_keyRoutes[i] = KeyRoute_v1_2_3();

	for (uint16 i = 0; i < TG_DEVICE_V1_2_2_NUM_GROUPS; i++)
		_deviceGroups[i] = DeviceGroup_v1_2_3();

	_mouseStickDeadRect = Common::Rect();
	_mouseStickOuterRect = Common::Rect();
	_mouseStickCode = 0;

	for (uint16 i = 0; i < TG_DEVICE_V1_2_2_NUM_DEVICES; i++)
		_deviceOwners[i] = 0;

	for (uint16 i = 0; i < TG_DEVICE_V1_2_2_NUM_KEYBOARD_SETS; i++)
		_keyboardDirections[i] = 0;

	for (uint16 i = 0; i < TG_JOYSTICK_V1_2_1_NUM_SCRIPTS; i++)
		_joystickStates[i] = 0;

	for (uint16 i = 0; i < TG_JOYSTICK_V1_2_1_NUM_STICKS; i++)
		_joystickLastIndex[i] = 0;
}

uint16 TopgunEngine_v1_2::insertDeviceBindingSorted(uint16 count, DeviceBinding_v1_2_3 *list, const byte *record) {
	uint16 position = 0;

	if (count != 0) {
		uint16 priority = READ_LE_UINT16(record + 8);

		do {
			if (priority >= list[position].priority)
				break;

			position++;
		} while (position < count);
	}

	for (uint16 i = count; i > position; i--)
		list[i] = list[i - 1];

	list[position].id = READ_LE_UINT16(record + 4);
	list[position].script = READ_LE_UINT16(record + 6);
	list[position].priority = READ_LE_UINT16(record + 8);
	list[position].mask = READ_LE_UINT16(record + 10);
	list[position].active = 1;
	return count + 1;
}

void TopgunEngine_v1_2::setAllBindingScripts(uint16 script, uint16 count, DeviceBinding_v1_2_3 *list) {
	for (uint16 i = 0; i < count; i++)
		list[i].script = script;
}

void TopgunEngine_v1_2::setAllBindingActive(byte active, uint16 count, DeviceBinding_v1_2_3 *list) {
	for (uint16 i = 0; i < count; i++)
		list[i].active = active;
}

void TopgunEngine_v1_2::fillJoystickStateBytes(byte value, byte stick) {
	if (stick >= TG_JOYSTICK_V1_2_1_NUM_STICKS) {
		warning("TopgunEngine_v1_2::fillJoystickStateBytes(): stick %d is out of range", stick);
		return;
	}

	for (uint16 i = 0; i < TG_DEVICE_V1_2_2_JOYSTICK_STATE_SIZE; i++)
		_joystickStates[stick * TG_DEVICE_V1_2_2_JOYSTICK_STATE_SIZE + i] = value;
}

uint16 TopgunEngine_v1_2::deleteDeviceBinding(uint16 index, uint16 count, DeviceBinding_v1_2_3 *list) {
	if (count <= index) {
		if (_version >= kVersionV1_2_2_1)
			return count;

		// Bug in the original: it takes one off the count and moves 64 KB of memory from past the last binding
		warning("TopgunEngine_v1_2::deleteDeviceBinding(): there is no binding %d to delete", index);
		return count != 0 ? count - 1 : 0;
	}

	count--;

	for (uint16 i = index; i < count; i++)
		list[i] = list[i + 1];

	return count;
}

uint16 TopgunEngine_v1_2::updateDeviceBindingList(uint16 maxEntries, uint16 count, DeviceBinding_v1_2_3 *list, const byte *record) {
	uint16 index = 0;

	if (count != 0) {
		uint16 id = READ_LE_UINT16(record + 4);

		do {
			if (id == list[index].id)
				break;

			index++;
		} while (index < count);
	}

	if (maxEntries <= index)
		return maxEntries;

	if (record[0x0D] != 0) {
		list[index].active = 1;
		return count;
	}

	if (record[0x0E] != 0) {
		list[index].active = 0;
		return count;
	}

	if (READ_LE_UINT16(record + 6) == 0)
		return deleteDeviceBinding(index, count, list);

	if (READ_LE_UINT16(record + 8) == 0) {
		list[index].script = READ_LE_UINT16(record + 6);
		list[index].id = READ_LE_UINT16(record + 4);
		return count;
	}

	if (maxEntries > count)
		return insertDeviceBindingSorted(count, list, record);

	return count;
}

void TopgunEngine_v1_2_3::fireDeviceGroupStateScript(byte group) {
	DeviceGroup_v1_2_3 &state = _deviceGroups[group];
	uint16 pressedMask = state.pressedMask;
	uint16 lastCode = state.lastCode;

	if (_version >= kVersionV1_2_6) {
		state.pressedMask = 0;
		state.lastCode = 0;
	}

	if (state.stateScript == 0)
		return;

	if ((pressedMask & 1) != 0)
		enqueuePostedScriptRun(state.stateScript, (uint16)(group - TG_HOST_V1_2_3_LITERAL_BIAS), (uint16)(0xFE - TG_HOST_V1_2_3_LITERAL_BIAS));

	if ((pressedMask & 2) != 0)
		enqueuePostedScriptRun(state.stateScript, (uint16)(group - TG_HOST_V1_2_3_LITERAL_BIAS), (uint16)(0xFD - TG_HOST_V1_2_3_LITERAL_BIAS));

	if (_version >= kVersionV1_2_6) {
		if ((pressedMask & 4) != 0)
			enqueuePostedScriptRun(state.stateScript, (uint16)(group - TG_HOST_V1_2_3_LITERAL_BIAS), (uint16)(0xFB - TG_HOST_V1_2_3_LITERAL_BIAS));

		if ((pressedMask & 8) != 0)
			enqueuePostedScriptRun(state.stateScript, (uint16)(group - TG_HOST_V1_2_3_LITERAL_BIAS), (uint16)(0xF7 - TG_HOST_V1_2_3_LITERAL_BIAS));
	}

	if (lastCode != 0)
		enqueuePostedScriptRun(state.stateScript, (uint16)(group - TG_HOST_V1_2_3_LITERAL_BIAS), (uint16)(0 - TG_HOST_V1_2_3_LITERAL_BIAS));
}

void TopgunEngine_v1_2_3::releaseOrRestoreDeviceGroups(bool release) {
	for (byte group = 0; group < TG_DEVICE_V1_2_2_NUM_GROUPS; group++) {
		if (_deviceGroups[group].attached == 0)
			continue;

		if (release)
			fireDeviceGroupStateScript(group);
		else
			resetDeviceGroup(1, group);
	}
}

void TopgunEngine_v1_2_3::resetDeviceGroup(byte value, byte group) {
	DeviceGroup_v1_2_3 &state = _deviceGroups[group];

	switch (state.type) {
	case TG_DEVICE_V1_2_2_TYPE_JOYSTICK:
		fillJoystickStateBytes(value, state.device - TG_DEVICE_V1_2_3_JOYSTICK_BASE);
		break;

	case TG_DEVICE_V1_2_2_TYPE_KEYBOARD:
		bindKeyboardSetToDevice(state.device - TG_DEVICE_V1_2_3_KEYBOARD_BASE, value);
		break;

	case TG_DEVICE_V1_2_3_TYPE_MOUSE:
		resetMouseStick(value);
		break;

	default:
		break;
	}
}

void TopgunEngine_v1_2::callDeviceGroupScript(uint16 group, uint16 argument, uint16 script) {
	byte record[18];
	uint16 literalBias = (_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS;

	memset(record, 0, sizeof(record));
	WRITE_LE_UINT16(record + 2, script);
	WRITE_LE_UINT16(record + 6, (uint16)(group - literalBias));
	WRITE_LE_UINT16(record + 8, argument);
	record[5] = 2;
	o_callScript(record);
}

void TopgunEngine_v1_2::comboHistoryTimeoutCallback(byte timerId) {
	byte groupIndex = (byte)(timerId - 2);

	if (groupIndex >= TG_DEVICE_V1_2_2_NUM_GROUPS) {
		warning("TopgunEngine_v1_2::comboHistoryTimeoutCallback(): timer %d is out of range", timerId);
		return;
	}

	DeviceGroup_v1_2_3 &group = _deviceGroups[groupIndex];

	if (group.numBindingsB != 0) {
		scanDeviceBindingsListB(groupIndex);
		return;
	}

	uint16 history = group.history;
	group.history = 0;

	for (uint16 i = 0; i < group.numBindingsA; i++) {
		const DeviceBinding_v1_2_3 &entry = group.bindingsA[i];

		if ((uint16)(entry.mask & history) == entry.priority && entry.active != 0 && entry.script != 0) {
			if (group.latchMode != 0)
				group.latch = entry;
			else if (_version < kVersionV1_2_3)
				callDeviceGroupScript(groupIndex, entry.id, entry.script);
			else
				static_cast<TopgunEngine_v1_2_3 *>(this)->enqueuePostedScriptRun(entry.script, (uint16)(groupIndex - TG_HOST_V1_2_3_LITERAL_BIAS), entry.id);

			return;
		}
	}
}

void TopgunEngine_v1_2::scanDeviceBindingsListB(byte group) {
	if (group >= TG_DEVICE_V1_2_2_NUM_GROUPS) {
		warning("TopgunEngine_v1_2::scanDeviceBindingsListB(): group %d is out of range", group);
		return;
	}

	DeviceGroup_v1_2_3 &state = _deviceGroups[group];
	uint16 history = state.history;

	state.history = 0;

	for (uint16 i = 0; i < state.numBindingsB; i++) {
		const DeviceBinding_v1_2_3 &entry = state.bindingsB[i];

		if ((uint16)(entry.mask & history) == entry.priority && entry.active != 0 && entry.script != 0) {
			if (state.latchMode != 0)
				state.latch = entry;
			else if (_version < kVersionV1_2_3)
				callDeviceGroupScript(group, entry.id, entry.script);
			else
				static_cast<TopgunEngine_v1_2_3 *>(this)->enqueuePostedScriptRun(entry.script, (uint16)(group - TG_HOST_V1_2_3_LITERAL_BIAS), entry.id);

			return;
		}
	}
}

void TopgunEngine_v1_2::deviceGroupInputEvent(byte code, byte device) {
	if (device >= TG_DEVICE_V1_2_2_NUM_DEVICES) {
		warning("TopgunEngine_v1_2::deviceGroupInputEvent(): device %d is out of range", device);
		return;
	}

	byte groupIndex = _deviceOwners[device];

	if (_version < kVersionV1_2_3) {
		if (device >= TG_DEVICE_V1_2_2_NUM_GROUPS) {
			warning("TopgunEngine_v1_2::deviceGroupInputEvent(): group %d is out of range", device);
			return;
		}

		groupIndex = device;
	}

	DeviceGroup_v1_2_3 &group = _deviceGroups[groupIndex];
	byte sentCode = code;
	bool released = false;

	switch (code) {
	case 0xF7:
	case 0xFB:
	case 0xFD:
	case 0xFE:
		group.pressedMask &= code;
		released = true;
		break;

	case 0:
		group.lastCode = 0;
		released = true;
		break;

	case 1:
	case 2:
	case 4:
	case 8:
		group.pressedMask |= code;
		break;

	default:
		if (group.remap == 1) {
			if (code < TG_DEVICE_V1_2_2_CODE_TABLE_SIZE)
				sentCode = kDeviceCodes[code];
			else
				warning("TopgunEngine_v1_2::deviceGroupInputEvent(): code %d is out of range", code);
		}

		group.lastCode = (_version >= kVersionV1_2_2_1) ? code : sentCode;
		break;
	}

	if (_version >= kVersionV1_2_3 && _textEditObjectId != 0)
		return;

	if (group.stateScript != 0) {
		uint16 literalBias = (_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS;

		if (_version < kVersionV1_2_3)
			callDeviceGroupScript(groupIndex, (uint16)(sentCode - literalBias), group.stateScript);
		else
			static_cast<TopgunEngine_v1_2_3 *>(this)->enqueuePostedScriptRun(group.stateScript, (uint16)(groupIndex - TG_HOST_V1_2_3_LITERAL_BIAS), (uint16)(sentCode - TG_HOST_V1_2_3_LITERAL_BIAS));
	}

	if (released)
		return;

	if (group.history == 0)
		addNativeTimer(groupIndex + 2, _inputTimeout, kNativeTimerV1_2_3ComboHistory, 0);

	group.history = (uint16)(((group.history & 0x00FF) << 4) + sentCode);
}

bool TopgunEngine_v1_2::keyboardRoutedKeyDown(uint16 vk) {
	if (_version >= kVersionV1_2_2_1 && _gamePaused)
		return true;

	if (vk >= TG_INPUT_V1_2_3_KEY_TABLE_SIZE)
		return true;

	// The original has only 128 routes and reads the device groups past them
	if (_version < kVersionV1_2_2_2 && vk >= TG_HOST_V1_2_KEY_TABLE_SIZE) {
		warning("TopgunEngine_v1_2::keyboardRoutedKeyDown(): key %d is out of range", vk);
		return true;
	}

	KeyRoute_v1_2_3 &route = _keyRoutes[vk];

	// Bug in the first version with device groups: it swallows the keys it does not route and hands on the routed ones
	if (route.armed == 0)
		return _version >= kVersionV1_2_2_1;

	if (_version < kVersionV1_2_2_2 && (_keyStates[TG_HOST_V1_2_VK_SHIFT] || _keyStates[TG_HOST_V1_2_VK_CONTROL]))
		return _version == kVersionV1_2_2_1;

	route.armed = 0;

	byte code = route.code;
	byte device = route.device;

	if (code == 0x10 || code == 0x20 || code == 0x40 || code == 0x80) {
		byte previousCode = kDirectionCodes[_keyboardDirections[device]];

		_keyboardDirections[device] |= (byte)(code >> 4);
		findNativeTimer(device);

		code = kDirectionCodes[_keyboardDirections[device]];
		if (code == TG_DEVICE_V1_2_2_NO_DIRECTION) {
			if (_version < kVersionV1_2_6 || previousCode == TG_DEVICE_V1_2_2_NO_DIRECTION)
				return false;

			code = 0;
		}
	}

	byte inputDevice = device + TG_DEVICE_V1_2_3_KEYBOARD_BASE;
	if (_version < kVersionV1_2_3)
		inputDevice = device;

	deviceGroupInputEvent(code, inputDevice);
	return _version == kVersionV1_2_2;
}

bool TopgunEngine_v1_2::keyboardRoutedKeyUp(uint16 vk) {
	if (_version >= kVersionV1_2_2_1 && _gamePaused)
		return true;

	if (vk >= TG_INPUT_V1_2_3_KEY_TABLE_SIZE)
		return true;

	if (_version < kVersionV1_2_2_2 && vk >= TG_HOST_V1_2_KEY_TABLE_SIZE) {
		warning("TopgunEngine_v1_2::keyboardRoutedKeyUp(): key %d is out of range", vk);
		return true;
	}

	KeyRoute_v1_2_3 &route = _keyRoutes[vk];

	if (route.code == 0)
		return _version >= kVersionV1_2_2_1;

	route.armed = 1;

	byte code = route.code;
	byte device = route.device;

	if (code == 0x10 || code == 0x20 || code == 0x40 || code == 0x80) {
		byte previousCode = kDirectionCodes[_keyboardDirections[device]];

		_keyboardDirections[device] &= (byte)~(code >> 4);

		if (_keyboardDirections[device] != 0 && (_version < kVersionV1_2_6 || previousCode != TG_DEVICE_V1_2_2_NO_DIRECTION)) {
			addNativeTimer(device, TG_DEVICE_V1_2_2_KEY_RELEASE_DELAY, kNativeTimerV1_2_3KeyRelease, 0);

			// Bug in the demo: the code of the released key is also sent right away
			if (_version != kVersionV1_2_2_1)
				return _version == kVersionV1_2_2;
		} else {
			findNativeTimer(device);

			code = kDirectionCodes[_keyboardDirections[device]];
			if (code == TG_DEVICE_V1_2_2_NO_DIRECTION)
				return false;
		}
	} else {
		code = (byte)~code;
	}

	byte inputDevice = device + TG_DEVICE_V1_2_3_KEYBOARD_BASE;
	if (_version < kVersionV1_2_3)
		inputDevice = device;

	deviceGroupInputEvent(code, inputDevice);
	return _version == kVersionV1_2_2;
}

void TopgunEngine_v1_2::keyReleaseDelayTimerCallback(byte device) {
	if (device >= TG_DEVICE_V1_2_2_NUM_KEYBOARD_SETS) {
		warning("TopgunEngine_v1_2::keyReleaseDelayTimerCallback(): device %d is out of range", device);
		return;
	}

	byte code = kDirectionCodes[_keyboardDirections[device]];

	byte inputDevice = device + TG_DEVICE_V1_2_3_KEYBOARD_BASE;
	if (_version < kVersionV1_2_3)
		inputDevice = device;

	if (code != TG_DEVICE_V1_2_2_NO_DIRECTION)
		deviceGroupInputEvent(code, inputDevice);
}

void TopgunEngine_v1_2::bindKeyboardSetToDevice(byte device, byte value) {
	if (device >= TG_DEVICE_V1_2_2_NUM_KEYBOARD_SETS) {
		warning("TopgunEngine_v1_2::bindKeyboardSetToDevice(): keyboard set %d is out of range", device);
		return;
	}

	// The demo keeps the other routes and also writes the ones of a set that is being released
	if (_version >= kVersionV1_2_2_2) {
		for (uint16 i = 0; i < TG_INPUT_V1_2_3_KEY_TABLE_SIZE; i++) {
			if (_keyRoutes[i].device == device) {
				_keyRoutes[i].armed = 0;
				_keyRoutes[i].code = 0;
			}
		}

		if (value == 0)
			return;
	}

	if (_version >= kVersionV1_2_3)
		_keyboardDirections[device] = 0;

	uint16 numSlots = TG_DEVICE_V1_2_2_KEYBOARD_SET_SLOTS;
	if (_version >= kVersionV1_2_5)
		numSlots = TG_DEVICE_V1_2_5_KEYBOARD_SET_SLOTS;

	for (uint16 slot = 0; slot < numSlots; slot++) {
		byte vk = _keyboardSetKeys[device][slot];

		_keyRoutes[vk].device = device;
		_keyRoutes[vk].armed = value;
		_keyRoutes[vk].code = _keyboardSetCodes[device][slot];

		if (_version >= kVersionV1_2_3 && _keyStates[vk])
			keyboardRoutedKeyDown(vk);
	}
}

bool TopgunEngine_v1_2_3::mouseAsStickMessage(Common::Point point, uint16 message) {
	if (_gamePaused)
		return true;

	byte code = 0;

	if (message == kMsgV1_2MouseMove) {
		transformPointForDisplayZoom(point);
		_variableTable[TG_HOST_V1_2_3_VARIABLE_MOUSE_X] = (uint16)point.x;
		_variableTable[TG_HOST_V1_2_3_VARIABLE_MOUSE_Y] = (uint16)point.y;

		if (_mouseStickDeadRect.contains(point.x, point.y)) {
			code = 0;
		} else {
			int16 angle = utilsLibrary()->getAngle(point.x, point.y);

			if (!_mouseStickOuterRect.contains(point.x, point.y)) {
				point = computeRayRectIntersection(angle, 0, 0,
					_mouseStickOuterRect.left + TG_DEVICE_V1_2_3_STICK_RAY_MARGIN, _mouseStickOuterRect.top + TG_DEVICE_V1_2_3_STICK_RAY_MARGIN,
					_mouseStickOuterRect.right - TG_DEVICE_V1_2_3_STICK_RAY_MARGIN, _mouseStickOuterRect.bottom - TG_DEVICE_V1_2_3_STICK_RAY_MARGIN);
				_variableTable[TG_HOST_V1_2_3_VARIABLE_MOUSE_X] = (uint16)point.x;
				_variableTable[TG_HOST_V1_2_3_VARIABLE_MOUSE_Y] = (uint16)point.y;
				g_system->warpMouse(point.x + display()->viewportOrigin().x, point.y + display()->viewportOrigin().y);
			}

			int16 quadrant = utilsLibrary()->getQuadrant((int16)((angle + TG_DEVICE_V1_2_3_STICK_ANGLE_ROTATION) % TG_DEVICE_V1_2_3_STICK_FULL_TURN));
			code = kMouseStickCodes[quadrant];
		}

		if (code == _mouseStickCode)
			return true;

		_mouseStickCode = code;
	} else if (message == kMsgLButtonDown) {
		code = 1;
	} else if (message == kMsgV1_2LButtonUp) {
		code = 0xFE;
	} else if (message == kMsgRButtonDown) {
		code = 2;
	} else if (message == kMsgV1_2RButtonUp) {
		code = 0xFD;
	} else {
		warning("TopgunEngine_v1_2_3::mouseAsStickMessage(): message %d is not handled", message);
		return true;
	}

	deviceGroupInputEvent(code, TG_DEVICE_V1_2_3_MOUSE);
	return true;
}

void TopgunEngine_v1_2_3::resetMouseStick(byte value) {
	if (value == 0)
		return;

	_mouseStickDeadRect = Common::Rect();
	_mouseStickOuterRect = Common::Rect();
	_mouseStickDeadRect.grow(TG_DEVICE_V1_2_3_STICK_DEAD_INFLATE);
	_mouseStickOuterRect.grow(TG_DEVICE_V1_2_3_STICK_OUTER_INFLATE);
	_mouseStickCode = 0;
	_variableTable[TG_HOST_V1_2_3_VARIABLE_MOUSE_X] = 0;
	_variableTable[TG_HOST_V1_2_3_VARIABLE_MOUSE_Y] = 0;
	g_system->warpMouse(display()->viewportOrigin().x, display()->viewportOrigin().y);
}

void TopgunEngine_v1_2::o_setDeviceGroupBinding(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_2_NUM_GROUPS) {
		warning("TopgunEngine_v1_2::o_setDeviceGroupBinding(): group %d is out of range", groupIndex);
		return;
	}

	DeviceGroup_v1_2_3 &group = _deviceGroups[groupIndex];

	if (record[0x0C] != 0) {
		if (record[0x0D] != 0)
			setAllBindingActive(1, group.numBindingsA, group.bindingsA);
		else if (record[0x0E] != 0)
			setAllBindingActive(0, group.numBindingsA, group.bindingsA);
		else if (READ_LE_UINT16(record + 6) == 0)
			group.numBindingsA = 0;
		else
			setAllBindingScripts(READ_LE_UINT16(record + 6), group.numBindingsA, group.bindingsA);
	} else {
		group.numBindingsA = updateDeviceBindingList(TG_DEVICE_V1_2_2_MAX_BINDINGS_A, group.numBindingsA, group.bindingsA, record);
	}
}

void TopgunEngine_v1_2::o_setDeviceGroupBindingB(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_2_NUM_GROUPS) {
		warning("TopgunEngine_v1_2::o_setDeviceGroupBindingB(): group %d is out of range", groupIndex);
		return;
	}

	DeviceGroup_v1_2_3 &group = _deviceGroups[groupIndex];

	if (record[0x0C] != 0) {
		if (record[0x0D] != 0)
			setAllBindingActive(1, group.numBindingsB, group.bindingsB);
		else if (record[0x0E] != 0)
			setAllBindingActive(0, group.numBindingsB, group.bindingsB);
		else if (READ_LE_UINT16(record + 6) == 0)
			group.numBindingsB = 0;
		else
			setAllBindingScripts(READ_LE_UINT16(record + 6), group.numBindingsB, group.bindingsB);
	} else {
		group.numBindingsB = updateDeviceBindingList(TG_DEVICE_V1_2_2_MAX_BINDINGS_B, group.numBindingsB, group.bindingsB, record);
	}
}

void TopgunEngine_v1_2::o_getDeviceGroupState(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_2_NUM_GROUPS) {
		warning("TopgunEngine_v1_2::o_getDeviceGroupState(): group %d is out of range", groupIndex);
		return;
	}

	DeviceGroup_v1_2_3 &group = _deviceGroups[groupIndex];
	uint16 code = group.lastCode;

	if (_version >= kVersionV1_2_2_1 && group.remap == 1) {
		if (code < TG_DEVICE_V1_2_2_CODE_TABLE_SIZE)
			code = kDeviceCodes[code];
		else
			warning("TopgunEngine_v1_2::o_getDeviceGroupState(): code %d is out of range", code);
	}

	writeVariable(READ_LE_UINT16(record + 4), code);
	writeVariable(READ_LE_UINT16(record + 6), group.pressedMask);
	writeVariable(READ_LE_UINT16(record + 8), (group.latchMode != 0 && group.latch.script != 0) ? 1 : 0);
}

void TopgunEngine_v1_2::o_deviceGroupLatch(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_2_NUM_GROUPS) {
		warning("TopgunEngine_v1_2::o_deviceGroupLatch(): group %d is out of range", groupIndex);
		return;
	}

	DeviceGroup_v1_2_3 &group = _deviceGroups[groupIndex];
	uint16 mode = operandValue(READ_LE_UINT16(record + 4));

	group.latchMode = (byte)mode;

	if (mode == 0 && operandValue(READ_LE_UINT16(record + 6)) != 0 && group.latch.script != 0) {
		if (_version < kVersionV1_2_3)
			callDeviceGroupScript(groupIndex, group.latch.id, group.latch.script);
		else
			static_cast<TopgunEngine_v1_2_3 *>(this)->enqueuePostedScriptRun(group.latch.script, (uint16)(READ_LE_UINT16(record + 2) - TG_HOST_V1_2_3_LITERAL_BIAS), group.latch.id);

		group.latch.script = 0;
		return;
	}

	group.latch.script = 0;
}

void TopgunEngine_v1_2::o_setDeviceGroupFlag(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_2_NUM_GROUPS) {
		warning("TopgunEngine_v1_2::o_setDeviceGroupFlag(): group %d is out of range", groupIndex);
		return;
	}

	_deviceGroups[groupIndex].remap = (operandValue(READ_LE_UINT16(record + 4)) != 0) ? 1 : 0;
}

void TopgunEngine_v1_2::o_setDeviceGroupScript(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_2_NUM_GROUPS) {
		warning("TopgunEngine_v1_2::o_setDeviceGroupScript(): group %d is out of range", groupIndex);
		return;
	}

	if (_version < kVersionV1_2_3)
		_deviceGroups[groupIndex].stateScript = READ_LE_UINT16(record + 4);
	else if (READ_LE_UINT16(record + 4) != 0)
		_deviceGroups[groupIndex].stateScript = (uint16)(operandValue(READ_LE_UINT16(record + 4)) - TG_HOST_V1_2_3_LITERAL_BIAS);
	else
		_deviceGroups[groupIndex].stateScript = 0;
}

void TopgunEngine_v1_2::o_deviceGroupButtonState(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_2_NUM_GROUPS) {
		warning("TopgunEngine_v1_2::o_deviceGroupButtonState(): group %d is out of range", groupIndex);
		return;
	}

	byte group = (byte)groupIndex;
	DeviceGroup_v1_2_3 &state = _deviceGroups[group];

	if (_version < kVersionV1_2_3) {
		if (state.type == TG_DEVICE_V1_2_2_TYPE_JOYSTICK)
			fillJoystickStateBytes(record[5], group);
		else if (state.type == TG_DEVICE_V1_2_2_TYPE_KEYBOARD)
			bindKeyboardSetToDevice(group, record[5]);

		if (record[5] != 0 || record[6] != 0)
			return;

		if (record[4] == TG_DEVICE_V1_2_2_TYPE_JOYSTICK)
			fillJoystickStateBytes(1, group);
		else if (record[4] == TG_DEVICE_V1_2_2_TYPE_KEYBOARD)
			bindKeyboardSetToDevice(group, 1);

		state.type = record[4];
		_keyboardRouted = (_deviceGroups[0].type == TG_DEVICE_V1_2_2_TYPE_KEYBOARD || _deviceGroups[1].type == TG_DEVICE_V1_2_2_TYPE_KEYBOARD);
		_joystickRouted = (_deviceGroups[0].type == TG_DEVICE_V1_2_2_TYPE_JOYSTICK || _deviceGroups[1].type == TG_DEVICE_V1_2_2_TYPE_JOYSTICK);
		return;
	}

	TopgunEngine_v1_2_3 *newer = static_cast<TopgunEngine_v1_2_3 *>(this);

	if (state.device != 0 && state.device < TG_DEVICE_V1_2_2_NUM_DEVICES && _deviceOwners[state.device] == group) {
		if (_version >= kVersionV1_2_6) {
			newer->resetDeviceGroup(record[7], group);

			if (state.type == TG_DEVICE_V1_2_3_TYPE_MOUSE)
				_mouseAsStick = (record[7] != 0);

			state.attached = record[7];
		} else {
			switch (state.type) {
			case TG_DEVICE_V1_2_2_TYPE_JOYSTICK:
				fillJoystickStateBytes(record[7], state.device - TG_DEVICE_V1_2_3_JOYSTICK_BASE);
				break;

			case TG_DEVICE_V1_2_2_TYPE_KEYBOARD:
				bindKeyboardSetToDevice(state.device - TG_DEVICE_V1_2_3_KEYBOARD_BASE, record[7]);
				break;

			case TG_DEVICE_V1_2_3_TYPE_MOUSE:
				newer->resetMouseStick(record[7]);
				_mouseAsStick = (record[7] != 0);
				break;

			default:
				break;
			}
		}
	}

	if (record[8] != 0)
		newer->fireDeviceGroupStateScript(group);

	if (record[7] != 0 || record[8] != 0)
		return;

	switch (record[6]) {
	case TG_DEVICE_V1_2_2_TYPE_JOYSTICK: {
		uint16 device = operandValue(READ_LE_UINT16(record + 4));

		state.device = (byte)(device + TG_DEVICE_V1_2_3_JOYSTICK_BASE);

		if (_version < kVersionV1_2_6)
			fillJoystickStateBytes(1, (byte)device);

		break;
	}

	case TG_DEVICE_V1_2_2_TYPE_KEYBOARD: {
		uint16 device = operandValue(READ_LE_UINT16(record + 4));

		state.device = (byte)(device + TG_DEVICE_V1_2_3_KEYBOARD_BASE);

		if (_version < kVersionV1_2_6)
			bindKeyboardSetToDevice((byte)device, 1);

		break;
	}

	case TG_DEVICE_V1_2_3_TYPE_MOUSE:
		state.device = TG_DEVICE_V1_2_3_MOUSE;

		if (_version < kVersionV1_2_6)
			newer->resetMouseStick(1);

		_mouseAsStick = true;
		break;

	default:
		break;
	}

	state.type = record[6];

	if (state.device >= TG_DEVICE_V1_2_2_NUM_DEVICES) {
		warning("TopgunEngine_v1_2::o_deviceGroupButtonState(): device %d is out of range", state.device);
		return;
	}

	if (state.type != 0)
		_deviceOwners[state.device] = group;
	else if (_deviceOwners[state.device] == group && state.device == TG_DEVICE_V1_2_3_MOUSE)
		_mouseAsStick = false;

	if (_version >= kVersionV1_2_6) {
		state.attached = (state.type != 0) ? 1 : 0;
		newer->resetDeviceGroup(state.attached, group);
	}

	_keyboardRouted = (_deviceGroups[0].type == TG_DEVICE_V1_2_2_TYPE_KEYBOARD || _deviceGroups[1].type == TG_DEVICE_V1_2_2_TYPE_KEYBOARD);
	_joystickRouted = (_deviceGroups[0].type == TG_DEVICE_V1_2_2_TYPE_JOYSTICK || _deviceGroups[1].type == TG_DEVICE_V1_2_2_TYPE_JOYSTICK);
}

uint16 TopgunEngine_v1_2_3::builtinSetKeyboardSetKey(uint16 set, uint16 kind, uint16 vk) {
	byte setIndex = (byte)set;

	if (setIndex >= TG_DEVICE_V1_2_2_NUM_KEYBOARD_SETS) {
		warning("TopgunEngine_v1_2_3::builtinSetKeyboardSetKey(): keyboard set %d is out of range", setIndex);
		return 107;
	}

	switch ((byte)kind) {
	case 1:
		_keyboardSetKeys[setIndex][4] = (byte)vk;
		break;

	case 2:
		_keyboardSetKeys[setIndex][5] = (byte)vk;
		break;

	case 3:
		_keyboardSetKeys[setIndex][0] = (byte)vk;
		break;

	case 4:
		if (_version >= kVersionV1_2_5)
			_keyboardSetKeys[setIndex][6] = (byte)vk;

		break;

	case 5:
		_keyboardSetKeys[setIndex][1] = (byte)vk;
		break;

	case 6:
		_keyboardSetKeys[setIndex][3] = (byte)vk;
		break;

	case 7:
		_keyboardSetKeys[setIndex][2] = (byte)vk;
		break;

	case 8:
		if (_version >= kVersionV1_2_5)
			_keyboardSetKeys[setIndex][7] = (byte)vk;

		break;

	default:
		break;
	}

	return 107;
}

uint16 TopgunEngine_v1_2_3::builtinReattachDeviceGroup(uint16 group, uint16 flag) {
	if (group >= TG_DEVICE_V1_2_2_NUM_GROUPS) {
		warning("TopgunEngine_v1_2_3::builtinReattachDeviceGroup(): group %d is out of range", group);
		return 137;
	}

	DeviceGroup_v1_2_3 &state = _deviceGroups[group];
	byte base = state.device;

	if (base >= TG_DEVICE_V1_2_2_NUM_DEVICES) {
		warning("TopgunEngine_v1_2_3::builtinReattachDeviceGroup(): device %d is out of range", base);
		return 137;
	}

	_deviceOwners[base] = (byte)group;

	if (flag != 0) {
		byte other = (byte)((group + 1) & 1);
		DeviceGroup_v1_2_3 &otherState = _deviceGroups[other];

		if (otherState.device != 0 && otherState.device < TG_DEVICE_V1_2_2_NUM_DEVICES && _deviceOwners[otherState.device] == other) {
			if (_version >= kVersionV1_2_6) {
				otherState.attached = 0;
				resetDeviceGroup(0, other);

				if (otherState.type == TG_DEVICE_V1_2_3_TYPE_MOUSE)
					_mouseAsStick = (base == TG_DEVICE_V1_2_3_MOUSE);
			} else {
				switch (otherState.type) {
				case TG_DEVICE_V1_2_2_TYPE_JOYSTICK:
					fillJoystickStateBytes(0, otherState.device - TG_DEVICE_V1_2_3_JOYSTICK_BASE);
					break;

				case TG_DEVICE_V1_2_2_TYPE_KEYBOARD:
					bindKeyboardSetToDevice(otherState.device - TG_DEVICE_V1_2_3_KEYBOARD_BASE, 0);
					break;

				case TG_DEVICE_V1_2_3_TYPE_MOUSE:
					resetMouseStick(0);
					_mouseAsStick = (base == TG_DEVICE_V1_2_3_MOUSE);
					break;

				default:
					break;
				}
			}
		}
	}

	if (_version >= kVersionV1_2_6) {
		state.attached = 1;
		resetDeviceGroup(1, (byte)group);

		if (state.type == TG_DEVICE_V1_2_3_TYPE_MOUSE)
			_mouseAsStick = true;

		return 137;
	}

	switch (state.type) {
	case TG_DEVICE_V1_2_2_TYPE_JOYSTICK:
		fillJoystickStateBytes(1, base - TG_DEVICE_V1_2_3_JOYSTICK_BASE);
		break;

	case TG_DEVICE_V1_2_2_TYPE_KEYBOARD:
		bindKeyboardSetToDevice(base - TG_DEVICE_V1_2_3_KEYBOARD_BASE, 1);
		break;

	case TG_DEVICE_V1_2_3_TYPE_MOUSE:
		resetMouseStick(1);
		_mouseAsStick = true;
		break;

	default:
		break;
	}

	return 137;
}

uint16 TopgunEngine_v1_2::joystickGetScriptForDirectionCode(uint16 offset, uint16 flags) {
	uint16 index = joystickButtonIndex(flags);

	if (index == TG_JOYSTICK_V1_2_1_NO_INDEX)
		return 0;

	return _joystickScripts[index + offset];
}

uint16 TopgunEngine_v1_2::joystickButtonIndex(uint16 flags) {
	byte changed = (byte)(flags >> 8);
	byte state = (byte)flags;

	if ((changed & 1) != 0)
		return (state & 1) != 0 ? 0x0A : 0x09;

	if ((changed & 2) != 0)
		return (state & 2) != 0 ? 0x0C : 0x0B;

	if ((changed & 4) != 0)
		return (state & 4) != 0 ? 0x0E : 0x0D;

	if ((changed & 8) != 0)
		return (state & 8) != 0 ? 0x10 : 0x0F;

	return TG_JOYSTICK_V1_2_1_NO_INDEX;
}

uint16 TopgunEngine_v1_2::joystickDirectionIndex(uint16 offset, const uint16 *center, uint16 x, uint16 y) {
	uint16 code;

	if (_joystickDirectionFlag)
		code = joystickDirectionByDistance(center, x, y);
	else
		code = joystickDirection8Way(center, x, y);

	return code + offset;
}

uint16 TopgunEngine_v1_2::joystickDirectionByDistance(const uint16 *center, uint16 x, uint16 y) {
	int32 deltaX = (int32)x - (int32)center[0];
	int32 deltaY = (int32)y - (int32)center[1];
	uint32 distanceX = (uint32)(deltaX < 0 ? -deltaX : deltaX);
	uint32 distanceY = (uint32)(deltaY < 0 ? -deltaY : deltaY);

	if (distanceX <= TG_JOYSTICK_V1_2_1_DISTANCE_THRESHOLD && distanceY <= TG_JOYSTICK_V1_2_1_DISTANCE_THRESHOLD)
		return 0;

	if (distanceY > distanceX)
		return deltaY > 0 ? 6 : 3;

	return deltaX < 0 ? 2 : 1;
}

uint16 TopgunEngine_v1_2::joystickDirection8Way(const uint16 *center, uint16 x, uint16 y) {
	int32 centerX = center[0];
	int32 centerY = center[1];
	int32 threshold = (_version >= kVersionV1_2_2_2) ? TG_JOYSTICK_V1_2_3_EIGHT_WAY_THRESHOLD : TG_JOYSTICK_V1_2_1_EIGHT_WAY_THRESHOLD;
	uint16 code = 0;

	if (centerY - threshold > (int32)y)
		code = 3;

	if (centerY + threshold < (int32)y)
		code += 6;

	if (centerX + threshold < (int32)x)
		code += 1;

	if (centerX - threshold > (int32)x)
		code += 2;

	return code;
}

uint16 TopgunEngine_v1_2::joystickGetScriptForPosition(uint16 offset, const uint16 *center, uint16 x, uint16 y) {
	return _joystickScripts[joystickDirectionIndex(offset, center, x, y)];
}

uint16 TopgunEngine_v1_2::joystickMessageToIndex(const Message &message) {
	uint16 x = (uint16)(message.data & 0xFFFF);
	uint16 y = (uint16)(message.data >> 16);
	uint16 index = 0;

	switch (message.id) {
	case kMsgV1_2_1Joystick1Move:
		index = joystickDirectionIndex(_joystickButtonFlag ? TG_JOYSTICK_V1_2_1_STICK_OFFSET : 0, _joystickCenter[0], x, y);
		break;

	case kMsgV1_2_1Joystick2Move:
		index = joystickDirectionIndex(_joystickButtonFlag ? 0 : TG_JOYSTICK_V1_2_1_STICK_OFFSET, _joystickCenter[1], x, y);
		break;

	case kMsgV1_2_1Joystick1ButtonDown:
	case kMsgV1_2_1Joystick1ButtonUp:
		index = joystickButtonIndex(message.param);

		if (index != TG_JOYSTICK_V1_2_1_NO_INDEX)
			index += _joystickButtonFlag ? TG_JOYSTICK_V1_2_1_STICK_OFFSET : 0;

		break;

	case kMsgV1_2_1Joystick2ButtonDown:
	case kMsgV1_2_1Joystick2ButtonUp:
		index = joystickButtonIndex(message.param);

		if (index != TG_JOYSTICK_V1_2_1_NO_INDEX)
			index += _joystickButtonFlag ? 0 : TG_JOYSTICK_V1_2_1_STICK_OFFSET;

		break;

	default:
		break;
	}

	return index;
}

void TopgunEngine_v1_2::handleJoystickButtonMessage(const Message &message) {
	if (_version >= kVersionV1_2_2_1 && _gamePaused)
		return;

	uint16 index = joystickMessageToIndex(message);

	if (index == TG_JOYSTICK_V1_2_1_NO_INDEX)
		return;

	if (index >= TG_JOYSTICK_V1_2_1_NUM_SCRIPTS) {
		warning("TopgunEngine_v1_2::handleJoystickButtonMessage(): slot %d is out of range", index);
		return;
	}

	if (_joystickStates[index] == 0)
		return;

	byte stick = (index >= TG_JOYSTICK_V1_2_1_STICK_OFFSET) ? 1 : 0;

	if (message.id == kMsgV1_2_1Joystick1ButtonDown || message.id == kMsgV1_2_1Joystick2ButtonDown) {
		if (index > 0)
			_joystickStates[index - 1] = 1;
	} else if (message.id == kMsgV1_2_1Joystick1ButtonUp || message.id == kMsgV1_2_1Joystick2ButtonUp) {
		if (index + 1 < TG_JOYSTICK_V1_2_1_NUM_SCRIPTS)
			_joystickStates[index + 1] = 1;
	} else {
		_joystickStates[_joystickLastIndex[stick]] = 1;
		_joystickLastIndex[stick] = (byte)index;
	}

	_joystickStates[index] = 0;

	byte code = kDeviceCodes[TG_DEVICE_V1_2_2_JOYSTICK_CODES_OFFSET + index - stick * TG_JOYSTICK_V1_2_1_STICK_OFFSET];

	byte inputDevice = stick + TG_DEVICE_V1_2_3_JOYSTICK_BASE;
	if (_version < kVersionV1_2_3)
		inputDevice = stick;

	deviceGroupInputEvent(code, inputDevice);
}

void TopgunEngine_v1_2::handleJoystickMessage(const Message &message) {
	if (_version >= kVersionV1_2_2_1 && callMessageFilter(message.id, message.param, (uint16)(message.data & 0xFFFF), (uint16)(message.data >> 16)))
		return;

	if (_joystickRouted) {
		handleJoystickButtonMessage(message);
		return;
	}

	uint16 x = (uint16)(message.data & 0xFFFF);
	uint16 y = (uint16)(message.data >> 16);
	uint16 script = 0;

	switch (message.id) {
	case kMsgV1_2_1Joystick1Move:
		script = joystickGetScriptForPosition(_joystickButtonFlag ? TG_JOYSTICK_V1_2_1_STICK_OFFSET : 0, _joystickCenter[0], x, y);
		break;

	case kMsgV1_2_1Joystick2Move:
		script = joystickGetScriptForPosition(_joystickButtonFlag ? 0 : TG_JOYSTICK_V1_2_1_STICK_OFFSET, _joystickCenter[1], x, y);
		break;

	case kMsgV1_2_1Joystick1ButtonDown:
	case kMsgV1_2_1Joystick1ButtonUp:
		script = joystickGetScriptForDirectionCode(_joystickButtonFlag ? TG_JOYSTICK_V1_2_1_STICK_OFFSET : 0, message.param);
		break;

	case kMsgV1_2_1Joystick2ButtonDown:
	case kMsgV1_2_1Joystick2ButtonUp:
		script = joystickGetScriptForDirectionCode(_joystickButtonFlag ? 0 : TG_JOYSTICK_V1_2_1_STICK_OFFSET, message.param);
		break;

	default:
		break;
	}

	if (script != 0)
		runScriptResource(script);
}

void TopgunEngine_v1_2::joystickInit() {
	_numJoysticks = 0;

	if (_maxJoySticks == 0)
		return;

	// The system offers a single joystick through its events; it is centred at the middle of the axis range
	if (ConfMan.hasKey("joystick_num") && ConfMan.getInt("joystick_num") >= 0) {
		_joystickCaptured = true;
		_joystickCenter[0][0] = TG_JOYSTICK_V1_2_1_AXIS_CENTER;
		_joystickCenter[0][1] = TG_JOYSTICK_V1_2_1_AXIS_CENTER;
		_numJoysticks++;
	}

	if (_numJoysticks < _maxJoySticks && _numJoysticks == 0)
		_maxJoySticks = 0;
}

void TopgunEngine_v1_2::joystickRelease() {
	if (!_joystickCaptured)
		return;

	_joystickCaptured = false;
}

} // End of namespace Topgun
