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

void BattlePlugin::aiSetupSkill(Fighter *f) {
	const uint16 *host = _host->variableTable();
	AiBlock *a = &f->aiBlock;
	AiRecord *rec;

	a->confidence = 0;
	if (host[kVarDifficulty] <= 3 && host[kVarHumanPlayers] != 0) {
		a->maxReactionDelay = 1000;
		a->reactionDelay = 1000;
		a->reactionDelayStep = 200;
		a->reuseCooldown = 12;
		a->highConfidenceLimit = 1;
		a->lowConfidenceLimit = -3;
	} else if (host[kVarDifficulty] <= 6 && host[kVarHumanPlayers] != 0) {
		a->maxReactionDelay = 400;
		a->reactionDelay = 400;
		a->reactionDelayStep = 150;
		a->reuseCooldown = 6;
		a->highConfidenceLimit = 2;
		a->lowConfidenceLimit = -2;
	} else {
		a->maxReactionDelay = 100;
		a->reactionDelay = 100;
		a->reactionDelayStep = 100;
		a->reuseCooldown = 1;
		a->highConfidenceLimit = 3;
		a->lowConfidenceLimit = -1;
	}

	a->windowBit = 0x80;
	a->strategy = 0;
	a->selectionCount = 0;
	a->repeatCount = 0;
	a->outcomeHistory = 0;
	a->windowHits = 0;
	a->timerArmed = 0;
	a->waiting = 0;
	a->unk_1f = 0;

	for (rec = a->list; !(rec->move == 0xFF && rec->move2 == 0xFF && rec->moves34 == 0xFFFF); rec++) {
		rec->lastUsedSeq = 0;
		rec->missCount = 0;
		rec->hitCount = 0;
	}

	for (rec = _aiLists0472[0]; !(rec->move == 0xFF && rec->move2 == 0xFF && rec->moves34 == 0xFFFF); rec++) {
		rec->lastUsedSeq = 0;
		rec->missCount = 0;
		rec->hitCount = 0;
	}
	for (int i = 0; i < 5; i++)
		_aiLists0472[1][i] = _aiLists0472[0][i];

	for (rec = _aiLists01DE[0]; !(rec->move == 0xFF && rec->move2 == 0xFF && rec->moves34 == 0xFFFF); rec++) {
		rec->lastUsedSeq = 0;
		rec->missCount = 0;
		rec->hitCount = 0;
	}
	for (int i = 0; i < 10; i++)
		_aiLists01DE[1][i] = _aiLists01DE[0][i];

	_aiList0166[0].lastUsedSeq = 0;
	_aiList0166[0].missCount = 0;
	_aiList0166[0].hitCount = 0;
	_aiList0166[1] = _aiList0166[0];

	_aiList01A2[0].lastUsedSeq = 0;
	_aiList01A2[0].missCount = 0;
	_aiList01A2[0].hitCount = 0;
	_aiList01A2[1] = _aiList01A2[0];

	aiSelectMove(f, &_aiList01A2[f->player]);

	a->delayQueue = 0;
	a->switchScore = 0;
	f->autoTurn = 1;
}

