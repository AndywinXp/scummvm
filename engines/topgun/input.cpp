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
#include "common/memstream.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "graphics/cursorman.h"
#include "graphics/wincursor.h"

#include "topgun/display.h"
#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/topgun_v0.h"

namespace Topgun {

enum {
	kHourglassHotspotX = 15,
	kHourglassHotspotY = 14
};

static const byte kHourglassCursor[] = {
	0x28, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xC0, 0x07, 0xFF, 0xFF, 0xEF, 0xEF, 0xFF, 0xFF, 0xE0, 0x0F, 0xFF,
	0xFF, 0xEF, 0xEF, 0xFF, 0xFF, 0xEF, 0xEF, 0xFF, 0xFF, 0xEF, 0xEF, 0xFF,
	0xFF, 0xEF, 0xEF, 0xFF, 0xFF, 0xF7, 0xDF, 0xFF, 0xFF, 0xFB, 0xBF, 0xFF,
	0xFF, 0xFD, 0x7F, 0xFF, 0xFF, 0xFD, 0x7F, 0xFF, 0xFF, 0xFD, 0x7F, 0xFF,
	0xFF, 0xFB, 0xBF, 0xFF, 0xFF, 0xF7, 0xDF, 0xFF, 0xFF, 0xEF, 0xEF, 0xFF,
	0xFF, 0xEF, 0xEF, 0xFF, 0xFF, 0xEF, 0xEF, 0xFF, 0xFF, 0xEF, 0xEF, 0xFF,
	0xFF, 0xE0, 0x0F, 0xFF, 0xFF, 0xEF, 0xEF, 0xFF, 0xFF, 0xC0, 0x07, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF
};

bool TopgunEngine_v0::peekInputMessage(Message &message) {
	Common::EventManager *eventManager = g_system->getEventManager();
	Common::Event event;

	while (true) {
		if (!eventManager->pollEvent(event))
			return false;

		if (translate(event, message))
			return true;
	}
}

void TopgunEngine_v0::discardPendingInput(uint16 id) {
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

bool TopgunEngine_v0::translate(const Common::Event &event, Message &message) {
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

uint16 TopgunEngine_v0::scriptKeyToKeyCodes(uint16 scriptKey, Common::KeyCode *keyCodes) const {
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

void TopgunEngine_v0::handleKeyDownMessage(Common::KeyCode keycode, bool shift, bool ctrl) {
	uint16 script = 0;

	if (keycode == Common::KEYCODE_F2) {
		_globalFlags[8] = (_globalFlags[8] == 0) ? 1 : 0;
		ConfMan.setInt("howie", _globalFlags[8]);
		ConfMan.flushToDisk();
	}

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

void TopgunEngine_v0::handleMouseButtonDown(int16 x, int16 y) {
	if (_inputBusy)
		return;

	if (!_cursorEnabled)
		return;

	uint16 script = 0;

	_globalFlags[3] = 1;

	x -= display()->viewportOrigin().x;
	y -= display()->viewportOrigin().y;

	uint16 sprite = spriteLibrary()->spritePicked(x, y);
	if (sprite != 0) {
		uint16 id = spriteLibrary()->spriteGetId(sprite);
		if (_records && id < _numResources)
			script = _records[id].clickScript;
		else
			warning("TopgunEngine_v0::handleMouseButtonDown(): resource id %d is out of range", id);
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

void TopgunEngine_v0::addOrUpdateClickRegion(const byte *record) {
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

	if (_numHotspots < kMaxHotspots) {
		HotspotRecord &hotspot = _hotspots[_numHotspots];
		hotspot.left = left;
		hotspot.top = top;
		hotspot.right = right;
		hotspot.bottom = bottom;
		hotspot.script = script;
		_numHotspots++;
	}
}

void TopgunEngine_v0::applyCursorFromRegister() {
	setCursorImage(_inputBusy || !_cursorEnabled);
}

void TopgunEngine_v0::setCursorImage(bool hourglass) {
	Graphics::Cursor *cursor;

	if (hourglass) {
		Common::MemoryReadStream stream(kHourglassCursor, sizeof(kHourglassCursor));
		cursor = Graphics::loadWindowsCursorFromDIB(stream, kHourglassHotspotX, kHourglassHotspotY);
	} else {
		cursor = Graphics::makeDefaultWinCursor();
	}

	if (!cursor) {
		warning("TopgunEngine_v0::setCursorImage(): cannot create the cursor");
		return;
	}

	CursorMan.replaceCursor(cursor);
	delete cursor;
}

void TopgunEngine_v0::adjustMasterVolumeByKey(Common::KeyCode keycode) {
	uint32 volume = audioLibrary()->audioCommand(0x82, 0, 0, nullptr);
	uint32 stepped = stepVolumeLevels5Bit(volume, keycode == Common::KEYCODE_KP_PLUS);
	audioLibrary()->audioCommand(0x83, 0, stepped, nullptr);
}

uint32 TopgunEngine_v0::stepVolumeLevels5Bit(uint32 volume, bool up) {
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

} // End of namespace Topgun
