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

#include "topgun/games/battledemo/battledemo.h"

namespace Topgun {

void BattleDemoPlugin::playAnim(Fighter *f, uint16 anim) {
	setQueueGate(f->player, 1, 0);
	_host->setSpriteQueue(f->sprite, anim, 0);
}

void BattleDemoPlugin::startKnockdown(Fighter *f) {
	uint16 *host = _host->variableTable();

	if (f->speed != kSpeedNone)
		setMoveHook(f, nullptr);

	if (f->state != kStateFinal)
		f->doneCallback = &BattlePlugin::endMove;

	f->state = kStateKnockedDown;
	f->anim = f->animTab[f->remap[(f->set == 0) ? 0x4A : 0x4C]];
	host[f->varList[0]] = 0xFFC4;
	host[f->varList[1]] = _ctx->bounds[1].y;
	_host->setSpriteQueue(f->sprite, f->anim, 0);
}

void BattleDemoPlugin::resetFighterState(Fighter *f) {
	setMoveHook(f, nullptr);

	f->specialTab = nullptr;
	f->state = kStateNormal;
	f->speed = kSpeedNone;
	f->damageMultiplier = 1;
	f->queued = 0;
	f->doneCallback = &BattlePlugin::endMove;

	setInFight(f, 1);
}

void BattleDemoPlugin::switchSprite(Fighter *f, uint16 k, uint16 anim, uint16 v) {
	uint16 *host = _host->variableTable();

	uint16 old = f->sprite;
	f->sprite = f->spriteTab[(byte)k];

	if (f->sprite == old) {
		_host->setSpriteQueue(old, anim, 0);
		return;
	}

	if (f->sprite > TG_BATTLE_V1_2_3_1_LAST_OPERAND_INDEX)
		f->spriteId = (uint16)(f->sprite + TG_BATTLE_V1_2_3_1_LITERAL_BIAS);
	else
		f->spriteId = host[f->sprite];

	host[TG_BATTLE_V1_2_3_1_VAR_BUSY_COUNTER]++;
	_host->loadResource(f->sprite);
	host[TG_BATTLE_V1_2_3_1_VAR_BUSY_COUNTER]--;

	_host->setSpriteLevel(f->sprite, f->z);
	_host->transferSprite(old, f->sprite, anim, (byte)v, 0);
}

uint16 BattleDemoPlugin::jumpMove(Fighter *f, uint16 anim, uint16 flip, int16 dx, int16 dy) {
	uint16 *host = _host->variableTable();

	f->doneCallback = &BattlePlugin::cbJumpEnd;

	int16 distance = (int16)(_jumpOffsets[f->set][1] + dx);
	if ((uint16)f->facing != flip)
		distance = -distance;

	distance = clampMove(f, distance, 0);

	Common::Rect rect;
	_host->getSpriteRect(f->sprite, rect);

	int16 y = (int16)((int16)(rect.top + rect.bottom) / 2 - f->yTab[f->set] + dy);

	if (f->level == 1)
		y = (int16)(y - _ctx->bounds[0].y);

	host[f->varList[2]] = (uint16)((int16)(distance * 2) / 3);
	host[f->varList[0]] = (uint16)distance;
	host[f->varList[3]] = (uint16)y;
	host[f->varList[1]] = 0;

	f->lastAnim = anim;
	playAnim(f, anim);
	setContact(f, 1);
	return 1;
}

uint16 BattleDemoPlugin::moveWallBounce(Fighter *f, uint16 move, uint16 anim) {
	uint16 *host = _host->variableTable();

	f->state = kStateAttack;
	f->doneCallback = &BattlePlugin::reactionDone;

	int16 dx = 0;
	const ArenaBounds *bounds = &_ctx->bounds[(move == kMoveSlotLevelUp) ? 1 : 0];

	if (bounds->left > f->x)
		dx = (int16)(bounds->left - f->x);
	else if (bounds->right < f->x)
		dx = (int16)(bounds->right - f->x);

	host[f->varList[0]] = (uint16)dx;
	host[f->varList[1]] = (uint16)-bounds->y;

	f->lastAnim = anim;
	playAnim(f, anim);
	return 1;
}

uint16 BattleDemoPlugin::moveSidestep(Fighter *f, uint16 move, uint16 anim) {
	uint16 *host = _host->variableTable();

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
		warning("BattleDemoPlugin::moveSidestep(): invalid level %d", f->level);

	f->moveTab[f->set][kMoveSlotSidestepMinus].available = (bounds->sidestepMinusLimit < lane) ? 1 : 0;
	f->moveTab[f->set][kMoveSlotSidestepPlus].available = (bounds->sidestepPlusLimit > lane) ? 1 : 0;

	f->z = (uint16)(f->z + step);

	Fighter *opp = f->opponent;
	_ctx->aiEngaged = (opp->level == f->level && opp->lane == f->lane) ? 1 : 0;

	_host->setSpriteLevel(f->sprite, f->z);
	f->lastAnim = anim;
	playAnim(f, anim);
	return 1;
}

uint16 BattleDemoPlugin::moveProjectile(Fighter *f, uint16 move, uint16 anim) {
	uint16 *host = _host->variableTable();
	Fighter *opp = f->opponent;

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
		_host->addCollisionPair(f->handlerParam, opp->sprite, TG_BATTLE_V1_2_3_1_HOOK_WEAPONS_SPECIAL_HIT, f->player, f->handlerParam, 0);

	moveAttack(f, (byte)move, anim);
	return 1;
}

uint16 BattleDemoPlugin::moveKnockOut(Fighter *f, uint16 move, uint16 anim) {
	setQueueGate(f->player, 1, 0);

	_host->callScript(TG_BATTLE_V1_2_3_1_SCRIPT_CHANGE_ENERGY, f->player, 0xFFFB);

	if (f->state != kStateFinal) {
		f->doneCallback = &BattlePlugin::cbGetUp;
		startDeath(f, anim);
	} else if (!f->doneCallback) {
		warning("BattleDemoPlugin::moveKnockOut(): fighter %d has no done callback", f->player);
	} else {
		(this->*f->doneCallback)(f);
	}

	return 1;
}

void BattleDemoPlugin::cbGetUp(Fighter *f) {
	uint16 index;

	if (f->set == 0) {
		setStanceSet(f, 1, 0);
		index = f->remap[20];
	} else {
		setStanceSet(f, 0, 0);
		index = f->remap[1];
	}

	uint16 anim = f->animTab[index];

	f->state = kStateNormal;
	f->z = (uint16)(f->z - f->lane);
	f->y = (int16)(f->y - 3 * f->lane);
	f->lane = 0;
	f->doneCallback = &BattlePlugin::endMove;

	Fighter *opponent = f->opponent;
	_ctx->aiEngaged = (opponent->level == f->level && opponent->lane == f->lane) ? 1 : 0;

	_host->setSpritePosition(f->sprite, (uint16)f->x, (uint16)f->y);
	switchSprite(f, 1, anim, 1);
	separateFighters(f);
	_host->callScript(TG_BATTLE_V1_2_3_1_SCRIPT_SET_CHANGED, f->player, f->set);
	updateDistance(f);
	updateWallFlags(f);
	setQueueGate(f->player, 0, 0);
	if (f->ai == 0)
		setInputDevice(f->player, kInputDeviceEnable);

	endMove(f);

	uint16 *host = _host->variableTable();
	host[TG_BATTLE_V1_2_3_1_VAR_BUSY_COUNTER]--;
}

void BattleDemoPlugin::cbFallAnim(Fighter *f) {
	_host->callScript(TG_BATTLE_V1_2_3_1_SCRIPT_CHANGE_ENERGY, f->player, 0xFFFD);

	if (f->state == kStateFinal) {
		if (!f->doneCallback)
			warning("BattleDemoPlugin::cbFallAnim(): fighter %d has no done callback", f->player);
		else
			(this->*f->doneCallback)(f);

		return;
	}

	_host->setSpriteQueue(f->sprite, f->anim, 0);
	if (f->ai != 0)
		aiStartTimer(f);
}

uint16 BattleDemoPlugin::hookJump(Fighter *f, uint16 code) {
	uint16 frameIds[4] = {};

	f->doneCallback = &BattlePlugin::reactionDone;
	setMoveHook(f, nullptr);

	if ((byte)code != kDirNone)
		return 1;

	if (f->set != 1)
		return 0;

	_host->getSpriteFrameIds(f->sprite, frameIds);
	if (frameIds[0] != f->lastFrame)
		return 0;

	uint16 anim = f->animTab[f->remap[24]];
	f->anim = anim;
	playAnim(f, anim);
	return 1;
}

void BattleDemoPlugin::initAiFlags(Fighter *f) {
	uint16 *host = _host->variableTable();

	f->moveTab[f->set][19].available = host[TG_BATTLE_V1_2_3_1_VAR_PLAYER_OPTIONS + f->player] & 1;
	f->moveTab[f->set][kMoveSlotProjectile].available = host[TG_BATTLE_V1_2_3_1_VAR_PLAYER_OPTIONS + f->player] & 2;
}

void BattleDemoPlugin::setStanceSet(Fighter *f, uint16 set, uint16 flag) {
	f->set = (byte)set;

	if ((byte)set != 0) {
		f->crouching = 0;
		f->posture = 1;
	} else {
		f->moveTab[f->set][kMoveSlotCrouch].available = flag;
		f->moveTab[f->set][21].available = flag;

		uint16 inverse = (flag == 0) ? 1 : 0;
		f->moveTab[f->set][kMoveSlotKnockOut].available = inverse;
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
			f->remap[0x49] = 0x5D;
			f->remap[0x4A] = 0x5E;
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
			f->remap[6] = 0x5B;
			f->remap[7] = 0x5C;
			f->remap[0x49] = 0x5F;
			f->remap[0x4A] = 0x60;
		}
	}

	updateDistance(f);
	updateWallFlags(f);
}

} // End of namespace Topgun
