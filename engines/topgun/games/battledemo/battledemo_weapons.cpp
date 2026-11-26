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

void BattleDemoPlugin::weaponInit(Fighter *f) {
	uint16 *host = _host->variableTable();

	switch (f->weapon) {
	case 1:
		f->ammo = &host[TG_BATTLE_V1_2_2_1_VAR_AMMO_WEAPON1 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 6;
		for (uint16 i = 0; i < kWeaponPoolSize; i++)
			_host->loadResource(_weaponPool1[kWeaponPoolSize * f->player + i]);
		_host->loadResource(_weaponLauncher1[f->player]);
		break;
	case 2:
		f->ammo = &host[TG_BATTLE_V1_2_2_1_VAR_AMMO_WEAPON2 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile2[f->player]);
		_host->loadResource(_weaponLauncher2[f->player]);
		break;
	case 4:
		f->ammo = &host[TG_BATTLE_V1_2_2_1_VAR_AMMO_WEAPON4 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile4[f->player]);
		break;
	case 8:
		f->ammo = &host[TG_BATTLE_V1_2_2_1_VAR_AMMO_WEAPON8 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile8[f->player]);
		break;
	case 0x10:
		f->ammo = &host[TG_BATTLE_V1_2_2_1_VAR_AMMO_WEAPON16 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile16[f->player]);
		break;
	case 0x20:
		f->ammo = &host[TG_BATTLE_V1_2_2_1_VAR_AMMO_WEAPON32 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile32[f->player]);
		break;
	case 0x40:
		f->ammo = &host[TG_BATTLE_V1_2_2_1_VAR_AMMO_WEAPON64 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile64[f->player]);
		break;
	case 0x80:
		f->ammo = &host[TG_BATTLE_V1_2_2_1_VAR_AMMO_WEAPON128 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile128[f->player]);
		break;
	default:
		warning("BattleDemoPlugin::weaponInit(): unknown weapon %d", f->weapon);
		break;
	}
}

void BattleDemoPlugin::weaponsFire(uint16 id, int16 dx, int16 dy) {
	uint16 *host = _host->variableTable();

	Fighter *f = &_ctx->fighters[0];
	if (f->spriteId != id)
		f = &_ctx->fighters[1];

	byte player = f->player;

	if (!f->ammo)
		warning("BattleDemoPlugin::weaponsFire(): the fighter %d has no ammunition counter", player);

	if (f->ammo && *f->ammo != 0) {
		Common::Rect rect;
		_host->getSpriteRect(f->sprite, rect);

		uint16 x;
		if (f->facing == 0)
			x = (uint16)(rect.left + dx);
		else
			x = (uint16)(rect.right - dx);

		uint16 y = (uint16)(rect.top + dy);

		uint16 sprite = 0;
		uint16 anim = 0;
		uint16 speed = 0;

		switch (f->weapon) {
		case 1:
			sprite = _weaponPool1[kWeaponPoolSize * player + _weaponRing[player]];
			_weaponRing[player]++;
			_weaponRing[player] = _weaponRing[player] % kWeaponPoolSize;
			anim = (f->facing == 0) ? 0x8884 : 0x8885;

			if (_host->isSpriteVisible(sprite) != 0)
				return;

			_host->setSpritePosition(_weaponLauncher1[player], x, y);
			_host->setSpriteLevel(_weaponLauncher1[player], (uint16)(f->z + 1));
			_host->setSpriteQueue(_weaponLauncher1[player], (f->facing == 0) ? 0x887C : 0x887D, 0);
			speed = 0xFA;
			break;
		case 2:
			sprite = _weaponProjectile2[player];
			anim = (f->facing == 0) ? 0x889F : 0x88A0;

			if (_host->isSpriteVisible(sprite) != 0)
				return;

			_host->setSpritePosition(_weaponLauncher2[player], x, y);
			_host->setSpriteLevel(_weaponLauncher2[player], (uint16)(f->z + 1));
			_host->setSpriteQueue(_weaponLauncher2[player], (f->facing == 0) ? 0x8895 : 0x8896, 0);
			speed = 0x2EE;
			break;
		case 4:
			sprite = _weaponProjectile4[player];
			anim = (f->facing == 0) ? 0x88AA : 0x88AB;
			speed = 0x2EE;
			break;
		case 8:
			sprite = _weaponProjectile8[player];
			anim = (f->facing == 0) ? 0x88B6 : 0x88B7;
			speed = 0x64;
			break;
		case 0x10:
			sprite = _weaponProjectile16[player];
			anim = (f->facing == 0) ? 0x88BF : 0x88C0;
			speed = 0x64;
			break;
		case 0x20:
			sprite = _weaponProjectile32[player];
			anim = (f->facing == 0) ? 0x88CD : 0x88CE;
			speed = 0xFA;
			break;
		case 0x40:
			sprite = _weaponProjectile64[player];
			anim = (f->facing == 0) ? 0x88DC : 0x88DD;
			speed = 0x2EE;
			break;
		case 0x80:
			sprite = _weaponProjectile128[player];
			anim = (f->facing == 0) ? 0x88E8 : 0x88E9;
			speed = 0x384;
			break;
		default:
			// The original goes on with an unset sprite, animation and speed
			warning("BattleDemoPlugin::weaponsFire(): unknown weapon %d", f->weapon);
			break;
		}

		if (sprite != 0) {
			if (_host->isSpriteVisible(sprite) != 0)
				return;

			(*f->ammo)--;

			_host->setSpritePosition(sprite, x, y);
			_host->setSpriteLevel(sprite, (uint16)(f->z + 1));
			_host->addCollisionPair(sprite, f->opponent->sprite, TG_BATTLE_V1_2_2_1_HOOK_WEAPONS_HIT, player, sprite, 0);
			host[f->varList[9]] = speed;
			_host->setSpriteQueue(sprite, anim, 0);
		}
	}

	_host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_AMMO_DISPLAY, player, 1);
}

