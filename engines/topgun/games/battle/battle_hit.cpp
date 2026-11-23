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

uint16 BattlePlugin::hitAttack(uint16 id) {
	if (_ctx->fightOver != 0)
		return 0;

	Fighter *attacker = (_ctx->fighters[0].spriteId == id) ? &_ctx->fighters[0] : &_ctx->fighters[1];
	Fighter *defender = attacker->opponent;

	if (_ctx->phase == kPhaseWalkIn)
		return hitPointTest(attacker);

	if (defender->inFight == 0)
		return 0;

	uint16 frameIds[4] = { 0, 0, 0, 0 };
	_host->getSpriteFrameIds(attacker->sprite, frameIds);
	uint16 frame = (frameIds[1] != 0) ? frameIds[1] : frameIds[0];

	const HitBox *hit = attacker->hitTab[attacker->set];
	if (hit == nullptr) {
		warning("BattlePlugin::hitAttack(): fighter %d has no hit box list for set %d", attacker->player, attacker->set);
		return 0;
	}

	for (;;) {
		if (hit->frameId == 0xFFFF)
			return 0;
		if (hit->frameId == frame)
			break;
		hit++;
	}

	if (hit->laneRel != 2) {
		if (hit->laneRel == 0 && attacker->z != defender->z)
			return 0;
		if (hit->laneRel < 0 && attacker->z < defender->z)
			return 0;
		if (hit->laneRel > 0 && attacker->z > defender->z)
			return 0;
	}

	Common::Rect hitBox;
	_host->getSpriteRect(attacker->sprite, hitBox);
	if (attacker->facing == hit->baseFacing) {
		hitBox.left += hit->x;
		hitBox.right = hitBox.left + hit->width;
	} else {
		hitBox.right -= hit->x;
		hitBox.left = hitBox.right - hit->width;
	}
	hitBox.top += hit->y;
	hitBox.bottom = hitBox.top + hit->height;

	Common::Rect hurtBox = defender->rectTab[defender->posture * 2 + defender->facing];

	Common::Rect defenderRect;
	_host->getSpriteRect(defender->sprite, defenderRect);
	hurtBox.translate((int16)(defenderRect.left + defenderRect.right) / 2, (int16)(defenderRect.top + defenderRect.bottom) / 2);

	if (hitBox.isEmpty() || hurtBox.isEmpty())
		return 0;
	Common::Rect overlap = hitBox.findIntersectingRect(hurtBox);
	if (overlap.isEmpty())
		return 0;

	if (!_host->spritesCollide(attacker->sprite, defender->sprite, nullptr))
		return 0;

	uint16 damageClass = hit->damageClass;
	uint16 set = defender->set * 2;
	if (defender->state >= kReactionTableCount) {
		warning("BattlePlugin::hitAttack(): fighter %d has the state %d with no reaction table", defender->player, defender->state);
		return 0;
	}
	const ReactionRecord *table = defender->reactTab[defender->state];

	if (defender->side != defender->facing) {
		set++;
		if (defender->state == kStateGuard)
			table = defender->reactTab[0];
		else if (defender->crouching != 0)
			damageClass = 6;
	} else if ((defender->flags & kFlagProjectile) != 0)
		return 0;

	uint16 damage = (uint16)(hit->force *attacker->damageMultiplier);
	const ReactionRecord *reaction = &table[(damageClass << 2) + set];
	bool blocked = reaction->limit > damage || (attacker->side != defender->facing && defender->move == kMoveSlotGuardDownForward);

	if (!blocked) {
		_host->callScript(kScriptChangeEnergy, defender->player, (uint16)-hit->energyLoss);
		spawnHitSpark(&overlap, hit->damageClass);

		if (defender->state == kStateGuard) {
			reaction = &defender->reactTab[0][(damageClass << 2) + set];
			defender->state = kStateNormal;
		}

		uint16 move = reaction->reactionMove;
		uint16 anim = defender->animTab[defender->remap[move]];
		if (move == kReactKnockBackSet1 || move == kReactKnockBackSet0)
			knockBackAnim(defender, anim, attacker->facing);
		else if (move == kReactPushBackSet0 || move == kReactPushBackSet1)
			pushBackAnim(defender, anim, damage, hit->damageClass, kPushFromPositions);
		else
			startReaction(defender, anim);

		if (hit->skipCount != 0)
			_host->spriteSkipMessage(attacker->sprite, hit->skipCount);

		return scoreAttack(attacker, 1, attacker->move, hit->scorePoints);
	}

	if (defender->state == kStateGuard) {
		_host->callScript(kScriptChangeEnergy, defender->player, 0xFFFB);
		if (defender->move != kMoveSlotGuardDownForward) {
			uint16 anim = defender->animTab[defender->remap[reaction->reactionMove]];
			if (reaction->reactionMove == kReactKnockBackSet1)
				knockBackAnim(defender, anim, attacker->facing);
			else
				hitStunAnim(defender, anim);
		}
		aiEvent(defender, kAiEventHitBlocked);
	}

	if (hit->skipCount != 0)
		_host->spriteSkipMessage(attacker->sprite, hit->skipCount);

	return 1;
}

