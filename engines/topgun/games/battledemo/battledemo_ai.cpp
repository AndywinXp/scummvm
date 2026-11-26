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
#include "common/util.h"

#include "topgun/games/battledemo/battledemo.h"

namespace Topgun {

void BattleDemoPlugin::aiSetupSkill(Fighter *f) {
	uint16 *host = _host->variableTable();
	AiBlock *a = &f->aiBlock;
	AiRecord *rec;

	host[TG_BATTLE_V1_2_2_1_VAR_DIFFICULTY] = 4;

	a->maxReactionDelay = (uint16)((host[TG_BATTLE_V1_2_2_1_VAR_DIFFICULTY] + 1) * 10);
	a->reactionDelay = (uint16)((100 - a->maxReactionDelay) * 10);
	a->opponentRatio = (byte)(g_system->getMillis() % 3);
	a->ownRatio = (byte)(g_system->getMillis() % 3);
	a->balance = (byte)(g_system->getMillis() % 3);
	a->strategy = 6;
	a->selectionCount = 0;
	a->outcomeHistory = 0;
	a->landedCount = 8;
	a->hurtCount = 0;
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

void BattleDemoPlugin::aiEvent(Fighter *f, uint16 event) {
	if (f->ai == 0)
		return;

	AiBlock *a = &f->aiBlock;
	Fighter *opp = f->opponent;

	switch (event) {
	case kAiEventMoveEnded:
		aiTimerStep(f);
		break;
	case kAiEventOpponentAttacking:
		if (f->level != opp->level || opp->side != opp->facing)
			return;

		if (a->strategy == 2 && f->move == kMoveSlotIdle) {
			aiRespond(f);
			return;
		}

		if (!a->weights)
			warning("BattleDemoPlugin::aiEvent(): no AI weights");

		if (a->weights && (a->weights[1] != 0 || a->weights[3] != 0))
			aiReact(f, opp);
		else
			aiChooseNext(f);
		break;
	case kAiEventOpponentVulnerable:
		if (a->waiting != 0)
			aiTimerStep(f);
		break;
	case kAiEventOpponentRepositioned:
		aiChooseNext(f);
		break;
	case kAiEventHitBlocked:
		if (!a->weights) {
			warning("BattleDemoPlugin::aiEvent(): no AI weights");
			return;
		}

		if (a->weights[3] != 0 && a->waiting != 0)
			aiRespond(f);
		break;
	case kAiEventHurt:
		if (a->outcomeHistory & 1)
			a->hurtCount--;
		else
			a->landedCount--;
		a->outcomeHistory = (a->outcomeHistory >> 1) | 0x80;
		a->hurtCount++;
		a->opponentRatio = (byte)(3 * (2 * opp->fightHits + opp->totalHits) / (2 * opp->fightAttacks + opp->totalAttacks + 1));
		a->balance = (byte)(ABS(a->landedCount - a->hurtCount) / 3);
		a->reactionDelay = (uint16)((a->landedCount + 3) * 50);
		a->moveQueue = 0;
		a->waiting = 0;
		a->unk_1f = 0;
		a->timerArmed = 0;
		_host->cancelScriptTimer(f->timerId);
		break;
	case kAiEventHitLanded:
		if (a->outcomeHistory & 1)
			a->hurtCount--;
		else
			a->landedCount--;
		a->outcomeHistory >>= 1;
		a->landedCount++;
		if (!a->currentRecord) {
			warning("BattleDemoPlugin::aiEvent(): no current AI record");
		} else {
			a->currentRecord->missCount++;
			a->currentRecord->hitCount++;
		}
		a->ownRatio = (byte)(3 * (2 * f->fightHits + f->totalHits) / (2 * f->fightAttacks + f->totalAttacks + 1));
		a->balance = (byte)(ABS(a->landedCount - a->hurtCount) / 3);
		a->reactionDelay = (uint16)((a->landedCount + 2) * 75);
		if (a->strategy == 7 && a->moveQueue == 0)
			aiFollowUp(f);
		break;
	case kAiEventAttackMissed:
		// missCount counts every outcome of a record, not only the misses
		if (!a->currentRecord)
			warning("BattleDemoPlugin::aiEvent(): no current AI record");
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
	default:
		break;
	}
}

void BattleDemoPlugin::aiReact(Fighter *self, Fighter *opp) {
	int32 best = 0;

	AiRecord *rec = aiPickReaction(self, _aiLists0472[self->player], &best);
	if (!rec)
		return;

	aiSelectMove(self, rec);

	if (self->move == kMoveSlotIdle || (self->state == kStateGuard && self->aiBlock.waiting != 0))
		aiTimerStep(self);
}

void BattleDemoPlugin::aiStartTimer(Fighter *f) {
	uint32 delay;

	if (f->opponent->attackActive != 0)
		delay = g_system->getMillis() % 1000 + f->aiBlock.reactionDelay;
	else
		delay = g_system->getMillis() % 150 + f->aiBlock.reactionDelay;

	f->timerCallback = &BattlePlugin::aiTimerStep;
	f->aiBlock.waiting = 1;
	f->aiBlock.timerArmed = 0;
	_host->setScriptTimer(f->timerId, delay, f->timerScript);
}

void BattleDemoPlugin::aiTimerStep(Fighter *f) {
	AiBlock *a = &f->aiBlock;
	uint32 delay;

	if (a->moveQueue == 0) {
		aiChooseNext(f);
		if (a->unk_1f != 0) {
			f->timerCallback = &BattlePlugin::aiTimerStep;
			a->waiting = 1;
			a->timerArmed = 0;
			delay = g_system->getMillis() % 150 + a->reactionDelay;
			_host->setScriptTimer(f->timerId, delay, f->timerScript);
			return;
		}
	}

	a->waiting = 0;
	bool noImmediate = (a->currentFlags & kAiFlagImmediateMask) == 0;
	AiRecord *rec = a->nextRecord;
	if (!rec) {
		warning("BattleDemoPlugin::aiTimerStep(): no next AI record");
		return;
	}

	a->currentRecord = rec;
	a->currentFlags = rec->flags;
	if (a->currentFlags & kAiFlagImmediateMask) {
		aiExecuteMove(f);
		return;
	}

	if (f->opponent->inFight == 0) {
		a->waiting = 1;
		f->timerCallback = &BattlePlugin::aiTimerStep;
		a->timerArmed = 0;
		delay = g_system->getMillis() % 1000 + a->reactionDelay;
		_host->setScriptTimer(f->timerId, delay, f->timerScript);
		moveIdle(f, 0, 0);
		return;
	}

	f->timerCallback = &BattlePlugin::aiPoll;
	if (noImmediate && a->strategy != 2 && a->strategy != 3)
		delay = g_system->getMillis() % 150 + a->reactionDelay;
	else
		delay = g_system->getMillis() % 150;
	a->timerArmed = 0;
	_host->setScriptTimer(f->timerId, delay, f->timerScript);
	if (delay > 0x7D)
		moveIdle(f, 0, 0);
}

void BattleDemoPlugin::aiExecuteMove(Fighter *f) {
	AiBlock *a = &f->aiBlock;
	byte previous = (byte)a->moveQueue;
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
		delay = g_system->getMillis() % 150 + a->reactionDelay;
		f->timerCallback = &BattlePlugin::aiTimerStep;
		_host->setScriptTimer(f->timerId, delay, f->timerScript);
	}

	a->moveQueue >>= 8;
	a->delayQueue >>= 8;
	a->unk_1f = 0;

	if (previous >= kMoveCount) {
		warning("BattleDemoPlugin::aiExecuteMove(): invalid move %d", previous);
		return;
	}

	const MoveEntry *entry = &f->moveTab[f->set][previous];
	f->anim = f->animTab[f->remap[entry->animIndex]];
	f->move = previous;
	f->handlerParam = entry->handlerParam;

	if (!a->currentRecord)
		warning("BattleDemoPlugin::aiExecuteMove(): no current AI record");
	else
		f->moveCel = a->currentRecord->cel;

	if (!entry->handler)
		warning("BattleDemoPlugin::aiExecuteMove(): move %d has no handler", f->move);
	else
		(this->*entry->handler)(f, f->move, f->anim);
}

AiRecord *BattleDemoPlugin::aiPickMove(Fighter *f, AiRecord *list, byte mask, int32 *best) {
	AiBlock *a = &f->aiBlock;
	AiRecord *bestRecord = nullptr;
	const MoveEntry *moves = f->moveTab[f->set];
	Fighter *opp = f->opponent;

	for (AiRecord *rec = list; !(rec->move == 0xFF && rec->move2 == 0xFF && rec->moves34 == 0xFFFF); rec++) {
		uint32 score = 0;
		uint16 flags = rec->flags;

		if (mask & flags)
			score = 2000;

		if (opp->lane != f->lane) {
			uint16 k = flags & 3;
			if (k == 1 && opp->lane < f->lane)
				score += 500;
			if (k == 2 && opp->lane > f->lane)
				score += 500;
		}

		if (_ctx->edgeGap > rec->maxDistance)
			score -= 20000;

		uint32 delta = a->selectionCount - rec->lastUsedSeq;
		if (rec->missCount == 0)
			score += 20;
		else
			score += delta * (rec->hitCount + 1) * 10 / rec->missCount;

		score += delta * rec->weight;

		if (moves[rec->move].available == 0)
			score = (uint32)-50000;

		if (*best < (int32)score || (*best == (int32)score && (bestRecord ? bestRecord->lastUsedSeq : (uint32)TG_BATTLE_V1_2_2_1_AI_NULL_RECORD_LAST_USED) > rec->lastUsedSeq)) {
			*best = (int32)score;
			bestRecord = rec;
		}
	}

	return bestRecord;
}

AiRecord *BattleDemoPlugin::aiPickReaction(Fighter *f, AiRecord *list, int32 *best) {
	AiBlock *a = &f->aiBlock;

	if ((g_system->getMillis() & 3) >= a->opponentRatio)
		return nullptr;

	AiRecord *bestRecord = nullptr;
	byte mask = moveClass(f->opponent->move);
	const MoveEntry *moves = f->moveTab[f->set];

	for (AiRecord *rec = list; !(rec->move == 0xFF && rec->move2 == 0xFF && rec->moves34 == 0xFFFF); rec++) {
		uint32 score = 0;

		if (moves[rec->move].available == 0)
			score = (uint32)-50000;
		else if (mask & rec->flags)
			score = 10000;

		score += (a->selectionCount - rec->lastUsedSeq) * rec->weight;

		if (*best < (int32)score || (*best == (int32)score && (bestRecord ? bestRecord->lastUsedSeq : (uint32)TG_BATTLE_V1_2_2_1_AI_NULL_RECORD_LAST_USED) > rec->lastUsedSeq)) {
			*best = (int32)score;
			bestRecord = rec;
		}
	}

	return bestRecord;
}

AiRecord *BattleDemoPlugin::aiPickMove2(Fighter *f, AiRecord *list, uint16 flag, int32 *best) {
	Fighter *opp = f->opponent;
	AiRecord *bestRecord = nullptr;
	const MoveEntry *moves = f->moveTab[f->set];
	int32 step = (flag != 0) ? 500 : -500;
	int32 base;

	if (flag != 0 && opp->level != f->level)
		base = 1000;
	else if (flag == 0 && opp->level == f->level)
		base = 1000;
	else
		base = -1000;

	for (AiRecord *rec = list; !(rec->move == 0xFF && rec->move2 == 0xFF && rec->moves34 == 0xFFFF); rec++) {
		uint32 score = 0;
		uint16 flags = rec->flags;

		if (flags & kAiFlagTurn) {
			if (f->side == f->facing)
				score = (uint32)-20000;
			else
				score = 50000;
		}

		if ((f->side == f->facing && (flags & kAiFlagWalkForward)) || (f->side != f->facing && (flags & kAiFlagWalkBack)))
			score += (uint32)step;
		else
			score -= (uint32)step;

		if (_ctx->edgeGap <= rec->maxDistance)
			score += 2000;

		if (flags & kAiFlagLevelUp) {
			if (moves[kMoveSlotLevelUp].available == 0)
				score = (uint32)-10000;
			else if (_ctx->edgeGap > 100)
				score += (uint32)base;
		} else if (flags & kAiFlagLevelDown) {
			if (moves[kMoveSlotLevelDown].available == 0)
				score = (uint32)-10000;
			else
				score += (uint32)base;
		}

		if (base > 0) {
			uint16 k = flags & kAiFlagDirectionMask;
			if ((k == kAiFlagWalkForward && f->facing == 0) || (k == kAiFlagWalkBack && f->facing == 1))
				score += 500;
		}

		score += (f->aiBlock.selectionCount - rec->lastUsedSeq) * rec->weight;

		if (moves[rec->move].available == 0
				|| ((flags & kAiFlagWalkForward) && moves[kMoveSlotWalkForward].available == 0)
				|| ((flags & kAiFlagWalkBack) && moves[kMoveSlotWalkBack].available == 0))
			score = (uint32)-50000;

		if (*best < (int32)score || (*best == (int32)score && (bestRecord ? bestRecord->lastUsedSeq : (uint32)TG_BATTLE_V1_2_2_1_AI_NULL_RECORD_LAST_USED) > rec->lastUsedSeq)) {
			*best = (int32)score;
			bestRecord = rec;
		}
	}

	return bestRecord;
}

void BattleDemoPlugin::aiChooseNext(Fighter *f) {
	AiBlock *a = &f->aiBlock;
	AiRecord *rec = nullptr;
	int32 best = 0;

	uint16 row = (uint16)((a->opponentRatio * 3 + a->ownRatio) * 3 + a->balance);
	if (row >= TG_BATTLE_V1_2_2_1_AI_WEIGHT_ROWS) {
		warning("BattleDemoPlugin::aiChooseNext(): invalid weight row %d", row);
		row = 0;
	}

	const byte *weights = &_aiWeights[row * TG_BATTLE_V1_2_2_1_AI_WEIGHT_ROW_SIZE];
	uint32 random = g_system->getMillis() % 100;
	byte sum = 0;
	byte k = 0;

	do {
		sum += weights[k];
		if (sum > random)
			break;
		k++;
	} while (k < TG_BATTLE_V1_2_2_1_AI_WEIGHT_ROW_SIZE);

	// Points at the chosen weight, not at the start of the row
	a->weights = weights + k;

	if (f->side != f->facing)
		k = 4;

	if (f->set == 0 && f->move != kMoveSlotKnockOut) {
		aiSelectMove(f, &_aiList01A2[f->player]);
		return;
	}

	a->strategy = k;
	switch (k) {
	case 0:
	case 7:
		rec = aiPickMove(f, a->list, 0, &best);
		if (rec)
			break;
		a->strategy = 4;
		// fall through
	case 4:
	case 5:
		rec = aiPickMove2(f, _aiLists01DE[f->player], (k == 4) ? 1 : 0, &best);
		break;
	case 1:
	case 3:
		rec = aiPickReaction(f, _aiLists0472[f->player], &best);
		break;
	case 2:
	case 6:
		rec = &_aiList0166[f->player];
		break;
	default:
		break;
	}

	if (!rec)
		rec = &_aiList0166[f->player];

	if (a->moveQueue == 0 || (best > a->switchScore && rec != a->currentRecord))
		aiSelectMove(f, rec);
}

} // End of namespace Topgun
