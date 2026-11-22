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

#include "common/endian.h"
#include "common/events.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "topgun/display.h"
#include "topgun/topgun_v1_2.h"

namespace Topgun {

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

		if (event.kbd.keycode == Common::KEYCODE_F10)
			return false;

		message.id = kMsgKeyDown;
		message.param = (uint16)event.kbd.keycode;
		message.data = event.kbd.ascii;
		message.shift = (event.kbd.flags & Common::KBD_SHIFT) != 0;
		message.ctrl = (event.kbd.flags & Common::KBD_CTRL) != 0;
		return true;
	}

	case Common::EVENT_KEYUP: {
		uint16 key = keyCodeToScriptKey(event.kbd.keycode);
		if (key != TG_HOST_V1_2_NO_KEY)
			_keyStates[key] = false;

		return false;
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

void TopgunEngine_v1_2::postRunScript(uint16 script, uint16 argument1, uint16 argument2) {
	postMessage(kMsgV1_2RunScript, script, ((uint32)argument2 << 16) | argument1);
}

void TopgunEngine_v1_2::postMouseEventScript(uint16 code) {
	byte record[9];

	memset(record, 0, sizeof(record));
	WRITE_LE_UINT16(record + 2, _mouseEventScript);
	record[4] = 1;
	WRITE_LE_UINT16(record + 5, (uint16)(code - TG_HOST_V1_2_LITERAL_BIAS));

	o_callScript(record);
}

void TopgunEngine_v1_2::handleMouseButtonMessage(const Message &message) {
	bool left = (message.id == kMsgLButtonDown);

	if (left)
		_leftButtonDown = true;
	else
		_rightButtonDown = true;

	if (_cursorMode == TG_HOST_V1_2_CURSOR_MODE_BUSY || _cursorMode == TG_HOST_V1_2_CURSOR_MODE_HIDDEN)
		return;

	if (_dragSpriteId != 0)
		return;

	int16 x = (int16)(message.data & 0xFFFF);
	int16 y = (int16)(message.data >> 16);

	if (left && message.shift && message.ctrl)
		warning("beginRubberBandSelection UNIMPLEMENTED");

	if (_rubberBandMode)
		return;

	Common::Point point(x, y);
	transformPointForDisplayZoom(point);

	_variables[TG_HOST_V1_2_VARIABLE_MOUSE_BUTTON] = left ? TG_HOST_V1_2_MOUSE_BUTTON_LEFT : TG_HOST_V1_2_MOUSE_BUTTON_RIGHT;
	_variables[TG_HOST_V1_2_VARIABLE_CLICK_X] = (uint16)point.x;
	_variables[TG_HOST_V1_2_VARIABLE_CLICK_Y] = (uint16)point.y;

	if (_mouseEventScript != 0) {
		postMouseEventScript(TG_HOST_V1_2_MOUSE_EVENT_DOWN);
		return;
	}

	uint16 script = 0;
	uint16 sprite = spriteLib()->spritePicked(point.x, point.y);

	if (sprite != 0) {
		uint16 id = spriteLib()->spriteGetId(sprite);
		uint16 index = operandValue(id);

		if (isRecordIndexValid(index, "handleMouseButtonMessage") && _records[index].data) {
			const byte *object = _records[index].data;

			if (object[TG_HOST_V1_2_SPRITE_DATA_DRAGGABLE] != 0) {
				_dragSpriteId = id;
				_dragStartX = point.x;
				_dragStartY = point.y;
				script = READ_LE_UINT16(object + TG_HOST_V1_2_SPRITE_DATA_DRAG_SCRIPT);
			} else if (object[TG_HOST_V1_2_SPRITE_DATA_DISABLED] == 0) {
				script = READ_LE_UINT16(object + TG_HOST_V1_2_SPRITE_DATA_CLICK_SCRIPT);
			}
		}
	}

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
		postRunScript(script, 0, 0);
}

void TopgunEngine_v1_2::handleMouseButtonUpMessage(const Message &message) {
	bool left = (message.id == kMsgV1_2LButtonUp);

	if (left)
		_leftButtonDown = false;
	else
		_rightButtonDown = false;

	if (_mouseEventScript != 0) {
		postMouseEventScript(TG_HOST_V1_2_MOUSE_EVENT_UP);
		return;
	}

	if (_rubberBandMode) {
		if (left)
			warning("endRubberBandSelection UNIMPLEMENTED");

		return;
	}

	if (_dragSpriteId != 0) {
		Common::Point point((int16)(message.data & 0xFFFF), (int16)(message.data >> 16));

		transformPointForDisplayZoom(point);
		dropDraggedSprite(point.x, point.y);
	}
}

void TopgunEngine_v1_2::handleMouseMoveMessage(const Message &message) {
	int16 x = (int16)(message.data & 0xFFFF);
	int16 y = (int16)(message.data >> 16);

	if (_rubberBandMode) {
		warning("updateRubberBand UNIMPLEMENTED");
		return;
	}

	Common::Point point(x, y);
	transformPointForDisplayZoom(point);

	_variables[TG_HOST_V1_2_VARIABLE_MOUSE_X] = (uint16)point.x;
	_variables[TG_HOST_V1_2_VARIABLE_MOUSE_Y] = (uint16)point.y;

	if (_mouseEventScript != 0) {
		postMouseEventScript(TG_HOST_V1_2_MOUSE_EVENT_MOVE);
		return;
	}

	if (_dragSpriteId != 0) {
		uint16 index = operandValue(_dragSpriteId);

		if (isRecordIndexValid(index, "handleMouseMoveMessage"))
			spriteLib()->spriteOffset(_records[index].handle, (int16)(point.x - _dragStartX), (int16)(point.y - _dragStartY));

		_dragStartX = point.x;
		_dragStartY = point.y;
		return;
	}

	if (_numHoverRegions != 0)
		updateHoverRegionFromPoint(point.x, point.y);
}

void TopgunEngine_v1_2::dropDraggedSprite(int16 x, int16 y) {
	_variables[TG_HOST_V1_2_VARIABLE_DROP_X] = (uint16)x;
	_variables[TG_HOST_V1_2_VARIABLE_DROP_Y] = (uint16)y;

	uint16 index = operandValue(_dragSpriteId);

	if (isRecordIndexValid(index, "dropDraggedSprite") && _records[index].data) {
		uint16 script = READ_LE_UINT16(_records[index].data + TG_HOST_V1_2_SPRITE_DATA_DROP_SCRIPT);

		if (script != 0)
			postRunScript(script, 0, 0);
	}

	_dragSpriteId = 0;
}

void TopgunEngine_v1_2::handleKeyDownMessage(Common::KeyCode keycode, bool shift, bool ctrl) {
	uint16 key = keyCodeToScriptKey(keycode);

	if (key == TG_HOST_V1_2_VK_ADD || key == TG_HOST_V1_2_VK_SUBTRACT) {
		adjustMasterVolumeByKey(keycode);
		return;
	}

	if (_rubberBandMode && key == TG_HOST_V1_2_VK_ESCAPE) {
		warning("endRubberBandSelection UNIMPLEMENTED");
		return;
	}

	if (_textEditObjectId != 0)
		return;

	if (key >= TG_HOST_V1_2_KEY_TABLE_SIZE)
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
	uint16 object = getTextObjectFromId(_textEditObjectId);
	if (object == 0)
		return;

	bool changed = false;

	if (character == TG_HOST_V1_2_CHAR_ESCAPE || character == TG_HOST_V1_2_CHAR_RETURN)
		changed = endTextEdit(object, character == TG_HOST_V1_2_CHAR_ESCAPE);
	else if (character == TG_HOST_V1_2_CHAR_BACKSPACE)
		changed = textEditBackspace(object);
	else if (character >= TG_HOST_V1_2_CHAR_FIRST_PRINTABLE)
		changed = textEditAppendChar(object, (byte)character);

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
	uint16 first = 0;
	uint16 last = TG_HOST_V1_2_KEY_TABLE_SIZE - 1;

	if (key != 0) {
		if (key >= TG_HOST_V1_2_KEY_TABLE_SIZE) {
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

	object[TG_HOST_V1_2_SPRITE_DATA_DISABLED] = 0;

	if (record[0x10] != 0)
		object[TG_HOST_V1_2_SPRITE_DATA_DISABLED] = 1;
	else if (record[0x0F] != 0)
		object[TG_HOST_V1_2_SPRITE_DATA_DISABLED] = 0;
	else
		WRITE_LE_UINT16(object + TG_HOST_V1_2_SPRITE_DATA_CLICK_SCRIPT, READ_LE_UINT16(record + 4));

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
		if (_numClickRegions >= TG_HOST_V1_2_MAX_CLICK_REGIONS)
			return;

		index = _numClickRegions;
		_numClickRegions++;
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
		region.disabled = 0;
	}
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

	for (uint16 i = 0; i < _numResources; i++) {
		if (resourceLib()->resGetType(_file, i) == kResV1_2Sprite && _records[i].data)
			WRITE_LE_UINT16(_records[i].data + TG_HOST_V1_2_SPRITE_DATA_CLICK_SCRIPT, value);
	}
}

void TopgunEngine_v1_2::setAllClickRegionsAndSpriteObjectsEnabled(byte disabled) {
	for (uint16 i = 0; i < _numClickRegions; i++)
		_clickRegions[i].disabled = disabled;

	if (!_records)
		return;

	for (uint16 i = 0; i < _numResources; i++) {
		if (resourceLib()->resGetType(_file, i) == kResV1_2Sprite && _records[i].data)
			_records[i].data[TG_HOST_V1_2_SPRITE_DATA_DISABLED] = disabled;
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

	WRITE_LE_UINT16(object + TG_HOST_V1_2_SPRITE_DATA_DROP_SCRIPT, READ_LE_UINT16(record + 6));
	WRITE_LE_UINT16(object + TG_HOST_V1_2_SPRITE_DATA_DRAG_SCRIPT, READ_LE_UINT16(record + 4));
	object[TG_HOST_V1_2_SPRITE_DATA_DRAGGABLE] = (record[8] == 0) ? 1 : 0;

	if (_dragSpriteId == id && record[8] != 0) {
		Common::Point cursor = g_system->getEventManager()->getMousePos();

		dropDraggedSprite(cursor.x, cursor.y);
	}

	return true;
}

} // End of namespace Topgun