uint16 BattlePlugin::hitButton(uint16 id) {
	if (hitAttack(id))
		return 1;

	// The original returns 0 here when the instance address is 0xF500, which never happens
	Fighter *attacker = (_ctx->fighters[0].spriteId == id) ? &_ctx->fighters[0] : &_ctx->fighters[1];
	if (attacker->attackActive == 0)
		return 0;

	uint16 frameIds[4] = { 0, 0, 0, 0 };
	_host->getSpriteFrameIds(attacker->sprite, frameIds);
	uint16 frame = (frameIds[1] != 0) ? frameIds[1] : frameIds[0];

	const HitBox *hit = attacker->hitTab[attacker->set];
	if (hit == nullptr) {
		warning("BattlePlugin::hitButton(): fighter %d has no hit box list for set %d", attacker->player, attacker->set);
		return 0;
	}

	for (;;) {
		if (hit->frameId == 0xFFFF)
			return 0;
		if (hit->frameId == frame)
			break;
		hit++;
	}

	Common::Rect hitBox;
	_host->getSpriteRect(attacker->sprite, hitBox);
	if (attacker->facing == hit->baseFacing) {
		hitBox.left += hit->x;
		hitBox.right = hitBox.left + hit->width;
	} else {
		hitBox.right -= hit->x;
		hitBox.left = hitBox.right - hit->width;
	}
	hitBox.top += hit->y;
	hitBox.bottom = hitBox.top + hit->height;

	for (uint16 i = 0; i < _ctx->buttonCount; i++) {
		const ButtonRecord *button = &_ctx->buttons[i];

		if (!hitBox.isEmpty() && !button->rect.isEmpty() && hitBox.intersects(button->rect)) {
			_host->callScript(button->script, button->spriteId, 0);
			setAttackActive(attacker, 0);
			return 1;
		}
	}

	return 0;
}

uint16 BattlePlugin::hitFighter(uint16 a, uint16 b, uint16 id) {
	if (_ctx->fightOver != 0)
		return 0;

	_host->setScriptResult(0);

	Fighter *f = (_ctx->fighters[0].spriteId == id) ? &_ctx->fighters[0] : &_ctx->fighters[1];

	if (b != 2 && f->level != b)
		return 0;

	if (f->inFight == 0)
		return 0;

	uint16 set = f->set * 2;
	if (f->state >= kReactionTableCount) {
		warning("BattlePlugin::hitFighter(): fighter %d has the state %d with no reaction table", f->player, f->state);
		return 0;
	}
	const ReactionRecord *table = f->reactTab[f->state];

	if (f->facing == a) {
		set++;
		if (f->state == kStateGuard)
			table = f->reactTab[0];
	} else if (f->move == kMoveSlotGuardDownForward) {
		_host->callScript(kScriptChangeEnergy, f->player, 0xFFF8);
		return 0;
	}

	_host->callScript(kScriptChangeEnergy, f->player, 0xFFE7);

	Common::Rect rect;
	_host->getSpriteRect(f->sprite, rect);
	spawnHitSpark(&rect, 0);

	const ReactionRecord *reaction = &table[24 + set];
	uint16 move = reaction->reactionMove;
	uint16 anim = f->animTab[f->remap[move]];
	if (move == kReactKnockBackSet1 || move == kReactKnockBackSet0)
		knockBackAnim(f, anim, (byte)a);
	else if (move == kReactPushBackSet0 || move == kReactPushBackSet1)
		pushBackAnim(f, anim, 0x4B, 6, a);
	else
		startReaction(f, anim);

	setInFight(f, 1);
	_host->setScriptResult(1);
	return 1;
}

