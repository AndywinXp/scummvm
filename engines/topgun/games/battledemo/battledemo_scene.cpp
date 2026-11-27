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

void BattleDemoPlugin::movesReposition(uint16 id, uint16 mode) {
	Fighter *f = &_ctx->fighters[0];
	if (f->spriteId != id)
		f = &_ctx->fighters[1];

	byte previousSide = f->side;

	if (f->anim == f->lastAnim) {
		Common::Point position;
		_host->getSpritePosition(f->sprite, position);
		f->x = position.x;
		f->lastAnim = 0;
	} else {
		Common::Rect spriteRect;
		_host->getSpriteRect(f->sprite, spriteRect);
		f->x = (int16)(spriteRect.right + spriteRect.left) / 2;
	}

	if (mode != 0) {
		int16 levelY = 0;
		if (f->level < kBoundsCount)
			levelY = _ctx->bounds[f->level].y;
		else
			warning("BattleDemoPlugin::movesReposition(): invalid level %d", f->level);

		f->y = (int16)(f->y + levelY);
		f->level = (f->level == 0) ? 1 : 0;
		f->z = (uint16)(f->z + ((f->level == 0) ? 10 : -10));
		_host->setSpriteLevel(f->sprite, f->z);

		Fighter *opponent = f->opponent;
		_ctx->aiEngaged = (opponent->level == f->level && opponent->lane == f->lane) ? 1 : 0;
	}

	separateFighters(f);
	updateDistance(f);
	updateWallFlags(f);

	if (mode == 0 && f->side != previousSide) {
		// 0x31 marks a side change in the combo history, which stays open for 500 ms
		f->combo = (f->combo << 8) | 0x31;
		f->comboEnd = g_system->getMillis() + 500;
	}

	if (f->level == 1 && f->x < _ctx->bounds[1].left)
		startKnockdown(f);
	else if (!f->doneCallback)
		warning("BattleDemoPlugin::movesReposition(): fighter %d has no done callback", f->player);
	else
		(this->*f->doneCallback)(f);

	aiEvent(f->opponent, kAiEventOpponentRepositioned);
}

void BattleDemoPlugin::storePlayer(Fighter *f) {
	uint16 *host = _host->variableTable();
	host[TG_BATTLE_V1_2_3_1_VAR_PLAYER] = f->player;
}

void BattleDemoPlugin::cbEmpty(Fighter *f) {
}

void BattleDemoPlugin::movesEnter(Fighter *f) {
	uint16 *host = _host->variableTable();

	f->doneCallback = &BattlePlugin::cbEnterAnimDone;
	host[f->varList[0]] = (uint16)(f->player == 0 ? -40 : 40);
	host[f->varList[1]] = 0;

	if ((f->player == 0 && f->facing == 1) || (f->player == 1 && f->facing == 0)) {
		f->facing = (f->facing + 1) % 2;
		host[f->varList[7]] = (host[f->varList[7]] == 0) ? 1 : 0;
		host[f->varList[8]] = (host[f->varList[8]] == 0) ? 1 : 0;
	}

	if (_host->isSpriteVisible(0x884A))
		_host->transferSprite(0x884A, 0x884C, 0x884D, 0, 0);

	playAnim(f, f->animTab[f->remap[0]]);
	initAiFlags(f);

	_ctx->phase++;
	if (_ctx->phase == kPhaseFight) {
		_host->callScript(TG_BATTLE_V1_2_3_1_SCRIPT_BOTH_ENTERED, (uint16)(0x89F3 + TG_BATTLE_V1_2_3_1_LITERAL_BIAS), 0);
		_host->unloadResource(0x8A12);
	}
}

void BattleDemoPlugin::enterFinished(Fighter *f) {
	Fighter *o = f->opponent;

	f->level = 0;
	_ctx->aiEngaged = (o->level == 0 && o->lane == f->lane) ? 1 : 0;
	f->doneCallback = &BattlePlugin::endMove;

	if (f->ai == 0)
		setInputDevice(f->player, identity(f->player, f->inputDevice));

	switchSprite(f, 1, f->animTab[f->remap[1]], 1);
	movesReposition(f->spriteId, 0);
	spawnShadow(f);

	if (_ctx->phase == kPhaseFight) {
		_host->unloadResource(0x89D2);
		_host->callScript(TG_BATTLE_V1_2_3_1_SCRIPT_FIGHT_START, 2, 0);
		_host->callScript(TG_BATTLE_V1_2_3_1_SCRIPT_FIGHT_BEGIN, 0, 0);
	}
}

