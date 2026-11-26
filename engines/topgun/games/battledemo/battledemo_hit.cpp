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

uint16 BattleDemoPlugin::hitAttack(uint16 id) {
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
	if (!hit) {
		warning("BattleDemoPlugin::hitAttack(): fighter %d has no hit box list for set %d", attacker->player, attacker->set);
		return 0;
	}

	for (;;) {
		if (hit->frameId == 0xFFFF)
			return 0;
		if (hit->frameId == frame)
			break;
		hit++;
	}

	if (hit->laneRel == 0 && attacker->lane != defender->lane)
		return 0;
	if (hit->laneRel == 0 && attacker->level != defender->level)
		return 0;
	if (hit->laneRel > 0 && defender->lane < 0)
		return 0;
	if (hit->laneRel < 0 && defender->lane > 0)
		return 0;

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
		warning("BattleDemoPlugin::hitAttack(): fighter %d has the state %d with no reaction table", defender->player, defender->state);
		return 0;
	}
	const ReactionRecord *table = defender->reactTab[defender->state];

	if (attacker->side == defender->facing) {
		set++;
		if (defender->state == kStateGuard)
			table = defender->reactTab[0];
		else if (defender->crouching != 0)
			damageClass = 6;
	}

	uint16 damage = (uint16)(hit->force * attacker->damageMultiplier);
	uint16 index = (uint16)((damageClass << 2) + set);
	if (index >= kReactionTableSize) {
		warning("BattleDemoPlugin::hitAttack(): invalid reaction record %d", index);
		return 0;
	}
	const ReactionRecord *reaction = &table[index];
	bool blocked = reaction->limit > damage || (attacker->side != defender->facing && defender->move == kMoveSlotGuardDownForward);

	if (!blocked) {
		_host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_CHANGE_ENERGY, defender->player, (uint16)-hit->energyLoss);
		spawnHitSpark(&overlap, hit->damageClass);

		if (defender->state == kStateGuard) {
			reaction = &defender->reactTab[0][index];
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

		return scoreAttack(attacker, 1, 0, 0);
	}

	if (defender->state == kStateGuard) {
		_host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_CHANGE_ENERGY, defender->player, 0xFFFB);
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

	return 0;
}

uint16 BattleDemoPlugin::hitFighter(uint16 a, uint16 b, uint16 id) {
	if (_ctx->fightOver != 0)
		return 0;

	_host->setScriptResult(0);

	Fighter *f = (_ctx->fighters[0].spriteId == id) ? &_ctx->fighters[0] : &_ctx->fighters[1];

	if (f->level != b)
		return 0;

	if (f->inFight == 0)
		return 0;

	uint16 set = f->set * 2;
	if (f->state >= kReactionTableCount) {
		warning("BattleDemoPlugin::hitFighter(): fighter %d has the state %d with no reaction table", f->player, f->state);
		return 0;
	}
	const ReactionRecord *table = f->reactTab[f->state];

	if (f->facing == a) {
		set++;
		if (f->state == kStateGuard)
			table = f->reactTab[0];
	} else if (f->move == kMoveSlotGuardDownForward) {
		_host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_CHANGE_ENERGY, f->player, 0xFFF8);
		return 0;
	}

	_host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_CHANGE_ENERGY, f->player, 0xFFCE);

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

uint16 BattleDemoPlugin::hitStub(uint16 id) {
	return 0;
}

void BattleDemoPlugin::startReaction(Fighter *f, uint16 anim) {
	setMoveHook(f, nullptr);
	f->specialTab = nullptr;
	setQueueGate(f->player, 1, 0);
	setContact(f, 0);
	setInFight(f, 0);
	f->speed = kSpeedNone;

	if (f->doneCallback == &BattlePlugin::cbEnterAnimDone) {
		f->doneCallback = &BattlePlugin::enterFinished;
		switchSprite(f, 1, anim, 1);
		return;
	}

	if (_ctx->fightOver == 0)
		f->doneCallback = &BattlePlugin::reactionDoneCombo;

	f->combo = 0;
	_host->setSpriteQueue(f->sprite, anim, 0);
	aiEvent(f, kAiEventHurt);
}

