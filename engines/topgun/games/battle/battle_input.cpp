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

#include "topgun/games/battle/battle.h"

namespace Topgun {

void BattlePlugin::initInputTimers() {
	_initFlags |= 3;

	uint16 timeout = _host->getPrivateProfileInt("Input", "Timeout", 350);

	Fighter *f = &_ctx->fighters[0];
	f->inputTimerId = kTimerInput0;
	f->inputTimerScript = kScriptInputTimer0;
	f->inputTimeout = timeout;

	f = &_ctx->fighters[1];
	f->inputTimerId = kTimerInput1;
	f->inputTimerScript = kScriptInputTimer1;
	f->inputTimeout = timeout;
}

uint16 BattlePlugin::inputEvent(byte code, byte player) {
	Fighter *f = &_ctx->fighters[player];

	if (code == kInputRelease4 || code == kInputRelease3 || code == kInputRelease2 || code == kInputRelease1) {
		f->buttons &= code;
		if (f->inputEnabled != 0 && f->hook != nullptr)
			(this->*f->hook)(f, code);

		return 1;
	}

	if (code == kDirNone) {
		f->dirCode = kDirNone;
		if (f->inputEnabled != 0 && f->hook != nullptr)
			(this->*f->hook)(f, kDirNone);

		return 1;
	}

	if (code == kInputButton1 || code == kInputButton2 || code == kInputButton3 || code == kInputButton4) {
		f->buttons |= code;
	} else {
		if (f->facing == 1) {
			if (code < sizeof(_directionMirror))
				code = _directionMirror[code];
			else
				warning("BattlePlugin::inputEvent(): direction code 0x%02X has no mirrored value", code);
		}

		f->dirCode = code;
	}

	if (f->inputEnabled == 0)
		return 1;

	if (f->hook != nullptr && (this->*f->hook)(f, code) == 0)
		return 1;

	if (f->history == 0)
		_host->setScriptTimer(f->inputTimerId, f->inputTimeout, f->inputTimerScript);

	f->history &= 0x0FFF;
	f->history = (uint16)((f->history << 4) + code);

	inputMatch(player, 0);
	return 1;
}

uint16 BattlePlugin::inputMatch(uint16 player, uint16 timeout) {
	if ((byte)player >= kFighterCount) {
		warning("BattlePlugin::inputMatch(): invalid fighter %d", (byte)player);
		return 0;
	}

	Fighter *f = &_ctx->fighters[(byte)player];

	if ((byte)timeout != 0 || f->inputEnabled == 0) {
		f->history = 0;
		return 0;
	}

	MoveEntry *moveTable = f->moveTab[f->set];
	uint16 history = f->history;
	bool noMatch = true;
	uint16 extra = 0;
	MoveEntry *selected = nullptr;
	byte move = 0;

	uint16 frameIds[4] = { 0, 0, 0, 0 };
	_host->getSpriteFrameIds(f->sprite, frameIds);
	uint16 current = frameIds[0];
	bool changed = (f->lastFrame != current);

	if (changed && f->specialTab != nullptr && f->specialTab[0].value != 0xFFFF) {
		for (const SpecialMove *special = f->specialTab; special->value != 0xFFFF; special++) {
			if ((special->mask & history) == special->value && current >= special->frameLo && current <= special->frameHi) {
				MoveEntry *entry = &moveTable[special->move];
				if (entry->available == 0)
					return 0;

				selected = entry;
				move = special->move;
				extra = special->cel;
				f->history = 0;
				noMatch = false;
				break;
			}
		}
	}

	if (noMatch) {
		const InputRecord *list = f->inputTab[f->set];
		if (list == nullptr) {
			warning("BattlePlugin::inputMatch(): fighter %d has no input list for set %d", (byte)player, f->set);
		} else {
			for (const InputRecord *record = list; record->value != 0xFFFF; record++) {
				if ((history & record->mask) == record->value) {
					MoveEntry *entry = &moveTable[record->move];
					if (entry->available == 0)
						return 0;

					selected = entry;
					if (f->queueGate != 0 && changed) {
						f->queued = record->move;
						return 0;
					}

					move = record->move;
					noMatch = false;
					break;
				}
			}
		}
	}

	if (noMatch)
		return 0;

	if (_ctx->networkGame == 0) {
		f->move = move;
		f->moveCel = extra;
		f->hook = nullptr;
		f->specialTab = selected->specialList;
		f->lastFrame = current;
		f->queued = 0;
		f->anim = f->animTab[f->remap[selected->animIndex]];
		f->handlerParam = selected->handlerParam;

		if (selected->handler == nullptr) {
			warning("BattlePlugin::inputMatch(): move %d has no handler", f->move);
			return 0;
		}

		return (this->*selected->handler)(f, f->move, f->anim);
	}

	warning("BattlePlugin::inputMatch(): network games are not supported");
	return 0;
}

uint16 BattlePlugin::setQueueGate(uint16 player, uint16 gate, uint16 run) {
	Fighter *f = &_ctx->fighters[(byte)player];
	byte queued = f->queued;

	f->queueGate = gate;
	f->queued = 0;

	if (gate != 0 || queued == 0 || run == 0)
		return 0;

	MoveEntry *entry = &f->moveTab[f->set][queued];
	if (entry->available == 0)
		return 0;

	uint16 frameIds[4] = { 0, 0, 0, 0 };
	_host->getSpriteFrameIds(f->sprite, frameIds);

	if (_ctx->networkGame == 0) {
		f->hook = nullptr;
		f->specialTab = entry->specialList;
		f->lastFrame = frameIds[0];
		f->anim = f->animTab[f->remap[entry->animIndex]];
		f->move = queued;
		f->handlerParam = entry->handlerParam;

		if (entry->handler == nullptr) {
			warning("BattlePlugin::setQueueGate(): move %d has no handler", queued);
			return 0;
		}

		return (this->*entry->handler)(f, queued, f->anim);
	}

	warning("BattlePlugin::setQueueGate(): network games are not supported");
	return 0;
}

void BattlePlugin::setMoveHook(Fighter *f, Hook on) {
	if (_ctx->networkGame != 0 && _ctx->networkFighter != f) {
		f->hook = (on != nullptr) ? &BattlePlugin::netInputHook : nullptr;
		_ctx->moveHook = on;
		if (on != nullptr)
			return;

		_ctx->pendingMoves[f->player].active = 0;
		_host->cancelScriptTimer(f->timerId);
		_ctx->inputQueueLength = 0;
	} else {
		f->hook = on;
	}
}

void BattlePlugin::setFacing(uint16 player, uint16 facing) {
	_ctx->fighters[(byte)player].facing = (byte)facing;
}

void BattlePlugin::setInputDevice(uint16 player, uint16 type) {
	Fighter *f = &_ctx->fighters[(byte)player];

	if (_ctx->networkGame != 0 && _ctx->networkFighter == f)
		return;

	if ((byte)type > kInputDeviceEnable)
		return;

	if ((byte)type == kInputDeviceJoystick || (byte)type == kInputDeviceKeyboard) {
		if (f->inputDevice != kInputDeviceNone)
			setInputDevice((byte)player, kInputDeviceNone);

		if ((byte)type == kInputDeviceJoystick)
			joyReset((byte)player, 1);
		else
			keyboardMap((byte)player, 1);

		f->inputDevice = (byte)type;
		f->inputEnabled = 1;
		return;
	}

	if (f->inputDevice == kInputDeviceJoystick)
		joyReset((byte)player, (byte)type == kInputDeviceEnable ? 1 : 0);
	else if (f->inputDevice == kInputDeviceKeyboard)
		keyboardMap((byte)player, (byte)type == kInputDeviceEnable ? 1 : 0);

	f->inputEnabled = ((byte)type == kInputDeviceEnable) ? 1 : 0;
}

uint16 BattlePlugin::joyMessage(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) {
	uint16 index = _host->joystickMessageToIndex(message, wParam, lParamLow, lParamHigh);
	if (index == 0xFFFF)
		return 0;

	if (index >= kJoystickFlagCount) {
		warning("BattlePlugin::joyMessage(): invalid joystick event %d", index);
		return 0;
	}

	if (_ctx->joystickFlags[index] == 0)
		return 0;

	byte player = (index >= 0x11) ? 1 : 0;
	Fighter *f = &_ctx->fighters[player];

	if (message == kMessageJoystick1ButtonDown || message == kMessageJoystick2ButtonDown) {
		if (index == 0)
			warning("BattlePlugin::joyMessage(): event flag -1 does not exist");
		else
			_ctx->joystickFlags[index - 1] = 1;
	} else if (message == kMessageJoystick1ButtonUp || message == kMessageJoystick2ButtonUp) {
		if (index + 1 >= kJoystickFlagCount)
			warning("BattlePlugin::joyMessage(): event flag %d does not exist", index + 1);
		else
			_ctx->joystickFlags[index + 1] = 1;
	} else {
		_ctx->joystickFlags[f->joyLast] = 1;
		f->joyLast = (byte)index;
	}

	_ctx->joystickFlags[index] = 0;

	byte code;
	if (player == 0)
		code = _joystickCodes[index];
	else
		code = _joystickCodes[index - 0x11];

	inputEvent(code, player);
	return 1;
}

void BattlePlugin::joyReset(uint16 player, uint16 value) {
	if ((byte)player >= kFighterCount) {
		warning("BattlePlugin::joyReset(): invalid fighter %d", (byte)player);
		return;
	}

	for (int i = 0; i < 0x11; i++)
		_ctx->joystickFlags[(byte)player * 0x11 + i] = (byte)value;
}

uint16 BattlePlugin::keyDown(uint16 virtualKey) {
	if (virtualKey >= kKeyTableSize)
		return 0;

	KeyEntry *entry = &_ctx->keyTable[virtualKey];
	if (entry->armed == 0)
		return 0;

	byte player = entry->player;
	Fighter *f = &_ctx->fighters[player];
	entry->armed = 0;
	byte code = entry->code;

	if (code == kInputUp || code == kInputDown || code == kInputRight || code == kInputLeft) {
		f->keyDirs |= code >> 4;
		if (f->keyTimerArmed != 0) {
			_host->cancelScriptTimer(f->keyTimerId);
			f->keyTimerArmed = 0;
		}

		code = _directionCombine[f->keyDirs];
		if (code == kDirNoCode)
			return 1;
	}

	inputEvent(code, player);
	return 1;
}

uint16 BattlePlugin::keyUp(uint16 virtualKey) {
	if (virtualKey >= kKeyTableSize)
		return 0;

	KeyEntry *entry = &_ctx->keyTable[virtualKey];
	if (entry->code == 0)
		return 0;

	byte player = entry->player;
	Fighter *f = &_ctx->fighters[player];
	if (f->inputDevice != kInputDeviceKeyboard)
		return 0;

	entry->armed = 1;
	byte code = entry->code;

	if (code == kInputUp || code == kInputDown || code == kInputRight || code == kInputLeft) {
		f->keyDirs &= (byte)~(code >> 4);
		if (f->keyDirs != 0) {
			_host->setScriptTimer(f->keyTimerId, 200, f->keyTimerScript);
			f->keyTimerArmed = 1;
			return 1;
		}

		if (f->keyTimerArmed != 0) {
			_host->cancelScriptTimer(f->keyTimerId);
			f->keyTimerArmed = 0;
		}

		code = _directionCombine[f->keyDirs];
		if (code == kDirNoCode)
			return 1;
	} else {
		code = ~code;
	}

	inputEvent(code, player);
	return 1;
}

void BattlePlugin::keyDirTimeout(uint16 player) {
	if ((byte)player >= kFighterCount) {
		warning("BattlePlugin::keyDirTimeout(): invalid fighter %d", (byte)player);
		return;
	}

	Fighter *f = &_ctx->fighters[(byte)player];
	f->keyTimerArmed = 0;

	byte code = _directionCombine[f->keyDirs];
	if (code != kDirNoCode)
		inputEvent(code, (byte)player);
}

void BattlePlugin::keyboardMap(uint16 player, uint16 enable) {
	if ((byte)player >= kFighterCount) {
		warning("BattlePlugin::keyboardMap(): invalid fighter %d", (byte)player);
		return;
	}

	Fighter *f = &_ctx->fighters[(byte)player];

	for (int i = 0; i < 256; i++) {
		if (_ctx->keyTable[i].player == (byte)player) {
			_ctx->keyTable[i].armed = 0;
			_ctx->keyTable[i].code = 0;
		}
	}

	const byte *binding = &_keyBindings[(byte)player * 12];
	for (int i = 0; i < 6; i++) {
		KeyEntry *entry = &_ctx->keyTable[binding[0]];
		entry->player = (byte)player;
		entry->armed = (byte)enable;
		entry->code = binding[1];
		binding += 2;
	}

	f->keyTimerArmed = 0;
	f->keyTimerId = ((byte)player == 0) ? kTimerKey0 : kTimerKey1;
	f->keyTimerScript = ((byte)player == 0) ? kScriptKeyTimer0 : kScriptKeyTimer1;

	if ((byte)enable == 0)
		_host->cancelScriptTimer(f->keyTimerId);
}

void BattlePlugin::keyboardSetKey(uint16 player, uint16 function, uint16 virtualKey) {
	if ((byte)player >= kFighterCount) {
		warning("BattlePlugin::keyboardSetKey(): invalid fighter %d", (byte)player);
		return;
	}

	byte *binding = &_keyBindings[(byte)player * 12];

	switch ((byte)function) {
	case 1:
		binding[8] = (byte)virtualKey;
		break;
	case 2:
		binding[10] = (byte)virtualKey;
		break;
	case 3:
		binding[0] = (byte)virtualKey;
		break;
	case 5:
		binding[2] = (byte)virtualKey;
		break;
	case 6:
		binding[6] = (byte)virtualKey;
		break;
	case 7:
		binding[4] = (byte)virtualKey;
		break;
	default:
		break;
	}
}

} // End of namespace Topgun