uint16 BattleDemoPlugin::weaponsHit(uint16 shooter, uint16 sprite) {
	if (_ctx->fightOver != 0)
		return 0;

	if (shooter >= kFighterCount) {
		warning("BattleDemoPlugin::weaponsHit(): invalid shooter %d", shooter);
		return 0;
	}

	Fighter *a = &_ctx->fighters[shooter];
	Fighter *b = a->opponent;

	if (b->state == kStateDying)
		return 0;

	if (b->lane != a->lane)
		return 0;

	Common::Rect rect;
	_host->getSpriteRect(sprite, rect);

	uint16 hitAnim = 0;
	uint16 flightAnim = 0;
	byte reflects = 0;

	uint16 amount = 0;
	uint16 damage = 0;
	byte damageClass = 0;

	switch (sprite) {
	case 0x8883:
	case 0x8889:
	case 0x888A:
	case 0x888B:
	case 0x888C:
	case 0x888D:
	case 0x888E:
	case 0x888F:
	case 0x8890:
	case 0x8891:
	case 0x8892:
	case 0x8893:
		if (_ctx->aiEngaged == 0)
			return 1;

		amount = 2;
		damage = 2;
		flightAnim = (a->facing == 0) ? 0x8887 : 0x8886;
		break;
	case 0x889E:
	case 0x88A8:
		amount = 0x14;
		damage = 8;
		flightAnim = (a->facing == 0) ? 0x88A2 : 0x88A1;
		break;
	case 0x88A9:
	case 0x88B4:
		amount = 0x19;
		damage = 0x0F;
		flightAnim = (a->facing == 0) ? 0x88AD : 0x88AC;
		break;
	case 0x88B5:
	case 0x88BD:
		if (_ctx->aiEngaged == 0)
			return 1;

		amount = 0x1E;
		damage = 0x14;
		reflects = 1;
		flightAnim = (a->facing == 0) ? 0x88B9 : 0x88B8;
		break;
	case 0x88BE:
	case 0x88CB:
		if (_ctx->aiEngaged == 0)
			return 1;

		amount = 0x23;
		damage = 0x16;
		reflects = 1;
		flightAnim = (a->facing == 0) ? 0x88C2 : 0x88C1;
		break;
	case 0x88CC:
	case 0x88DA:
		if (_ctx->aiEngaged == 0)
			return 1;

		amount = 0x28;
		damage = 0x0C;
		flightAnim = (a->facing == 0) ? 0x88D0 : 0x88CF;
		break;
	case 0x88DB:
	case 0x88E6:
		amount = 0x2D;
		damage = 0x15;
		flightAnim = (a->facing == 0) ? 0x88E0 : 0x88DF;
		hitAnim = flightAnim;
		break;
	case 0x88E7:
	case 0x88FB:
		if (_ctx->aiEngaged == 0)
			return 1;

		amount = 0x32;
		damage = 0x1A;
		flightAnim = (a->facing == 0) ? 0x88EC : 0x88EB;
		hitAnim = (a->facing == 0) ? 0x88EB : 0x88EC;
		break;
	default:
		// The original goes on with an unset amount, damage and damage class
		warning("BattleDemoPlugin::weaponsHit(): unknown projectile 0x%04X", sprite);
		break;
	}

	uint16 classIndex = damageClass;
	uint16 setIndex = b->set * 2;

	if (b->state >= kReactionTableCount) {
		warning("BattleDemoPlugin::weaponsHit(): no reaction table for the state %d", b->state);
		return 0;
	}
	const ReactionRecord *table = b->reactTab[b->state];

	if (b->facing == a->side) {
		setIndex++;
		if (b->state == kStateGuard)
			table = b->reactTab[0];
		else if (b->crouching != 0)
			classIndex = 6;
	}

	const ReactionRecord *record = &table[(classIndex << 2) + setIndex];

	if (b->state == kStateGuard && b->facing != a->side) {
		_host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_CHANGE_ENERGY, b->player, (uint16)(-damage));

		byte reactionMove = record->reactionMove;
		uint16 anim = b->animTab[b->remap[reactionMove]];
		if (reactionMove == kReactKnockBackSet1)
			knockBackAnim(b, anim, a->facing);
		else
			hitStunAnim(b, anim);

		if (flightAnim != 0)
			_host->setSpriteQueue(sprite, flightAnim, 0);

		_host->addCollisionPair(sprite, b->sprite, 0, 0, 0, 0);

		if (reflects != 0 && b->move == kMoveSlotGuardDownForward) {
			_host->addCollisionPair(sprite, a->sprite, TG_BATTLE_V1_2_2_1_HOOK_WEAPONS_HIT, b->player, sprite, 0);
			aiEvent(a, kAiEventOpponentAttacking);
		}

		return 0;
	}

	_host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_CHANGE_ENERGY, b->player, (uint16)(-damage));
	spawnHitSpark(&rect, 0);
	_host->setSpriteQueue(sprite, hitAnim, 1);

	byte reactionMove = record->reactionMove;
	uint16 anim = b->animTab[b->remap[reactionMove]];
	if (reactionMove == kReactKnockBackSet1 || reactionMove == kReactKnockBackSet0)
		knockBackAnim(b, anim, a->facing);
	else if (reactionMove == kReactPushBackSet0 || reactionMove == kReactPushBackSet1)
		pushBackAnim(b, anim, amount, damageClass, kPushFromPositions);
	else
		startReaction(b, anim);

	return scoreAttack(a, 1, 0, 0);
}