void BattlePlugin::aiEvent(Fighter *f, uint16 event) {
	if (f->ai == 0)
		return;

	AiBlock *a = &f->aiBlock;

	switch (event) {
	case kAiEventMoveEnded:
		aiTimerStep(f);
		break;
	case kAiEventOpponentAttacking:
		aiReact(f, f->opponent);
		break;
	case kAiEventOpponentVulnerable:
		if (a->waiting == 0)
			return;
		aiTimerStep(f);
		break;
	case kAiEventOpponentRepositioned:
		aiChooseNext(f);
		break;
	case kAiEventHitBlocked:
		a->confidence += _aiStrategyTable[a->strategy].successDelta;
		if (a->waiting != 0 || f->state == kStateGuard || f->move == kMoveSlotCrouch || f->move == kMoveSlotIdle)
			aiRespond(f);
		break;
	case kAiEventHurt:
		a->confidence += _aiStrategyTable[a->strategy].failureDelta;
		if (a->outcomeHistory & a->windowBit)
			a->windowHits--;
		a->outcomeHistory <<= 1;
		if (a->reactionDelay < a->reactionDelayStep)
			a->reactionDelay = 0;
		else
			a->reactionDelay -= a->reactionDelayStep;
		a->moveQueue = 0;
		a->waiting = 0;
		a->unk_1f = 0;
		a->timerArmed = 0;
		_host->cancelScriptTimer(f->timerId);
		aiUpdateStrategy(a);
		break;
	case kAiEventHitLanded:
		a->confidence += _aiStrategyTable[a->strategy].successDelta;
		if (!(a->outcomeHistory & a->windowBit))
			a->windowHits++;
		a->outcomeHistory = (a->outcomeHistory << 1) | 1;
		a->reactionDelay += a->reactionDelayStep;
		if (a->maxReactionDelay < a->reactionDelay)
			a->reactionDelay = a->maxReactionDelay;
		aiUpdateStrategy(a);
		if (a->strategy == 4 && a->moveQueue == 0)
			aiFollowUp(f);
		break;
	case kAiEventAttackMissed:
		a->confidence += _aiStrategyTable[a->strategy].failureDelta;
		if (a->currentRecord == nullptr)
			warning("BattlePlugin::aiEvent(): no current AI record");
		else
			a->currentRecord->missCount++;
		break;
	case kAiEventFightWon:
	case kAiEventFightLost:
		a->moveQueue = 0;
		a->timerArmed = 0;
		_host->cancelScriptTimer(f->timerId);
		f->ai = 0;
		break;
	case kAiEventPushed:
		a->moveQueue = 0;
		a->timerArmed = 0;
		_host->cancelScriptTimer(f->timerId);
		break;
	case kAiEventOpponentStunned:
		a->confidence += _aiStrategyTable[a->strategy].opponentStunnedDelta;
		break;
	default:
		break;
	}
}

void BattlePlugin::aiUpdateStrategy(AiBlock *a) {
	if (a->confidence <= a->lowConfidenceLimit) {
		if ((a->outcomeHistory & 7) == 0)
			a->strategy = 0;
		else if (a->outcomeHistory & 1)
			a->strategy = 1;
		else
			a->strategy = 2;
	} else if (a->confidence > a->highConfidenceLimit) {
		a->strategy = ((a->outcomeHistory & 7) == 1) ? 6 : 0;
	} else {
		a->strategy = ((a->outcomeHistory & 0xF) == 1) ? 5 : 0;
	}
}

