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

#include "common/rect.h"
#include "common/textconsole.h"

#include "topgun/games/battle/battle.h"

namespace Topgun {

void BattlePlugin::movesEnter(uint16 index) {
	if (index >= kFighterCount) {
		warning("BattlePlugin::movesEnter(): invalid fighter %d", index);
		return;
	}

	uint16 *host = _host->variableTable();
	Fighter *f = &_ctx->fighters[index];

	f->doneCallback = &BattlePlugin::cbEnterAnimDone;
	host[f->varList[0]] = (uint16)(f->player == 0 ? -40 : 40);
	host[f->varList[1]] = 0;

	if ((f->player == 0 && f->facing == 1) || (f->player == 1 && f->facing == 0)) {
		f->facing = (f->facing + 1) % 2;
		host[f->varList[7]] = (host[f->varList[7]] == 0) ? 1 : 0;
		host[f->varList[8]] = (host[f->varList[8]] == 0) ? 1 : 0;
	}

	if (host[kVarMode] == 6)
		_host->transferSprite(0xA2A4, 0xA2A6, 0xA2A7, 0, 0);
	else if (_host->isSpriteVisible(0x9C88))
		_host->transferSprite(0x9C88, 0x9C8A, 0x9C8B, 0, 0);

	playAnim(f, f->animTab[f->remap[0]]);
	initAiFlags(f);

	_ctx->phase++;
	if (_ctx->phase == kPhaseFight)
		_host->callScript(0x9B2E, 0x1198, 0);
}

void BattlePlugin::cbEnterAnimDone(Fighter *f) {
	enterFinished(f);
}

void BattlePlugin::enterFinished(Fighter *f) {
	uint16 *host = _host->variableTable();
	Fighter *o = f->opponent;

	f->level = 0;
	f->doneCallback = &BattlePlugin::endMove;

	if (_ctx->networkGame == 0 && f->ai == 0)
		setInputDevice(f->player, identity(f->player, f->inputDevice));

	switchSprite(f, 0, f->animTab[f->remap[1]], 1);
	movesReposition(f->spriteId, 0);
	spawnShadow(f);

	if ((f->flags & kFlagNoKnockOut) || host[kVarMode] == 6)
		f->moveTab[f->set][kMoveSlotKnockOut].available = 0;

	if (_ctx->phase == kPhaseFight) {
		_ctx->aiEngaged = (f->level == o->level && f->lane == o->lane) ? 1 : 0;

		_host->unloadResource(0x9C1B);
		_host->unloadResource(0x9C1F);

		if (_ctx->fightOver != 0) {
			movesDie(1, f->player);
			return;
		}

		if (_ctx->networkGame == 0) {
			_host->callScript(0x8AF0, 2, 0);
			if (host[kVarMode] != 7)
				_host->callScript(0x8AF7, 0, 0);
		} else {
			_ctx->enteredCount++;
			if (_ctx->enteredCount == 2)
				warning("BattlePlugin::enterFinished(): network play is not supported");
		}
	} else {
		if (host[kVarMode] < 6)
			_ctx->aiEngaged = 0;
		else {
			_host->unloadResource(0x9C1B);
			if (host[kVarMode] == 6)
				_host->callScript(0x8AF7, 0, 0);
		}
	}
}

void BattlePlugin::fightSetup(uint16 slot, uint16 flag) {
	if (slot >= kFighterCount) {
		warning("BattlePlugin::fightSetup(): invalid fighter %d", slot);
		return;
	}

	uint16 *host = _host->variableTable();
	Fighter *f = &_ctx->fighters[slot];

	if (f->facing != flag) {
		f->facing = (f->facing + 1) % 2;
		host[f->varList[7]] = (host[f->varList[7]] == 0) ? 1 : 0;
		host[f->varList[8]] = (host[f->varList[8]] == 0) ? 1 : 0;
	}

	initAiFlags(f);

	_ctx->phase++;
	if (_ctx->phase == kPhaseFight)
		_host->callScript(0x9B2E, 0x1198, 0);

	f->level = 0;
	f->doneCallback = &BattlePlugin::endMove;
	_ctx->aiEngaged = 1;

	if (f->flags & kFlagNoKnockOut)
		f->moveTab[f->set][kMoveSlotKnockOut].available = 0;

	f->sprite = f->spriteTab[0];
	f->spriteId = (f->sprite > kLastOperandIndex) ? (uint16)(f->sprite + kLiteralBias) : host[f->sprite];

	if (f->ai == 0) {
		setInputDevice(f->player, identity(f->player, f->inputDevice));
		switchSprite(f, 0, f->animTab[f->remap[1]], 1);
	}

	movesReposition(f->spriteId, 0);

	_ctx->aiEngaged = (f->level == f->opponent->level && f->lane == f->opponent->lane) ? 1 : 0;
}

void BattlePlugin::movesDie(uint16 flag, uint16 who) {
	if (_ctx->fightOver != 0)
		return;

	_ctx->fightOver = 1;

	if (who == 2)
		who = _ctx->lastScorer;

	if (who >= kFighterCount) {
		warning("BattlePlugin::movesDie(): invalid fighter %d", who);
		return;
	}

	uint16 *host = _host->variableTable();
	Fighter *f = &_ctx->fighters[who];
	Fighter *o = f->opponent;

	if (_ctx->phase == kPhaseWalkIn) {
		movesEnter(f->player);
		return;
	}

	host[kVarWinner] = f->player;

	if (f->type != kFighterEvilToadMan) {
		if (f->player == 0)
			host[kVarWins]++;
		else
			host[kVarWins + 1]++;
	}

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

	o->doneCallback = &BattlePlugin::loserFall;
	f->timerCallback = &BattlePlugin::winnerPose;

	_host->setScriptTimer(f->timerId, 2000, f->timerScript);

	if (flag != 0 && (o->move == kMoveSlotIdle || o->move == kMoveSlotCrouch)) {
		loserFall(o);
		return;
	}

	o->timerCallback = &BattlePlugin::loserFall;
	_host->setScriptTimer(o->timerId, 2000, o->timerScript);
	o->doneCallback = &BattlePlugin::loserFall;
}

void BattlePlugin::loserFall(Fighter *f) {
	uint16 *host = _host->variableTable();

	_host->cancelScriptTimer(f->timerId);

	if (f->level == 1) {
		f->doneCallback = &BattlePlugin::fightOver;
		f->anim = f->animTab[f->remap[f->set == 0 ? 0x4A : 0x4C]];
		host[f->varList[0]] = 0;
		host[f->varList[1]] = (uint16)_ctx->bounds[1].y;
		_host->setSpriteQueue(f->sprite, f->anim, 0);
		f->timerCallback = &BattlePlugin::fightOver;
		_host->setScriptTimer(f->timerId, 2000, f->timerScript);
	} else
		fightOver(f);
}

void BattlePlugin::fightOver(Fighter *f) {
	uint16 *host = _host->variableTable();

	_host->cancelScriptTimer(f->timerId);

	host[kVarBusyCounter]++;

	_host->unloadResource(0x9C83);
	_host->unloadResource(0x9C68);
	_host->unloadResource(0x9C70);

	_host->callScript(0x9B2B, 0x11B2, 0);
	_host->callScript(0x9B2B, 0x1151, 0);

	Fighter *o = f->opponent;

	host[f->varList[14]] = f->z;
	host[o->varList[14]] = o->z;

	f->state = kStateFinal;
	o->state = kStateWinner;

	storeStats(o);
	storeStats(f);

	_host->callScript(kScriptEndRoundBonus, 0, 0);

	if (o->posture == 2) {
		moveTransform(o, kMoveSlotTransform, o->animTab[o->remap[10]]);
		o->doneCallback = &BattlePlugin::winnerPose;
	} else if (o->move == kMoveSlotIdle || o->move == kMoveSlotCrouch)
		winnerPose(o);
	else
		o->doneCallback = &BattlePlugin::winnerPose;

	f->doneCallback = &BattlePlugin::endFight;

	uint16 sprite = f->spriteTab[4];
	f->spriteId = (sprite > kLastOperandIndex) ? (uint16)(sprite + kLiteralBias) : host[sprite];

	_host->callScript(f->set == 0 ? f->varList[10] : f->varList[11], 0, 0);
}

void BattlePlugin::winnerPose(Fighter *f) {
	_host->cancelScriptTimer(f->timerId);

	f->doneCallback = (f->level == 1) ? &BattlePlugin::winnerWall : &BattlePlugin::collapse;

	_host->callScript(f->set == 0 ? f->varList[12] : f->varList[13], 0, 0);
}

void BattlePlugin::winnerWall(Fighter *f) {
	moveWallBounce(f, kMoveSlotLevelDown, f->animTab[11]);
	f->doneCallback = &BattlePlugin::collapse;
}

void BattlePlugin::collapse(Fighter *f) {
	f->level = kLevelNone;
	_ctx->aiEngaged = 0;
	f->state = kStateFinal;

	if ((f->x > 0 && f->facing == 0) || (f->x < 0 && f->facing == 1)) {
		moveTurn(f, kMoveSlotTurn, f->animTab[f->remap[4]]);
		return;
	}

	f->doneCallback = &BattlePlugin::endFight;
	f->z = 0x1F4;
	_host->cancelScriptTimer(f->timerId);
	switchSprite(f, _collapseSpriteIndex, f->animTab[f->facing == 0 ? 0x51 : 0x52], 0);
}

void BattlePlugin::endFight(Fighter *f) {
	uint16 *host = _host->variableTable();

	_host->cancelScriptTimer(f->timerId);
	_host->setSpriteQueue(f->sprite, 0, 0);
	_host->unloadResource(f->spriteTab[2]);
	_host->unloadResource(f->spriteTab[4]);

	_ctx->phase--;
	if (_ctx->phase != kPhaseNone) {
		if (host[kVarMode] == 7) {
			_host->callScript(0xA5D9, 0, 0);
			return;
		}

		Fighter *o = f->opponent;
		o->timerCallback = &BattlePlugin::endFight;
		_host->setScriptTimer(o->timerId, 10000, o->timerScript);
		return;
	}

	_host->unloadResource(0x9C81);
	_host->unloadResource(0x9C20);
	_host->callScript(0x9DD3, 0, 0);
}

void BattlePlugin::movesDieFinal(uint16 index) {
	_ctx->fightOver = 1;

	if (index >= kFighterCount) {
		warning("BattlePlugin::movesDieFinal(): invalid fighter %d", index);
		return;
	}

	Fighter *f = &_ctx->fighters[index];
	Fighter *o = f->opponent;

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
}

void BattlePlugin::fighterReset(Fighter *f) {
	resetFighterState(f);
	f->move = kMoveSlotIdle;

	if (f->opponent->move == kMoveSlotIdle)
		eventTrigger(0x15, 0, 0);
}

void BattlePlugin::eventTrigger15(Fighter *f) {
	eventTrigger(0x15, 0, 0);
}

void BattlePlugin::eventTrigger16(Fighter *f) {
	eventTrigger(0x16, 0, 0);
}

void BattlePlugin::eventTrigger18(Fighter *f) {
	eventTrigger(0x18, 0, 0);
}

void BattlePlugin::eventSlotClear(byte index) {
	if (index >= kEventSlotCount) {
		warning("BattlePlugin::eventSlotClear(): invalid event slot %d", index);
		return;
	}

	EventSlot *slot = &_ctx->eventSlots[index];

	slot->fired = 0;
	slot->armed = 0;
}

void BattlePlugin::eventWait(uint16 index, uint16 function, uint16 variable) {
	index &= 0xFF;

	if (index >= kEventSlotCount) {
		warning("BattlePlugin::eventWait(): invalid event slot %d", index);
		return;
	}

	EventSlot *slot = &_ctx->eventSlots[index];

	if (slot->fired != 0) {
		slot->armed = 0;
		slot->fired = 0;

		if (function != 0)
			warning("BattlePlugin::eventWait(): event function 0x%04X is not supported", function);
		else if (variable != 0)
			_host->callScript((uint16)(variable - kLiteralBias), slot->scriptArg0, slot->scriptArg1);
	} else {
		slot->armed = 1;
		slot->callback = function;
		slot->script = (uint16)(variable - kLiteralBias);
	}
}

void BattlePlugin::eventTrigger(uint16 index, uint16 a, uint16 b) {
	if (index > 0x24)
		return;

	EventSlot *slot = &_ctx->eventSlots[index & 0xFF];

	if (slot->armed != 0) {
		slot->armed = 0;
		slot->fired = 0;

		if (slot->callback != 0)
			warning("BattlePlugin::eventTrigger(): event function 0x%04X is not supported", slot->callback);
		else if (slot->script != 0)
			_host->callScript(slot->script, a, b);
	} else {
		slot->fired = 1;
		slot->scriptArg0 = a;
		slot->scriptArg1 = b;
	}
}

void BattlePlugin::sceneRegisterButton(uint16 id, uint16 variable, uint16 remove) {
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
	button->script = (uint16)(variable - kLiteralBias);

	_host->getSpriteRect((uint16)(button->spriteId - kLiteralBias), button->rect);

	button->rect.top -= 0x28;
	button->rect.left -= 0x0A;
	button->rect.right += 0x0A;
}

void BattlePlugin::sceneSetBounds(uint16 index, uint16 left, uint16 right, uint16 y, uint16 f1, uint16 f2) {
	if (index >= kBoundsCount)
		return;

	ArenaBounds *bounds = &_ctx->bounds[index];

	bounds->left = (int16)left;
	bounds->right = (int16)right;
	bounds->y = (int16)y;
	bounds->sidestepMinusLimit = (f1 != 0) ? -1 : 0;
	bounds->sidestepPlusLimit = (f2 != 0) ? 1 : 0;

	if (index == 0) {
		_ctx->outerLeft = (int16)(left + 0x3C);
		_ctx->outerRight = (int16)(right - 0x3C);
	}
}

void BattlePlugin::sceneSetDoor(uint16 w0, uint16 w1, uint16 w2, uint16 w3, uint16 w4, uint16 w5) {
	_ctx->doorX = (int16)w0;
	_ctx->doorY = (int16)w1;
	_ctx->doorRect.left = (int16)w2;
	_ctx->doorRect.top = (int16)w3;
	_ctx->doorRect.right = (int16)w4;
	_ctx->doorRect.bottom = (int16)w5;
}

void BattlePlugin::spawnShadow(Fighter *f) {
	if (_ctx->effectsOff == 0 && f->level == 0) {
		uint16 sprite = _shadowSprites[(f->player == 0) ? 0 : 1];
		uint16 animation = _shadowAnimations[(f->player == 0) ? 0 : 1];

		_host->setSpritePosition(sprite, f->x, f->y);
		_host->setSpriteQueue(sprite, animation, 0);
	}
}

void BattlePlugin::spawnHitSpark(const Common::Rect *rect, byte damageClass) {
	if (_ctx->effectsOff == 0) {
		if (damageClass >= kDamageClassCount) {
			warning("BattlePlugin::spawnHitSpark(): invalid damage class %d", damageClass);
			return;
		}

		int16 x = (int16)(rect->right + rect->left) / 2;
		int16 y = (int16)(rect->top + rect->bottom) / 2;

		_host->setSpritePosition(_hitSparkSprite, x, y);
		_host->setSpriteQueue(_hitSparkSprite, _hitSparkAnimations[damageClass], 0);
	}
}

void BattlePlugin::initFightersFromHost() {
	uint16 *host = _host->variableTable();

	for (uint16 i = 0; i < kFighterCount; i++) {
		Fighter *f = &_ctx->fighters[i];
		uint16 type = host[kVarFighterType + i];

		if (type == kFighterEvilToadMan)
			f->ai = 1;
		else {
			f->ai = (host[kVarHumanPlayers] > i) ? 0 : 1;
			f->inputDevice = (byte)host[kVarInputType + i];
		}

		f->type = type;
		f->weapon = host[kVarWeapon + i];
		f->autoTurn = (byte)host[kVarAutoTurn + i];
	}

	_ctx->effectsOff = (byte)host[kVarEffectsOff];
	host[kVarBusyCounter] = 0;
}

void BattlePlugin::initFighter(Fighter *f, uint16 player, uint16 type) {
	const FighterInitParams *params = &_fighterInitParams[type];
	FighterTypeData *data = &_typeData[type];

	f->sprite = params->sprite;
	f->spriteId = params->spriteId;

	f->player = (byte)player;
	f->inFight = 1;
	if (type == kFighterEvilToadMan) {
		f->set = 1;
		f->posture = 1;
	} else {
		f->set = 0;
		f->posture = 0;
	}
	f->state = kStateNormal;
	f->spriteTab = data->sprites;
	f->rectTab = data->rects;
	f->xTab = data->offsetsX;
	f->level = 0;
	f->move = kMoveSlotIdle;
	f->queued = 0;
	f->z = 0x64;
	f->facing = params->facing;
	if (type == kFighterEvilToadMan) {
		f->inputTab[0] = nullptr;
		f->hitTab[0] = nullptr;
	} else {
		f->inputTab[0] = _inputRecords[0];
		f->hitTab[0] = data->hits[0];
	}
	f->inputTab[1] = _inputRecords[1];
	f->hitTab[1] = data->hits[1];
	f->reactTab[0] = _reactionTables[0];
	f->reactTab[1] = _reactionTables[1];
	f->reactTab[2] = _reactionTables[2];
	f->animTab = data->animations;
	if (type == kFighterEvilToadMan) {
		f->doneCallback = &BattlePlugin::endMove;
		f->timerCallback = &BattlePlugin::endMove;
	} else {
		f->doneCallback = &BattlePlugin::cbEnterAnimDone;
		f->timerCallback = &BattlePlugin::cbEnterAnimDone;
	}
	f->varList = data->varList;
	f->comboTab = data->combos;
	f->yTab = data->offsetsY;
	f->damageMultiplier = 1;
	f->walkTab = data->walkSteps;
	f->knockTab = data->knockDistances;
	f->flags |= params->flags;

	for (int i = 0; i < kRemapSize; i++)
		f->remap[i] = data->remap[i];
	if (type != kFighterEvilToadMan) {
		for (int i = 0; i < kMoveCount; i++)
			f->moveTab[0][i] = data->moves[0][i];
	}
	for (int i = 0; i < kMoveCount; i++)
		f->moveTab[1][i] = data->moves[1][i];

	f->aiBlock.list = data->aiList;
	if (type == kFighterEvilToadMan)
		f->ai = 1;
	if (f->ai != 0)
		aiSetupSkill(f);

	if (type == kFighterEvilToadMan) {
		uint16 *host = _host->variableTable();

		f->weapon = 4;
		if (f->player == 0)
			host[kVarAmmoWeapon4] = 0x7530;
		else
			host[kVarAmmoWeapon4 + 1] = 0x7530;
	}

	_host->callScript(params->script, 0, 0);

	if (type == kFighterEvilToadMan) {
		Common::Rect rect;
		_host->getSpriteRect(0xA41B, rect);
		f->x = (int16)(rect.right + rect.left) / 2;
		f->y = 0;

		_host->setSpriteLevel(params->sprite, 100);
		_host->transferSprite(0xA41B, params->sprite, 0x9A50, 1, 0);
	}
}

} // End of namespace Topgun