uint16 BattleDemoPlugin::weaponsSpecialHit(uint16 shooter, uint16 sprite) {
	if (_ctx->fightOver != 0)
		return 0;

	if (shooter >= kFighterCount) {
		warning("BattleDemoPlugin::weaponsSpecialHit(): invalid shooter %d", shooter);
		return 0;
	}

	Fighter *a = &_ctx->fighters[shooter];
	Fighter *b = a->opponent;

	_host->setSpriteQueue(sprite, a->animTab[0x5A], 0);

	if (b->inFight == 0)
		return 0;

	Common::Point position;
	_host->getSpritePosition(sprite, position);

	Common::Rect rect;
	rect.left = position.x;
	rect.top = position.y;
	rect.right = position.x;
	rect.bottom = position.y;

	uint16 setIndex = b->set * 2;

	if (b->state >= kReactionTableCount) {
		warning("BattleDemoPlugin::weaponsSpecialHit(): no reaction table for the state %d", b->state);
		return 0;
	}
	const ReactionRecord *table = b->reactTab[b->state];

	if (b->facing == a->side) {
		setIndex++;
		if (b->state == kStateGuard)
			table = b->reactTab[0];
	}

	const ReactionRecord *record = &table[24 + setIndex];

	spawnHitSpark(&rect, 0);

	if (b->state == kStateGuard) {
		if (b->facing == a->side) {
			_host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_CHANGE_ENERGY, b->player, (uint16)-40);
		} else {
			_host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_CHANGE_ENERGY, b->player, (uint16)-5);
			if (b->move == kMoveSlotGuardDownForward) {
				aiEvent(b, kAiEventHitBlocked);
				return 0;
			}
		}

		if (b->state == kStateGuard)
			b->state = kStateNormal;

		knockBackAnim(b, b->animTab[b->remap[record->reactionMove]], a->facing);
		return 0;
	}

	_host->callScript(TG_BATTLE_V1_2_2_1_SCRIPT_CHANGE_ENERGY, b->player, (uint16)-40);

	byte reactionMove = record->reactionMove;
	uint16 anim = b->animTab[b->remap[reactionMove]];
	if (reactionMove == kReactKnockBackSet1 || reactionMove == kReactKnockBackSet0)
		knockBackAnim(b, anim, a->facing);
	else if (reactionMove == kReactPushBackSet0 || reactionMove == kReactPushBackSet1)
		pushBackAnim(b, anim, 0x28, 6, kPushFromPositions);
	else
		startReaction(b, anim);

	return scoreAttack(a, 1, 0, 0);
}

} // End of namespace Topgun
