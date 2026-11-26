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

#include "topgun/games/battledemo/battledemo.h"

namespace Topgun {

BattleDemoPlugin::BattleDemoPlugin(PluginHost *host) : BattlePlugin(host) {
	_scriptContact = TG_BATTLE_V1_2_2_1_HOOK_CONTACT_FLIP;
	_scriptSetSpriteCel = TG_BATTLE_V1_2_2_1_SCRIPT_SET_SPRITE_CEL;
	_scriptWeaponSelect = TG_BATTLE_V1_2_2_1_SCRIPT_WEAPON_SELECT;
	_varWeapon = TG_BATTLE_V1_2_2_1_VAR_WEAPON;
	_varBusyCounter = TG_BATTLE_V1_2_2_1_VAR_BUSY_COUNTER;
	_collapseSpriteIndex = 3;
	_shadowSprites[0] = 0x8859;
	_shadowSprites[1] = 0x8861;
	_shadowAnimations[0] = 0x885A;
	_shadowAnimations[1] = 0x8862;
	_hitSparkSprite = 0x886A;

	initDemoTablesType0();
	initDemoTablesType1();
	initDemoTablesShared();

	_host->setScriptHook(this);
	_host->setMessageFilter(this);
}

void BattleDemoPlugin::free() {
	_host->setScriptHook(nullptr);
	_host->setMessageFilter(nullptr);

	freeCtx(_ctx);
	_ctx = nullptr;
}

int BattleDemoPlugin::findFunction(const Common::String &name) const {
	return -1;
}

uint16 BattleDemoPlugin::callFunction(int function, const uint16 *arguments, uint16 count) {
	warning("BattleDemoPlugin::callFunction(): invalid function %d", function);
	return 0;
}

uint16 BattleDemoPlugin::messageFilter(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) {
	if (!_ctx)
		return 0;

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
		joyMessage(message, wParam, lParamLow, lParamHigh);
		return 1;
	case TG_BATTLE_V1_2_2_1_MESSAGE_COMMAND:
		if (wParam != 0 || lParamHigh != 0)
			return 0;
		// fall through
	case TG_BATTLE_V1_2_2_1_MESSAGE_CLOSE:
	case TG_BATTLE_V1_2_2_1_MESSAGE_GOTO_SCENE:
	case TG_BATTLE_V1_2_2_1_MESSAGE_5F4:
		_host->setMessageFilter(nullptr);
		_host->repostMessage(message, wParam, lParamLow, lParamHigh);
		return 1;
	default:
		return 0;
	}
}

bool BattleDemoPlugin::scriptHook(uint16 script, const uint16 *arguments) {
	if (!_ctx)
		return false;

	switch (script) {
	case TG_BATTLE_V1_2_2_1_HOOK_MOVE_DONE:
		moveDone(arguments[0]);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_MOVES_REPOSITION:
		movesReposition(arguments[0], arguments[-1]);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_HIT_ATTACK:
		hitAttack(arguments[0]);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_MOVES_DIE:
		movesDie(arguments[-1], arguments[0]);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_HIT_BUTTON:
		hitButton(arguments[0]);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_STUB:
		hitStub(arguments[0]);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_CONTACT_FLIP:
		movesContactFlip();
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_WEAPONS_FIRE:
		weaponsFire(arguments[0], (int16)arguments[-1], (int16)arguments[-2]);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_WEAPONS_HIT:
		weaponsHit(arguments[0], arguments[-1]);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_WEAPONS_SPECIAL_HIT:
		weaponsSpecialHit(arguments[0], arguments[-1]);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_REGISTER_BUTTON:
		sceneRegisterButton(arguments[0], arguments[-1], arguments[-2]);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_HIT_FIGHTER:
		hitFighter(arguments[-2], arguments[-1], arguments[0]);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_INIT:
		beastInit();
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_KEY_TIMEOUT0:
		kbdTimeout(0);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_KEY_TIMEOUT1:
		kbdTimeout(1);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_INPUT_TIMEOUT0:
		inputTimeout(0);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_INPUT_TIMEOUT1:
		inputTimeout(1);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_DELAY_TIMEOUT0:
		delayTimeout(0);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_DELAY_TIMEOUT1:
		delayTimeout(1);
		break;
	case TG_BATTLE_V1_2_2_1_HOOK_SET_BOUNDS:
		if (arguments[0] < kBoundsCount)
			sceneSetBounds(arguments[0], arguments[-1], arguments[-2], arguments[-3], arguments[-4], arguments[-5]);
		break;
	default:
		return false;
	}

	return true;
}

