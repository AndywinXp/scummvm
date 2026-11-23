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

void BattlePlugin::cbHoldCount(Fighter *f) {
	if (f->holdFlag != 0) {
		f->holdFlag = 0;
		f->holdCount++;
		f->damageMultiplier++;
		if (f->holdCount < 3)
			return;
	}

	f->damageMultiplier = 1;
	f->doneCallback = &BattlePlugin::endMove;
	setMoveHook(f, nullptr);
	_host->stopSprite(f->sprite);
}

void BattlePlugin::cbJumpEnd(Fighter *f) {
	setContact(f, 0);
	reactionDone(f);
}

void BattlePlugin::cbReplayAnim(Fighter *f) {
	playAnim(f, f->anim);
}

void BattlePlugin::cbReplayAnimSync(Fighter *f) {
	spawnShadow(f);
	playAnim(f, f->anim);
}

void BattlePlugin::cbWalkContinue(Fighter *f) {
	if (f->atWall != 0) {
		setMoveHook(f, nullptr);
		endMove(f);
		return;
	}

	moveWalk(f, f->move, f->anim);
}

void BattlePlugin::cbSidestepContinue(Fighter *f) {
	if (f->moveTab[f->set][f->move].available == 0)
		endMove(f);
	else
		moveSidestep(f, f->move, f->anim);
}

void BattlePlugin::cbGetUp(Fighter *f) {
	uint16 index;

	if (f->set != 0 && f->type != kFighterEvilToadMan) {
		setStanceSet(f, 0, 0);
		index = f->remap[1];
	} else {
		setStanceSet(f, 1, 0);
		index = f->remap[20];
	}

	uint16 anim = f->animTab[index];

	f->state = kStateNormal;
	f->z = (uint16)(f->z - f->lane);
	f->y = (int16)(f->y - 3 * f->lane);
	f->lane = 0;
	f->doneCallback = &BattlePlugin::endMove;

	Fighter *opponent = f->opponent;
	if (opponent->level == f->level && opponent->lane == f->lane && _ctx->phase == kPhaseFight)
		_ctx->aiEngaged = 1;
	else
		_ctx->aiEngaged = 0;

	_host->setSpritePosition(f->sprite, (uint16)f->x, (uint16)f->y);
	switchSprite(f, 0, anim, 1);
	separateFighters(f);
	_host->callScript(kScriptTableSetChanged, f->player, f->set);
	updateDistance(f);
	updateWallFlags(f);
	setQueueGate(f->player, 0, 0);
	if (f->ai == 0)
		setInputDevice(f->player, kInputDeviceEnable);

	endMove(f);

	uint16 *host = _host->variableTable();
	host[kVarBusyCounter]--;
}

void BattlePlugin::cbFallAnim(Fighter *f) {
	_host->callScript(kScriptChangeEnergy, f->player, 0xFFFD);

	if (f->state == kStateFinal) {
		if (f->doneCallback == nullptr)
			warning("BattlePlugin::cbFallAnim(): fighter %d has no done callback", f->player);
		else
			(this->*f->doneCallback)(f);

		return;
	}

	uint16 anim = f->animTab[f->remap[f->set == 0 ? 0x4E : 0x4F]];
	f->anim = anim;
	_host->setSpriteQueue(f->sprite, anim, 0);
	if (f->ai != 0)
		aiStartTimer(f);
}

void BattlePlugin::cbTurnAround(Fighter *f) {
	f->facing = (f->facing + 1) % 2;

	uint16 *host = _host->variableTable();
	host[f->varList[7]] = (host[f->varList[7]] == 0) ? 1 : 0;
	host[f->varList[8]] = (host[f->varList[8]] == 0) ? 1 : 0;

	separateFighters(f);
	updateDistance(f);
	updateWallFlags(f);
	if (f->state == kStateFinal)
		collapse(f);
	else
		endMove(f);
}

void BattlePlugin::cbNull(Fighter *f) {
}