void BattlePlugin::aiReact(Fighter *self, Fighter *opp) {
	uint32 t = g_system->getMillis();
	AiRecord *rec = nullptr;
	int32 best = 0;

	Fighter *other = self->opponent;
	if (!(other->side == other->facing || opp->move == 0x17))
		return;

	moveClass(opp->move);

	switch (opp->move) {
	case 9:
	case 10:
	case 18:
	case 20:
		if (_ctx->aiEngaged == 0)
			return;
		if (_ctx->centerDistance > 0xB4)
			return;
		goto list0472Two;
	case 17:
		if (_ctx->aiEngaged == 0)
			return;
		if (_ctx->edgeGap >= 0x96) {
			switch (t & 3) {
			case 0:
				goto list0472Zero;
			case 1:
				goto list0472Two;
			case 2:
				goto list01DEEight;
			case 3:
				if (self->level != 1)
					goto list01DEEight;
				rec = &_aiLists01DE[self->player][4];
				break;
			}
			break;
		}
		if (t & 1)
			goto list0472Three;
		rec = &_aiLists0472[self->player][1];
		break;
	case 19:
		if (_ctx->aiEngaged == 0)
			return;
		if (_ctx->centerDistance > 0x12C)
			return;
		switch (t & 3) {
		case 0:
			goto list0472Zero;
		case 1:
			goto list0472Three;
		case 2:
			rec = &_aiLists01DE[self->player][7];
			break;
		case 3:
			goto pickFromOwnList;
		}
		break;
	case 21:
		if (_ctx->aiEngaged == 0)
			return;
		if (_ctx->centerDistance > 0x96)
			return;
		if (t & 1)
			goto list0472Two;
		goto pickFromOwnList;
	case 22:
	case 23:
		if (_ctx->aiEngaged == 0)
			return;
		if (_ctx->centerDistance > 0xC8)
			return;
		goto randomPick;
	case 24:
		switch (other->type) {
		case kFighterSparky:
		case kFighterKuli:
			if (_ctx->aiEngaged == 0)
				return;
			if (_ctx->centerDistance > 0x12C)
				return;
			// fall through
		case kFighterVermin:
		case kFighterNicky:
			if (_ctx->aiEngaged == 0)
				return;
			if (t & 1)
				goto list0472Three;
			goto pickFromOwnList;
		case kFighterTor:
			if (_ctx->centerDistance > 0xFA)
				return;
			// fall through
		case kFighterMortie:
		case kFighterEvilToadMan:
			if (_ctx->aiEngaged == 0)
				return;
			goto randomPick;
		default:
			break;
		}
		break;
	case 25:
		if (_ctx->aiEngaged == 0)
			return;
		if (_ctx->centerDistance > 0xC8)
			return;
		if (t & 1) {
			rec = &_aiList0436[0];
			break;
		}
		goto list01DEOne;
	case 26:
		if (_ctx->aiEngaged == 0)
			return;
		switch (t & 3) {
		case 0:
			rec = &_aiList0436[0];
			break;
		case 1:
			rec = &_aiList0436[1];
			break;
		case 2:
			goto list0472Three;
		case 3:
			goto list0472Zero;
		}
		break;
	case 28:
		if (_ctx->aiEngaged != 0)
			return;
		if (_ctx->centerDistance > 0xC8)
			return;
		if (t & 1) {
			rec = &_aiList0436[0];
			break;
		}
		goto list01DEOne;
	default:
		break;
	}
	goto selectRecord;

list01DEEight:
	rec = &_aiLists01DE[self->player][8];
	goto selectRecord;

pickFromOwnList:
	rec = aiPickMove(self, self->aiBlock.list, 1, &best);
	goto selectRecord;

list0472Two:
	rec = &_aiLists0472[self->player][2];
	goto selectRecord;

list0472Three:
	rec = &_aiLists0472[self->player][3];
	goto selectRecord;

randomPick:
	if (!(t & 1))
		goto pickFromOwnList;

list0472Zero:
	rec = &_aiLists0472[self->player][0];
	goto selectRecord;

list01DEOne:
	rec = &_aiLists01DE[self->player][1];

selectRecord:
	if (rec != nullptr)
		aiSelectMove(self, rec);
}

void BattlePlugin::aiStartTimer(Fighter *f) {
	uint32 delay;

	if (f->move == kMoveSlotGuardDownForward)
		delay = g_system->getMillis() % 150;
	else if (f->opponent->attackActive != 0)
		delay = g_system->getMillis() % 1000 + f->aiBlock.reactionDelay;
	else
		delay = g_system->getMillis() % 150 + f->aiBlock.maxReactionDelay;

	f->timerCallback = &BattlePlugin::aiTimerStep;
	f->aiBlock.waiting = 1;
	f->aiBlock.timerArmed = 0;
	_host->setScriptTimer(f->timerId, delay, f->timerScript);
}