void BattleDemoPlugin::pushBackAnim(Fighter *f, uint16 anim, uint16 amount, uint16 p, uint16 mode) {
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

	if (f->level == 1 && (int16)(distance + f->x) < _ctx->bounds[1].left) {
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

void BattleDemoPlugin::hitStunAnim(Fighter *f, uint16 anim) {
	f->combo = 0;
	_host->setSpriteQueue(f->sprite, anim, 0);
}

uint16 BattleDemoPlugin::clashPush(Fighter *d, Fighter *s, uint16 v) {
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
	if (s->state == kStateAttack)
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
		_host->setSpriteQueue(s->sprite, s->animTab[s->remap[_clashSourceAnimations[s->posture]]], 0);
	}

	d->lastAnim = d->animTab[d->remap[_clashDefenderAnimations[s->set + 2 * d->posture]]];
	return d->lastAnim;
}

uint16 BattleDemoPlugin::hitPointTest(Fighter *a) {
	if (a->posture != 2)
		return 0;

	uint16 frameIds[4] = { 0, 0, 0, 0 };
	_host->getSpriteFrameIds(a->sprite, frameIds);
	uint16 frame = (frameIds[1] != 0) ? frameIds[1] : frameIds[0];

	const HitBox *hit = a->hitTab[a->set];
	if (!hit) {
		warning("BattleDemoPlugin::hitPointTest(): fighter %d has no hit box list for set %d", a->player, a->set);
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
	rect.bottom = hit->height + rect.top;

	// Only the last sprite's test counts
	uint16 result = 0;
	for (uint16 sprite = TG_BATTLE_V1_2_2_1_HIT_POINT_FIRST_SPRITE; sprite <= TG_BATTLE_V1_2_2_1_HIT_POINT_LAST_SPRITE; sprite++) {
		Common::Point point;
		_host->getSpritePosition(sprite, point);

		result = 0;
		if (rect.contains(point.x, point.y))
			result = _host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_HIT_POINT, (uint16)(sprite + TG_BATTLE_V1_2_2_1_LITERAL_BIAS), 0);
	}

	return result;
}

void BattleDemoPlugin::loadStats(Fighter *f) {
	uint16 *host = _host->variableTable();

	byte opponent = (f->player == 0) ? 1 : 0;

	f->fightAttacks = 0;
	f->fightHits = 0;
	f->fightMaxEnergy += host[TG_BATTLE_V1_2_2_1_VAR_MAX_ENERGY + opponent];
	f->fightDamage = 0;
}

void BattleDemoPlugin::storeStats(Fighter *f) {
	uint16 *host = _host->variableTable();
	byte opponent = (f->player == 0) ? 1 : 0;

	f->fightDamage += (uint16)(host[TG_BATTLE_V1_2_2_1_VAR_MAX_ENERGY + opponent] - host[TG_BATTLE_V1_2_2_1_VAR_ENERGY + opponent]);
	f->totalMaxEnergy += f->fightMaxEnergy;
	f->totalDamage += f->fightDamage;

	uint16 rating = 0;
	if (f->fightMaxEnergy == 0 || f->totalMaxEnergy == 0) {
		warning("BattleDemoPlugin::storeStats(): fighter %d has no maximum energy", f->player);
	} else {
		uint32 damagePerHit = (f->totalHits != 0) ? f->totalDamage / f->totalHits : 0;
		rating = (uint16)((f->totalDamage * 100 / f->totalMaxEnergy) * damagePerHit);
	}

	if (f->player == 0) {
		host[TG_BATTLE_V1_2_2_1_VAR_STAT_ATTACKS0] = (uint16)f->totalAttacks;
		host[TG_BATTLE_V1_2_2_1_VAR_STAT_HITS0] = (uint16)f->totalHits;
		host[TG_BATTLE_V1_2_2_1_VAR_STAT_RATING0] = rating;
	} else {
		host[TG_BATTLE_V1_2_2_1_VAR_STAT_ATTACKS1] = (uint16)f->totalAttacks;
		host[TG_BATTLE_V1_2_2_1_VAR_STAT_HITS1] = (uint16)f->totalHits;
		host[TG_BATTLE_V1_2_2_1_VAR_STAT_RATING1] = rating;
	}
}

uint16 BattleDemoPlugin::scoreAttack(Fighter *f, uint16 hit, uint16 move, uint16 value) {
	setAttackActive(f, 0);

	f->fightAttacks++;
	f->totalAttacks++;

	if (hit != 0) {
		f->fightHits++;
		f->totalHits++;
		_ctx->lastScorer = f->player;

		if (g_system->getMillis() > f->comboEnd)
			f->combo = 0;

		f->combo = (f->combo << 8) | f->move;

		for (const ComboRecord *record = f->comboTab; record->pattern != 0xFFFFFFFF; record++) {
			if ((record->mask & f->combo) == record->pattern) {
				uint16 code;
				if (record->mask == 0x000000FF)
					code = 1;
				else if (record->mask == 0x0000FFFF)
					code = 2;
				else if (record->mask == 0x00FFFFFF)
					code = 3;
				else
					code = 4;

				_host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_COMBO, f->player, code);
				break;
			}
		}
	}

	if (hit == 0)
		f->combo = 0;

	aiEvent(f, hit == 0 ? kAiEventAttackMissed : kAiEventHitLanded);
	return hit;
}

} // End of namespace Topgun
