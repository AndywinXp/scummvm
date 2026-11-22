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
#include "topgun/topgun_v1_2_1.h"

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

		if (_version < kVersionV1_2_1)
			return false;

		if ((event.kbd.flags & Common::KBD_ALT) != 0 || event.kbd.keycode == Common::KEYCODE_F10)
			return false;

		message.id = kMsgV1_2_1KeyUp;
		message.param = (uint16)event.kbd.keycode;
		message.shift = (event.kbd.flags & Common::KBD_SHIFT) != 0;
		message.ctrl = (event.kbd.flags & Common::KBD_CTRL) != 0;
		return true;
	}

	case Common::EVENT_JOYAXIS_MOTION: {
		if (_version < kVersionV1_2_1)
			return false;

		TopgunEngine_v1_2_1 *newer = static_cast<TopgunEngine_v1_2_1 *>(this);

		if (!newer->_joystickCaptured)
			return false;

		if (event.joystick.axis == 0)
			newer->_joystickX = (uint16)(event.joystick.position - Common::JOYAXIS_MIN);
		else if (event.joystick.axis == 1)
			newer->_joystickY = (uint16)(event.joystick.position - Common::JOYAXIS_MIN);
		else
			return false;

		message.id = kMsgV1_2_1Joystick1Move;
		message.param = newer->_joystickButtons;
		message.data = ((uint32)newer->_joystickY << 16) | newer->_joystickX;
		return true;
	}

	case Common::EVENT_JOYBUTTON_DOWN:
	case Common::EVENT_JOYBUTTON_UP: {
		if (_version < kVersionV1_2_1)
			return false;

		TopgunEngine_v1_2_1 *newer = static_cast<TopgunEngine_v1_2_1 *>(this);

		if (!newer->_joystickCaptured)
			return false;

		int button = event.joystick.button - Common::JOYSTICK_BUTTON_A;
		if (button < 0 || button > 3)
			return false;

		bool down = (event.type == Common::EVENT_JOYBUTTON_DOWN);

		if (down)
			newer->_joystickButtons |= (byte)(1 << button);
		else
			newer->_joystickButtons &= (byte)~(1 << button);

		message.id = down ? kMsgV1_2_1Joystick1ButtonDown : kMsgV1_2_1Joystick1ButtonUp;
		message.param = (uint16)((0x100 << button) | newer->_joystickButtons);
		message.data = ((uint32)newer->_joystickY << 16) | newer->_joystickX;
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

	if (message.id == kMsgKeyDown && message.data != 0) {
		_pendingChar = Message();
		_pendingChar.id = kMsgV1_2Char;
		_pendingChar.param = (uint16)message.data;
		_hasPendingChar = true;
	}

	return true;
}

