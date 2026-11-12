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

#include "common/events.h"
#include "common/keyboard.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/translation.h"

#include "backends/keymapper/action.h"
#include "backends/keymapper/keymap.h"
#include "backends/keymapper/standard-actions.h"

#include "engines/grim/emi/graphics/cdrawcontext.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/input/eventgenerator.h"
#include "engines/grim/emi/input/inputrecorder.h"
#include "engines/grim/emi/input/joystick.h"
#include "engines/grim/emi/input/keygenerator.h"
#include "engines/grim/emi/lua_v2_opcodes/lua_v2.h"
#include "engines/grim/emi/input/sputcontrol_v2.h"

namespace Grim {

static const int kVirtualKeyBackspace = 0x08;
static const int kVirtualKeyControl = 0x11;
static const int kVirtualKeyPause = 0x13;
static const int kVirtualKeyF = 0x46;

void EmiEngine::pumpEvents() {
	Common::EventManager *eventManager = g_system->getEventManager();
	Common::Event event;

	while (eventManager->pollEvent(event)) {
		switch (event.type) {
		case Common::EVENT_QUIT:
		case Common::EVENT_RETURN_TO_LAUNCHER:
			if (!callExitHook()) {
				eventManager->resetQuit();
				eventManager->resetReturnToLauncher();
			}

			break;
		case Common::EVENT_FOCUS_GAINED:
			handleActivation(true);
			break;
		case Common::EVENT_FOCUS_LOST:
			handleActivation(false);
			break;
		case Common::EVENT_KEYDOWN:
			handleKeyPress(event.kbd, event.kbdRepeat);
			break;
		case Common::EVENT_KEYUP:
			handleKeyRelease(event.kbd);
			break;
		case Common::EVENT_JOYAXIS_MOTION:
			_joystick->setDeviceAxis(event.joystick.axis, event.joystick.position);
			break;
		case Common::EVENT_JOYBUTTON_DOWN:
			_joystick->setDeviceButton(event.joystick.button, true);
			break;
		case Common::EVENT_JOYBUTTON_UP:
			_joystick->setDeviceButton(event.joystick.button, false);
			break;
		default:
			break;
		}
	}
}

void EmiEngine::handleKeyPress(const Common::KeyState &key, bool repeat) {
	int virtualKey = KeyGenerator::keyCodeToVirtualKey(key.keycode);

	if (virtualKey != 0) {
		int keyFlags = 0;

		if (repeat)
			keyFlags |= KeyGenerator::kFlagRepeat;

		if (key.keycode == Common::KEYCODE_KP_ENTER)
			keyFlags |= KeyGenerator::kFlagExtended;

		_keyGenerator->keyDown(virtualKey, keyFlags, (int)g_system->getMillis());
		handleDebugKeys(virtualKey);
	}

	bool altOnly = (key.flags & Common::KBD_ALT) != 0 && (key.flags & Common::KBD_CTRL) == 0;

	if (key.ascii > 0 && key.ascii < 256 && key.ascii != 127 && !altOnly)
		onCharacter((byte)key.ascii);
}

void EmiEngine::handleKeyRelease(const Common::KeyState &key) {
	int virtualKey = KeyGenerator::keyCodeToVirtualKey(key.keycode);

	if (virtualKey == 0)
		return;

	_keyGenerator->keyUp(virtualKey, 0, (int)g_system->getMillis());

	if (virtualKey == kVirtualKeyBackspace)
		_backspaceHeld = false;
	else if (virtualKey == kVirtualKeyControl)
		_controlHeld = false;
}

void EmiEngine::handleDebugKeys(int virtualKey) {
	switch (virtualKey) {
	case kVirtualKeyBackspace:
		_backspaceHeld = true;
		break;
	case kVirtualKeyControl:
		_controlHeld = true;
		break;
	case kVirtualKeyPause:
		// PS2 has no pause handler
		if (_lua && getGamePlatform() != Common::kPlatformPS2)
			_lua->doPauseCallback();

		break;
	case kVirtualKeyF:
		if (_showFps && _backspaceHeld && _controlHeld)
			_lua->toggleStatsOverlay(false);

		break;
	default:
		break;
	}
}

void EmiEngine::handleActivation(bool active) {
	if (active) {
		if (_keyGenerator) {
			_keyGenerator->clearEvents();
			_control->clearKeyStates();
		}

		if (_drawContext)
			_drawContext->restoreDisplay();
	} else if (_keyGenerator) {
		_keyGenerator->clearEvents();
	}
}

bool EmiEngine::onCharacter(byte character) {
	if (!_lua) {
		warning("EmiEngine::onCharacter(): Lua is not started");

		return false;
	}

	// PS2 has no character handler
	if (getGamePlatform() == Common::kPlatformPS2)
		return false;

	bool handled = _lua->doCharacterCallback(character) != 0;

	if (handled && _inputRecorder)
		_inputRecorder->recordChar(character);

	return handled;
}

void EmiEngine::pauseEngineIntern(bool pause) {
	if (pause) {
		_enginePaused = _mode != kModePaused;
		setPause(true);
	} else if (_enginePaused) {
		_enginePaused = 0;
		setPause(false);
	}
}

Common::Array<Common::Keymap *> EmiEngine::initKeymapsEmi(const char *target) {
	Common::Keymap *engineKeyMap = new Common::Keymap(Common::Keymap::kKeymapTypeGame, "monkey4", "Escape from Monkey Island");
	Common::Action *act;

	act = new Common::Action(Common::kStandardActionMoveUp, _("Up"));
	act->setKeyEvent(Common::KEYCODE_UP);
	act->addDefaultInputMapping("JOY_UP");
	engineKeyMap->addAction(act);

	act = new Common::Action(Common::kStandardActionMoveDown, _("Down"));
	act->setKeyEvent(Common::KEYCODE_DOWN);
	act->addDefaultInputMapping("JOY_DOWN");
	engineKeyMap->addAction(act);

	act = new Common::Action(Common::kStandardActionMoveLeft, _("Left"));
	act->setKeyEvent(Common::KEYCODE_LEFT);
	act->addDefaultInputMapping("JOY_LEFT");
	engineKeyMap->addAction(act);

	act = new Common::Action(Common::kStandardActionMoveRight, _("Right"));
	act->setKeyEvent(Common::KEYCODE_RIGHT);
	act->addDefaultInputMapping("JOY_RIGHT");
	engineKeyMap->addAction(act);

	// I18N: Cycle means rotate through
	act = new Common::Action("COUP", _("Cycle objects up"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_PAGEUP));
	act->addDefaultInputMapping("JOY_LEFT_TRIGGER");
	engineKeyMap->addAction(act);

	// I18N: Cycle means rotate through
	act = new Common::Action("CODW", _("Cycle objects down"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_PAGEDOWN));
	act->addDefaultInputMapping("JOY_RIGHT_TRIGGER");
	engineKeyMap->addAction(act);

	// I18N: Run is a movement type
	act = new Common::Action("BRUN", _("Run"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_LSHIFT));
	act->addDefaultInputMapping("JOY_RIGHT_SHOULDER");
	engineKeyMap->addAction(act);

	act = new Common::Action("QEXT", _("Quick room exit"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_o, 'o'));
	act->addDefaultInputMapping("JOY_LEFT_SHOULDER");
	engineKeyMap->addAction(act);

	act = new Common::Action("EXAM", _("Examine / Look"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_e, 'e'));
	act->addDefaultInputMapping("JOY_X");
	engineKeyMap->addAction(act);

	act = new Common::Action("BUSE", _("Use / Talk"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_u, 'u'));
	act->addDefaultInputMapping("JOY_A");
	engineKeyMap->addAction(act);

	act = new Common::Action("PICK", _("Pick up / Put away"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_KP_PLUS, '+'));
	act->addDefaultInputMapping("JOY_B");
	engineKeyMap->addAction(act);

	act = new Common::Action("INVT", _("Inventory"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_INSERT, 'i'));
	act->addDefaultInputMapping("JOY_Y");
	engineKeyMap->addAction(act);

	act = new Common::Action("SKLI", _("Skip dialog lines"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_PERIOD, '.'));
	act->addDefaultInputMapping("PERIOD");
	act->addDefaultInputMapping("JOY_A");
	engineKeyMap->addAction(act);

	// I18N: Skipping cutscene playback
	act = new Common::Action(Common::kStandardActionSkip, _("Skip"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_ESCAPE, Common::ASCII_ESCAPE));
	act->addDefaultInputMapping("ESCAPE");
	act->addDefaultInputMapping("JOY_B");
	engineKeyMap->addAction(act);

	act = new Common::Action("RETURN", _("Confirm"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_RETURN, Common::ASCII_RETURN));
	act->addDefaultInputMapping("RETURN");
	act->addDefaultInputMapping("JOY_A");
	engineKeyMap->addAction(act);

	act = new Common::Action("GMNU", _("Menu"));
	act->setKeyEvent(Common::KeyState(Common::KEYCODE_F1, 0));
	act->addDefaultInputMapping("JOY_GUIDE");
	engineKeyMap->addAction(act);

	return Common::Keymap::arrayOf(engineKeyMap);
}

} // End of namespace Grim
