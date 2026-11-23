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

void BattlePlugin::weaponInit(Fighter *f) {
	uint16 *host = _host->variableTable();

	switch (f->weapon) {
	case 1:
		f->ammo = &host[kVarAmmoWeapon1 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 6;
		for (uint16 i = 0; i < kWeaponPoolSize; i++)
			_host->loadResource(_weaponPool1[kWeaponPoolSize * f->player + i]);
		_host->loadResource(_weaponLauncher1[f->player]);
		break;
	case 2:
		f->ammo = &host[kVarAmmoWeapon2 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile2[f->player]);
		_host->loadResource(_weaponLauncher2[f->player]);
		break;
	case 4:
		f->ammo = &host[kVarAmmoWeapon4 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile4[f->player]);
		break;
	case 8:
		f->ammo = &host[kVarAmmoWeapon8 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile8[f->player]);
		break;
	case 0x10:
		f->ammo = &host[kVarAmmoWeapon16 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile16[f->player]);
		break;
	case 0x20:
		f->ammo = &host[kVarAmmoWeapon32 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile32[f->player]);
		break;
	case 0x40:
		f->ammo = &host[kVarAmmoWeapon64 + f->player];
		f->moveTab[1][kMoveSlotWeaponHold].handlerParam = 0;
		_host->loadResource(_weaponProjectile64[f->player]);
		break;
	default:
		break;
	}
}

void BattlePlugin::weaponsFire(uint16 id, int16 dx, int16 dy) {
	uint16 *host = _host->variableTable();

	Fighter *f = &_ctx->fighters[0];
	if (f->spriteId != id)
		f = &_ctx->fighters[1];

	byte player = f->player;

	if (f->ammo == nullptr)
		warning("BattlePlugin::weaponsFire(): the fighter %d has no ammunition counter", player);

	if (f->ammo == nullptr || *f->ammo == 0) {
		_host->callScript(kScriptAmmoDisplay, player, 0);
		return;
	}

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
		anim = (f->facing == 0) ? 0x9CC2 : 0x9CC3;

		if (_host->isSpriteVisible(sprite) != 0)
			return;

		_host->setSpritePosition(_weaponLauncher1[player], x, y);
		_host->setSpriteLevel(_weaponLauncher1[player], (uint16)(f->z + 1));
		_host->setSpriteQueue(_weaponLauncher1[player], (f->facing == 0) ? 0x9CBA : 0x9CBB, 0);
		speed = 0xFA;
		break;
	case 2:
		sprite = _weaponProjectile2[player];
		anim = (f->facing == 0) ? 0x9CDD : 0x9CDE;

		if (_host->isSpriteVisible(sprite) != 0)
			return;

		_host->setSpritePosition(_weaponLauncher2[player], x, y);
		_host->setSpriteLevel(_weaponLauncher2[player], (uint16)(f->z + 1));
		_host->setSpriteQueue(_weaponLauncher2[player], (f->facing == 0) ? 0x9CD3 : 0x9CD4, 0);
		speed = 0x2EE;
		break;
	case 4:
		sprite = _weaponProjectile4[player];
		anim = (f->facing == 0) ? 0x9CE8 : 0x9CE9;
		speed = 0x2EE;
		break;
	case 8:
		sprite = _weaponProjectile8[player];
		anim = (f->facing == 0) ? 0x9CF4 : 0x9CF5;
		speed = 0x64;
		break;
	case 0x10:
		sprite = _weaponProjectile16[player];
		anim = (f->facing == 0) ? 0x9CFD : 0x9CFE;
		speed = 0x64;
		break;
	case 0x20:
		sprite = _weaponProjectile32[player];
		anim = (f->facing == 0) ? 0x9D0B : 0x9D0C;
		speed = 0x2EE;
		break;
	case 0x40:
		sprite = _weaponProjectile64[player];
		anim = (f->facing == 0) ? 0x9D17 : 0x9D18;
		speed = 0x384;
		break;
	default:
		break;
	}

	if (_host->isSpriteVisible(sprite) != 0)
		return;

	if (!(f->flags & kFlagInfiniteAmmo))
		(*f->ammo)--;

	_host->setSpritePosition(sprite, x, y);
	_host->setSpriteLevel(sprite, (uint16)(f->z + 1));
	_host->addCollisionPair(sprite, f->opponent->sprite, kScriptWeaponsHit, player, sprite, 0);
	host[f->varList[9]] = speed;
	_host->setSpriteQueue(sprite, anim, 0);

	_host->callScript(kScriptAmmoDisplay, player, 0);
}