uint16 BattlePlugin::hookGuard(Fighter *f, uint16 code) {
	uint16 animIndex;

	if ((byte)code == kDirDown) {
		f->move = kMoveSlotGuardDown;
		f->doneCallback = &BattlePlugin::cbNull;
		animIndex = f->moveTab[f->set][kMoveSlotGuardDown].animIndex;
	} else if ((byte)code == kDirDownForward) {
		f->move = kMoveSlotGuardDownForward;
		f->doneCallback = &BattlePlugin::cbFallAnim;
		animIndex = f->moveTab[f->set][kMoveSlotGuardDownForward].animIndex;
	} else {
		setMoveHook(f, nullptr);
		endMove(f);
		return 1;
	}

	uint16 anim = f->animTab[f->remap[animIndex]];
	f->anim = anim;
	_host->setSpriteQueue(f->sprite, anim, 0);
	return 0;
}

uint16 BattlePlugin::hookEndAfterAnim(Fighter *f, uint16 code) {
	f->doneCallback = &BattlePlugin::endMove;
	setMoveHook(f, nullptr);
	return 1;
}

uint16 BattlePlugin::hookRelease(Fighter *f, uint16 code) {
	if ((byte)code == kInputRelease2) {
		f->doneCallback = &BattlePlugin::endMove;
		setMoveHook(f, nullptr);
		_host->stopSprite(f->sprite);
	}

	return 0;
}

uint16 BattlePlugin::hookEndNow(Fighter *f, uint16 code) {
	setMoveHook(f, nullptr);
	endMove(f);
	return 1;
}

uint16 BattlePlugin::hookHoldTrack(Fighter *f, uint16 code) {
	if ((byte)code == f->handlerParam) {
		f->holdFlag = 1;
		return 0;
	}

	if ((byte)code != kDirNone)
		setMoveHook(f, nullptr);

	return 1;
}

uint16 BattlePlugin::hookJump(Fighter *f, uint16 code) {
	uint16 frameIds[4] = {};

	f->doneCallback = &BattlePlugin::reactionDone;
	setMoveHook(f, nullptr);

	if ((byte)code != kDirNone)
		return 1;

	if (f->set != 1)
		return 0;

	_host->getSpriteFrameIds(f->sprite, frameIds);
	if (frameIds[0] != f->lastFrame && frameIds[0] != f->handlerParam)
		return 0;

	uint16 anim = f->animTab[f->remap[24]];
	f->anim = anim;
	playAnim(f, anim);
	return 1;
}

uint16 BattlePlugin::hookWalk(Fighter *f, uint16 code) {
	uint16 frameIds[4] = {};

	switch ((byte)code) {
	case kDirNone: {
		f->doneCallback = &BattlePlugin::endMove;
		setMoveHook(f, nullptr);

		_host->getSpriteFrameIds(f->sprite, frameIds);
		if (frameIds[0] != f->lastFrame)
			return 0;

		if (f->posture == 0 && f->speed == kSpeedWalk) {
			// The original reads an uninitialised stack word when the move is neither 1 nor 2
			uint16 anim = 0;
			if (f->move == kMoveSlotWalkForward)
				anim = f->animTab[4];
			if (f->move == kMoveSlotWalkBack)
				anim = f->animTab[6];

			f->walkStep = (int16)f->walkTab[f->posture].shortStep;
			playAnim(f, anim);
		}

		return 1;
	}
	case kInputButton1:
		if (f->speed != kSpeedWalk)
			return 0;

		f->speed = kSpeedSlow;
		_host->setSpriteSpeedMotion(f->sprite, kMotionSpeedSlow, kMotionSpeedSlow);
		return 1;
	case kInputRelease1:
		if (f->speed != kSpeedSlow)
			return 0;

		f->speed = kSpeedWalk;
		_host->setSpriteSpeedMotion(f->sprite, kMotionSpeedNormal, kMotionSpeedNormal);
		return 1;
	default:
		f->doneCallback = &BattlePlugin::endMove;
		setMoveHook(f, nullptr);
		return 1;
	}
}

uint16 BattlePlugin::netInputHook(Fighter *f, uint16 code) {
	warning("BattlePlugin::netInputHook(): network games are not supported");
	return 0;
}

void BattlePlugin::initAiFlags(Fighter *f) {
	uint16 *host = _host->variableTable();

	f->moveTab[1][19].available = host[kVarPlayerOptions + f->player] & 1;
	if (!(f->flags & kFlagSidestep)) {
		f->moveTab[0][kMoveSlotSidestepMinus].available = 0;
		f->moveTab[0][kMoveSlotSidestepPlus].available = 0;
	}
}

