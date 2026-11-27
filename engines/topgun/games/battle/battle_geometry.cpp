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

#include "topgun/games/battle/battle.h"
#include "topgun/libraries/sprite/sprite_v1_2_4.h"

namespace Topgun {

uint16 BattlePlugin::hurtBoxOverlap(Fighter *f, Common::Rect *destination, Common::Point *points, uint16 wide) {
	if (_ctx->phase == kPhaseWalkIn)
		return 0;

	Common::Rect rects[2];
	Common::Rect spriteRect;
	Fighter *current = f;

	for (int i = 0; i < 2; i++) {
		_host->getSpriteRect(current->sprite, spriteRect);

		rects[i] = current->rectTab[current->posture * 2 + current->facing];

		if (wide != 0) {
			rects[i].top = -2000;
			rects[i].bottom = 2000;
		}

		int16 offsetX = (int16)(spriteRect.right + spriteRect.left) / 2;
		int16 offsetY = (int16)(spriteRect.top + spriteRect.bottom) / 2;
		rects[i].translate(offsetX, offsetY);

		points[i].x = (int16)(rects[i].right + rects[i].left) / 2;
		points[i].y = (int16)(rects[i].top + rects[i].bottom) / 2;

		current = current->opponent;
	}

	Common::Rect intersection;
	intersection.left = MAX(rects[0].left, rects[1].left);
	intersection.top = MAX(rects[0].top, rects[1].top);
	intersection.right = MIN(rects[0].right, rects[1].right);
	intersection.bottom = MIN(rects[0].bottom, rects[1].bottom);

	if (intersection.left >= intersection.right || intersection.top >= intersection.bottom) {
		*destination = Common::Rect();
		return 0;
	}

	*destination = intersection;
	return 1;
}

void BattlePlugin::updateDistance(Fighter *f) {
	if (_ctx->phase == kPhaseWalkIn)
		return;

	Fighter *source = f;
	Fighter *target = f->opponent;
	uint16 sourceIndex = source->posture * 2 + source->facing;
	uint16 targetIndex = target->posture * 2 + target->facing;
	const Common::Rect *sourceRect = &source->rectTab[sourceIndex];
	const Common::Rect *targetRect = &target->rectTab[targetIndex];

	Common::Point targetPosition;
	if (target->state == kStateAttack) {
		targetPosition.x = target->x;
		targetPosition.y = target->y;
	} else {
		_host->getSpritePosition(target->sprite, targetPosition);
	}

	targetPosition.x += target->xTab[targetIndex];

	int16 sourceX = (int16)(source->xTab[sourceIndex] + source->x);
	if (sourceX > targetPosition.x) {
		_ctx->edgeGap = (int16)(source->xTab[sourceIndex] - targetRect->right - targetPosition.x + sourceRect->left + source->x);
		_ctx->centerDistance = (int16)(source->xTab[sourceIndex] - targetPosition.x + source->x);
		source->side = 1;
		target->side = 0;
	} else {
		_ctx->edgeGap = (int16)(targetRect->left - source->xTab[sourceIndex] - sourceRect->right - source->x + targetPosition.x);
		_ctx->centerDistance = (int16)(targetPosition.x - source->xTab[sourceIndex] - source->x);
		source->side = 0;
		target->side = 1;
	}

	_ctx->gapMinusMargins = (int16)(_ctx->edgeGap - _contactMargins[target->set] - _contactMargins[source->set]);
	_ctx->contact = (_ctx->gapMinusMargins < 10) ? 1 : 0;
}

int16 BattlePlugin::clampMove(Fighter *f, int16 dx, uint16 useSprite) {
	const ArenaBounds *bounds = &_ctx->bounds[0];
	uint16 index = f->posture * 2 + f->facing;

	int16 x;
	if (useSprite != 0) {
		Common::Rect spriteRect;
		_host->getSpriteRect(f->sprite, spriteRect);
		x = (int16)(spriteRect.right + spriteRect.left) / 2;
	} else {
		x = (int16)(f->xTab[index] + f->x);
	}

	const Common::Rect *hurtBox = &f->rectTab[index];
	int16 lower = (int16)(bounds->left - hurtBox->left - x);
	int16 upper = (int16)(bounds->right - hurtBox->right - x);

	if (lower > dx)
		return lower;

	if (upper <= dx)
		return upper;

	return dx;
}

void BattlePlugin::movesContactFlip() {
	if (_ctx->aiEngaged != 0) {
		Fighter *f = &_ctx->fighters[0];
		if (f->contact != 0)
			contactPush(f);

		f = &_ctx->fighters[1];
		if (f->contact != 0)
			contactPush(f);
	}
}

void BattlePlugin::contactPush(Fighter *f) {
	Fighter *opponent = f->opponent;

	if (f->attackActive != 0) {
		if (hitAttack(f->spriteId) == 0)
			return;

		setContact(f, 0);
		return;
	}

	Common::Rect rect;
	Common::Point points[2];
	if (!hurtBoxOverlap(f, &rect, points, (opponent->set > f->set) ? 1 : 0))
		return;

	uint16 index = f->posture * 2 + f->facing;
	f->x = (int16)(points[0].x - f->xTab[index]);

	int16 width = (int16)(rect.right - rect.left);

	SpriteInfo_v1_2_4 info;
	_host->getSpriteInfo(f->sprite, info);

	uint16 *host = _host->variableTable();
	host[f->varList[4]] = info.nextFrame;
	host[f->varList[5]] = info.rangeEnd;

	int16 extent = (int16)(info.rangeEnd - info.nextFrame);
	if (extent < 0)
		extent = -extent;
	host[f->varList[6]] = (uint16)(extent + 1);

	width = (int16)(width + _contactMargins[f->set] + _contactMargins[opponent->set]);
	if (points[1].x > points[0].x)
		width = -width;

	int16 moved = clampMove(f, width, 0);
	host[f->varList[2]] = (uint16)((int16)(moved * 2) / 3);
	host[f->varList[0]] = (uint16)moved;
	host[f->varList[1]] = (uint16)(f->y - info.y);
	host[f->varList[3]] += (uint16)(f->y - info.y);

	f->lastAnim = f->animTab[f->remap[(f->set == 0) ? 0x3F : 0x40]];

	setContact(f, 0);
	aiEvent(f, kAiEventPushed);
	playAnim(f, f->lastAnim);
}

void BattlePlugin::updateWallFlags(Fighter *f) {
	const ArenaBounds *floorBounds = &_ctx->bounds[0];
	const ArenaBounds *raisedBounds = &_ctx->bounds[1];
	uint16 index = f->posture * 2 + f->facing;
	const Common::Rect *hurtBox = &f->rectTab[index];

	int16 left = (int16)(hurtBox->left + f->xTab[index] + f->x);
	int16 right = (int16)(f->xTab[index] + hurtBox->right + f->x);

	bool nearLeft = (left <= floorBounds->left);
	bool nearRight = (right >= floorBounds->right);
	bool farLeft = (left < _ctx->outerLeft);
	bool farRight = (right > _ctx->outerRight);

	MoveEntry *moves = f->moveTab[f->set];
	const ArenaBounds *source = (f->level == 0) ? floorBounds : raisedBounds;

	moves[17].available = (source->sidestepMinusLimit < f->lane) ? 1 : 0;
	moves[18].available = (source->sidestepPlusLimit > f->lane) ? 1 : 0;

	if (f->facing == 0) {
		moves[kMoveSlotWalkForward].available = nearRight ? 0 : 1;
		moves[kMoveSlotWalkBack].available = nearLeft ? 0 : 1;

		if (f->posture == 0) {
			f->remap[2] = farRight ? 4 : 3;
			f->remap[3] = farLeft ? 6 : 5;
		}
	} else {
		moves[kMoveSlotWalkForward].available = nearLeft ? 0 : 1;
		moves[kMoveSlotWalkBack].available = nearRight ? 0 : 1;

		if (f->posture == 0) {
			f->remap[2] = farLeft ? 4 : 3;
			f->remap[3] = farRight ? 6 : 5;
		}
	}

	f->beyondOuterWall = (farLeft || farRight) ? 1 : 0;
	f->atWall = (nearLeft || nearRight) ? 1 : 0;

	if (f->level == 0 && floorBounds->y != 0 && (int16)(f->xTab[index] + f->x) >= (int16)(raisedBounds->left - 100))
		moves[kMoveSlotLevelUp].available = 1;
	else
		moves[kMoveSlotLevelUp].available = 0;

	moves[kMoveSlotLevelDown].available = (f->level == 1) ? 1 : 0;
}

void BattlePlugin::movesReposition(uint16 id, uint16 mode) {
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
		f->x = (int16)((int16)(spriteRect.right + spriteRect.left) / 2 - f->xTab[f->posture * 2 + f->facing]);
	}