uint16 BattlePlugin::weaponsHit(uint16 shooter, uint16 sprite) {
	uint16 *host = _host->variableTable();

	if (_ctx->fightOver != 0)
		return 0;

	if (shooter >= kFighterCount) {
		warning("BattlePlugin::weaponsHit(): invalid shooter %d", shooter);
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
	byte flagA = 0;
	uint16 power = host[kVarWeaponPower + shooter];

	uint16 amount = 0;
	uint16 damage = 0;
	byte damageClass = 0;

	switch (sprite) {
	case 0x9CC1:
	case 0x9CC7:
	case 0x9CC8:
	case 0x9CC9:
	case 0x9CCA:
	case 0x9CCB:
	case 0x9CCC:
	case 0x9CCD:
	case 0x9CCE:
	case 0x9CCF:
	case 0x9CD0:
	case 0x9CD1:
		if (_ctx->aiEngaged == 0)
			return 1;

		amount = 2;
		damage = _weaponDamage[b->type][0];
		damageClass = 0;
		flightAnim = (a->facing == 0) ? 0x9CC5 : 0x9CC4;
		break;
	case 0x9CDC:
	case 0x9CE6:
		amount = 0x14;
		damage = _weaponDamage[b->type][1];
		if (power & 2)
			damage <<= 1;
		damageClass = 0;
		flightAnim = (a->facing == 0) ? 0x9CE0 : 0x9CDF;
		break;
	case 0x9CE7:
	case 0x9CF2:
		amount = 0x19;
		damage = _weaponDamage[b->type][2];
		if (power & 2)
			damage <<= 1;
		damageClass = 0;
		flightAnim = (a->facing == 0) ? 0x9CEB : 0x9CEA;
		break;
	case 0x9CF3:
	case 0x9CFB:
		if (_ctx->aiEngaged == 0)
			return 1;

		amount = 0x1E;
		damage = _weaponDamage[b->type][3];
		if (power & 4)
			damage <<= 1;
		damageClass = 0;
		flagA = 1;
		flightAnim = (a->facing == 0) ? 0x9CF7 : 0x9CF6;
		break;
	case 0x9CFC:
	case 0x9D09:
		if (_ctx->aiEngaged == 0)
			return 1;

		amount = 0x23;
		damage = _weaponDamage[b->type][4];
		if (power & 1)
			damage <<= 1;
		damageClass = 0;
		flagA = 1;
		flightAnim = (a->facing == 0) ? 0x9D00 : 0x9CFF;
		break;
	case 0x9D0A:
	case 0x9D15:
		amount = 0x2D;
		damage = _weaponDamage[b->type][5];
		if (power & 2)
			damage <<= 1;
		damageClass = 0;
		flightAnim = (a->facing == 0) ? 0x9D0D : 0x9D0E;
		hitAnim = flightAnim;
		break;
	case 0x9D16:
	case 0x9D27:
		if (_ctx->aiEngaged == 0)
			return 1;

		amount = 0x32;
		damage = _weaponDamage[b->type][6];
		damageClass = 0;
		flightAnim = (a->facing == 0) ? 0x9D1A : 0x9D19;
		hitAnim = (a->facing == 0) ? 0x9D19 : 0x9D1A;
		if (damage == 0)
			return 0;
		break;
	default:
		break;
	}

	uint16 classIndex = damageClass;
	uint16 setIndex = b->set * 2;

	const ReactionRecord *table;
	if (b->state < kReactionTableCount) {
		table = b->reactTab[b->state];
	} else {
		warning("BattlePlugin::weaponsHit(): no reaction table for the state %d", b->state);
		table = b->reactTab[0];
	}

	if (b->side != b->facing) {
		setIndex++;
		if (b->state == kStateGuard)
			table = b->reactTab[0];
		else if (b->crouching != 0)
			classIndex = 6;
	}

	uint16 recordIndex = (uint16)((classIndex << 2) + setIndex);
	if (recordIndex >= kReactionTableSize) {
		warning("BattlePlugin::weaponsHit(): invalid reaction record %d", recordIndex);
		recordIndex = 0;
	}
	const ReactionRecord *record = &table[recordIndex];

	if (b->state == kStateGuard && b->facing != a->side) {
		_host->callScript(kScriptChangeEnergy, b->player, (uint16)(-damage));

		byte reactionMove = record->reactionMove;
		uint16 anim = b->animTab[b->remap[reactionMove]];
		if (reactionMove == kReactKnockBackSet1)
			knockBackAnim(b, anim, a->facing);
		else
			hitStunAnim(b, anim);

		if (flightAnim != 0)
			_host->setSpriteQueue(sprite, flightAnim, 0);

		_host->addCollisionPair(sprite, b->sprite, 0, 0, 0, 0);

		if (flagA != 0 && b->move == kMoveSlotGuardDownForward) {
			_host->addCollisionPair(sprite, a->sprite, kScriptWeaponsHit, b->player, sprite, 0);
			aiEvent(a, kAiEventOpponentAttacking);
		}

		return 0;
	}

	_host->callScript(kScriptChangeEnergy, b->player, (uint16)(-damage));
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

	return scoreAttack(a, 1, 0x11, (uint16)(damage * 10));
}

uint16 BattlePlugin::weaponsSpecialHit(uint16 shooter, uint16 sprite) {
	if (_ctx->fightOver != 0)
		return 0;

	if (shooter >= kFighterCount) {
		warning("BattlePlugin::weaponsSpecialHit(): invalid shooter %d", shooter);
		return 0;
	}

	Fighter *a = &_ctx->fighters[shooter];
	Fighter *b = a->opponent;

	_host->setSpriteQueue(sprite, a->animTab[0x59], 0);

	if (b->inFight == 0)
		return 0;

	Common::Rect rect;
	_host->getSpriteRect(sprite, rect);

	uint16 setIndex = b->set * 2;

	const ReactionRecord *table;
	if (b->state < kReactionTableCount) {
		table = b->reactTab[b->state];
	} else {
		warning("BattlePlugin::weaponsSpecialHit(): no reaction table for the state %d", b->state);
		table = b->reactTab[0];
	}

	if (b->side != b->facing) {
		setIndex++;
		if (b->state == kStateGuard)
			table = b->reactTab[0];
	}

	const ReactionRecord *record = &table[24 + setIndex];

	spawnHitSpark(&rect, 0);

	if (b->state == kStateGuard) {
		if (b->facing == a->side) {
			_host->callScript(kScriptChangeEnergy, b->player, (uint16)-30);
		} else {
			_host->callScript(kScriptChangeEnergy, b->player, (uint16)-5);
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

	_host->callScript(kScriptChangeEnergy, b->player, (uint16)-30);

	byte reactionMove = record->reactionMove;
	uint16 anim = b->animTab[b->remap[reactionMove]];
	if (reactionMove == kReactKnockBackSet1 || reactionMove == kReactKnockBackSet0)
		knockBackAnim(b, anim, a->facing);
	else if (reactionMove == kReactPushBackSet0 || reactionMove == kReactPushBackSet1)
		pushBackAnim(b, anim, 0x28, 6, kPushFromPositions);
	else
		startReaction(b, anim);

	return scoreAttack(a, 1, 0x18, 0x3E8);
}

} // End of namespace Topgun
