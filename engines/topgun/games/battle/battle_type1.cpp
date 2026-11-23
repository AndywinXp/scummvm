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

void BattlePlugin::initTablesType1() {
	FighterTypeData *data = &_typeData[1];

	const uint16 varList[kVarListSize] = {
		0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D,
		0x003E, 0x003F, 0x8FE3, 0x8FE4, 0x8FE8, 0x8FE9, 0x0041
	};
	const Common::Rect rects[kRectCount] = {
		{-17, -30, 54, 33},
		{-54, -30, 17, 33},
		{-46, -53, 63, 40},
		{-63, -53, 46, 40},
		{1, -12, 50, 62},
		{-50, -12, -1, 62}
	};
	const int16 offsetsX[kRectCount] = {
		0, 0, 0, 0, -22, 22
	};
	const WalkStep walkSteps[kWalkStepCount] = {
		{0x003C, 0x0028},
		{0x0028, 0x0028},
		{0x0028, 0x0028}
	};
	const uint16 knockDistances[kSetCount] = {
		0x0085, 0x0062
	};
	const int16 offsetsY[kSetCount] = {
		135, 118
	};
	const uint16 sprites[kSpriteTableSize] = {
		0x8E0F, 0x900A, 0x8DC9, 0x8E08, 0x8DE6
	};
	const uint16 animations[kAnimationTableSize] = {
		0x8DBD, 0x8E16, 0x8E19, 0x0042, 0x8E1E, 0x0043, 0x8E1F, 0x8E12, 0x0044, 0x8E22, 0x8E23, 0x8E24,
		0x8E33, 0x8E34, 0x8E3A, 0x8E39, 0x8E36, 0x8E35, 0x8E38, 0x8E30, 0x8E6A, 0x8E3C, 0x8E3B, 0x8E3D,
		0x0000, 0x0000, 0x8E3F, 0x8E31, 0x8E43, 0x8E46, 0x8E47, 0x8E42, 0x8E5B, 0x8E5C, 0x8E57, 0x8E58,
		0x8E4C, 0x8E48, 0x8E5D, 0x8E4A, 0x8E4E, 0x8E4D, 0x8E50, 0x8E51, 0x8E52, 0x8E48, 0x8E49, 0x8E4B,
		0x8E53, 0x8E55, 0x8E54, 0x8E6D, 0x8E5E, 0x8E5F, 0x8E60, 0x8E61, 0x8E62, 0x8E46, 0x8E47, 0x8E2B,
		0x8E2C, 0x8E2B, 0x8E2C, 0x8E64, 0x8E65, 0x8E66, 0x8E67, 0x8E3E, 0x8E3E, 0x8E63, 0x8E63, 0x8E3E,
		0x8E63, 0x8E2F, 0x8E68, 0x8E40, 0x8E69, 0x8E14, 0x8E14, 0x8E41, 0x8E15, 0x8DCA, 0x8DCB, 0x8FFB,
		0x9016, 0x900B, 0x8E13, 0x8E37, 0x8E4F, 0x902C, 0x8E25, 0x8E26, 0x8E27, 0x8E28, 0x8E29, 0x8E2A,
		0x8E59, 0x8E5A, 0x8E2D, 0x8E32, 0x8E56, 0x8E09, 0x8E2E
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
		{0x12121415, 0xFFFFFFFF},
		{0x00313109, 0x00FFFFFF},
		{0x00001116, 0x0000FFFF},
		{0x00001617, 0x0000FFFF},
		{0x0000090A, 0x0000FFFF},
		{0xFFFFFFFF, 0x00000000}
	};
	const MoveEntry moves0[22] = {
		{&BattlePlugin::moveIdle, 0x0001, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveWalk, 0x0002, 0x0001, _specialMoves.verminPetWalkTurn, 0x0000},
		{&BattlePlugin::moveWalk, 0x0003, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveTurn, 0x0004, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpUp, 0x0005, 0x0001, _specialMoves.verminPetJumpSteer, 0x0000},
		{&BattlePlugin::moveWallBounce, 0x0006, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveWallBounce, 0x0007, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveCrouch, 0x000B, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveGuard, 0x000C, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackHoldAnyButton, 0x0008, 0x0001, nullptr, 0x0007},
		{&BattlePlugin::moveAttack, 0x0009, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveKnockOut, 0x000D, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0031, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0032, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpAttack, 0x0033, 0x0001, nullptr, 0x8E85},
		{&BattlePlugin::moveJumpAttack, 0x0034, 0x0001, nullptr, 0x8E85},
		{&BattlePlugin::moveGuard, 0x004E, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveSidestep, 0x0043, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveSidestep, 0x0044, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveTransform, 0x000A, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveGrab, 0x0045, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0047, 0x0000, nullptr, 0x0000}
	};
	const MoveEntry moves1[29] = {
		{&BattlePlugin::moveIdle, 0x0014, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveWalk, 0x0015, 0x0001, _specialMoves.verminBeastWalkTurn, 0x0000},
		{&BattlePlugin::moveWalk, 0x0016, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveTurn, 0x0017, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpUp, 0x0019, 0x0001, _specialMoves.verminBeastJumpSteerAndDive, 0x8F42},
		{&BattlePlugin::moveWallBounce, 0x001A, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveWallBounce, 0x001B, 0x0000, _specialMoves.verminBeastDiveAirAttack, 0x0000},
		{&BattlePlugin::moveCrouch, 0x0025, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveGuard, 0x0026, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackCloseRange, 0x001C, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackCloseRange, 0x001D, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveKnockOut, 0x0029, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0035, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0036, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpAttack, 0x0037, 0x0001, _specialMoves.verminBeastJumpAttackBackBoost, 0x8F42},
		{&BattlePlugin::moveJumpAttack, 0x0038, 0x0001, _specialMoves.verminBeastJumpAttackForwardBoost, 0x8F42},
		{&BattlePlugin::moveGuard, 0x004F, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackHoldButton2, 0x001E, 0x0001, nullptr, 0x0006},
		{&BattlePlugin::moveAttack, 0x001F, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0020, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0021, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0022, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0023, 0x0001, _specialMoves.verminBeastSpinAttackChain, 0x0000},
		{&BattlePlugin::moveAttack, 0x0024, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveProjectile, 0x0048, 0x0001, nullptr, 0x9029},
		{&BattlePlugin::moveAttack, 0x004D, 0x0001, _specialMoves.verminBeastDiveAirAttack, 0x0000},
		{&BattlePlugin::moveAttackBoost, 0x0000, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveSelectWeapon, 0x0000, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAirAttack, 0x0050, 0x0001, nullptr, 0x0000}
	};
	const HitBox hits0[9] = {
		{0x8EB2, 1, 0, 1, 2, 10, 50, 5, 3, 5, 49, 44},
		{0x8EB3, 1, 0, 1, 0, 10, 50, 5, 0, 0, 100, 83},
		{0x8EB8, 1, 0, 0, 2, 20, 50, 5, 0, 14, 35, 45},
		{0x8EB9, 1, 0, 0, 0, 20, 50, 5, 0, 4, 58, 66},
		{0x8EBF, 1, 0, 4, 0, 20, 50, 5, 0, 25, 114, 37},
		{0x8EC3, 1, 0, 2, 2, 5, 50, 5, 0, 23, 113, 30},
		{0x8EC6, 1, 0, 2, 0, 5, 50, 5, 1, 75, 108, 60},
		{0x8F26, 1, -1, 2, 0, 5, 50, 5, 0, 30, 80, 58},
		{0xFFFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};
	const HitBox hits1[28] = {
		{0x8F2E, 0, 1, 0, 2, 20, 100, 10, 73, 72, 51, 64},
		{0x8F2F, 0, 1, 0, 0, 20, 100, 10, 96, 67, 141, 46},
		{0x8F65, 0, 0, 0, 0, 20, 100, 10, 132, 8, 66, 93},
		{0x8F30, 0, 0, 0, 0, 20, 150, 15, 6, 122, 126, 84},
		{0x8F31, 0, 0, 0, 0, 20, 150, 15, 132, 0, 186, 103},
		{0x8F36, 0, 0, 0, 0, 20, 200, 20, 125, 17, 121, 58},
		{0x8F3A, 0, -1, 1, 2, 20, 100, 10, 103, 35, 125, 60},
		{0x8F3B, 0, -1, 1, 0, 20, 100, 10, 132, 0, 71, 109},
		{0x8F48, 0, 0, 0, 4, 20, 500, 20, 140, 11, 51, 50},
		{0x8F49, 0, 0, 0, 2, 30, 500, 20, 208, 0, 49, 96},
		{0x8F4A, 0, 0, 6, 0, 40, 500, 20, 229, 0, 85, 44},
		{0x8F56, 0, 1, 4, 0, 20, 150, 15, 45, 92, 53, 60},
		{0x8F57, 0, 1, 4, 0, 20, 150, 15, 93, 49, 170, 61},
		{0x8F5F, 0, 1, 3, 0, 20, 100, 10, 0, 60, 125, 58},
		{0x8F60, 0, 1, 3, 0, 20, 100, 10, 140, 21, 223, 54},
		{0x8F61, 0, -1, 3, 0, 20, 100, 10, 50, 1, 87, 63},
		{0x8F63, 0, 0, 3, 0, 20, 150, 15, 0, 0, 113, 56},
		{0x8F64, 0, 1, 3, 0, 20, 150, 15, 0, 44, 110, 57},
		{0x8F9B, 0, 2, 2, 7, 40, 200, 20, 68, 102, 82, 130},
		{0x8F9C, 0, 2, 2, 5, 40, 200, 20, 20, 94, 117, 64},
		{0x8F9D, 0, 2, 2, 3, 40, 200, 20, 20, 94, 117, 64},
		{0x8F2D, 0, 0, 0, 0, 20, 250, 25, 125, 17, 121, 58},
		{0x8F2C, 0, 0, 0, 0, 20, 150, 15, 96, 67, 141, 46},
		{0x8F94, 0, 2, 2, 9, 40, 1000, 30, 68, 102, 82, 130},
		{0x8F95, 0, 2, 2, 7, 40, 1000, 30, 68, 102, 82, 130},
		{0x8F96, 0, 2, 2, 5, 40, 1000, 30, 20, 94, 117, 64},
		{0x8F97, 0, 2, 2, 3, 40, 1000, 30, 20, 94, 117, 64},
		{0xFFFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};
	const AiRecord aiList[21] = {
		{0x09, 0x00, 0x0000, 0x0000, 0x0000, 20, 192, 0x0080, 0x0000, 0, 0, 0},
		{0x0A, 0x00, 0x0000, 0x0000, 0x0000, 10, 183, 0x0002, 0x0000, 0, 0, 0},
		{0x13, 0x00, 0x0000, 0x0000, 0x0000, 30, 208, 0x0008, 0x0000, 0, 0, 0},
		{0x09, 0x0A, 0x0000, 0x0000, 0x0000, 20, 183, 0x018A, 0x0000, 0, 0, 0},
		{0x11, 0x00, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000, 0, 0, 0},
		{0x14, 0x00, 0x0000, 0x0000, 0x0000, 10, 155, 0x0189, 0x0000, 0, 0, 0},
		{0x15, 0x00, 0x0000, 0x0000, 0x0000, 15, 193, 0x0103, 0x0000, 0, 0, 0},
		{0x16, 0x00, 0x0000, 0x0000, 0x0000, 10, 200, 0x000B, 0x0000, 0, 0, 0},
		{0x18, 0x00, 0x0000, 0x0000, 0x0000, 30, 600, 0x0023, 0x0000, 0, 0, 0},
		{0x12, 0x12, 0x1514, 0x0000, 0x0000, 14, 190, 0x010B, 0x0000, 0, 0, 0},
		{0x11, 0x16, 0x0000, 0x0000, 0x0000, 20, 300, 0x0063, 0x0000, 0, 0, 0},
		{0x16, 0x17, 0x0000, 0x0008, 0x0000, 12, 200, 0x000F, 0x0000, 0, 0, 0},
		{0x12, 0x00, 0x0000, 0x0000, 0x0000, 15, 190, 0x0000, 0x0000, 0, 0, 0},
		{0x04, 0x0E, 0x001A, 0x0707, 0x0000, 25, 250, 0x0464, 0x00BC, 0, 0, 0},
		{0x04, 0x0F, 0x001A, 0x0704, 0x0000, 15, 250, 0x0260, 0x00BD, 0, 0, 0},
		{0x04, 0x19, 0x0000, 0x000A, 0x0000, 20, 140, 0x0233, 0x0000, 0, 0, 0},
		{0x04, 0x0E, 0x0018, 0x0004, 0x0000, 30, 240, 0x0463, 0x0000, 0, 0, 0},
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