void BattleDemoPlugin::movesDie(uint16 flag, uint16 who) {
	_ctx->fightOver = 1;

	if (who == 2)
		who = _ctx->lastScorer;

	if (who >= kFighterCount) {
		warning("BattleDemoPlugin::movesDie(): invalid fighter %d", who);
		return;
	}

	uint16 *host = _host->variableTable();
	Fighter *f = &_ctx->fighters[who];
	Fighter *o = f->opponent;

	host[TG_BATTLE_V1_2_3_1_VAR_WINNER] = f->player;

	setInputDevice(0, kInputDeviceDisable);
	setInputDevice(1, kInputDeviceDisable);

	aiEvent(f, kAiEventFightWon);
	aiEvent(o, kAiEventFightLost);

	if (f->state == kStateGuard)
		f->move = kMoveSlotIdle;
	if (o->state == kStateGuard)
		o->move = kMoveSlotIdle;

	resetFighterState(f);
	resetFighterState(o);

	setQueueGate(f->player, 1, 0);
	setQueueGate(o->player, 1, 0);

	setInFight(f, 0);
	setInFight(o, 0);

	setContact(f, 0);
	setContact(o, 0);

	f->autoTurn = 0;
	o->autoTurn = 0;

	o->state = kStateFinal;
	f->state = kStateWinner;

	o->doneCallback = &BattlePlugin::fightOver;
	f->timerCallback = &BattlePlugin::winnerPose;

	_host->setScriptTimer(f->timerId, 2000, f->timerScript);

	if (flag != 0 && (o->move == kMoveSlotIdle || o->move == kMoveSlotCrouch)) {
		fightOver(o);
		return;
	}

	o->doneCallback = &BattlePlugin::fightOver;
}

void BattleDemoPlugin::fightOver(Fighter *f) {
	uint16 *host = _host->variableTable();

	_host->callScript(TG_BATTLE_V1_2_3_1_SCRIPT_FIGHT_OVER, 2, 0);

	host[TG_BATTLE_V1_2_3_1_VAR_LOSER_ARGUMENT_7] = host[TG_BATTLE_V1_2_3_1_VAR_ARGUMENT_7];
	host[TG_BATTLE_V1_2_3_1_VAR_LOSER_ARGUMENT_8] = host[TG_BATTLE_V1_2_3_1_VAR_ARGUMENT_8];

	host[TG_BATTLE_V1_2_3_1_VAR_BUSY_COUNTER]++;

	_host->unloadResource(0x8A0F);
	_host->unloadResource(0x89F4);
	_host->unloadResource(0x89FC);

	_host->callScript(TG_BATTLE_V1_2_3_1_SCRIPT_FIGHT_OVER_SPRITE, 0x8A0C, 0);
	_host->callScript(TG_BATTLE_V1_2_3_1_SCRIPT_FIGHT_OVER_SPRITE, 0x89D6, 0);

	Fighter *o = f->opponent;

	host[f->varList[14]] = f->z;
	host[o->varList[14]] = o->z;

	f->state = kStateFinal;
	o->state = kStateWinner;

	storeStats(o);
	storeStats(f);

	if (o->posture == 2) {
		moveTransform(o, kMoveSlotTransform, o->animTab[o->remap[10]]);
		o->doneCallback = &BattlePlugin::winnerPose;
	} else if (o->move == kMoveSlotIdle || o->move == kMoveSlotCrouch)
		winnerPose(o);
	else
		o->doneCallback = &BattlePlugin::winnerPose;

	f->doneCallback = (f->level == 1) ? &BattlePlugin::winnerWall : &BattlePlugin::collapse;

	_host->callScript(f->set == 0 ? f->varList[10] : f->varList[11], 0, 0);
}

void BattleDemoPlugin::endFight(Fighter *f) {
	_host->cancelScriptTimer(f->timerId);
	_host->setSpriteQueue(f->sprite, 0, 0);
	_host->unloadResource(f->spriteTab[3]);

	_ctx->phase--;
	if (_ctx->phase != kPhaseNone) {
		Fighter *o = f->opponent;
		o->timerCallback = &BattlePlugin::endFight;
		_host->setScriptTimer(o->timerId, 10000, o->timerScript);
		return;
	}

	_host->unloadResource(0x8A0C);
	_host->unloadResource(0x89D6);
	_host->callScript(TG_BATTLE_V1_2_3_1_SCRIPT_FIGHT_END, 0, 0);
}

void BattleDemoPlugin::sceneRegisterButton(uint16 id, uint16 variable, uint16 remove) {
	if (remove != 0) {
		for (uint16 i = 0; i < _ctx->buttonCount; i++) {
			if (_ctx->buttons[i].spriteId == id) {
				for (uint16 j = i; j + 1 < _ctx->buttonCount; j++)
					_ctx->buttons[j] = _ctx->buttons[j + 1];

				_ctx->buttonCount--;
				return;
			}
		}

		return;
	}

	if (_ctx->buttonCount >= kButtonCount)
		return;

	ButtonRecord *button = &_ctx->buttons[_ctx->buttonCount];
	_ctx->buttonCount++;

	button->spriteId = id;
	button->script = (uint16)(variable - TG_BATTLE_V1_2_3_1_LITERAL_BIAS);

	_host->getSpriteRect((uint16)(button->spriteId - TG_BATTLE_V1_2_3_1_LITERAL_BIAS), button->rect);
}

} // End of namespace Topgun