void BattlePlugin::aiTimerStep(Fighter *f) {
	AiBlock *a = &f->aiBlock;
	uint32 delay;

	if (a->moveQueue == 0) {
		aiChooseNext(f);
		if (a->unk_1f != 0) {
			f->timerCallback = &BattlePlugin::aiTimerStep;
			a->waiting = 1;
			a->timerArmed = 0;
			delay = g_system->getMillis() % 150 + f->aiBlock.reactionDelay;
			_host->setScriptTimer(f->timerId, delay, f->timerScript);
			return;
		}
	}

	a->waiting = 0;
	bool noHigh = (a->currentFlags & kAiFlagImmediateMask) == 0;
	AiRecord *rec = a->nextRecord;
	a->currentRecord = rec;
	a->currentFlags = rec->flags;
	if (a->currentFlags & kAiFlagImmediateMask) {
		aiExecuteMove(f);
		return;
	}

	Fighter *opp = f->opponent;
	if (opp->inFight == 0) {
		a->waiting = 1;
		f->timerCallback = &BattlePlugin::aiTimerStep;
		a->timerArmed = 0;
		delay = g_system->getMillis() % 150 + f->aiBlock.reactionDelay;
		_host->setScriptTimer(f->timerId, delay, f->timerScript);
		moveIdle(f, 0, 0);
		return;
	}

	f->timerCallback = &BattlePlugin::aiPoll;
	if (noHigh && a->strategy != 1)
		delay = g_system->getMillis() % 150 + a->reactionDelay;
	else
		delay = g_system->getMillis() % 150;
	a->timerArmed = 0;
	_host->setScriptTimer(f->timerId, delay, f->timerScript);
	if (delay > 0x7D)
		moveIdle(f, 0, 0);
}

void BattlePlugin::aiSelectMove(Fighter *f, AiRecord *rec) {
	AiBlock *a = &f->aiBlock;

	if (a->timerArmed != 0) {
		a->timerArmed = 0;
		_host->cancelScriptTimer(f->timerId);
	}

	if (a->unk_1f == 0 || a->currentRecord != rec) {
		a->selectionCount++;
		a->unk_1f = 0;
		a->moveQueue = (uint32)rec->move | ((uint32)rec->move2 << 8) | ((uint32)rec->moves34 << 16);
		a->delayQueue = (uint32)rec->delays12 | ((uint32)rec->delays34 << 16);
		a->nextRecord = rec;
		rec->lastUsedSeq = a->selectionCount;
		if (f->timerCallback == &BattlePlugin::aiPoll)
			f->timerCallback = &BattlePlugin::aiAdvance;
	}
}

void BattlePlugin::aiAdvance(Fighter *f) {
	AiBlock *a = &f->aiBlock;

	a->waiting = 0;
	if (a->currentRecord == a->nextRecord)
		a->repeatCount++;
	else
		a->repeatCount = 0;

	AiRecord *rec = a->nextRecord;
	a->currentRecord = rec;
	a->moveQueue = (uint32)rec->move | ((uint32)rec->move2 << 8) | ((uint32)rec->moves34 << 16);
	a->delayQueue = (uint32)rec->delays12 | ((uint32)rec->delays34 << 16);
	a->currentFlags = rec->flags;

	if ((a->currentFlags & kAiFlagImmediateMask) || f->opponent->inFight != 0)
		aiExecuteMove(f);
	else
		aiPoll(f);
}

void BattlePlugin::aiPoll(Fighter *f) {
	if (f->opponent->inFight != 0) {
		aiExecuteMove(f);
		return;
	}

	f->timerCallback = &BattlePlugin::aiPoll;
	_host->setScriptTimer(f->timerId, 100, f->timerScript);
}

void BattlePlugin::aiExecuteMove(Fighter *f) {
	AiBlock *a = &f->aiBlock;
	byte previous = (byte)a->moveQueue;
	byte armed = a->timerArmed;
	uint32 delay;

	a->timerArmed = 0;
	_host->cancelScriptTimer(f->timerId);

	uint32 frames = a->delayQueue & 0xFF;
	if (frames != 0) {
		f->timerCallback = &BattlePlugin::aiExecuteMove;
		a->timerArmed = 1;
		delay = frames * 1000 / 24;
		_host->setScriptTimer(f->timerId, delay, f->timerScript);
	} else if (previous == 0) {
		delay = g_system->getMillis() % 150 + f->aiBlock.reactionDelay;
		f->timerCallback = &BattlePlugin::aiTimerStep;
		_host->setScriptTimer(f->timerId, delay, f->timerScript);
	}

	a->moveQueue >>= 8;
	a->delayQueue >>= 8;
	a->unk_1f = 0;

	const MoveEntry *entry = &f->moveTab[f->set][previous];
	f->anim = f->animTab[f->remap[entry->animIndex]];
	f->move = previous;
	f->handlerParam = entry->handlerParam;
	f->moveCel = a->currentRecord->cel;

	uint16 result = 0;
	if (entry->handler == nullptr)
		warning("BattlePlugin::aiExecuteMove(): move %d has no handler", f->move);
	else
		result = (this->*entry->handler)(f, f->move, f->anim);

	if (result == 0 && armed == 0) {
		a->moveQueue = 0;
		delay = g_system->getMillis() % 150;
		f->timerCallback = &BattlePlugin::aiTimerStep;
		_host->setScriptTimer(f->timerId, delay, f->timerScript);
	}
}