void BattleDemoPlugin::beastInit() {
	uint16 *host = _host->variableTable();

	// Fighters share the move and animation tables of their type, so changes persist until the library is freed
	for (int i = 0; i < kFighterCount; i++) {
		Fighter *old = &_ctx->fighters[i];
		if (!old->animTab || old->type >= TG_BATTLE_V1_2_2_1_FIGHTER_TYPE_COUNT)
			continue;

		FighterTypeData *data = &_typeData[old->type];
		for (int j = 0; j < kMoveCount; j++) {
			data->moves[0][j] = old->moveTab[0][j];
			data->moves[1][j] = old->moveTab[1][j];
		}
		for (int j = 0; j < kRemapSize; j++)
			data->remap[j] = old->remap[j];
	}

	_ctx->fightOver = 0;
	_ctx->largeValue1003 = (host[TG_BATTLE_V1_2_2_1_VAR_1003] >= TG_BATTLE_V1_2_2_1_WIDE_VALUE) ? 1 : 0;

	initFightersFromHost();
	initInputTimers();

	_ctx->lastScorer = g_system->getMillis() & 1;
	_ctx->aiEngaged = 0;

	Fighter *f = &_ctx->fighters[0];
	Fighter *g = &_ctx->fighters[1];

	if (f->type >= TG_BATTLE_V1_2_2_1_FIGHTER_TYPE_COUNT || g->type >= TG_BATTLE_V1_2_2_1_FIGHTER_TYPE_COUNT) {
		warning("BattleDemoPlugin::beastInit(): invalid fighter types %d and %d", f->type, g->type);
		return;
	}

	f->timerId = TG_BATTLE_V1_2_2_1_TIMER_FIGHTER0;
	f->timerScript = TG_BATTLE_V1_2_2_1_HOOK_DELAY_TIMEOUT0;

	initFighter(f, 0, f->type);
	_host->loadResource(f->spriteTab[0]);
	weaponInit(f);
	loadStats(f);
	f->level = kLevelNone;

	f->opponent = g;
	g->opponent = f;

	g->timerId = TG_BATTLE_V1_2_2_1_TIMER_FIGHTER1;
	g->timerScript = TG_BATTLE_V1_2_2_1_HOOK_DELAY_TIMEOUT1;

	initFighter(g, 1, g->type);
	_host->loadResource(g->spriteTab[0]);
	weaponInit(g);
	loadStats(g);
	g->level = kLevelNone;
}

void BattleDemoPlugin::emptyFunction() {
}

void BattleDemoPlugin::initFightersFromHost() {
	uint16 *host = _host->variableTable();

	for (uint16 i = 0; i < kFighterCount; i++) {
		Fighter *f = &_ctx->fighters[i];

		f->ai = (host[TG_BATTLE_V1_2_2_1_VAR_HUMAN_PLAYERS] > i) ? 0 : 1;
		f->type = host[TG_BATTLE_V1_2_2_1_VAR_FIGHTER_TYPE + i];
		f->inputDevice = (byte)host[TG_BATTLE_V1_2_2_1_VAR_INPUT_TYPE + i];
		f->weapon = host[TG_BATTLE_V1_2_2_1_VAR_WEAPON + i];
		f->autoTurn = (byte)host[TG_BATTLE_V1_2_2_1_VAR_AUTO_TURN + i];
	}

	_ctx->effectsOff = (byte)host[TG_BATTLE_V1_2_2_1_VAR_EFFECTS_OFF];
	host[TG_BATTLE_V1_2_2_1_VAR_BUSY_COUNTER] = 0;
}

void BattleDemoPlugin::initFighter(Fighter *f, uint16 player, uint16 type) {
	const FighterInitParams *params = &_fighterInitParams[type];
	FighterTypeData *data = &_typeData[type];

	f->sprite = params->sprite;
	f->spriteId = params->spriteId;

	f->player = (byte)player;
	f->set = 0;
	f->posture = 0;
	f->state = kStateNormal;
	f->spriteTab = data->sprites;
	f->rectTab = data->rects;
	f->xTab = data->offsetsX;
	f->level = 0;
	f->move = kMoveSlotIdle;
	f->queued = 0;
	f->z = 0x64;
	f->facing = params->facing;

	if (type == kFighterSparky) {
		f->inputTab[0] = _inputRecords[0];
		f->inputTab[1] = _inputRecords[1];
	} else {
		f->inputTab[0] = _inputRecords[0];
		f->inputTab[1] = _verminBeastInputRecords;
	}

	for (int i = 0; i < kMoveCount; i++) {
		f->moveTab[0][i] = data->moves[0][i];
		f->moveTab[1][i] = data->moves[1][i];
	}

	f->hitTab[0] = data->hits[0];
	f->hitTab[1] = data->hits[1];
	f->reactTab[0] = _reactionTables[0];
	f->reactTab[1] = _reactionTables[1];
	f->reactTab[2] = _reactionTables[2];
	f->animTab = data->animations;

	for (int i = 0; i < kRemapSize; i++)
		f->remap[i] = data->remap[i];

	f->doneCallback = static_cast<Callback>(&BattleDemoPlugin::movesEnter);
	f->timerCallback = static_cast<Callback>(&BattleDemoPlugin::movesEnter);
	f->varList = data->varList;
	f->comboTab = data->combos;
	f->yTab = data->offsetsY;
	f->damageMultiplier = 1;
	f->walkTab = data->walkSteps;
	f->knockTab = data->knockDistances;
	f->aiBlock.list = data->aiList;

	if (f->ai != 0)
		aiSetupSkill(f);

	_host->callScript(params->script, 0, 0);
}

} // End of namespace Topgun