void BattlePlugin::startReaction(Fighter *f, uint16 anim) {
	setMoveHook(f, nullptr);
	f->specialTab = nullptr;
	setQueueGate(f->player, 1, 0);
	setContact(f, 0);
	setInFight(f, 0);
	f->speed = kSpeedNone;

	if (f->doneCallback == &BattlePlugin::cbEnterAnimDone) {
		f->doneCallback = &BattlePlugin::enterFinished;
		switchSprite(f, 0, anim, 1);
		return;
	}

	if (_ctx->fightOver == 0)
		f->doneCallback = &BattlePlugin::reactionDoneCombo;

	f->combo = 0;
	f->anim = anim;
	_ctx->pendingMoves[f->player].active = 0;
	_host->setSpriteQueue(f->sprite, anim, 0);
	aiEvent(f, kAiEventHurt);
}

void BattlePlugin::knockBackAnim(Fighter *f, uint16 anim, uint16 dir) {
	int16 knockDistance = (int16)f->knockTab[f->set];
	uint16 *host = _host->variableTable();

	int16 distance = clampMove(f, (dir == 0) ? knockDistance : (int16)-knockDistance, 1);

	if (f->level == 1 && (int16)(f->x + distance) < _ctx->bounds[1].left) {
		Common::Rect rect;
		_host->getSpriteRect(f->sprite, rect);
		int16 centerX = (int16)(rect.left + rect.right) / 2;

		host[f->varList[2]] = (uint16)(centerX + (int16)(2 * distance) / 3);
		host[f->varList[0]] = (uint16)(centerX + distance);
		host[f->varList[3]] = (uint16)_ctx->bounds[0].y;
		host[f->varList[1]] = 0;
		anim = f->animTab[f->remap[_knockAnimations[f->set]]];
	} else {
		host[f->varList[1]] = 0;
		host[f->varList[0]] = (uint16)distance;
	}

	f->lastAnim = anim;
	if (f->state == kStateGuard)
		f->state = kStateNormal;

	startReaction(f, anim);
	if (f->doneCallback == &BattlePlugin::reactionDoneCombo)
		f->doneCallback = &BattlePlugin::reactionDoneKnock;
}

void BattlePlugin::pushBackAnim(Fighter *f, uint16 anim, uint16 amount, uint16 p, uint16 mode) {
	Fighter *opponent = f->opponent;
	int16 distance = (int16)((opponent->set - f->set) * 40 + amount + 100);

	Common::Point position;
	Common::Point opponentPosition;
	_host->getSpritePosition(f->sprite, position);
	_host->getSpritePosition(opponent->sprite, opponentPosition);

	if ((mode == kPushFromPositions && opponentPosition.x > position.x) || mode == 1)
		distance = -distance;

	f->x = position.x;
	distance = clampMove(f, distance, 0);

	int16 offsetY;
	if (p == 1)
		offsetY = -60;
	else if (p == 0)
		offsetY = -20;
	else
		offsetY = 0;

	uint16 *host = _host->variableTable();

	if ((f->level == 1 && (int16)(distance + f->x) < _ctx->bounds[1].left) || f->move == kMoveSlotLevelDown || f->move == kMoveSlotAirAttack) {
		host[f->varList[1]] = 0;
		anim = f->animTab[f->remap[_knockAnimations[f->set]]];
	} else {
		host[f->varList[1]] = (uint16)f->y;
	}

	host[f->varList[3]] = (uint16)(position.y + offsetY - (amount >> 1));
	host[f->varList[2]] = (uint16)(position.x + (int16)(2 * distance) / 3);
	host[f->varList[0]] = (uint16)(position.x + distance);

	f->lastAnim = anim;
	startReaction(f, anim);
	if (f->doneCallback == &BattlePlugin::reactionDoneCombo)
		f->doneCallback = &BattlePlugin::reactionDoneKnock;
}