void BattlePlugin::setContact(Fighter *f, uint16 v) {
	if (v != 0) {
		f->contact = 1;
		_host->addCollisionPair(f->sprite, f->opponent->sprite, kScriptHitAttack, 0, 0, 1);
	} else {
		f->contact = 0;
		if (f->opponent->contact == 0)
			_host->addCollisionPair(f->sprite, f->opponent->sprite, 0, 0, 0, 0);
	}
}

void BattlePlugin::setStanceSet(Fighter *f, uint16 set, uint16 flag) {
	uint16 *host = _host->variableTable();

	host[kVarStanceFlags] ^= (f->player == 0) ? 0x01 : 0x10;
	f->set = (byte)set;

	if ((byte)set != 0) {
		f->crouching = 0;
		f->posture = 1;
	} else {
		f->moveTab[f->set][kMoveSlotCrouch].available = flag;
		f->moveTab[f->set][21].available = flag;
		if (!(f->flags & kFlagNoKnockOut) && host[kVarMode] != 6)
			f->moveTab[f->set][kMoveSlotKnockOut].available = 1;
		else
			f->moveTab[f->set][kMoveSlotKnockOut].available = 0;

		uint16 inverse = (flag == 0) ? 1 : 0;
		f->moveTab[f->set][kMoveSlotJumpBack].available = inverse;
		f->moveTab[f->set][kMoveSlotJumpForward].available = inverse;

		if (flag == 0) {
			f->crouching = 0;
			f->posture = 0;
			f->remap[1] = 1;
			f->remap[10] = 0x0E;
			f->remap[8] = 0x0C;
			f->remap[9] = 0x0D;
			f->remap[5] = 8;
			f->remap[4] = 7;
			f->remap[14] = 0x15;
			f->remap[15] = 0x16;
			f->remap[18] = 0x1A;
			f->remap[0x43] = 0x4D;
			f->remap[0x44] = 0x4E;
			f->remap[2] = 3;
			f->remap[3] = 5;
			f->remap[0x3A] = 0x44;
			f->remap[0x39] = 0x43;
			f->remap[0x3D] = 0x47;
			f->remap[6] = 0x0A;
			f->remap[7] = 0x0B;
			f->remap[0x49] = 0x5C;
			f->remap[0x4A] = 0x5D;
		} else {
			f->crouching = 1;
			f->posture = 2;
			f->remap[1] = 2;
			f->remap[10] = 0x0F;
			f->remap[8] = 0x10;
			f->remap[9] = 0x11;
			f->remap[5] = 9;
			f->remap[4] = 0x56;
			f->remap[14] = 0x50;
			f->remap[15] = 0x17;
			f->remap[18] = 0x17;
			f->remap[0x43] = 0x50;
			f->remap[0x44] = 0x50;
			f->remap[2] = 0x50;
			f->remap[3] = 0x50;
			f->remap[0x3A] = 0x50;
			f->remap[0x39] = 0x50;
			f->remap[0x3D] = 0x50;
			f->remap[6] = 0x5A;
			f->remap[7] = 0x5B;
			f->remap[0x49] = 0x5E;
			f->remap[0x4A] = 0x5F;
		}
	}

	updateDistance(f);
	updateWallFlags(f);
}

uint16 BattlePlugin::autoTurn(Fighter *f) {
	if (_ctx->aiEngaged == 0 || _ctx->fightOver != 0)
		return 0;

	Fighter *opponent = f->opponent;
	if (opponent->autoTurn != 0 && opponent->side != opponent->facing) {
		if (opponent->move == kMoveSlotIdle || opponent->state == kStateGuard) {
			if (opponent->state == kStateGuard) {
				opponent->state = kStateNormal;
				byte previousMove = opponent->move;
				forceMove(opponent, kMoveSlotTurn);
				opponent->queued = previousMove;
			} else {
				forceMove(opponent, kMoveSlotTurn);
			}
		}
	}

	if (f->autoTurn != 0 && f->side != f->facing) {
		forceMove(f, kMoveSlotTurn);
		return 1;
	}

	return 0;
}

} // End of namespace Topgun