uint16 TopgunEngine_v1_2::keyCodeToScriptKey(Common::KeyCode keycode) {
	Common::KeyCode keyCodes[2];

	for (uint16 key = 1; key < TG_HOST_V1_2_KEY_TABLE_SIZE; key++) {
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

	if (_version >= kVersionV1_2_1) {
		WRITE_LE_UINT16(record + 6, (uint16)(code - TG_HOST_V1_2_1_LITERAL_BIAS));
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

	if (_version >= kVersionV1_2_1 && _mouseAsStick && _flagActivated && static_cast<TopgunEngine_v1_2_1 *>(this)->mouseAsStickMessage(point, message.id))
		return;

	if (left)
		_leftButtonDown = true;
	else
		_rightButtonDown = true;

	if (_gamePaused)
		return;

	if (_cursorMode == TG_HOST_V1_2_CURSOR_MODE_BUSY || _cursorMode == TG_HOST_V1_2_CURSOR_MODE_HIDDEN)
		return;

	if (_dragSpriteId != 0)
		return;

	if (left && message.shift && message.ctrl)
		warning("beginRubberBandSelection UNIMPLEMENTED");

	if (_rubberBandMode)
		return;

	transformPointForDisplayZoom(point);

	writeVariable((_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_VARIABLE_MOUSE_BUTTON : TG_HOST_V1_2_VARIABLE_MOUSE_BUTTON, left ? TG_HOST_V1_2_MOUSE_BUTTON_LEFT : TG_HOST_V1_2_MOUSE_BUTTON_RIGHT);
	writeVariable((_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_VARIABLE_CLICK_X : TG_HOST_V1_2_VARIABLE_CLICK_X, (uint16)point.x);
	writeVariable((_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_VARIABLE_CLICK_Y : TG_HOST_V1_2_VARIABLE_CLICK_Y, (uint16)point.y);

	if (_mouseEventScript != 0 && postMouseEventScript(TG_HOST_V1_2_MOUSE_EVENT_DOWN) == 0)
		return;

	uint16 script = 0;
	uint16 clickedId = 0;
	uint16 sprite = spriteLib()->spritePicked(point.x, point.y);

	if (sprite != 0) {
		clickedId = spriteLib()->spriteGetId(sprite);
		uint16 index = operandValue(clickedId);

		if (isRecordIndexValid(index, "handleMouseButtonMessage") && _records[index].data) {
			const byte *object = _records[index].data;
			uint16 draggableOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_DRAGGABLE : TG_HOST_V1_2_SPRITE_DATA_DRAGGABLE;
			uint16 dragScriptOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_DRAG_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_DRAG_SCRIPT;
			uint16 disabledOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_DISABLED : TG_HOST_V1_2_SPRITE_DATA_DISABLED;
			uint16 clickScriptOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_CLICK_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_CLICK_SCRIPT;

			if (object[draggableOffset] != 0) {
				_dragSpriteId = clickedId;
				_dragStartX = point.x;
				_dragStartY = point.y;
				script = READ_LE_UINT16(object + dragScriptOffset);
			} else if (object[disabledOffset] == 0) {
				script = READ_LE_UINT16(object + clickScriptOffset);
			}
		}
	}

	if (script == 0)
		clickedId = 0;

	if (script == 0) {
		int16 region = (int16)_numClickRegions - 1;

		while (region >= 0) {
			const ClickRegion_v1_2 &candidate = _clickRegions[region];

			if (candidate.disabled == 0 && candidate.left <= point.x && candidate.right > point.x && candidate.top <= point.y && candidate.bottom > point.y && candidate.script != 0)
				break;

			region--;
		}

		if (region >= 0)
			script = _clickRegions[region].script;
	}

	if (script != 0)
		postRunScript(script, (_version >= kVersionV1_2_1) ? clickedId : 0, 0);
}

void TopgunEngine_v1_2::handleMouseButtonUpMessage(const Message &message) {
	bool left = (message.id == kMsgV1_2LButtonUp);
	Common::Point point((int16)(message.data & 0xFFFF), (int16)(message.data >> 16));

	if (_version >= kVersionV1_2_1 && _mouseAsStick && _flagActivated && static_cast<TopgunEngine_v1_2_1 *>(this)->mouseAsStickMessage(point, message.id))
		return;

	if (left)
		_leftButtonDown = false;
	else
		_rightButtonDown = false;

	if (_gamePaused)
		return;

	if (_rubberBandMode) {
		if (left)
			warning("endRubberBandSelection UNIMPLEMENTED");

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

	if (_version >= kVersionV1_2_1) {
		if (!_flagActivated)
			return;

		if (_mouseAsStick && static_cast<TopgunEngine_v1_2_1 *>(this)->mouseAsStickMessage(point, message.id))
			return;
	}

	if (_rubberBandMode) {
		warning("updateRubberBand UNIMPLEMENTED");
		return;
	}

	transformPointForDisplayZoom(point);

	writeVariable((_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_VARIABLE_MOUSE_X : TG_HOST_V1_2_VARIABLE_MOUSE_X, (uint16)point.x);
	writeVariable((_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_VARIABLE_MOUSE_Y : TG_HOST_V1_2_VARIABLE_MOUSE_Y, (uint16)point.y);

	if (_gamePaused)
		return;

	if (_mouseEventScript != 0 && postMouseEventScript(TG_HOST_V1_2_MOUSE_EVENT_MOVE) == 0)
		return;

	if (_dragSpriteId != 0) {
		uint16 index = operandValue(_dragSpriteId);

		if (isRecordIndexValid(index, "handleMouseMoveMessage"))
			spriteLib()->spriteOffset(_records[index].handle, (int16)(point.x - _dragStartX), (int16)(point.y - _dragStartY));

		_dragStartX = point.x;
		_dragStartY = point.y;
		return;
	}

	if (_version >= kVersionV1_2_1 && _hoverSpriteScript != 0 && static_cast<TopgunEngine_v1_2_1 *>(this)->updateSpriteHoverFromPoint(point.x, point.y))
		return;

	if (_numHoverRegions != 0)
		updateHoverRegionFromPoint(point.x, point.y);
}

void TopgunEngine_v1_2::dropDraggedSprite(int16 x, int16 y) {
	writeVariable((_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_VARIABLE_DROP_X : TG_HOST_V1_2_VARIABLE_DROP_X, (uint16)x);
	writeVariable((_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_VARIABLE_DROP_Y : TG_HOST_V1_2_VARIABLE_DROP_Y, (uint16)y);

	uint16 index = operandValue(_dragSpriteId);

	if (isRecordIndexValid(index, "dropDraggedSprite") && _records[index].data) {
		uint16 dropScriptOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_DROP_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_DROP_SCRIPT;
		uint16 script = READ_LE_UINT16(_records[index].data + dropScriptOffset);

		if (script != 0)
			postRunScript(script, (_version >= kVersionV1_2_1) ? _dragSpriteId : 0, 0);
	}

	_dragSpriteId = 0;
}

void TopgunEngine_v1_2::handleKeyDownMessage(Common::KeyCode keycode, bool shift, bool ctrl) {
	uint16 key = keyCodeToScriptKey(keycode);

	if (_version >= kVersionV1_2_1) {
		TopgunEngine_v1_2_1 *newer = static_cast<TopgunEngine_v1_2_1 *>(this);

		if (key == TG_HOST_V1_2_NO_KEY)
			return;

		if (newer->callMessageFilter(kMsgKeyDown, key, 0, 0))
			return;

		if (_keyboardRouted && !newer->keyboardRoutedKeyDown(key))
			return;

		if (key == TG_INPUT_V1_2_1_VK_PAUSE)
			newer->togglePauseState();

		if (_gamePaused)
			return;
	} else if (key == TG_HOST_V1_2_VK_ADD || key == TG_HOST_V1_2_VK_SUBTRACT) {
		adjustMasterVolumeByKey(keycode);
		return;
	}

	if (_rubberBandMode && key == TG_HOST_V1_2_VK_ESCAPE) {
		warning("endRubberBandSelection UNIMPLEMENTED");
		return;
	}

	if (_version >= kVersionV1_2_1 && _keyHookScript != 0 && static_cast<TopgunEngine_v1_2_1 *>(this)->postKeyHookScript(key) == 0)
		return;

	if (_textEditObjectId != 0)
		return;

	uint16 keyTableSize = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_KEY_TABLE_SIZE : TG_HOST_V1_2_KEY_TABLE_SIZE;

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

	if (script != 0)
		runScriptResource(script);
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
		changed = textEditBackspace(object);
	} else if (character >= TG_HOST_V1_2_CHAR_FIRST_PRINTABLE || (_version >= kVersionV1_2_1 && character == 0x0A)) {
		if (_version < kVersionV1_2_1 || character != 0x40)
			changed = textEditAppendChar(object, (byte)character);
	}

	if (changed && _textEditObjectId != 0)
		refreshTextObjectBitmap(object);
}

void TopgunEngine_v1_2::handleSysKeyDownAltTab(const Message &message) {
	if (message.param == TG_HOST_V1_2_VK_TAB && !_altTab) {
		_appActive = false;
		_flagActivated = false;
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
	uint16 keyTableSize = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_KEY_TABLE_SIZE : TG_HOST_V1_2_KEY_TABLE_SIZE;
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
			else if (_version >= kVersionV1_2_1 && record[0x0A] != 0)
				entry.keyUp = READ_LE_UINT16(record + 4);
			else
				entry.plain = READ_LE_UINT16(record + 4);

			entry.disabled = 0;
		}
	}
}

bool TopgunEngine_v1_2::o_setClickScript(const byte *record) {
	if (record[0x0E] != 0) {
		if (record[0x0F] != 0)
			setAllClickRegionsAndSpriteObjectsEnabled(0);
		else if (record[0x10] != 0)
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
	uint16 disabledOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_DISABLED : TG_HOST_V1_2_SPRITE_DATA_DISABLED;
	uint16 clickScriptOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_CLICK_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_CLICK_SCRIPT;

	object[disabledOffset] = 0;

	if (record[0x10] != 0)
		object[disabledOffset] = 1;
	else if (record[0x0F] != 0)
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

	if (record[0x10] != 0) {
		region.disabled = 1;
	} else if (record[0x0F] != 0) {
		region.disabled = 0;
	} else {
		region.left = left;
		region.top = top;
		region.right = right;
		region.bottom = bottom;
		region.script = READ_LE_UINT16(record + 4);

		if (_version >= kVersionV1_2_1 && region.script != 0)
			region.script = (uint16)(operandValue(region.script) - TG_HOST_V1_2_1_LITERAL_BIAS);

		region.disabled = 0;
	}
}

bool TopgunEngine_v1_2::appendClickRegion() {
	uint16 maxRegions = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_MAX_CLICK_REGIONS : TG_HOST_V1_2_MAX_CLICK_REGIONS;

	if (_numClickRegions >= maxRegions)
		return false;

	if (_numClickRegions >= _clickRegions.size())
		_clickRegions.push_back(ClickRegion_v1_2());
	else if (_version >= kVersionV1_2_1)
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

	uint16 spriteType = (_version >= kVersionV1_2_1) ? kResV1_2_1Sprite : kResV1_2Sprite;
	uint16 clickScriptOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_CLICK_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_CLICK_SCRIPT;

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

	uint16 spriteType = (_version >= kVersionV1_2_1) ? kResV1_2_1Sprite : kResV1_2Sprite;
	uint16 disabledOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_DISABLED : TG_HOST_V1_2_SPRITE_DATA_DISABLED;

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
	uint16 dropScriptOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_DROP_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_DROP_SCRIPT;
	uint16 dragScriptOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_DRAG_SCRIPT : TG_HOST_V1_2_SPRITE_DATA_DRAG_SCRIPT;
	uint16 draggableOffset = (_version >= kVersionV1_2_1) ? TG_INPUT_V1_2_1_OBJECT_DRAGGABLE : TG_HOST_V1_2_SPRITE_DATA_DRAGGABLE;

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
	if (_version >= kVersionV1_2_1 && record[0x0E] != 0) {
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
	uint16 index = 0;

	if (_numHoverRegions != 0) {
		do {
			const HoverRegion_v1_2 &region = _hoverRegions[index];

			if (region.left == left && region.top == top && region.right == right && region.bottom == bottom)
				break;

			index++;
		} while (index < _numHoverRegions);
	}

	if (_numHoverRegions <= index) {
		if (_numHoverRegions >= TG_HOST_V1_2_MAX_HOVER_REGIONS)
			return false;

		index = _numHoverRegions;
		_numHoverRegions++;
	}

	HoverRegion_v1_2 &region = _hoverRegions[index];

	region.left = left;
	region.top = top;
	region.right = right;
	region.bottom = bottom;
	region.enterScript = READ_LE_UINT16(record + 0x0A);
	region.leaveScript = READ_LE_UINT16(record + 0x0C);

	if (_version >= kVersionV1_2_1) {
		if (region.enterScript != 0)
			region.enterScript = (uint16)(operandValue(region.enterScript) - TG_HOST_V1_2_1_LITERAL_BIAS);

		if (region.leaveScript != 0)
			region.leaveScript = (uint16)(operandValue(region.leaveScript) - TG_HOST_V1_2_1_LITERAL_BIAS);
	}

	Common::Point cursor = g_system->getEventManager()->getMousePos();
	transformPointForDisplayZoom(cursor);

	if (region.left <= cursor.x && region.right > cursor.x && region.top <= cursor.y && region.bottom > cursor.y)
		updateHoverRegionFromPoint(cursor.x, cursor.y);

	return true;
}

bool TopgunEngine_v1_2::removeHoverRegion(const byte *record) {
	int16 left = (int16)operandValue(READ_LE_UINT16(record + 2));
	int16 top = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 right = (int16)operandValue(READ_LE_UINT16(record + 6));
	int16 bottom = (int16)operandValue(READ_LE_UINT16(record + 8));
	uint16 index = 0;

	if (_numHoverRegions != 0) {
		do {
			const HoverRegion_v1_2 &region = _hoverRegions[index];

			if (region.left == left && region.top == top && region.right == right && region.bottom == bottom)
				break;

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

		if (region.left <= x && region.right > x && region.top <= y && region.bottom > y)
			break;

		found--;
	}

	uint16 leaveArgument = (_version >= kVersionV1_2_1) ? (uint16)(0 - TG_HOST_V1_2_1_LITERAL_BIAS) : 0;
	uint16 enterArgument = (_version >= kVersionV1_2_1) ? (uint16)(1 - TG_HOST_V1_2_1_LITERAL_BIAS) : 0;
	int16 current = (int16)_hoverRegionCurrent - 1;

	if (current != found) {
		if (current >= 0) {
			uint16 script = _hoverRegions[current].leaveScript;

			if (script != 0 && !postRunScript(script, leaveArgument, 0))
				return;
		}

		_hoverRegionCurrent = 0;
	}

	if (found < 0 || current == found)
		return;

	uint16 script = _hoverRegions[found].enterScript;

	if (script != 0 && !postRunScript(script, enterArgument, 0))
		return;

	_hoverRegionCurrent = (uint16)(found + 1);
}

void TopgunEngine_v1_2_1::handleKeyUpMessage(Common::KeyCode keycode) {
	uint16 key = keyCodeToScriptKey(keycode);

	if (key == TG_HOST_V1_2_NO_KEY)
		return;

	if (callMessageFilter(kMsgV1_2_1KeyUp, key, 0, 0))
		return;

	if (_keyboardRouted && !keyboardRoutedKeyUp(key))
		return;

	if (_gamePaused)
		return;

	if (key >= TG_INPUT_V1_2_1_KEY_TABLE_SIZE)
		return;

	uint16 script = _keyTable[key].keyUp;

	if (script != 0)
		runScriptResource(script);
}

void TopgunEngine_v1_2_1::togglePauseState() {
	if (_pauseScript != 0) {
		byte record[18];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record + 2, _pauseScript);
		record[5] = 1;
		WRITE_LE_UINT16(record + 6, (uint16)((_gamePaused ? 0 : 1) - TG_HOST_V1_2_1_LITERAL_BIAS));
		o_callScript(record);
		spriteLib()->spriteAnimate();
	} else {
		audioLib()->audioPause(_gamePaused ? 0 : 1);
	}

	_gamePaused = !_gamePaused;
}

uint16 TopgunEngine_v1_2_1::postKeyHookScript(uint16 vk) {
	byte record[18];

	memset(record, 0, sizeof(record));
	WRITE_LE_UINT16(record + 2, _keyHookScript);
	WRITE_LE_UINT16(record + 6, (uint16)(vk - TG_HOST_V1_2_1_LITERAL_BIAS));
	record[5] = 1;

	uint16 saved = _scriptResult;
	_scriptResult = 0;
	o_callScript(record);

	uint16 result = _scriptResult;
	_scriptResult = saved;
	return result;
}

bool TopgunEngine_v1_2_1::postRunScript(uint16 script, uint16 argument1, uint16 argument2) {
	return enqueuePostedScriptRun(script, argument1, argument2);
}

bool TopgunEngine_v1_2_1::updateSpriteHoverFromPoint(int16 x, int16 y) {
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

	_hoverSprite = id;
	enqueuePostedScriptRun(_hoverSpriteScript, id, (uint16)(1 - TG_HOST_V1_2_1_LITERAL_BIAS));
	return true;
}

void TopgunEngine_v1_2_1::postSpriteLeaveScript() {
	enqueuePostedScriptRun(_hoverSpriteScript, _hoverSprite, (uint16)(0 - TG_HOST_V1_2_1_LITERAL_BIAS));
	_hoverSprite = 0;
}

void TopgunEngine_v1_2_1::o_getKeyState(const byte *record) {
	writeVariable(READ_LE_UINT16(record + 2), builtinGetKeyState(READ_LE_UINT16(record + 4)));
}

void TopgunEngine_v1_2_1::o_setMouseEventScript(const byte *record) {
	_mouseEventScript = READ_LE_UINT16(record + 2);
}

void TopgunEngine_v1_2_1::o_setJoystickScript(const byte *record) {
	uint16 index = READ_LE_UINT16(record + 2);

	if (index >= TG_INPUT_V1_2_1_NUM_JOYSTICK_SCRIPTS) {
		warning("TopgunEngine_v1_2_1::o_setJoystickScript(): slot %d is out of range", index);
		return;
	}

	_joystickScripts[index] = READ_LE_UINT16(record + 4);
}

uint16 TopgunEngine_v1_2_1::builtinGetKeyState(uint16 vk) {
	if (vk < TG_HOST_V1_2_KEY_STATE_SIZE && _keyStates[vk])
		return 1;

	return 0;
}

uint16 TopgunEngine_v1_2_1::builtinSetDraggedSpriteId(uint16 id) {
	uint16 raw = (uint16)(id - TG_HOST_V1_2_1_LITERAL_BIAS);

	if (!isResourceObjectLoaded(raw, kResV1_2_1Sprite))
		return 0;

	_dragSpriteId = raw;
	return 1;
}

uint16 TopgunEngine_v1_2_1::builtinSetKeyHookScript(uint16 script) {
	uint16 previous = _keyHookScript;

	_keyHookScript = script != 0 ? (uint16)(script - TG_HOST_V1_2_1_LITERAL_BIAS) : 0;
	return operandValue(previous);
}

uint16 TopgunEngine_v1_2_1::builtinSetSpriteHoverScript(uint16 script) {
	uint16 previous = _hoverSpriteScript;

	if (script == 0 || resourceLib()->resGetType(_file, script) == kResV1_2_1Script) {
		if (_hoverSpriteScript != 0 && _hoverSprite != 0)
			postSpriteLeaveScript();

		_hoverSpriteScript = script != 0 ? (uint16)(script - TG_HOST_V1_2_1_LITERAL_BIAS) : 0;

		if (script != 0) {
			Common::Point cursor = g_system->getEventManager()->getMousePos();

			transformPointForDisplayZoom(cursor);
			updateSpriteHoverFromPoint(cursor.x, cursor.y);
		}
	}

	return operandValue(previous);
}

uint16 TopgunEngine_v1_2_1::builtinGetMouseEventScript() {
	return operandValue(_mouseEventScript);
}

uint16 TopgunEngine_v1_2_1::builtinSetJoystickDistanceMode(uint16 value) {
	_joystickDirectionFlag = (value != 0);
	return 110;
}

uint16 TopgunEngine_v1_2_1::builtinSetJoystickButtonOffsetMode(uint16 value) {
	_joystickButtonFlag = (value != 0);
	return 130;
}

static const byte kDeviceCodes[TG_DEVICE_V1_2_1_CODE_TABLE_SIZE] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x06, 0x08, 0x0A, 0x09, 0x0C, 0x0B, 0x00, 0x00, 0x07,
	0x06, 0x03, 0x0A, 0x09, 0x05, 0x0C, 0x0B, 0xFE, 0x01, 0xFD, 0x02, 0xFB, 0x04, 0xF7, 0x08, 0x00
};

static const byte kDirectionCodes[16] = {
	0x00, 0x03, 0x05, 0x88, 0x07, 0x0A, 0x0C, 0x88, 0x06, 0x09, 0x0B, 0x88, 0x88, 0x88, 0x88, 0x88
};

static const byte kMouseStickCodes[4] = { 5, 7, 3, 6 };

void TopgunEngine_v1_2_1::clearJoystickScripts() {
	for (uint16 i = 0; i < TG_INPUT_V1_2_1_NUM_JOYSTICK_SCRIPTS; i++)
		_joystickScripts[i] = 0;
}

void TopgunEngine_v1_2_1::clearDeviceState() {
	for (uint16 i = 0; i < TG_INPUT_V1_2_1_KEY_TABLE_SIZE; i++)
		_keyRoutes[i] = KeyRoute_v1_2_1();

	for (uint16 i = 0; i < TG_DEVICE_V1_2_1_NUM_GROUPS; i++)
		_deviceGroups[i] = DeviceGroup_v1_2_1();

	_mouseStickDeadRect = Common::Rect();
	_mouseStickOuterRect = Common::Rect();
	_mouseStickCode = 0;

	for (uint16 i = 0; i < TG_DEVICE_V1_2_1_NUM_DEVICES; i++)
		_deviceOwners[i] = 0;

	for (uint16 i = 0; i < TG_DEVICE_V1_2_1_NUM_KEYBOARD_SETS; i++)
		_keyboardDirections[i] = 0;

	for (uint16 i = 0; i < TG_INPUT_V1_2_1_NUM_JOYSTICK_SCRIPTS; i++)
		_joystickStates[i] = 0;

	for (uint16 i = 0; i < TG_JOYSTICK_V1_2_1_NUM_STICKS; i++)
		_joystickLastIndex[i] = 0;
}

uint16 TopgunEngine_v1_2_1::insertDeviceBindingSorted(uint16 count, DeviceBinding_v1_2_1 *list, const byte *record) {
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

void TopgunEngine_v1_2_1::setAllBindingScripts(uint16 script, uint16 count, DeviceBinding_v1_2_1 *list) {
	for (uint16 i = 0; i < count; i++)
		list[i].script = script;
}

void TopgunEngine_v1_2_1::setAllBindingActive(byte active, uint16 count, DeviceBinding_v1_2_1 *list) {
	for (uint16 i = 0; i < count; i++)
		list[i].active = active;
}

void TopgunEngine_v1_2_1::fillJoystickStateBytes(byte value, byte stick) {
	if (stick >= TG_JOYSTICK_V1_2_1_NUM_STICKS) {
		warning("TopgunEngine_v1_2_1::fillJoystickStateBytes(): stick %d is out of range", stick);
		return;
	}

	for (uint16 i = 0; i < TG_DEVICE_V1_2_1_JOYSTICK_STATE_SIZE; i++)
		_joystickStates[stick * TG_DEVICE_V1_2_1_JOYSTICK_STATE_SIZE + i] = value;
}

uint16 TopgunEngine_v1_2_1::deleteDeviceBinding(uint16 index, uint16 count, DeviceBinding_v1_2_1 *list) {
	if (count <= index)
		return count;

	count--;

	for (uint16 i = index; i < count; i++)
		list[i] = list[i + 1];

	return count;
}

uint16 TopgunEngine_v1_2_1::updateDeviceBindingList(uint16 maxEntries, uint16 count, DeviceBinding_v1_2_1 *list, const byte *record) {
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

void TopgunEngine_v1_2_1::fireDeviceGroupStateScript(byte group) {
	DeviceGroup_v1_2_1 &state = _deviceGroups[group];

	if (state.stateScript == 0)
		return;

	if ((state.pressedMask & 1) != 0)
		enqueuePostedScriptRun(state.stateScript, (uint16)(group - TG_HOST_V1_2_1_LITERAL_BIAS), (uint16)(0xFE - TG_HOST_V1_2_1_LITERAL_BIAS));

	if ((state.pressedMask & 2) != 0)
		enqueuePostedScriptRun(state.stateScript, (uint16)(group - TG_HOST_V1_2_1_LITERAL_BIAS), (uint16)(0xFD - TG_HOST_V1_2_1_LITERAL_BIAS));

	if (state.lastCode != 0)
		enqueuePostedScriptRun(state.stateScript, (uint16)(group - TG_HOST_V1_2_1_LITERAL_BIAS), (uint16)(0 - TG_HOST_V1_2_1_LITERAL_BIAS));
}

void TopgunEngine_v1_2_1::comboHistoryTimeoutCallback(byte timerId) {
	byte groupIndex = (byte)(timerId - 2);

	if (groupIndex >= TG_DEVICE_V1_2_1_NUM_GROUPS) {
		warning("TopgunEngine_v1_2_1::comboHistoryTimeoutCallback(): timer %d is out of range", timerId);
		return;
	}

	DeviceGroup_v1_2_1 &group = _deviceGroups[groupIndex];

	if (group.numBindingsB != 0) {
		scanDeviceBindingsListB(groupIndex);
		return;
	}

	uint16 history = group.history;
	group.history = 0;

	for (uint16 i = 0; i < group.numBindingsA; i++) {
		const DeviceBinding_v1_2_1 &entry = group.bindingsA[i];

		if ((uint16)(entry.mask & history) == entry.priority && entry.active != 0 && entry.script != 0) {
			if (group.latchMode != 0)
				group.latch = entry;
			else
				enqueuePostedScriptRun(entry.script, (uint16)(groupIndex - TG_HOST_V1_2_1_LITERAL_BIAS), entry.id);

			return;
		}
	}
}

void TopgunEngine_v1_2_1::scanDeviceBindingsListB(byte group) {
	if (group >= TG_DEVICE_V1_2_1_NUM_GROUPS) {
		warning("TopgunEngine_v1_2_1::scanDeviceBindingsListB(): group %d is out of range", group);
		return;
	}

	DeviceGroup_v1_2_1 &state = _deviceGroups[group];
	uint16 history = state.history;

	state.history = 0;

	for (uint16 i = 0; i < state.numBindingsB; i++) {
		const DeviceBinding_v1_2_1 &entry = state.bindingsB[i];

		if ((uint16)(entry.mask & history) == entry.priority && entry.active != 0 && entry.script != 0) {
			if (state.latchMode != 0)
				state.latch = entry;
			else
				enqueuePostedScriptRun(entry.script, (uint16)(group - TG_HOST_V1_2_1_LITERAL_BIAS), entry.id);

			return;
		}
	}
}

void TopgunEngine_v1_2_1::deviceGroupInputEvent(byte code, byte device) {
	if (device >= TG_DEVICE_V1_2_1_NUM_DEVICES) {
		warning("TopgunEngine_v1_2_1::deviceGroupInputEvent(): device %d is out of range", device);
		return;
	}

	byte groupIndex = _deviceOwners[device];
	DeviceGroup_v1_2_1 &group = _deviceGroups[groupIndex];
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
		group.lastCode = code;

		if (group.remap == 1) {
			if (code < TG_DEVICE_V1_2_1_CODE_TABLE_SIZE)
				sentCode = kDeviceCodes[code];
			else
				warning("TopgunEngine_v1_2_1::deviceGroupInputEvent(): code %d is out of range", code);
		}

		break;
	}

	if (_textEditObjectId != 0)
		return;

	if (group.stateScript != 0)
		enqueuePostedScriptRun(group.stateScript, (uint16)(groupIndex - TG_HOST_V1_2_1_LITERAL_BIAS), (uint16)(sentCode - TG_HOST_V1_2_1_LITERAL_BIAS));

	if (released)
		return;

	if (group.history == 0)
		addNativeTimer(groupIndex + 2, _inputTimeout, kNativeTimerV1_2_1ComboHistory, 0);

	group.history = (uint16)(((group.history & 0x00FF) << 4) + sentCode);
}

bool TopgunEngine_v1_2_1::keyboardRoutedKeyDown(uint16 vk) {
	if (_gamePaused)
		return true;

	if (vk >= TG_INPUT_V1_2_1_KEY_TABLE_SIZE)
		return true;

	KeyRoute_v1_2_1 &route = _keyRoutes[vk];

	if (route.armed == 0)
		return true;

	route.armed = 0;

	byte code = route.code;
	byte device = route.device;

	if (code == 0x10 || code == 0x20 || code == 0x40 || code == 0x80) {
		_keyboardDirections[device] |= (byte)(code >> 4);
		findNativeTimer(device);

		code = kDirectionCodes[_keyboardDirections[device]];
		if (code == TG_DEVICE_V1_2_1_NO_DIRECTION)
			return false;
	}

	deviceGroupInputEvent(code, device + TG_DEVICE_V1_2_1_KEYBOARD_BASE);
	return false;
}

bool TopgunEngine_v1_2_1::keyboardRoutedKeyUp(uint16 vk) {
	if (_gamePaused)
		return true;

	if (vk >= TG_INPUT_V1_2_1_KEY_TABLE_SIZE)
		return true;

	KeyRoute_v1_2_1 &route = _keyRoutes[vk];

	if (route.code == 0)
		return true;

	route.armed = 1;

	byte code = route.code;
	byte device = route.device;

	if (code == 0x10 || code == 0x20 || code == 0x40 || code == 0x80) {
		_keyboardDirections[device] &= (byte)~(code >> 4);

		if (_keyboardDirections[device] != 0) {
			addNativeTimer(device, TG_DEVICE_V1_2_1_KEY_RELEASE_DELAY, kNativeTimerV1_2_1KeyRelease, 0);
			return false;
		}

		findNativeTimer(device);

		code = kDirectionCodes[_keyboardDirections[device]];
		if (code == TG_DEVICE_V1_2_1_NO_DIRECTION)
			return false;
	} else {
		code = (byte)~code;
	}

	deviceGroupInputEvent(code, device + TG_DEVICE_V1_2_1_KEYBOARD_BASE);
	return false;
}

void TopgunEngine_v1_2_1::keyReleaseDelayTimerCallback(byte device) {
	if (device >= TG_DEVICE_V1_2_1_NUM_KEYBOARD_SETS) {
		warning("TopgunEngine_v1_2_1::keyReleaseDelayTimerCallback(): device %d is out of range", device);
		return;
	}

	byte code = kDirectionCodes[_keyboardDirections[device]];

	if (code != TG_DEVICE_V1_2_1_NO_DIRECTION)
		deviceGroupInputEvent(code, device + TG_DEVICE_V1_2_1_KEYBOARD_BASE);
}

void TopgunEngine_v1_2_1::bindKeyboardSetToDevice(byte device, byte value) {
	if (device >= TG_DEVICE_V1_2_1_NUM_KEYBOARD_SETS) {
		warning("TopgunEngine_v1_2_1::bindKeyboardSetToDevice(): keyboard set %d is out of range", device);
		return;
	}

	for (uint16 i = 0; i < TG_INPUT_V1_2_1_KEY_TABLE_SIZE; i++) {
		if (_keyRoutes[i].device == device) {
			_keyRoutes[i].armed = 0;
			_keyRoutes[i].code = 0;
		}
	}

	if (value == 0)
		return;

	_keyboardDirections[device] = 0;

	for (uint16 slot = 0; slot < TG_DEVICE_V1_2_1_KEYBOARD_SET_SLOTS; slot++) {
		byte vk = _keyboardSetKeys[device][slot];

		_keyRoutes[vk].device = device;
		_keyRoutes[vk].armed = value;
		_keyRoutes[vk].code = _keyboardSetCodes[device][slot];

		if (_keyStates[vk])
			keyboardRoutedKeyDown(vk);
	}
}

bool TopgunEngine_v1_2_1::mouseAsStickMessage(Common::Point point, uint16 message) {
	if (_gamePaused)
		return true;

	byte code = 0;

	if (message == kMsgV1_2MouseMove) {
		transformPointForDisplayZoom(point);
		_variableTable[TG_HOST_V1_2_1_VARIABLE_MOUSE_X] = (uint16)point.x;
		_variableTable[TG_HOST_V1_2_1_VARIABLE_MOUSE_Y] = (uint16)point.y;

		if (_mouseStickDeadRect.contains(point.x, point.y)) {
			code = 0;
		} else {
			int16 angle = utilsLibrary()->getAngle(point.x, point.y);

			if (!_mouseStickOuterRect.contains(point.x, point.y)) {
				point = computeRayRectIntersection(angle, 0, 0,
					_mouseStickOuterRect.left + TG_DEVICE_V1_2_1_STICK_RAY_MARGIN, _mouseStickOuterRect.top + TG_DEVICE_V1_2_1_STICK_RAY_MARGIN,
					_mouseStickOuterRect.right - TG_DEVICE_V1_2_1_STICK_RAY_MARGIN, _mouseStickOuterRect.bottom - TG_DEVICE_V1_2_1_STICK_RAY_MARGIN);
				_variableTable[TG_HOST_V1_2_1_VARIABLE_MOUSE_X] = (uint16)point.x;
				_variableTable[TG_HOST_V1_2_1_VARIABLE_MOUSE_Y] = (uint16)point.y;
				g_system->warpMouse(point.x + display()->viewportOrigin().x, point.y + display()->viewportOrigin().y);
			}

			int16 quadrant = utilsLibrary()->getQuadrant((int16)((angle + TG_DEVICE_V1_2_1_STICK_ANGLE_ROTATION) % TG_DEVICE_V1_2_1_STICK_FULL_TURN));
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
		warning("TopgunEngine_v1_2_1::mouseAsStickMessage(): message %d is not handled", message);
		return true;
	}

	deviceGroupInputEvent(code, TG_DEVICE_V1_2_1_MOUSE);
	return true;
}

void TopgunEngine_v1_2_1::resetMouseStick(byte value) {
	if (value == 0)
		return;

	_mouseStickDeadRect = Common::Rect();
	_mouseStickOuterRect = Common::Rect();
	_mouseStickDeadRect.grow(TG_DEVICE_V1_2_1_STICK_DEAD_INFLATE);
	_mouseStickOuterRect.grow(TG_DEVICE_V1_2_1_STICK_OUTER_INFLATE);
	_mouseStickCode = 0;
	_variableTable[TG_HOST_V1_2_1_VARIABLE_MOUSE_X] = 0;
	_variableTable[TG_HOST_V1_2_1_VARIABLE_MOUSE_Y] = 0;
	g_system->warpMouse(display()->viewportOrigin().x, display()->viewportOrigin().y);
}

void TopgunEngine_v1_2_1::o_setDeviceGroupBinding(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_1_NUM_GROUPS) {
		warning("TopgunEngine_v1_2_1::o_setDeviceGroupBinding(): group %d is out of range", groupIndex);
		return;
	}

	DeviceGroup_v1_2_1 &group = _deviceGroups[groupIndex];

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
		group.numBindingsA = updateDeviceBindingList(TG_DEVICE_V1_2_1_MAX_BINDINGS_A, group.numBindingsA, group.bindingsA, record);
	}
}

void TopgunEngine_v1_2_1::o_setDeviceGroupBindingB(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_1_NUM_GROUPS) {
		warning("TopgunEngine_v1_2_1::o_setDeviceGroupBindingB(): group %d is out of range", groupIndex);
		return;
	}

	DeviceGroup_v1_2_1 &group = _deviceGroups[groupIndex];

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
		group.numBindingsB = updateDeviceBindingList(TG_DEVICE_V1_2_1_MAX_BINDINGS_B, group.numBindingsB, group.bindingsB, record);
	}
}

void TopgunEngine_v1_2_1::o_getDeviceGroupState(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_1_NUM_GROUPS) {
		warning("TopgunEngine_v1_2_1::o_getDeviceGroupState(): group %d is out of range", groupIndex);
		return;
	}

	DeviceGroup_v1_2_1 &group = _deviceGroups[groupIndex];
	uint16 code = group.lastCode;

	if (group.remap == 1) {
		if (code < TG_DEVICE_V1_2_1_CODE_TABLE_SIZE)
			code = kDeviceCodes[code];
		else
			warning("TopgunEngine_v1_2_1::o_getDeviceGroupState(): code %d is out of range", code);
	}

	writeVariable(READ_LE_UINT16(record + 4), code);
	writeVariable(READ_LE_UINT16(record + 6), group.pressedMask);
	writeVariable(READ_LE_UINT16(record + 8), (group.latchMode != 0 && group.latch.script != 0) ? 1 : 0);
}

void TopgunEngine_v1_2_1::o_deviceGroupLatch(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_1_NUM_GROUPS) {
		warning("TopgunEngine_v1_2_1::o_deviceGroupLatch(): group %d is out of range", groupIndex);
		return;
	}

	DeviceGroup_v1_2_1 &group = _deviceGroups[groupIndex];
	uint16 mode = operandValue(READ_LE_UINT16(record + 4));

	group.latchMode = (byte)mode;

	if (mode == 0 && operandValue(READ_LE_UINT16(record + 6)) != 0 && group.latch.script != 0) {
		enqueuePostedScriptRun(group.latch.script, (uint16)(READ_LE_UINT16(record + 2) - TG_HOST_V1_2_1_LITERAL_BIAS), group.latch.id);
		group.latch.script = 0;
		return;
	}

	group.latch.script = 0;
}

void TopgunEngine_v1_2_1::o_setDeviceGroupFlag(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_1_NUM_GROUPS) {
		warning("TopgunEngine_v1_2_1::o_setDeviceGroupFlag(): group %d is out of range", groupIndex);
		return;
	}

	_deviceGroups[groupIndex].remap = (operandValue(READ_LE_UINT16(record + 4)) != 0) ? 1 : 0;
}

void TopgunEngine_v1_2_1::o_setDeviceGroupScript(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_1_NUM_GROUPS) {
		warning("TopgunEngine_v1_2_1::o_setDeviceGroupScript(): group %d is out of range", groupIndex);
		return;
	}

	if (READ_LE_UINT16(record + 4) != 0)
		_deviceGroups[groupIndex].stateScript = (uint16)(operandValue(READ_LE_UINT16(record + 4)) - TG_HOST_V1_2_1_LITERAL_BIAS);
	else
		_deviceGroups[groupIndex].stateScript = 0;
}

void TopgunEngine_v1_2_1::o_deviceGroupButtonState(const byte *record) {
	uint16 groupIndex = operandValue(READ_LE_UINT16(record + 2));

	if (groupIndex >= TG_DEVICE_V1_2_1_NUM_GROUPS) {
		warning("TopgunEngine_v1_2_1::o_deviceGroupButtonState(): group %d is out of range", groupIndex);
		return;
	}

	byte group = (byte)groupIndex;
	DeviceGroup_v1_2_1 &state = _deviceGroups[group];

	if (state.device != 0 && state.device < TG_DEVICE_V1_2_1_NUM_DEVICES && _deviceOwners[state.device] == group) {
		switch (state.type) {
		case TG_DEVICE_V1_2_1_TYPE_JOYSTICK:
			fillJoystickStateBytes(record[7], state.device - TG_DEVICE_V1_2_1_JOYSTICK_BASE);
			break;

		case TG_DEVICE_V1_2_1_TYPE_KEYBOARD:
			bindKeyboardSetToDevice(state.device - TG_DEVICE_V1_2_1_KEYBOARD_BASE, record[7]);
			break;

		case TG_DEVICE_V1_2_1_TYPE_MOUSE:
			resetMouseStick(record[7]);
			_mouseAsStick = (record[7] != 0);
			break;

		default:
			break;
		}
	}

	if (record[8] != 0)
		fireDeviceGroupStateScript(group);

	if (record[7] != 0 || record[8] != 0)
		return;

	switch (record[6]) {
	case TG_DEVICE_V1_2_1_TYPE_JOYSTICK: {
		uint16 device = operandValue(READ_LE_UINT16(record + 4));

		state.device = (byte)(device + TG_DEVICE_V1_2_1_JOYSTICK_BASE);
		fillJoystickStateBytes(1, (byte)device);
		break;
	}

	case TG_DEVICE_V1_2_1_TYPE_KEYBOARD: {
		uint16 device = operandValue(READ_LE_UINT16(record + 4));

		state.device = (byte)(device + TG_DEVICE_V1_2_1_KEYBOARD_BASE);
		bindKeyboardSetToDevice((byte)device, 1);
		break;
	}

	case TG_DEVICE_V1_2_1_TYPE_MOUSE:
		state.device = TG_DEVICE_V1_2_1_MOUSE;
		resetMouseStick(1);
		_mouseAsStick = true;
		break;

	default:
		break;
	}

	state.type = record[6];

	if (state.device >= TG_DEVICE_V1_2_1_NUM_DEVICES) {
		warning("TopgunEngine_v1_2_1::o_deviceGroupButtonState(): device %d is out of range", state.device);
		return;
	}

	if (state.type != 0)
		_deviceOwners[state.device] = group;
	else if (_deviceOwners[state.device] == group && state.device == TG_DEVICE_V1_2_1_MOUSE)
		_mouseAsStick = false;

	_keyboardRouted = (_deviceGroups[0].type == TG_DEVICE_V1_2_1_TYPE_KEYBOARD || _deviceGroups[1].type == TG_DEVICE_V1_2_1_TYPE_KEYBOARD);
	_joystickRouted = (_deviceGroups[0].type == TG_DEVICE_V1_2_1_TYPE_JOYSTICK || _deviceGroups[1].type == TG_DEVICE_V1_2_1_TYPE_JOYSTICK);
}

uint16 TopgunEngine_v1_2_1::builtinSetKeyboardSetKey(uint16 set, uint16 kind, uint16 vk) {
	byte setIndex = (byte)set;

	if (setIndex >= TG_DEVICE_V1_2_1_NUM_KEYBOARD_SETS) {
		warning("TopgunEngine_v1_2_1::builtinSetKeyboardSetKey(): keyboard set %d is out of range", setIndex);
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

	case 5:
		_keyboardSetKeys[setIndex][1] = (byte)vk;
		break;

	case 6:
		_keyboardSetKeys[setIndex][3] = (byte)vk;
		break;

	case 7:
		_keyboardSetKeys[setIndex][2] = (byte)vk;
		break;

	default:
		break;
	}

	return 107;
}

uint16 TopgunEngine_v1_2_1::builtinReattachDeviceGroup(uint16 group, uint16 flag) {
	if (group >= TG_DEVICE_V1_2_1_NUM_GROUPS) {
		warning("TopgunEngine_v1_2_1::builtinReattachDeviceGroup(): group %d is out of range", group);
		return 137;
	}

	DeviceGroup_v1_2_1 &state = _deviceGroups[group];
	byte base = state.device;

	if (base >= TG_DEVICE_V1_2_1_NUM_DEVICES) {
		warning("TopgunEngine_v1_2_1::builtinReattachDeviceGroup(): device %d is out of range", base);
		return 137;
	}

	_deviceOwners[base] = (byte)group;

	if (flag != 0) {
		byte other = (byte)((group + 1) & 1);
		DeviceGroup_v1_2_1 &otherState = _deviceGroups[other];

		if (otherState.device != 0 && otherState.device < TG_DEVICE_V1_2_1_NUM_DEVICES && _deviceOwners[otherState.device] == other) {
			switch (otherState.type) {
			case TG_DEVICE_V1_2_1_TYPE_JOYSTICK:
				fillJoystickStateBytes(0, otherState.device - TG_DEVICE_V1_2_1_JOYSTICK_BASE);
				break;

			case TG_DEVICE_V1_2_1_TYPE_KEYBOARD:
				bindKeyboardSetToDevice(otherState.device - TG_DEVICE_V1_2_1_KEYBOARD_BASE, 0);
				break;

			case TG_DEVICE_V1_2_1_TYPE_MOUSE:
				resetMouseStick(0);
				_mouseAsStick = (base == TG_DEVICE_V1_2_1_MOUSE);
				break;

			default:
				break;
			}
		}
	}

	switch (state.type) {
	case TG_DEVICE_V1_2_1_TYPE_JOYSTICK:
		fillJoystickStateBytes(1, base - TG_DEVICE_V1_2_1_JOYSTICK_BASE);
		break;

	case TG_DEVICE_V1_2_1_TYPE_KEYBOARD:
		bindKeyboardSetToDevice(base - TG_DEVICE_V1_2_1_KEYBOARD_BASE, 1);
		break;

	case TG_DEVICE_V1_2_1_TYPE_MOUSE:
		resetMouseStick(1);
		_mouseAsStick = true;
		break;

	default:
		break;
	}

	return 137;
}

uint16 TopgunEngine_v1_2_1::joystickGetScriptForDirectionCode(uint16 offset, uint16 flags) {
	uint16 index = joystickButtonIndex(flags);

	if (index == TG_JOYSTICK_V1_2_1_NO_INDEX)
		return 0;

	return _joystickScripts[index + offset];
}

uint16 TopgunEngine_v1_2_1::joystickButtonIndex(uint16 flags) {
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

uint16 TopgunEngine_v1_2_1::joystickDirectionIndex(uint16 offset, const uint16 *center, uint16 x, uint16 y) {
	uint16 code;

	if (_joystickDirectionFlag)
		code = joystickDirectionByDistance(center, x, y);
	else
		code = joystickDirection8Way(center, x, y);

	return code + offset;
}

uint16 TopgunEngine_v1_2_1::joystickDirectionByDistance(const uint16 *center, uint16 x, uint16 y) {
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

uint16 TopgunEngine_v1_2_1::joystickDirection8Way(const uint16 *center, uint16 x, uint16 y) {
	int32 centerX = center[0];
	int32 centerY = center[1];
	uint16 code = 0;

	if (centerY - TG_JOYSTICK_V1_2_1_EIGHT_WAY_THRESHOLD > (int32)y)
		code = 3;

	if (centerY + TG_JOYSTICK_V1_2_1_EIGHT_WAY_THRESHOLD < (int32)y)
		code += 6;

	if (centerX + TG_JOYSTICK_V1_2_1_EIGHT_WAY_THRESHOLD < (int32)x)
		code += 1;

	if (centerX - TG_JOYSTICK_V1_2_1_EIGHT_WAY_THRESHOLD > (int32)x)
		code += 2;

	return code;
}

uint16 TopgunEngine_v1_2_1::joystickGetScriptForPosition(uint16 offset, const uint16 *center, uint16 x, uint16 y) {
	return _joystickScripts[joystickDirectionIndex(offset, center, x, y)];
}

uint16 TopgunEngine_v1_2_1::joystickMessageToIndex(const Message &message) {
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

void TopgunEngine_v1_2_1::handleJoystickButtonMessage(const Message &message) {
	uint16 index = joystickMessageToIndex(message);

	if (index == TG_JOYSTICK_V1_2_1_NO_INDEX)
		return;

	if (index >= TG_INPUT_V1_2_1_NUM_JOYSTICK_SCRIPTS) {
		warning("TopgunEngine_v1_2_1::handleJoystickButtonMessage(): slot %d is out of range", index);
		return;
	}

	if (_joystickStates[index] == 0)
		return;

	byte stick = (index >= TG_JOYSTICK_V1_2_1_STICK_OFFSET) ? 1 : 0;

	if (message.id == kMsgV1_2_1Joystick1ButtonDown || message.id == kMsgV1_2_1Joystick2ButtonDown) {
		if (index > 0)
			_joystickStates[index - 1] = 1;
	} else if (message.id == kMsgV1_2_1Joystick1ButtonUp || message.id == kMsgV1_2_1Joystick2ButtonUp) {
		if (index + 1 < TG_INPUT_V1_2_1_NUM_JOYSTICK_SCRIPTS)
			_joystickStates[index + 1] = 1;
	} else {
		_joystickStates[_joystickLastIndex[stick]] = 1;
		_joystickLastIndex[stick] = (byte)index;
	}

	_joystickStates[index] = 0;

	byte code = kDeviceCodes[TG_DEVICE_V1_2_1_JOYSTICK_CODES_OFFSET + index - stick * TG_JOYSTICK_V1_2_1_STICK_OFFSET];
	deviceGroupInputEvent(code, stick + TG_DEVICE_V1_2_1_JOYSTICK_BASE);
}

void TopgunEngine_v1_2_1::handleJoystickMessage(const Message &message) {
	if (callMessageFilter(message.id, message.param, (uint16)(message.data & 0xFFFF), (uint16)(message.data >> 16)))
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

void TopgunEngine_v1_2_1::joystickInit() {
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

void TopgunEngine_v1_2_1::joystickRelease() {
	if (!_joystickCaptured)
		return;

	_joystickCaptured = false;
}

} // End of namespace Topgun
