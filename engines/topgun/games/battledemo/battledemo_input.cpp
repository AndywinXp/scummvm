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

#include "topgun/games/battledemo/battledemo.h"

namespace Topgun {

void BattleDemoPlugin::initInputTimers() {
	Fighter *f = &_ctx->fighters[0];
	f->inputTimerId = TG_BATTLE_V1_2_2_1_TIMER_INPUT0;
	f->inputTimerScript = TG_BATTLE_V1_2_2_1_HOOK_INPUT_TIMEOUT0;
	f->inputTimeout = TG_BATTLE_V1_2_2_1_INPUT_TIMEOUT;

	f = &_ctx->fighters[1];
	f->inputTimerId = TG_BATTLE_V1_2_2_1_TIMER_INPUT1;
	f->inputTimerScript = TG_BATTLE_V1_2_2_1_HOOK_INPUT_TIMEOUT1;
	f->inputTimeout = TG_BATTLE_V1_2_2_1_INPUT_TIMEOUT;
}

uint16 BattleDemoPlugin::identity(uint16 player, uint16 device) {
	uint16 *host = _host->variableTable();

	if ((byte)device == kInputDeviceJoystick && host[TG_BATTLE_V1_2_2_1_VAR_JOYSTICKS] > (byte)player)
		return kInputDeviceJoystick;

	return kInputDeviceKeyboard;
}

uint16 BattleDemoPlugin::inputMatch(uint16 player, uint16 timeout) {
	if ((byte)player >= kFighterCount) {
		warning("BattleDemoPlugin::inputMatch(): invalid fighter %d", (byte)player);
		return 0;
	}

	Fighter *f = &_ctx->fighters[(byte)player];

	if ((byte)timeout != 0) {
		f->history = 0;
		return 0;
	}

	MoveEntry *moveTable = f->moveTab[f->set];
	uint16 history = f->history;
	bool noMatch = true;
	MoveEntry *selected = nullptr;

	uint16 frameIds[4] = { 0, 0, 0, 0 };
	_host->getSpriteFrameIds(f->sprite, frameIds);
	uint16 current = frameIds[0];
	bool changed = (f->lastFrame != current);

	if (changed && f->specialTab && f->specialTab[0].value != 0xFFFF) {
		for (const SpecialMove *special = f->specialTab; special->value != 0xFFFF; special++) {
			if ((special->mask & history) == special->value && current >= special->frameLo && current <= special->frameHi) {
				MoveEntry *entry = &moveTable[special->move];
				if (entry->available == 0)
					return 0;

				selected = entry;
				f->move = special->move;
				f->moveCel = special->cel;
				f->history = 0;
				noMatch = false;
				break;
			}
		}
	}

	if (noMatch) {
		const InputRecord *list = f->inputTab[f->set];
		if (!list) {
			warning("BattleDemoPlugin::inputMatch(): fighter %d has no input list for set %d", (byte)player, f->set);
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

					f->move = record->move;
					noMatch = false;
					break;
				}
			}
		}
	}

	if (noMatch)
		return 0;

	f->hook = nullptr;
	f->specialTab = selected->specialList;
	f->lastFrame = current;
	f->queued = 0;
	f->anim = f->animTab[f->remap[selected->animIndex]];
	f->handlerParam = selected->handlerParam;

	if (!selected->handler) {
		warning("BattleDemoPlugin::inputMatch(): move %d has no handler", f->move);
		return 0;
	}

	return (this->*selected->handler)(f, f->move, f->anim);
}

uint16 BattleDemoPlugin::keyDown(uint16 virtualKey) {
	if (virtualKey > TG_BATTLE_V1_2_2_1_KEY_TABLE_LAST)
		return 0;

	if (virtualKey == TG_BATTLE_V1_2_2_1_KEY_TABLE_LAST) {
		// In the original, this key's table entry overlaps the first joystick flags
		warning("BattleDemoPlugin::keyDown(): key 0x%02X has no key table entry", virtualKey);
		return 0;
	}

	if (g_system->getEventManager()->getModifierState() & (Common::KBD_SHIFT | Common::KBD_CTRL))
		return 0;

	return BattlePlugin::keyDown(virtualKey);
}

uint16 BattleDemoPlugin::keyUp(uint16 virtualKey) {
	if (virtualKey > TG_BATTLE_V1_2_2_1_KEY_TABLE_LAST)
		return 0;

	if (virtualKey == TG_BATTLE_V1_2_2_1_KEY_TABLE_LAST) {
		warning("BattleDemoPlugin::keyUp(): key 0x%02X has no key table entry", virtualKey);
		return 0;
	}

	KeyEntry *entry = &_ctx->keyTable[virtualKey];
	if (entry->code == 0)
		return 0;

	byte player = entry->player;
	Fighter *f = &_ctx->fighters[player];

	entry->armed = f->inputEnabled;
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

void BattleDemoPlugin::keyboardMap(uint16 player, uint16 enable) {
	if ((byte)player >= kFighterCount) {
		warning("BattleDemoPlugin::keyboardMap(): invalid fighter %d", (byte)player);
		return;
	}

	Fighter *f = &_ctx->fighters[(byte)player];

	const byte *binding = &_keyBindings[(byte)player * 12];
	for (int i = 0; i < 6; i++) {
		KeyEntry *entry = &_ctx->keyTable[binding[0]];
		entry->player = (byte)player;
		entry->armed = (byte)enable;
		entry->code = binding[1];
		binding += 2;
	}

	f->keyTimerArmed = 0;
	f->keyTimerId = ((byte)player == 0) ? TG_BATTLE_V1_2_2_1_TIMER_KEY0 : TG_BATTLE_V1_2_2_1_TIMER_KEY1;
	f->keyTimerScript = ((byte)player == 0) ? TG_BATTLE_V1_2_2_1_HOOK_KEY_TIMEOUT0 : TG_BATTLE_V1_2_2_1_HOOK_KEY_TIMEOUT1;

	if ((byte)enable == 0)
		_host->cancelScriptTimer(f->keyTimerId);
}

} // End of namespace Topgun