	if (mode != 0 || (f->flags & kFlagLevelChange)) {
		int16 levelY = 0;
		if (f->level < kBoundsCount)
			levelY = _ctx->bounds[f->level].y;
		else
			warning("BattlePlugin::movesReposition(): invalid level %d", f->level);

		f->y = (int16)(f->y + levelY);
		f->level = (f->level == 0) ? 1 : 0;
		f->z = (uint16)(f->z + ((f->level == 0) ? 10 : -10));
		_host->setSpriteLevel(f->sprite, f->z);

		Fighter *opponent = f->opponent;
		_ctx->aiEngaged = (opponent->level == f->level && f->lane == opponent->lane && _ctx->phase == kPhaseFight) ? 1 : 0;

		f->moveTab[1][kMoveSlotAirAttack].available = (f->level == 1) ? 1 : 0;
		f->flags &= (byte)~kFlagLevelChange;
	}

	separateFighters(f);
	updateDistance(f);
	updateWallFlags(f);

	uint16 index = f->posture * 2 + f->facing;

	if (mode == 0 && f->side != previousSide) {
		// 0x31 is the code of a side change in the combo history, which stays open for 500 ms
		f->combo = (f->combo << 8) | 0x31;
		f->comboEnd = g_system->getMillis() + 500;
	}