void BattlePlugin::aiRespond(Fighter *f) {
	byte mask = moveClass(f->opponent->move);
	int32 best = 0;

	AiRecord *rec = aiPickMove(f, f->aiBlock.list, mask, &best);
	if (rec != nullptr) {
		aiSelectMove(f, rec);
		aiTimerStep(f);
	}
}

void BattlePlugin::aiFollowUp(Fighter *f) {
	int32 best = 0;

	AiRecord *rec = aiPickMove2(f, _aiLists01DE[f->player], 0, &best);
	if (rec != nullptr) {
		aiSelectMove(f, rec);
		if (f->move == kMoveSlotIdle)
			aiTimerStep(f);
	}
}

byte BattlePlugin::moveClass(byte move) {
	switch (move) {
	case 16:
	case 17:
		return 0x40;
	case 10:
	case 20:
		return 0x18;
	case 22:
	case 23:
		return 0x10;
	case 24:
		return 0x20;
	case 26:
		return 0x80;
	default:
		return 8;
	}
}

AiRecord *BattlePlugin::aiPickMove(Fighter *f, AiRecord *list, byte mask, int32 *best) {
	AiBlock *a = &f->aiBlock;
	AiRecord *bestRecord = nullptr;
	const MoveEntry *moves = f->moveTab[f->set];
	Fighter *opp = f->opponent;

	for (AiRecord *rec = list; !(rec->move == 0xFF && rec->move2 == 0xFF && rec->moves34 == 0xFFFF); rec++) {
		uint32 score = 0;
		uint16 flags = rec->flags;

		if (mask & flags)
			score = 2000;

		if (opp->z != f->z) {
			uint16 k = flags & 3;
			if (k == 1 && opp->z < f->z)
				score += 500;
			if (k == 2 && opp->z > f->z)
				score += 500;
		}

		if (opp->level < f->level) {
			if (!(flags & kAiFlagLevelDown))
				continue;
			if (opp->state != 2)
				score += 7500;
		} else if (opp->level > f->level) {
			if (!(flags & kAiFlagLevelUp))
				continue;
			if (moves[kMoveSlotLevelUp].available == 0)
				continue;
		}

		if (_ctx->centerDistance > rec->maxDistance)
			continue;

		uint32 delta = a->selectionCount - rec->lastUsedSeq;
		if (rec->missCount == 0)
			score += 20;
		else
			score += delta * (rec->hitCount + 1) * 10 / rec->missCount;

		if (delta < a->reuseCooldown)
			continue;

		score += delta * rec->weight;

		if (moves[rec->move].available == 0)
			continue;

		if (rec->move == kMoveSlotWeaponHold || rec->move2 == kMoveSlotWeaponHold) {
			if (*f->ammo == 0)
				continue;
		}

		if (a->repeatCount >= 3)
			continue;

		if (*best < (int32)score || (*best == (int32)score && (bestRecord != nullptr ? bestRecord->lastUsedSeq : (uint32)kAiNullRecordLastUsed) > rec->lastUsedSeq)) {
			*best = (int32)score;
			bestRecord = rec;
		}
	}

	return bestRecord;
}

