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

#include "topgun/games/battle/battle.h"

namespace Topgun {

void BattlePlugin::setAttackActive(Fighter *f, uint16 v) {
	f->attackActive = (byte)v;
	if ((byte)v != 0)
		aiEvent(f->opponent, kAiEventOpponentAttacking);
}

void BattlePlugin::playAnim(Fighter *f, uint16 anim) {
	setQueueGate(f->player, 1, 0);
	f->anim = anim;
	_host->setSpriteQueue(f->sprite, anim, 0);
}

void BattlePlugin::startKnockdown(Fighter *f) {
	uint16 *host = _host->variableTable();

	if (f->speed != kSpeedNone)
		setMoveHook(f, nullptr);

	if (f->state != kStateFinal)
		f->doneCallback = &BattlePlugin::endMove;

	f->state = kStateKnockedDown;
	f->anim = f->animTab[f->remap[(f->set == 0) ? 0x4A : 0x4C]];
	host[f->varList[0]] = 0xFFC4;
	host[f->varList[1]] = _ctx->bounds[1].y;

	setMoveHook(f, nullptr);
	f->specialTab = nullptr;
	setContact(f, 0);
	setInFight(f, 0);
	f->speed = kSpeedNone;
	f->combo = 0;
	playAnim(f, f->anim);
}

uint16 BattlePlugin::forceMove(Fighter *f, uint16 move) {
	setMoveHook(f, nullptr);

	MoveEntry *entry = &f->moveTab[f->set][(byte)move];
	f->anim = f->animTab[f->remap[entry->animIndex]];
	f->move = (byte)move;
	f->handlerParam = entry->handlerParam;

	if (entry->handler == nullptr) {
		warning("BattlePlugin::forceMove(): move %d has no handler", f->move);
		return 0;
	}

	return (this->*entry->handler)(f, f->move, f->anim);
}

void BattlePlugin::reactionDone(Fighter *f) {
	spawnShadow(f);
	endMove(f);
}

void BattlePlugin::reactionDoneKnock(Fighter *f) {
	spawnShadow(f);
	reactionDoneCombo(f);
}

void BattlePlugin::reactionDoneCombo(Fighter *f) {
	f->opponent->comboEnd = g_system->getMillis() + 500;
	endMove(f);
}

void BattlePlugin::setInFight(Fighter *f, uint16 v) {
	f->flags &= ~(kFlagProjectile | kFlagLevelChange);
	f->inFight = (byte)v;
	if ((byte)v != 0)
		aiEvent(f->opponent, kAiEventOpponentVulnerable);
}

void BattlePlugin::resetFighterState(Fighter *f) {
	setMoveHook(f, nullptr);
	setContact(f, 0);

	f->specialTab = nullptr;
	f->state = kStateNormal;
	f->speed = kSpeedNone;
	f->damageMultiplier = 1;
	f->queued = 0;
	f->doneCallback = &BattlePlugin::endMove;
	f->flags &= ~kFlagResetOnEnd;

	setInFight(f, 1);
}

void BattlePlugin::switchSprite(Fighter *f, uint16 k, uint16 anim, uint16 v) {
	uint16 *host = _host->variableTable();

	uint16 old = f->sprite;
	f->sprite = f->spriteTab[(byte)k];
	f->anim = anim;

	if (f->sprite == old) {
		_host->setSpriteQueue(old, anim, 0);
		_host->setSpriteLevel(f->sprite, f->z);
		return;
	}

	if (f->sprite > kLastOperandIndex)
		f->spriteId = (uint16)(f->sprite + kLiteralBias);
	else
		f->spriteId = host[f->sprite];

	host[kVarBusyCounter]++;
	_host->loadResource(f->sprite);
	host[kVarBusyCounter]--;

	_host->setSpriteLevel(f->sprite, f->z);
	_host->transferSprite(old, f->sprite, anim, (byte)v, 0);
}

void BattlePlugin::endMove(Fighter *f) {
	if (f->flags & kFlagResetOnEnd) {
		fighterReset(f);
		return;
	}

	f->move = kMoveSlotIdle;
	f->state = kStateNormal;
	f->speed = kSpeedNone;
	f->damageMultiplier = 1;
	f->doneCallback = &BattlePlugin::endMove;
	f->lastFrame = 0;
	f->specialTab = nullptr;

	if (f->attackActive != 0)
		scoreAttack(f, 0, 0, 0);

	setInFight(f, 1);

	if (f->ai != 0) {
		if (autoTurn(f) == 0)
			aiEvent(f, kAiEventMoveEnded);
	} else {
		if (autoTurn(f) == 0 && setQueueGate(f->player, 0, 1) == 0)
			moveIdle(f, 0, 0);
	}
}

uint16 BattlePlugin::jumpMove(Fighter *f, uint16 anim, uint16 flip, int16 dx, int16 dy) {
	uint16 *host = _host->variableTable();

	f->doneCallback = &BattlePlugin::cbJumpEnd;
	setMoveHook(f, nullptr);

	int16 distance = (int16)(_jumpOffsets[f->set][1] + dx);
	if ((uint16)f->facing != flip)
		distance = -distance;

	distance = clampMove(f, distance, 0);

	Common::Rect rect;
	_host->getSpriteRect(f->sprite, rect);

	int16 y = (int16)((int16)(rect.top + rect.bottom) / 2 - f->yTab[f->set] + dy);
	int16 hostY = 0;

	if (f->level == 1) {
		y = (int16)(y - _ctx->bounds[0].y);
		if ((int16)(distance + f->x) < _ctx->bounds[1].left) {
			hostY = _ctx->bounds[1].y;
			f->flags |= kFlagLevelChange;
		}
	}

	host[f->varList[2]] = (uint16)((int16)(distance * 2) / 3);
	host[f->varList[0]] = (uint16)distance;
	host[f->varList[3]] = (uint16)y;
	host[f->varList[1]] = (uint16)hostY;

	f->lastAnim = anim;
	playAnim(f, anim);
	setContact(f, 1);
	return 1;
}

uint16 BattlePlugin::moveGrab(Fighter *f, uint16 move, uint16 anim) {
	if (_ctx->networkGame != 0)
		return 0;

	_host->spriteSwapOrder(f->sprite, f->opponent->sprite, 1, 0);
	playAnim(f, anim);

	if (f->buttons == kInputButton2) {
		setMoveHook(f, &BattlePlugin::hookRelease);
		f->doneCallback = &BattlePlugin::cbReplayAnim;
	} else {
		_host->stopSprite(f->sprite);
	}

	return 1;
}

uint16 BattlePlugin::moveJump(Fighter *f, uint16 move, uint16 anim) {
	f->state = kStateAttack;
	return jumpMove(f, anim, ((byte)move == kMoveSlotJumpBack) ? 1 : 0, 0, _jumpOffsets[f->set][0]);
}

uint16 BattlePlugin::moveJumpUp(Fighter *f, uint16 move, uint16 anim) {
	f->state = kStateAttack;

	if (f->dirCode == kDirUp) {
		f->doneCallback = &BattlePlugin::cbReplayAnimSync;
		setMoveHook(f, &BattlePlugin::hookJump);
	} else if (f->set == 1 && f->ai == 0) {
		anim = f->animTab[f->remap[24]];
		f->anim = anim;
	}

	playAnim(f, anim);
	return 1;
}

uint16 BattlePlugin::moveJumpAttack(Fighter *f, uint16 move, uint16 anim) {
	f->state = kStateAttack;
	return jumpMove(f, anim, ((byte)move == kMoveSlotJumpAttackBack) ? 1 : 0, 0, _jumpOffsets[f->set][0]);
}

uint16 BattlePlugin::moveWallBounce(Fighter *f, uint16 move, uint16 anim) {
	uint16 *host = _host->variableTable();

	f->state = kStateAttack;
	f->doneCallback = &BattlePlugin::reactionDone;

	int16 dx = 0;
	const ArenaBounds *bounds;

	if (move == kMoveSlotLevelUp) {
		if (f->level == 1)
			return 0;

		bounds = &_ctx->bounds[1];
		if (bounds->y == 0)
			return 0;
	} else {
		if (f->level == 0)
			return 0;

		bounds = &_ctx->bounds[0];
	}

	uint16 k = f->posture * 2 + f->facing;
	int16 x = (int16)(f->xTab[k] + f->x);

	if (x < bounds->left)
		dx = (int16)(-(int16)(f->xTab[k] - bounds->left) - f->rectTab[k].left - f->x);
	else if (x > bounds->right)
		dx = (int16)(-(int16)(f->rectTab[k].right - bounds->right) - f->xTab[k] - f->x);

	host[f->varList[0]] = (uint16)dx;
	host[f->varList[1]] = (uint16)-bounds->y;

	f->lastAnim = anim;
	playAnim(f, anim);
	return 1;
}

uint16 BattlePlugin::moveIdle(Fighter *f, uint16 move, uint16 anim) {
	_host->setSpriteQueue(f->sprite, f->animTab[_idleAnimations[f->posture]], 0);
	return 1;
}

uint16 BattlePlugin::moveTurn(Fighter *f, uint16 move, uint16 anim) {
	f->doneCallback = &BattlePlugin::cbTurnAround;
	playAnim(f, anim);
	return 1;
}

uint16 BattlePlugin::moveWalk(Fighter *f, uint16 move, uint16 anim) {
	uint16 *host = _host->variableTable();

	if (f->dirCode == kDirForward || f->dirCode == kDirBack || f->ai != 0) {
		if (f->ai == 0) {
			f->doneCallback = &BattlePlugin::cbWalkContinue;
			setMoveHook(f, &BattlePlugin::hookWalk);
		} else {
			f->doneCallback = &BattlePlugin::endMove;
		}

		f->walkStep = (int16)f->walkTab[f->posture].fullStep;
	} else if (f->posture == 0) {
		if (move == kMoveSlotWalkForward)
			anim = f->animTab[4];
		else if (move == kMoveSlotWalkBack)
			anim = f->animTab[6];

		f->walkStep = (int16)f->walkTab[f->posture].shortStep;
	}

	f->speed &= (byte)~kSpeedPushing;
	if (!(f->speed & kSpeedSlow))
		f->speed = kSpeedWalk;

	if (f->posture == 2) {
		host[f->varList[1]] = 0;
		f->walkStep = (int16)f->walkTab[f->posture].shortStep;

		if ((move == kMoveSlotWalkForward && f->facing == 1) || (move == kMoveSlotWalkBack && f->facing == 0))
			host[f->varList[0]] = (uint16)clampMove(f, (int16)-f->walkStep, 0);
		else
			host[f->varList[0]] = (uint16)clampMove(f, f->walkStep, 0);

		f->lastAnim = anim;
	}

	bool toward;
	if (move == kMoveSlotWalkForward)
		toward = (f->side == f->facing);
	else
		toward = (f->side != f->facing);

	if (toward && _ctx->aiEngaged != 0 && f->walkStep > _ctx->gapMinusMargins && f->opponent->state != kStateKnockedDown
			&& !(f->opponent->flags & kFlagLevelChange)) {
		anim = clashPush(f, f->opponent, anim);
		f->speed |= kSpeedPushing;
	}

	playAnim(f, anim);

	if (f->speed == kSpeedSlow)
		_host->setSpriteSpeedMotion(f->sprite, kMotionSpeedSlow, kMotionSpeedSlow);
	else
		_host->setSpriteSpeedMotion(f->sprite, kMotionSpeedNormal, kMotionSpeedNormal);

	return 1;
}

uint16 BattlePlugin::moveSidestep(Fighter *f, uint16 move, uint16 anim) {
	uint16 *host = _host->variableTable();

	if (!(f->flags & kFlagSidestep)) {
		endMove(f);
		return 1;
	}

	int16 step = (move == kMoveSlotSidestepMinus) ? -5 : 5;
	int16 lane = (int16)(step + f->lane);

	Common::Rect destination;
	Common::Point points[2];
	if (lane == f->opponent->lane && hurtBoxOverlap(f, &destination, points, 0) != 0) {
		endMove(f);
		return 1;
	}

	if (f->dirCode != kDirNone && f->buttons != 0) {
		setMoveHook(f, &BattlePlugin::hookEndAfterAnim);
		f->doneCallback = &BattlePlugin::cbSidestepContinue;
	}

	host[f->varList[0]] = 0;
	host[f->varList[1]] = (uint16)(3 * step);
	f->y = (int16)(f->y + 3 * step);
	f->lane = lane;

	const ArenaBounds *bounds = &_ctx->bounds[0];
	if (f->level < kBoundsCount)
		bounds = &_ctx->bounds[f->level];
	else
		warning("BattlePlugin::moveSidestep(): invalid level %d", f->level);

	f->moveTab[f->set][kMoveSlotSidestepMinus].available = (bounds->sidestepMinusLimit < lane) ? 1 : 0;
	f->moveTab[f->set][kMoveSlotSidestepPlus].available = (bounds->sidestepPlusLimit > lane) ? 1 : 0;

	f->z = (uint16)(f->z + step);

	Fighter *opp = f->opponent;
	if (opp->level == f->level && opp->lane == f->lane && _ctx->phase == kPhaseFight)
		_ctx->aiEngaged = 1;
	else
		_ctx->aiEngaged = 0;

	_host->setSpriteLevel(f->sprite, f->z);
	f->lastAnim = anim;
	playAnim(f, anim);
	return 1;
}

uint16 BattlePlugin::moveAttack(Fighter *f, uint16 move, uint16 anim) {
	if (f->opponent->inFight == 0)
		return 0;

	setAttackActive(f, 1);
	f->doneCallback = &BattlePlugin::endMove;
	_host->spriteSwapOrder(f->sprite, f->opponent->sprite, 1, 0);
	playAnim(f, anim);
	return 1;
}

uint16 BattlePlugin::moveAttackCloseRange(Fighter *f, uint16 move, uint16 anim) {
	if (f->opponent->inFight == 0)
		return 0;

	updateDistance(f);

	if (_ctx->contact != 0 && _ctx->aiEngaged != 0)
		anim = f->animTab[((byte)move == 0x0A) ? 0x2D : 0x2E];

	moveAttack(f, (byte)move, anim);
	return 1;
}

uint16 BattlePlugin::moveAttackHoldButton2(Fighter *f, uint16 move, uint16 anim) {
	uint16 *host = _host->variableTable();
	Fighter *opp = f->opponent;

	if (opp->inFight == 0)
		return 0;

	_host->spriteSwapOrder(f->sprite, opp->sprite, 1, 0);

	if (_ctx->networkGame == 0 && f->handlerParam != 0 && f->buttons == kInputButton2)
		setMoveHook(f, &BattlePlugin::hookHoldTrack);

	setAttackActive(f, 1);
	f->doneCallback = &BattlePlugin::cbHoldCount;
	f->holdFlag = 0;
	f->holdCount = 0;
	host[f->varList[9]] = 0;
	playAnim(f, anim);
	return 1;
}

uint16 BattlePlugin::moveAirAttack(Fighter *f, uint16 move, uint16 anim) {
	if (f->level != 1)
		return 0;

	setAttackActive(f, 1);
	f->state = kStateAttack;
	f->doneCallback = &BattlePlugin::reactionDone;
	playAnim(f, anim);
	return 1;
}

uint16 BattlePlugin::moveProjectile(Fighter *f, uint16 move, uint16 anim) {
	uint16 *host = _host->variableTable();
	Fighter *opp = f->opponent;

	if ((f->player == 0 && !(host[kVarOptions1] & 0x80)) || (f->player == 1 && !(host[kVarOptions1] & 0x100))) {
		if (host[kVarAmmoCounter + f->player] == 0)
			return 0;
	}

	if (opp->inFight == 0)
		return 0;

	int16 distance;
	if (f->side == f->facing)
		distance = _ctx->edgeGap;
	else
		distance = kProjectileDistance;

	if (f->facing == 1)
		distance = -distance;

	host[f->varList[0]] = (uint16)clampMove(f, distance, 0);
	f->state = kStateAttack;

	if (f->handlerParam != 0)
		_host->addCollisionPair(f->handlerParam, opp->sprite, kScriptWeaponsSpecialHit, f->player, f->handlerParam, 0);
	else if (f->flags & kFlagTypeInit)
		f->flags |= kFlagProjectile;

	moveAttack(f, (byte)move, anim);

	if (f->player == 0) {
		if (host[kVarOptions1] & 0x80)
			return 1;

		host[kVarAmmoCounter]--;
	} else {
		if (host[kVarOptions1] & 0x100)
			return 1;

		host[kVarAmmoCounter + 1]--;
	}

	_host->callScript(kScriptAmmoDisplay, f->player, 0);
	return 1;
}

uint16 BattlePlugin::moveAttackBoost(Fighter *f, uint16 move, uint16 anim) {
	if (f->opponent->inFight == 0)
		return 0;

	f->damageMultiplier++;
	setAttackActive(f, 1);
	_host->spriteSwapOrder(f->sprite, f->opponent->sprite, 1, 0);
	f->lastFrame = 0;
	_host->callScript(kScriptSetSpriteCel, f->spriteId, f->moveCel);
	return 1;
}

uint16 BattlePlugin::moveAttackHoldAnyButton(Fighter *f, uint16 move, uint16 anim) {
	if (f->opponent->inFight == 0)
		return 0;

	setAttackActive(f, 1);
	f->doneCallback = &BattlePlugin::cbHoldCount;
	_host->spriteSwapOrder(f->sprite, f->opponent->sprite, 1, 0);

	if (f->buttons != 0)
		setMoveHook(f, &BattlePlugin::hookHoldTrack);

	f->holdFlag = 0;
	f->holdCount = 0;
	playAnim(f, anim);
	return 1;
}

uint16 BattlePlugin::moveGuard(Fighter *f, uint16 move, uint16 anim) {
	f->state = kStateGuard;

	if (!(((f->dirCode == kDirDown || f->dirCode == kDirDownForward) && f->buttons == kInputButton1) || f->ai != 0)) {
		f->doneCallback = &BattlePlugin::endMove;
	} else {
		if (f->ai == 0)
			setMoveHook(f, &BattlePlugin::hookGuard);

		if ((byte)move == kMoveSlotGuardDownForward)
			f->doneCallback = &BattlePlugin::cbFallAnim;
		else
			f->doneCallback = (f->ai != 0) ? &BattlePlugin::aiStartTimer : &BattlePlugin::cbNull;
	}

	playAnim(f, anim);
	return 1;
}

uint16 BattlePlugin::moveCrouch(Fighter *f, uint16 move, uint16 anim) {
	if (f->dirCode != kDirDown && f->ai == 0) {
		f->doneCallback = &BattlePlugin::endMove;
	} else if (f->ai == 0) {
		setMoveHook(f, &BattlePlugin::hookEndNow);
		f->doneCallback = &BattlePlugin::cbNull;
	} else {
		f->doneCallback = &BattlePlugin::aiStartTimer;
	}

	playAnim(f, anim);
	return 1;
}

uint16 BattlePlugin::startDeath(Fighter *f, uint16 anim) {
	uint16 *host = _host->variableTable();

	setInputDevice(f->player, kInputDeviceDisable);
	setInFight(f, 0);
	f->state = kStateDying;
	host[kVarBusyCounter]++;
	_host->setSpriteQueue(f->sprite, anim, 0);
	return 1;
}

uint16 BattlePlugin::moveKnockOut(Fighter *f, uint16 move, uint16 anim) {
	setQueueGate(f->player, 1, 0);

	if (f->flags & kFlagKnockOutSeen)
		_host->callScript(kScriptChangeEnergy, f->player, 0xFFFB);
	else
		f->flags |= kFlagKnockOutSeen;

	if (f->state != kStateFinal) {
		f->doneCallback = &BattlePlugin::cbGetUp;
		startDeath(f, anim);
	} else if (f->doneCallback == nullptr) {
		warning("BattlePlugin::moveKnockOut(): fighter %d has no done callback", f->player);
	} else {
		(this->*f->doneCallback)(f);
	}

	return 1;
}

uint16 BattlePlugin::moveSelectWeapon(Fighter *f, uint16 move, uint16 anim) {
	uint16 *host = _host->variableTable();

	_host->callScript(kScriptWeaponSelect, f->player, 0);
	f->weapon = host[kVarWeapon + f->player];
	weaponInit(f);
	endMove(f);
	return 1;
}

uint16 BattlePlugin::moveTransform(Fighter *f, uint16 move, uint16 anim) {
	setStanceSet(f, 0, (f->posture == 0) ? 1 : 0);
	separateFighters(f);
	updateDistance(f);
	updateWallFlags(f);
	playAnim(f, anim);
	return 1;
}

} // End of namespace Topgun
