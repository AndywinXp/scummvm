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

#include "topgun/games/battledemo/battledemo.h"

namespace Topgun {

void BattleDemoPlugin::initDemoTablesShared() {
	const InputRecord verminBeastInputRecords[kInputRecordCount] = {
		{0x0157, 0x0FFF, 0x10},
		{0x0127, 0x0FFF, 0x13},
		{0x0126, 0x0FFF, 0x18},
		{0x0026, 0x00FF, 0x11},
		{0x0016, 0x00FF, 0x12},
		{0x0017, 0x00FF, 0x09},
		{0x0013, 0x00FF, 0x14},
		{0x0031, 0x00FF, 0x05},
		{0x0051, 0x00FF, 0x06},
		{0x0023, 0x00FF, 0x15},
		{0x0025, 0x00FF, 0x16},
		{0x0015, 0x00FF, 0x08},
		{0x0021, 0x00FF, 0x0B},
		{0x0076, 0x00FF, 0x03},
		{0x0011, 0x00FF, 0x03},
		{0x0036, 0x00FF, 0x0C},
		{0x0037, 0x00FF, 0x0D},
		{0x0027, 0x00FF, 0x0A},
		{0x0022, 0x00FF, 0x1B},
		{0x001C, 0x00FF, 0x10},
		{0x0003, 0x000F, 0x04},
		{0x0005, 0x000F, 0x07},
		{0x0007, 0x000F, 0x01},
		{0x0006, 0x000F, 0x02},
		{0x0009, 0x000F, 0x0C},
		{0x000A, 0x000F, 0x0D},
		{0xFFFF, 0x0000, 0x00}
	};
	for (int i = 0; i < kInputRecordCount; i++)
		_verminBeastInputRecords[i] = verminBeastInputRecords[i];

	const byte aiWeights[TG_BATTLE_V1_2_3_1_AI_WEIGHTS_SIZE] = {
		50, 0, 0, 0, 20, 0, 30, 0,
		50, 0, 0, 0, 30, 0, 20, 0,
		70, 0, 0, 0, 30, 0, 0, 0,
		60, 0, 0, 0, 20, 0, 20, 0,
		80, 0, 0, 0, 20, 0, 0, 0,
		80, 0, 0, 0, 20, 0, 0, 0,
		60, 0, 0, 0, 10, 0, 30, 0,
		50, 0, 0, 40, 10, 0, 0, 0,
		30, 0, 0, 0, 30, 0, 0, 40,
		0, 5, 0, 5, 50, 0, 0, 40,
		0, 5, 0, 5, 40, 10, 0, 40,
		40, 0, 0, 0, 30, 30, 0, 0,
		50, 5, 0, 5, 40, 0, 0, 0,
		60, 5, 0, 5, 30, 0, 0, 0,
		60, 5, 0, 5, 30, 0, 0, 0,
		20, 0, 5, 5, 20, 0, 30, 20,
		40, 5, 0, 5, 40, 0, 0, 10,
		30, 0, 0, 5, 20, 0, 0, 45,
		10, 0, 0, 10, 40, 10, 0, 30,
		30, 10, 0, 0, 35, 20, 0, 5,
		0, 0, 0, 10, 40, 10, 0, 40,
		20, 0, 40, 0, 20, 20, 0, 0,
		0, 10, 0, 10, 40, 0, 0, 40,
		40, 0, 0, 10, 20, 30, 0, 0,
		40, 0, 0, 40, 20, 0, 0, 0,
		70, 0, 0, 5, 25, 0, 0, 0,
		0, 5, 0, 5, 20, 30, 0, 40,
		51, 0, 52, 0
	};
	for (int i = 0; i < TG_BATTLE_V1_2_3_1_AI_WEIGHTS_SIZE; i++)
		_aiWeights[i] = aiWeights[i];

	const uint16 hitSparkAnimations[kDamageClassCount] = {
		0x886D, 0x886C, 0x886E, 0x886C, 0x886B, 0x886E, 0x886D
	};
	for (int i = 0; i < kDamageClassCount; i++)
		_hitSparkAnimations[i] = hitSparkAnimations[i];

	const uint16 weaponLauncher1[kFighterCount] = {
		0x887B, 0x8882
	};
	for (int i = 0; i < kFighterCount; i++)
		_weaponLauncher1[i] = weaponLauncher1[i];

	const uint16 weaponPool1[kFighterCount * kWeaponPoolSize] = {
		0x8883, 0x8889, 0x888A, 0x888B, 0x888C, 0x888D,
		0x888E, 0x888F, 0x8890, 0x8891, 0x8892, 0x8893
	};
	for (int i = 0; i < kFighterCount * kWeaponPoolSize; i++)
		_weaponPool1[i] = weaponPool1[i];

	const uint16 weaponLauncher2[kFighterCount] = {
		0x8894, 0x889D
	};
	for (int i = 0; i < kFighterCount; i++)
		_weaponLauncher2[i] = weaponLauncher2[i];

	const uint16 weaponProjectile2[kFighterCount] = {
		0x889E, 0x88A8
	};
	for (int i = 0; i < kFighterCount; i++)
		_weaponProjectile2[i] = weaponProjectile2[i];

	const uint16 weaponProjectile4[kFighterCount] = {
		0x88A9, 0x88B4
	};
	for (int i = 0; i < kFighterCount; i++)
		_weaponProjectile4[i] = weaponProjectile4[i];

	const uint16 weaponProjectile8[kFighterCount] = {
		0x88B5, 0x88BD
	};
	for (int i = 0; i < kFighterCount; i++)
		_weaponProjectile8[i] = weaponProjectile8[i];

	const uint16 weaponProjectile16[kFighterCount] = {
		0x88BE, 0x88CB
	};
	for (int i = 0; i < kFighterCount; i++)
		_weaponProjectile16[i] = weaponProjectile16[i];

	const uint16 weaponProjectile32[kFighterCount] = {
		0x88CC, 0x88DA
	};
	for (int i = 0; i < kFighterCount; i++)
		_weaponProjectile32[i] = weaponProjectile32[i];

	const uint16 weaponProjectile64[kFighterCount] = {
		0x88DB, 0x88E6
	};
	for (int i = 0; i < kFighterCount; i++)
		_weaponProjectile64[i] = weaponProjectile64[i];

	const uint16 weaponProjectile128[kFighterCount] = {
		0x88E7, 0x88FB
	};
	for (int i = 0; i < kFighterCount; i++)
		_weaponProjectile128[i] = weaponProjectile128[i];

	const FighterInitParams fighterInitParams[TG_BATTLE_V1_2_3_1_FIGHTER_TYPE_COUNT] = {
		{0x834B, 0x004C, 1, 0, 0x8344},
		{0x85B9, 0x02BA, 1, 0, 0x85B2}
	};
	for (int i = 0; i < TG_BATTLE_V1_2_3_1_FIGHTER_TYPE_COUNT; i++)
		_fighterInitParams[i] = fighterInitParams[i];
}

} // End of namespace Topgun