AiRecord *BattlePlugin::aiPickMove2(Fighter *f, AiRecord *list, uint16 flag, int32 *best) {
	Fighter *opp = f->opponent;
	AiRecord *bestRecord = nullptr;
	const MoveEntry *moves = f->moveTab[f->set];
	int32 step = (flag != 0) ? 500 : -500;
	int32 base;

	if (flag != 0 && opp->level != f->level)
		base = 50000;
	else if (flag == 0 && opp->level == f->level)
		base = 50000;
	else
		base = -50000;

	for (AiRecord *rec = list; !(rec->move == 0xFF && rec->move2 == 0xFF && rec->moves34 == 0xFFFF); rec++) {
		uint32 score = 0;
		uint16 flags = rec->flags;

		if (flags & kAiFlagTurn) {
			if (f->side == f->facing)
				score = (uint32)-20000;
			else
				score = 50000;
		}

		if (flags & kAiFlagWalkMask) {
			if (f->side == f->facing && (flags & kAiFlagWalkForward))
				score += (uint32)step;
			else if (f->side != f->facing && (flags & kAiFlagWalkBack))
				score += (uint32)step;
			else
				score -= (uint32)step;
		}

		if (_ctx->centerDistance <= rec->maxDistance)
			score += 2000;

		if (flags & kAiFlagLevelUp) {
			if (moves[kMoveSlotLevelUp].available == 0)
				continue;
			if (_ctx->centerDistance > 0x50)
				score += (uint32)base;
		} else if (flags & kAiFlagLevelDown) {
			if (moves[kMoveSlotLevelDown].available == 0)
				continue;
			score += (uint32)base;
		}

		if (base > 0) {
			uint16 k = flags & kAiFlagDirectionMask;
			if ((k == kAiFlagWalkForward && f->facing == 0) || (k == kAiFlagWalkBack && f->facing == 1))
				score += 500;
		}

		uint32 delta = f->aiBlock.selectionCount - rec->lastUsedSeq;
		if (delta > 2)
			score += delta * rec->weight;

		if (moves[rec->move].available == 0)
			continue;
		if ((flags & kAiFlagWalkForward) && moves[kMoveSlotWalkForward].available == 0)
			continue;
		if ((flags & kAiFlagWalkBack) && moves[kMoveSlotWalkBack].available == 0)
			continue;

		if (*best < (int32)score || (*best == (int32)score && (bestRecord != nullptr ? bestRecord->lastUsedSeq : (uint32)kAiNullRecordLastUsed) > rec->lastUsedSeq)) {
			*best = (int32)score;
			bestRecord = rec;
		}
	}

	return bestRecord;
}

void BattlePlugin::aiChooseNext(Fighter *f) {
	AiBlock *a = &f->aiBlock;
	AiRecord *rec = nullptr;
	AiRecord *selected;
	int32 best = 0;
	byte strategy = a->strategy;

	if (f->set == 0 && f->move != kMoveSlotKnockOut) {
		aiSelectMove(f, &_aiList01A2[f->player]);
		return;
	}

	a->strategy = strategy;
	switch (strategy) {
	case 0:
	case 4:
		rec = aiPickMove(f, a->list, 0, &best);
		if (rec != nullptr)
			break;
		a->strategy = 6;
		// fall through
	case 2:
	case 6:
		best = 0;
		rec = aiPickMove2(f, _aiLists01DE[f->player], (strategy != 2) ? 1 : 0, &best);
		a->confidence++;
		a->outcomeHistory <<= 1;
		aiUpdateStrategy(a);
		if (rec != nullptr)
			break;
		// fall through
	case 1:
	case 3:
	case 5:
		rec = &_aiList0166[f->player];
		a->confidence++;
		aiUpdateStrategy(a);
		a->outcomeHistory <<= 1;
		break;
	default:
		break;
	}

	selected = rec;
	if (selected == nullptr)
		selected = &_aiList0166[f->player];

	if (a->moveQueue == 0 || (best >= a->switchScore && selected != a->currentRecord))
		aiSelectMove(f, selected);
}

} // End of namespace Topgun
