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

#include "common/system.h"
#include "common/textconsole.h"
#include "engines/engine.h"

#include "topgun/games/battle/battle.h"

namespace Topgun {

enum {
	kFunctionKbdSetKey,
	kFunctionDelayTimeout,
	kFunctionKbdTimeout,
	kFunctionInputTimeout,
	kFunctionBeastInit,
	kFunctionHitAttack,
	kFunctionHitButton,
	kFunctionHitFighter,
	kFunctionMoveDone,
	kFunctionMovesContactFlip,
	kFunctionMovesReposition,
	kFunctionMovesDie,
	kFunctionMovesDieFinal,
	kFunctionSceneRegisterButton,
	kFunctionSceneSetBounds,
	kFunctionWeaponsFire,
	kFunctionWeaponsHit,
	kFunctionWeaponsSpecialHit,
	kFunctionBeastEnable,
	kFunctionSceneSetDoor,
	kFunctionMovesEnter,
	kFunctionBeastFinalFight,
	kFunctionBeastSaveRestoreDlg,
	kFunctionBeastSaveRestore,
	kFunctionBeastNetwork,
	kFunctionCount
};

BattlePlugin::BattlePlugin(PluginHost *host) : _host(host), _ctx(nullptr), _initFlags(0) {
	_ctx = allocCtx();

	initTablesType0();
	initTablesType1();
	initTablesType2();
	initTablesType3();
	initTablesType4();
	initTablesType5();
	initTablesType6();
	initTablesSpecial();
	initTablesShared();
}

BattlePlugin::~BattlePlugin() {
	freeCtx(_ctx);
}

void BattlePlugin::free() {
	beastNetwork(1, 0, 0);

	_ctx->enabled = 0;
	_host->setMessageFilter(nullptr);

	freeCtx(_ctx);
	_ctx = nullptr;
}

int BattlePlugin::findFunction(const Common::String &name) const {
	if (name.equalsIgnoreCase("KBD_SETKEY") || name.equalsIgnoreCase("KEYBOARD_SETKEY"))
		return kFunctionKbdSetKey;
	if (name.equalsIgnoreCase("DELAY_TIMEOUT"))
		return kFunctionDelayTimeout;
	if (name.equalsIgnoreCase("KBD_TIMEOUT"))
		return kFunctionKbdTimeout;
	if (name.equalsIgnoreCase("INPUT_TIMEOUT"))
		return kFunctionInputTimeout;
	if (name.equalsIgnoreCase("BEAST_INIT"))
		return kFunctionBeastInit;
	if (name.equalsIgnoreCase("HIT_ATTACK"))
		return kFunctionHitAttack;
	if (name.equalsIgnoreCase("HIT_BUTTON"))
		return kFunctionHitButton;
	if (name.equalsIgnoreCase("HIT_FIGHTER"))
		return kFunctionHitFighter;
	if (name.equalsIgnoreCase("MOVE_DONE"))
		return kFunctionMoveDone;
	if (name.equalsIgnoreCase("MOVES_CONTACTFLIP"))
		return kFunctionMovesContactFlip;
	if (name.equalsIgnoreCase("MOVES_REPOSITION"))
		return kFunctionMovesReposition;
	if (name.equalsIgnoreCase("MOVES_DIE"))
		return kFunctionMovesDie;
	if (name.equalsIgnoreCase("MOVES_DIE_FINAL"))
		return kFunctionMovesDieFinal;
	if (name.equalsIgnoreCase("SCENE_REGISTER_BUTTON"))
		return kFunctionSceneRegisterButton;
	if (name.equalsIgnoreCase("SCENE_SETBOUNDS"))
		return kFunctionSceneSetBounds;
	if (name.equalsIgnoreCase("WEAPONS_FIRE"))
		return kFunctionWeaponsFire;
	if (name.equalsIgnoreCase("WEAPONS_HIT"))
		return kFunctionWeaponsHit;
	if (name.equalsIgnoreCase("WEAPONS_SPECIALHIT"))
		return kFunctionWeaponsSpecialHit;
	if (name.equalsIgnoreCase("BEAST_ENABLE"))
		return kFunctionBeastEnable;
	if (name.equalsIgnoreCase("SCENE_SETDOOR"))
		return kFunctionSceneSetDoor;
	if (name.equalsIgnoreCase("MOVES_ENTER"))
		return kFunctionMovesEnter;
	if (name.equalsIgnoreCase("BEAST_FINALFIGHT"))
		return kFunctionBeastFinalFight;
	if (name.equalsIgnoreCase("BEAST_SAVERESTOREDLG"))
		return kFunctionBeastSaveRestoreDlg;
	if (name.equalsIgnoreCase("BEAST_SAVERESTORE"))
		return kFunctionBeastSaveRestore;
	if (name.equalsIgnoreCase("BEAST_NETWORK"))
		return kFunctionBeastNetwork;

	return -1;
}

uint16 BattlePlugin::callFunction(int function, const uint16 *arguments, uint16 count) {
	if (function < 0 || function >= kFunctionCount) {
		warning("BattlePlugin::callFunction(): invalid function %d", function);
		return 0;
	}

	uint16 expected = 0;
	switch (function) {
	case kFunctionBeastInit:
	case kFunctionMovesContactFlip:
	case kFunctionBeastFinalFight:
		expected = 0;
		break;
	case kFunctionDelayTimeout:
	case kFunctionKbdTimeout:
	case kFunctionInputTimeout:
	case kFunctionHitAttack:
	case kFunctionHitButton:
	case kFunctionMoveDone:
	case kFunctionMovesDieFinal:
	case kFunctionBeastEnable:
	case kFunctionMovesEnter:
	case kFunctionBeastSaveRestoreDlg:
		expected = 1;
		break;
	case kFunctionMovesReposition:
	case kFunctionMovesDie:
	case kFunctionWeaponsHit:
	case kFunctionWeaponsSpecialHit:
	case kFunctionBeastSaveRestore:
		expected = 2;
		break;
	case kFunctionKbdSetKey:
	case kFunctionHitFighter:
	case kFunctionSceneRegisterButton:
	case kFunctionWeaponsFire:
	case kFunctionBeastNetwork:
		expected = 3;
		break;
	case kFunctionSceneSetBounds:
	case kFunctionSceneSetDoor:
		expected = 6;
		break;
	}

	if (count < expected)
		warning("BattlePlugin::callFunction(): function %d expects %d arguments, got %d", function, expected, count);

	uint16 values[6];
	for (int i = 0; i < 6; i++)
		values[i] = (i < count) ? arguments[i] : 0;

	switch (function) {
	case kFunctionKbdSetKey:
		keyboardSetKey(values[0], values[1], values[2]);
		return 0;
	case kFunctionDelayTimeout:
		delayTimeout(values[0]);
		return 0;
	case kFunctionKbdTimeout:
		kbdTimeout((byte)values[0]);
		return 0;
	case kFunctionInputTimeout:
		inputTimeout((byte)values[0]);
		return 0;
	case kFunctionBeastInit:
		beastInit();
		return 0;
	case kFunctionHitAttack:
		return hitAttack(values[0]);
	case kFunctionHitButton:
		return hitButton(values[0]);
	case kFunctionHitFighter:
		return hitFighter(values[2], values[1], values[0]);
	case kFunctionMoveDone:
		moveDone(values[0]);
		return 0;
	case kFunctionMovesContactFlip:
		movesContactFlip();
		return 0;
	case kFunctionMovesReposition:
		movesReposition(values[0], values[1]);
		return 0;
	case kFunctionMovesDie:
		movesDie(values[1], values[0]);
		return 0;
	case kFunctionMovesDieFinal:
		movesDieFinal(values[0]);
		return 0;
	case kFunctionSceneRegisterButton:
		sceneRegisterButton(values[0], values[1], values[2]);
		return 0;
	case kFunctionSceneSetBounds:
		sceneSetBounds(values[0], values[1], values[2], values[3], values[4], values[5]);
		return 0;
	case kFunctionWeaponsFire:
		weaponsFire(values[0], (int16)values[1], (int16)values[2]);
		return 0;
	case kFunctionWeaponsHit:
		return weaponsHit(values[0], values[1]);
	case kFunctionWeaponsSpecialHit:
		return weaponsSpecialHit(values[0], values[1]);
	case kFunctionBeastEnable:
		beastEnable(values[0]);
		return 0;
	case kFunctionSceneSetDoor:
		sceneSetDoor(values[0], values[1], values[2], values[3], values[4], values[5]);
		return 0;
	case kFunctionMovesEnter:
		movesEnter(values[0]);
		return 0;
	case kFunctionBeastFinalFight:
		beastFinalFight();
		return 0;
	case kFunctionBeastSaveRestoreDlg:
		beastSaveRestoreDlg(values[0]);
		return 0;
	case kFunctionBeastSaveRestore:
		beastSaveRestore(values[1], values[0]);
		return 0;
	case kFunctionBeastNetwork:
		return beastNetwork(values[0], values[1], values[2]);
	}

	return 0;
}

uint16 BattlePlugin::messageFilter(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) {
	switch (message) {
	case kMessageKeyDown:
		return keyDown(wParam);
	case kMessageKeyUp:
		return keyUp(wParam);
	case kMessageJoystick1Move:
	case kMessageJoystick2Move:
	case kMessageJoystick1ButtonDown:
	case kMessageJoystick2ButtonDown:
	case kMessageJoystick1ButtonUp:
	case kMessageJoystick2ButtonUp:
		return joyMessage(message, wParam, lParamLow, lParamHigh);
	default:
		return 0;
	}
}

uint32 BattlePlugin::windowMessage(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) {
	return 0;
}

void BattlePlugin::beastInit() {
}

void BattlePlugin::beastEnable(uint16 enable) {
	if (enable == 0) {
		_ctx->enabled = 0;
		_host->setMessageFilter(nullptr);
		return;
	}

	uint16 *host = _host->variableTable();

	_ctx->fighters[0] = Fighter();
	_ctx->fighters[1] = Fighter();
	_ctx->buttonCount = 0;
	_ctx->doorX = 0;
	_ctx->doorY = 0;
	_ctx->doorRect = Common::Rect();
	_ctx->phase = kPhaseNone;
	_ctx->enteredCount = 0;
	_ctx->networkGame = (byte)host[kVarNetworkGame];

	if (_ctx->networkGame != 0 && !_networkMessageShown) {
		_networkMessageShown = true;
		GUIErrorMessage("Network play is not supported.");
	}

	initInputTimers();
	initFightersFromHost();

	_ctx->fightOver = 0;
	_ctx->lastScorer = g_system->getMillis() & 1;
	_ctx->aiEngaged = 0;

	_host->setMessageFilter(this);
	_ctx->enabled = 1;

	Fighter *f = &_ctx->fighters[0];
	f->timerId = kTimerFighter0;
	f->timerScript = kScriptFighterTimer0;

	if (f->type < kFighterTypeCount)
		initFighter(f, 0, f->type);
	else
		warning("BattlePlugin::beastEnable(): invalid fighter type %d", f->type);

	weaponInit(f);
	loadStats(f);
	f->level = kLevelNone;

	if (!(host[kVarPlayerOptions] & 1))
		f->moveTab[1][19].available = 0;

	if (f->ai == 0)
		f->moveTab[1][kMoveSlotProjectile].available = 1;
	else if (!(host[kVarPlayerOptions] & 2) || host[kVarAmmoCounter] == 0)
		f->moveTab[1][kMoveSlotProjectile].available = 0;

	if (host[kVarOptions1] & 0x10)
		f->flags |= kFlagInfiniteAmmo;

	if (host[kVarOptions1] & 0x40)
		f->flags |= kFlagNoKnockOut;

	if (host[kVarOptions1] & 0x2000)
		f->flags |= kFlagSidestep;

	Fighter *g = &_ctx->fighters[1];
	f->opponent = g;
	g->opponent = f;

	g->timerId = kTimerFighter1;
	g->timerScript = kScriptFighterTimer1;

	if (g->type < kFighterTypeCount)
		initFighter(g, 1, g->type);
	else
		warning("BattlePlugin::beastEnable(): invalid fighter type %d", g->type);

	weaponInit(g);
	loadStats(g);
	g->level = kLevelNone;

	if (!(host[kVarPlayerOptions + 1] & 1))
		g->moveTab[1][19].available = 0;

	if (g->ai == 0)
		g->moveTab[1][kMoveSlotProjectile].available = 1;
	else if (!(host[kVarPlayerOptions + 1] & 2) || host[kVarAmmoCounter + 1] == 0)
		g->moveTab[1][kMoveSlotProjectile].available = 0;

	if (host[kVarOptions1] & 0x20)
		g->flags |= kFlagInfiniteAmmo;

	if (host[kVarOptions1] & 0x40)
		g->flags |= kFlagNoKnockOut;

	if (host[kVarOptions1] & 0x2000)
		g->flags |= kFlagSidestep;
}

void BattlePlugin::beastFinalFight() {
	uint16 *host = _host->variableTable();

	beastEnable(1);
	_ctx->phase = kPhaseNone;

	fightSetup(0, host[kVarFighterType] == kFighterEvilToadMan ? 1 : 0);
	fightSetup(1, host[kVarFighterType + 1] == kFighterEvilToadMan ? 1 : 0);
}

void BattlePlugin::beastSaveRestoreDlg(uint16 mode) {
	if (_ctx->networkGame != 0)
		beastNetwork(0x19, mode == 0 ? 1 : 0, 1);

	if (_ctx->enabled != 0) {
		if (mode != 0)
			_host->setMessageFilter(nullptr);
		else
			_host->setMessageFilter(this);
	}
}

uint16 BattlePlugin::beastNetwork(uint16 op, uint16 a, uint16 b) {
	if (op == 1) {
		_ctx->networkGame = 0;
		_host->setNetworkMessageCallback(nullptr);
	} else if (!_networkMessageShown) {
		_networkMessageShown = true;
		GUIErrorMessage("Network play is not supported.");
	}

	_host->setScriptResult(0);
	return 0;
}

BattleContext *BattlePlugin::allocCtx() {
	return new BattleContext();
}

uint16 BattlePlugin::freeCtx(BattleContext *context) {
	delete context;
	return 0;
}

void BattlePlugin::delayTimeout(uint16 index) {
	if (index >= kFighterCount) {
		warning("BattlePlugin::delayTimeout(): invalid fighter %d", index);
		return;
	}

	Fighter *f = &_ctx->fighters[index];
	if (f->timerCallback == nullptr) {
		warning("BattlePlugin::delayTimeout(): fighter %d has no timer callback", index);
		return;
	}

	(this->*f->timerCallback)(f);
}

void BattlePlugin::kbdTimeout(byte player) {
	keyDirTimeout(player);
}

void BattlePlugin::inputTimeout(byte player) {
	inputMatch(player, 1);
}

void BattlePlugin::moveDone(uint16 id) {
	Fighter *f = &_ctx->fighters[0];
	if (f->spriteId != id)
		f++;

	if (f->doneCallback == nullptr) {
		warning("BattlePlugin::moveDone(): fighter %d has no done callback", f->player);
		return;
	}

	(this->*f->doneCallback)(f);
}

uint16 BattlePlugin::libMain(uint16 instance, uint16 dataSegment, uint16 heapSize, uint16 commandLineSegment, uint16 commandLineOffset) {
	return 1;
}

uint16 BattlePlugin::wep(uint16 exitType) {
	return 1;
}

uint16 BattlePlugin::identity(uint16 ignored, uint16 value) {
	return (byte)value;
}

} // End of namespace Topgun