void BattlePlugin::hitStunAnim(Fighter *f, uint16 anim) {
	f->combo = 0;
	f->anim = anim;
	_host->setSpriteQueue(f->sprite, anim, 0);

	if (f->opponent->ai != 0)
		aiEvent(f->opponent, kAiEventOpponentStunned);
}

uint16 BattlePlugin::clashPush(Fighter *d, Fighter *s, uint16 v) {
	if (_ctx->aiEngaged == 0)
		return v;

	uint16 defense = (d->set == 0) ? 10 : 25;
	if (d->speed == kSpeedSlow)
		defense += 10;

	uint16 offense = (s->set != 0) ? 15 : 0;
	if (s->speed != kSpeedNone)
		offense += (s->speed == kSpeedWalk) ? 10 : 20;
	if (s->state == kStateGuard)
		offense += 100;
	if (s->state == 2)
		offense += 100;
	if (s->atWall != 0)
		offense += 200;
	if (s->state == kStateNormal && s->move != kMoveSlotIdle)
		offense += 50;

	uint16 *host = _host->variableTable();
	host[d->varList[1]] = 0;

	int16 sourceDistance;
	int16 defenderDistance;
	if (defense > offense && s->inFight != 0) {
		sourceDistance = (int16)(d->walkStep - _ctx->gapMinusMargins);
		defenderDistance = d->walkStep;
	} else {
		sourceDistance = 0;
		defenderDistance = _ctx->gapMinusMargins;
	}

	if (d->side == 1) {
		sourceDistance = -sourceDistance;
		defenderDistance = -defenderDistance;
	}

	int16 sourceMoved = clampMove(s, sourceDistance, 1);
	defenderDistance = (int16)(defenderDistance + (int16)(sourceMoved - sourceDistance));
	host[d->varList[0]] = (uint16)clampMove(d, defenderDistance, 1);

	if (sourceMoved != 0) {
		host[s->varList[0]] = (uint16)sourceMoved;
		host[s->varList[1]] = 0;
		s->combo = 0;
		s->anim = s->animTab[s->remap[_clashSourceAnimations[s->posture]]];
		_host->setSpriteQueue(s->sprite, s->anim, 0);
	}

	d->lastAnim = d->animTab[d->remap[_clashDefenderAnimations[s->set + 2 * d->posture]]];
	return d->lastAnim;
}

uint16 BattlePlugin::hitPointTest(Fighter *a) {
	if (a->posture != 2)
		return 0;

	uint16 *host = _host->variableTable();
	uint16 count = host[kVarHitPointCount];
	if (count == 0)
		return 0;

	uint16 frameIds[4] = { 0, 0, 0, 0 };
	_host->getSpriteFrameIds(a->sprite, frameIds);
	uint16 frame = (frameIds[1] != 0) ? frameIds[1] : frameIds[0];

	const HitBox *hit = a->hitTab[a->set];
	if (hit == nullptr) {
		warning("BattlePlugin::hitPointTest(): fighter %d has no hit box list for set %d", a->player, a->set);
		return 0;
	}

	for (;;) {
		if (hit->frameId == 0xFFFF)
			return 0;
		if (hit->frameId == frame)
			break;
		hit++;
	}

	Common::Rect rect;
	_host->getSpriteRect(a->sprite, rect);
	if (a->facing == hit->baseFacing) {
		rect.left += hit->x;
		rect.right = rect.left + hit->width;
	} else {
		rect.right -= hit->x;
		rect.left = rect.right - hit->width;
	}
	rect.top += hit->y;
	rect.bottom = hit->height + rect.top + 0x28;

	// Operands are encoded literals: the first sprite is 9593 and the script is 9574
	for (uint16 handle = kHitPointSpriteBase; count > 0; handle++, count--) {
		Common::Point point;
		_host->getSpritePosition(handle, point);
		if (rect.contains(point.x, point.y)) {
			uint16 value = (handle > kLastOperandIndex) ? (uint16)(handle + kLiteralBias) : host[handle];
			_host->callScript(kScriptHitPointHit, value, 0);
			return 1;
		}
	}

	return 0;
}

} // End of namespace Topgun
