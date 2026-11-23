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

#include "common/util.h"

#include "topgun/games/battle/battle.h"

namespace Topgun {

void BattlePlugin::initTablesType2() {
	FighterTypeData *data = &_typeData[2];

	const uint16 varList[kVarListSize] = {
		0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E,
		0x004F, 0x0050, 0x9262, 0x9263, 0x9267, 0x9268, 0x0052
	};
	const Common::Rect rects[kRectCount] = {
		{-30, -33, 30, 30},
		{-30, -33, 30, 30},
		{-45, -36, 48, 46},
		{-48, -36, 45, 46},
		{7, -18, 53, 63},
		{-53, -18, -7, 63}
	};
	const int16 offsetsX[kRectCount] = {
		0, 0, 0, 0, -50, 50
	};
	const WalkStep walkSteps[kWalkStepCount] = {
		{0x0060, 0x0020},
		{0x0028, 0x0028},
		{0x0028, 0x0028}
	};
	const uint16 knockDistances[kSetCount] = {
		0x0084, 0x003C
	};
	const int16 offsetsY[kSetCount] = {
		134, 118
	};
	const uint16 sprites[kSpriteTableSize] = {
		0x908E, 0x9287, 0x9045, 0x9086, 0x907C
	};
	const uint16 animations[kAnimationTableSize] = {
		0x9039, 0x9095, 0x9098, 0x0053, 0x909D, 0x0054, 0x909E, 0x9091, 0x0055, 0x90A1, 0x90A2, 0x90A3,
		0x90B2, 0x90B3, 0x90B9, 0x90B8, 0x90B5, 0x90B4, 0x90B7, 0x90AF, 0x90E9, 0x90BB, 0x90BA, 0x90BC,
		0x0000, 0x0000, 0x90BE, 0x90B0, 0x90C2, 0x90C5, 0x90C6, 0x90C1, 0x90DA, 0x90DB, 0x90D6, 0x90D7,
		0x90CB, 0x90C7, 0x90DC, 0x90C9, 0x90CD, 0x90CC, 0x90CF, 0x90D0, 0x90D1, 0x90C7, 0x90C8, 0x90CA,
		0x90D2, 0x90D4, 0x90D3, 0x90EC, 0x90DD, 0x90DE, 0x90DF, 0x90E0, 0x90E1, 0x90C5, 0x90C6, 0x90AA,
		0x90AB, 0x90AA, 0x90AB, 0x90E3, 0x90E4, 0x90E5, 0x90E6, 0x90BD, 0x90BD, 0x90E2, 0x90E2, 0x90BD,
		0x90E2, 0x90AE, 0x90E7, 0x90BF, 0x90E8, 0x9093, 0x9093, 0x90C0, 0x9094, 0x9046, 0x9047, 0x9274,
		0x9295, 0x9288, 0x9092, 0x90B6, 0x90CE, 0x92A9, 0x90A4, 0x90A5, 0x90A6, 0x90A7, 0x90A8, 0x90A9,
		0x90D8, 0x90D9, 0x90AC, 0x90B1, 0x90D5, 0x9087, 0x90AD
	};
	const uint16 remap[kRemapSize] = {
		0x0000, 0x0001, 0x0003, 0x0005, 0x0007, 0x0008, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x0012,
		0x0013, 0x0014, 0x0015, 0x0016, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
		0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B,
		0x002C, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039,
		0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045,
		0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0057,
		0x0058, 0x005C, 0x005D, 0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0066
	};
	const ComboRecord combos[kComboCount] = {
		{0x19151512, 0xFFFFFFFF},
		{0x001A1416, 0x00FFFFFF},
		{0x00001215, 0x0000FFFF},
		{0x00001617, 0x0000FFFF},
		{0x0000090A, 0x0000FFFF},
		{0xFFFFFFFF, 0x00000000}
	};
	const MoveEntry moves0[22] = {
		{&BattlePlugin::moveIdle, 0x0001, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveWalk, 0x0002, 0x0001, _specialMoves.nickyPetWalkTurn, 0x0000},
		{&BattlePlugin::moveWalk, 0x0003, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveTurn, 0x0004, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpUp, 0x0005, 0x0001, _specialMoves.nickyPetJumpSteer, 0x0000},
		{&BattlePlugin::moveWallBounce, 0x0006, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveWallBounce, 0x0007, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveCrouch, 0x000B, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveGuard, 0x000C, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackHoldAnyButton, 0x0008, 0x0001, nullptr, 0x0007},
		{&BattlePlugin::moveAttack, 0x0009, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveKnockOut, 0x000D, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0031, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0032, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpAttack, 0x0033, 0x0001, nullptr, 0x9104},
		{&BattlePlugin::moveJumpAttack, 0x0034, 0x0001, nullptr, 0x9104},
		{&BattlePlugin::moveGuard, 0x004E, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveSidestep, 0x0043, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveSidestep, 0x0044, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveTransform, 0x000A, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveGrab, 0x0045, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0047, 0x0000, nullptr, 0x0000}
	};
	const MoveEntry moves1[29] = {
		{&BattlePlugin::moveIdle, 0x0014, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveWalk, 0x0015, 0x0001, _specialMoves.nickyBeastWalkTurn, 0x0000},
		{&BattlePlugin::moveWalk, 0x0016, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveTurn, 0x0017, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpUp, 0x0019, 0x0001, _specialMoves.nickyBeastJumpSteerAndDive, 0x91C8},
		{&BattlePlugin::moveWallBounce, 0x001A, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveWallBounce, 0x001B, 0x0000, _specialMoves.nickyBeastDiveAirAttack, 0x0000},
		{&BattlePlugin::moveCrouch, 0x0025, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveGuard, 0x0026, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackCloseRange, 0x001C, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackCloseRange, 0x001D, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveKnockOut, 0x0029, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0035, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0036, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpAttack, 0x0037, 0x0001, _specialMoves.nickyBeastJumpAttackBackBoost, 0x91C8},
		{&BattlePlugin::moveJumpAttack, 0x0038, 0x0001, _specialMoves.nickyBeastJumpAttackForwardBoost, 0x91C8},
		{&BattlePlugin::moveGuard, 0x004F, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackHoldButton2, 0x001E, 0x0001, nullptr, 0x0006},
		{&BattlePlugin::moveAttack, 0x001F, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0020, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0021, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0022, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0023, 0x0001, _specialMoves.nickyBeastSpinAttackChain, 0x0000},
		{&BattlePlugin::moveAttack, 0x0024, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveProjectile, 0x0048, 0x0001, nullptr, 0x92A6},
		{&BattlePlugin::moveAttack, 0x004D, 0x0001, _specialMoves.nickyBeastDiveAirAttack, 0x0000},
		{&BattlePlugin::moveAttackBoost, 0x0000, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveSelectWeapon, 0x0000, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAirAttack, 0x0050, 0x0001, nullptr, 0x0000}
	};
	const HitBox hits0[9] = {
		{0x9132, 1, 0, 1, 0, 10, 50, 5, 0, 0, 42, 47},
		{0x9133, 1, 0, 1, 0, 10, 50, 5, 0, 0, 37, 82},
		{0x9138, 1, 0, 0, 2, 20, 50, 5, 0, 0, 41, 56},
		{0x9139, 1, 0, 0, 0, 20, 50, 5, 0, 0, 44, 73},
		{0x913F, 1, 0, 4, 0, 20, 50, 5, 0, 25, 108, 37},
		{0x9143, 1, 0, 2, 2, 5, 50, 5, 0, 11, 134, 48},
		{0x9146, 1, 0, 2, 0, 5, 50, 5, 0, 70, 119, 41},
		{0x91AB, 1, -1, 2, 0, 5, 50, 5, 0, 25, 117, 65},
		{0xFFFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};
	const HitBox hits1[24] = {
		{0x91B3, 0, 1, 0, 0, 20, 150, 15, 86, 80, 68, 75},
		{0x91B4, 0, 1, 0, 0, 20, 150, 15, 144, 16, 92, 66},
		{0x91EB, 0, 0, 0, 0, 20, 100, 10, 120, 27, 82, 59},
		{0x91B8, 0, 0, 0, 0, 20, 100, 10, 175, 21, 115, 72},
		{0x91BE, 0, 0, 0, 0, 20, 100, 10, 149, 14, 78, 62},
		{0x91C1, 0, 1, 1, 0, 20, 100, 10, 148, 60, 62, 51},
		{0x91C2, 0, 1, 1, 0, 20, 100, 10, 138, 1, 54, 91},
		{0x91CE, 0, 0, 0, 4, 30, 500, 20, 150, 0, 69, 59},
		{0x91CF, 0, 0, 6, 2, 40, 500, 20, 195, 0, 116, 60},
		{0x91D8, 0, -1, 4, 0, 20, 150, 15, 51, 100, 76, 55},
		{0x91D9, 0, -1, 4, 0, 20, 150, 15, 127, 77, 114, 74},
		{0x91E0, 0, 1, 3, 0, 20, 150, 15, 0, 107, 97, 58},
		{0x91E1, 0, 1, 3, 0, 20, 150, 15, 101, 82, 162, 47},
		{0x91E2, 0, -1, 3, 0, 20, 150, 15, 114, 38, 126, 48},
		{0x91E5, 0, 0, 3, 0, 20, 250, 25, 0, 54, 113, 50},
		{0x91E6, 0, 1, 3, 0, 20, 250, 25, 0, 101, 109, 67},
		{0x9221, 0, 2, 2, 5, 40, 150, 15, 30, 89, 83, 50},
		{0x9222, 0, 2, 2, 3, 40, 150, 15, 30, 89, 83, 50},
		{0x91B2, 0, 0, 0, 0, 20, 150, 15, 149, 14, 78, 62},
		{0x91B1, 0, 0, 0, 0, 20, 200, 20, 144, 16, 92, 66},
		{0x921A, 0, 2, 2, 7, 40, 1000, 30, 30, 89, 83, 50},
		{0x921B, 0, 2, 2, 5, 40, 1000, 30, 30, 89, 83, 50},
		{0x921C, 0, 2, 2, 3, 40, 1000, 30, 30, 89, 83, 50},
		{0xFFFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};
	const AiRecord aiList[22] = {
		{0x09, 0x00, 0x0000, 0x0000, 0x0000, 10, 198, 0x0080, 0x0000, 0, 0, 0},
		{0x0A, 0x00, 0x0000, 0x0000, 0x0000, 15, 188, 0x0002, 0x0000, 0, 0, 0},
		{0x13, 0x00, 0x0000, 0x0000, 0x0000, 30, 259, 0x0008, 0x0000, 0, 0, 0},
		{0x09, 0x0A, 0x0000, 0x0000, 0x0000, 17, 190, 0x018A, 0x0000, 0, 0, 0},
		{0x11, 0x00, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000, 0, 0, 0},
		{0x14, 0x00, 0x0000, 0x0000, 0x0000, 10, 176, 0x0189, 0x0000, 0, 0, 0},
		{0x15, 0x00, 0x0000, 0x0000, 0x0000, 15, 200, 0x0103, 0x0000, 0, 0, 0},
		{0x16, 0x00, 0x0000, 0x0000, 0x0000, 15, 225, 0x000B, 0x0000, 0, 0, 0},
		{0x18, 0x00, 0x0000, 0x0000, 0x0000, 30, 600, 0x0023, 0x0000, 0, 0, 0},
		{0x15, 0x15, 0x0012, 0x0000, 0x0000, 13, 200, 0x010B, 0x0000, 0, 0, 0},
		{0x0C, 0x1A, 0x1614, 0x0007, 0x0000, 12, 300, 0x010B, 0x00C2, 0, 0, 0},
		{0x12, 0x15, 0x0000, 0x0000, 0x0000, 10, 240, 0x0063, 0x0000, 0, 0, 0},
		{0x16, 0x17, 0x0000, 0x0008, 0x0000, 22, 225, 0x000F, 0x0000, 0, 0, 0},
		{0x12, 0x00, 0x0000, 0x0000, 0x0000, 5, 240, 0x0000, 0x0000, 0, 0, 0},
		{0x04, 0x0E, 0x001A, 0x0707, 0x0000, 15, 250, 0x0464, 0x00C2, 0, 0, 0},
		{0x04, 0x0F, 0x001A, 0x0704, 0x0000, 20, 250, 0x0260, 0x00C3, 0, 0, 0},
		{0x04, 0x19, 0x0000, 0x000A, 0x0000, 15, 140, 0x0233, 0x0000, 0, 0, 0},
		{0x04, 0x0E, 0x0018, 0x0004, 0x0000, 30, 120, 0x0463, 0x0000, 0, 0, 0},
		{0x1B, 0x11, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000, 0, 0, 0},
		{0x11, 0x0D, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000, 0, 0, 0},
		{0x11, 0x0C, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000, 0, 0, 0},
		{0xFF, 0xFF, 0xFFFF, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};

	for (uint i = 0; i < ARRAYSIZE(varList); i++)
		data->varList[i] = varList[i];
	for (uint i = 0; i < ARRAYSIZE(rects); i++)
		data->rects[i] = rects[i];
	for (uint i = 0; i < ARRAYSIZE(offsetsX); i++)
		data->offsetsX[i] = offsetsX[i];
	for (uint i = 0; i < ARRAYSIZE(walkSteps); i++)
		data->walkSteps[i] = walkSteps[i];
	for (uint i = 0; i < ARRAYSIZE(knockDistances); i++)
		data->knockDistances[i] = knockDistances[i];
	for (uint i = 0; i < ARRAYSIZE(offsetsY); i++)
		data->offsetsY[i] = offsetsY[i];
	for (uint i = 0; i < ARRAYSIZE(sprites); i++)
		data->sprites[i] = sprites[i];
	for (uint i = 0; i < ARRAYSIZE(animations); i++)
		data->animations[i] = animations[i];
	for (uint i = 0; i < ARRAYSIZE(remap); i++)
		data->remap[i] = remap[i];
	for (uint i = 0; i < ARRAYSIZE(combos); i++)
		data->combos[i] = combos[i];
	for (uint i = 0; i < ARRAYSIZE(moves0); i++)
		data->moves[0][i] = moves0[i];
	for (uint i = 0; i < ARRAYSIZE(moves1); i++)
		data->moves[1][i] = moves1[i];
	for (uint i = 0; i < ARRAYSIZE(hits0); i++)
		data->hits[0][i] = hits0[i];
	for (uint i = 0; i < ARRAYSIZE(hits1); i++)
		data->hits[1][i] = hits1[i];
	for (uint i = 0; i < ARRAYSIZE(aiList); i++)
		data->aiList[i] = aiList[i];
}

} // End of namespace Topgun