	if (f->level == 1 && (int16)(f->xTab[index] + f->x) < _ctx->bounds[1].left) {
		startKnockdown(f);
		aiEvent(f->opponent, kAiEventOpponentRepositioned);
		return;
	}

	if (_ctx->phase == kPhaseWalkIn && f->set == 0) {
		int16 x = (int16)(f->xTab[index] + f->x);
		int16 y = (int16)(f->yTab[0] + f->y);

		if (_ctx->doorRect.contains(x, y)) {
			setInputDevice(f->player, kInputDeviceDisable);
			_host->callScript(kScriptDoorEntered, 0, 0);

			if (_ctx->doorX == kNoDoor) {
				_host->setSpriteQueue(f->sprite, 0x9F77, 0);
				return;
			}

			x = (int16)(x - f->xTab[index]);

			uint16 *host = _host->variableTable();
			host[f->varList[0]] = (uint16)(_ctx->doorX - x);
			host[f->varList[1]] = (uint16)(_ctx->doorY - y);
			host[f->varList[2]] = host[f->varList[0]] >> 1;
			host[f->varList[3]] = host[f->varList[1]] >> 1;
			host[f->varList[8]] = (f->facing == 1) ? 1 : 0;

			switchSprite(f, 3, f->animTab[0x65], 0);
			return;
		}
	} else if (_ctx->networkGame != 0 && _ctx->fightOver == 0) {
		warning("BattlePlugin::movesReposition(): network hit synchronisation is not supported");
	}

	if (f->doneCallback == nullptr)
		warning("BattlePlugin::movesReposition(): fighter %d has no done callback", f->player);
	else
		(this->*f->doneCallback)(f);

	aiEvent(f->opponent, kAiEventOpponentRepositioned);
}

void BattlePlugin::setPosition(Fighter *f, int16 x, uint16 level) {
	if (level == kLevelNone)
		return;

	if ((byte)level != f->level) {
		f->level = (byte)level;
		f->z = (uint16)(f->z + ((f->level == 0) ? 10 : -10));
		_host->setSpriteLevel(f->sprite, f->z);

		Fighter *opponent = f->opponent;
		_ctx->aiEngaged = (opponent->level == f->level && f->lane == opponent->lane && _ctx->phase == kPhaseFight) ? 1 : 0;

		f->moveTab[1][kMoveSlotAirAttack].available = (level == 1) ? 1 : 0;
		f->flags &= (byte)~kFlagLevelChange;
	}

	f->x = x;

	int16 y = (f->level == 0) ? 0 : _ctx->bounds[0].y;
	f->y = y;
	_host->setSpritePosition(f->sprite, x, y);

	updateDistance(f);
	updateWallFlags(f);
}

void BattlePlugin::separateFighters(Fighter *f) {
	int16 delta = clampMove(f, 0, 0);
	if (delta != 0)
		f->x = (int16)(f->x + delta);

	Fighter *opponent = f->opponent;

	if (_ctx->aiEngaged != 0) {
		updateDistance(f);

		int16 gap = (int16)(-(_contactMargins[opponent->set] - _ctx->edgeGap) - _contactMargins[f->set]);
		if (gap < 0) {
			int16 fighterX = (int16)(f->xTab[f->posture * 2 + f->facing] + f->x);
			int16 opponentX = (int16)(opponent->xTab[opponent->posture * 2 + opponent->facing] + opponent->x);
			if (opponentX < fighterX)
				gap = -gap;

			int16 moved = clampMove(f, gap, 0);
			f->x = (int16)(f->x + moved);

			if (moved != gap) {
				Common::Point position;
				_host->getSpritePosition(opponent->sprite, position);
				opponent->x = (int16)(position.x - gap + moved);
				_host->setSpritePosition(opponent->sprite, opponent->x, position.y);
			}
		}
	}

	_host->setSpritePosition(f->sprite, f->x, f->y);
}

} // End of namespace Topgun
