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

void BattlePlugin::initTablesType0() {
	FighterTypeData *data = &_typeData[0];

	const uint16 varList[kVarListSize] = {
		0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E,
		0x002F, 0x0030, 0x8D6C, 0x8D6D, 0x8D71, 0x8D72, 0x0032
	};
	const Common::Rect rects[kRectCount] = {
		{-37, -19, 53, 22},
		{-53, -19, 37, 22},
		{-35, -53, 60, 44},
		{-60, -53, 35, 44},
		{-3, -10, 57, 52},
		{-57, -10, 3, 52}
	};
	const int16 offsetsX[kRectCount] = {
		0, 0, 0, 0, -48, 48
	};
	const WalkStep walkSteps[kWalkStepCount] = {
		{0x005A, 0x0028},
		{0x0028, 0x0028},
		{0x0028, 0x0028}
	};
	const uint16 knockDistances[kSetCount] = {
		0x0091, 0x0096
	};
	const int16 offsetsY[kSetCount] = {
		141, 114
	};
	const uint16 sprites[kSpriteTableSize] = {
		0x8B96, 0x8D97, 0x8B47, 0x8B8E, 0x8B64
	};
	const uint16 animations[kAnimationTableSize] = {
		0x8B3B, 0x8B9D, 0x8BA0, 0x0033, 0x8BA5, 0x0034, 0x8BA6, 0x8B99, 0x0035, 0x8BA9, 0x8BAA, 0x8BAB,
		0x8BB8, 0x8BB9, 0x8BBF, 0x8BBE, 0x8BBB, 0x8BBA, 0x8BBD, 0x8BB5, 0x8BF1, 0x8BC1, 0x8BC0, 0x8BC2,
		0x0000, 0x0000, 0x8BC4, 0x8BB6, 0x8BC8, 0x8BCB, 0x8BCC, 0x8BC7, 0x8BE0, 0x8BE1, 0x8BDC, 0x8BDD,
		0x8BD1, 0x8BCD, 0x8BE2, 0x8BCF, 0x8BD3, 0x8BD2, 0x8BD5, 0x8BD6, 0x8BD7, 0x8BCE, 0x8BD1, 0x8BD0,
		0x8BD8, 0x8BDA, 0x8BD9, 0x8BF4, 0x8BE3, 0x8BE4, 0x8BE5, 0x8BE6, 0x8BE7, 0x8BCB, 0x8BCC, 0x8BB2,
		0x8BB3, 0x8BB2, 0x8BB3, 0x8BE9, 0x8BEA, 0x8BEB, 0x8BEC, 0x8BC3, 0x8BC3, 0x8BE8, 0x8BE8, 0x8BC3,
		0x8BE8, 0x8BB4, 0x8BEF, 0x8BC5, 0x8BF0, 0x8B9B, 0x8B9B, 0x8BC6, 0x8B9C, 0x8B48, 0x8B49, 0x8D7F,
		0x8DA5, 0x8D98, 0x8B9A, 0x8BBC, 0x8BD4, 0x0000, 0x8BAC, 0x8BAD, 0x8BAE, 0x8BAF, 0x8BB0, 0x8BB1,
		0x8BDE, 0x8BDF, 0x8BED, 0x8BB7, 0x8BDB, 0x8B8F, 0x8BEE
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
		{0x13131409, 0xFFFFFFFF},
		{0x00161513, 0x00FFFFFF},
		{0x00003117, 0x0000FFFF},
		{0x0000090A, 0x0000FFFF},
		{0x00001412, 0x0000FFFF},
		{0xFFFFFFFF, 0x00000000}
	};
	const MoveEntry moves0[23] = {
		{&BattlePlugin::moveIdle, 0x0001, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveWalk, 0x0002, 0x0001, _specialMoves.sparkyPetWalkTurn, 0x0000},
		{&BattlePlugin::moveWalk, 0x0003, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveTurn, 0x0004, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpUp, 0x0005, 0x0001, _specialMoves.sparkyPetJumpSteer, 0x0000},
		{&BattlePlugin::moveWallBounce, 0x0006, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveWallBounce, 0x0007, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveCrouch, 0x000B, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveGuard, 0x000C, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackHoldAnyButton, 0x0008, 0x0001, nullptr, 0x0007},
		{&BattlePlugin::moveAttack, 0x0009, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveKnockOut, 0x000D, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0031, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0032, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpAttack, 0x0033, 0x0001, nullptr, 0x8C0C},
		{&BattlePlugin::moveJumpAttack, 0x0034, 0x0001, nullptr, 0x8C0C},
		{&BattlePlugin::moveGuard, 0x004E, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveSidestep, 0x0043, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveSidestep, 0x0044, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveTransform, 0x000A, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveGrab, 0x0045, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0047, 0x0000, nullptr, 0x0000},
		{nullptr, 0x0000, 0x0000, nullptr, 0x0000}
	};
	const MoveEntry moves1[30] = {
		{&BattlePlugin::moveIdle, 0x0014, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveWalk, 0x0015, 0x0001, _specialMoves.sparkyBeastWalkTurn, 0x0000},
		{&BattlePlugin::moveWalk, 0x0016, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveTurn, 0x0017, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpUp, 0x0019, 0x0001, _specialMoves.sparkyBeastJumpSteerAndDive, 0x8CCC},
		{&BattlePlugin::moveWallBounce, 0x001A, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveWallBounce, 0x001B, 0x0000, nullptr, 0x0000},
		{&BattlePlugin::moveCrouch, 0x0025, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveGuard, 0x0026, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackCloseRange, 0x001C, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackCloseRange, 0x001D, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveKnockOut, 0x0029, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0035, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJump, 0x0036, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveJumpAttack, 0x0037, 0x0001, _specialMoves.sparkyBeastJumpAttackBackBoost, 0x8CCC},
		{&BattlePlugin::moveJumpAttack, 0x0038, 0x0001, _specialMoves.sparkyBeastJumpAttackForwardBoost, 0x8CCC},
		{&BattlePlugin::moveGuard, 0x004F, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttackHoldButton2, 0x001E, 0x0001, nullptr, 0x0006},
		{&BattlePlugin::moveAttack, 0x001F, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0020, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0021, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0022, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x0023, 0x0001, _specialMoves.sparkyBeastSpinAttackChain, 0x0000},
		{&BattlePlugin::moveAttack, 0x0024, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveProjectile, 0x0048, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAttack, 0x004D, 0x0001, _specialMoves.sparkyBeastDiveAirAttack, 0x0000},
		{&BattlePlugin::moveAttackBoost, 0x0000, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveSelectWeapon, 0x0000, 0x0001, nullptr, 0x0000},
		{&BattlePlugin::moveAirAttack, 0x0050, 0x0001, nullptr, 0x0000},
		{nullptr, 0x0000, 0x0000, nullptr, 0x0000}
	};
	const HitBox hits0[10] = {
		{0x8C39, 1, 0, 1, 2, 10, 50, 5, 7, 16, 37, 28},
		{0x8C3A, 1, 0, 1, 0, 10, 50, 5, 6, 9, 110, 75},
		{0x8C3F, 1, 0, 0, 2, 20, 50, 5, 0, 18, 35, 46},
		{0x8C40, 1, 0, 0, 0, 20, 50, 5, 0, 15, 63, 46},
		{0x8C46, 1, 1, 4, 0, 20, 50, 5, 0, 21, 77, 52},
		{0x8C4A, 1, 0, 2, 1, 5, 50, 5, 0, 1, 77, 67},
		{0x8C4D, 1, 0, 2, 0, 5, 50, 5, 1, 65, 68, 65},
		{0x8CB0, 1, -1, 2, 2, 5, 50, 5, 0, 0, 22, 76},
		{0x8CB1, 1, -1, 2, 0, 5, 50, 5, 0, 30, 80, 58},
		{0xFFFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};
	const HitBox hits1[31] = {
		{0x8CB9, 0, 1, 0, 2, 20, 100, 10, 118, 87, 42, 48},
		{0x8CBA, 0, 0, 0, 0, 20, 100, 10, 145, 1, 45, 61},
		{0x8CF2, 0, 0, 0, 0, 20, 100, 10, 66, 58, 49, 81},
		{0x8CBC, 0, 0, 0, 0, 20, 100, 10, 135, 21, 99, 59},
		{0x8CC0, 0, 1, 0, 4, 20, 100, 10, 78, 61, 65, 50},
		{0x8CC1, 0, 0, 0, 2, 20, 100, 10, 154, 21, 57, 69},
		{0x8CC2, 0, 1, 0, 0, 20, 100, 10, 128, 3, 54, 79},
		{0x8CC3, 0, -1, 1, 2, 20, 150, 15, 154, 14, 43, 60},
		{0x8CC4, 0, -1, 1, 0, 20, 150, 15, 114, 1, 50, 103},
		{0x8CD2, 0, 0, 0, 4, 20, 500, 20, 140, 11, 51, 50},
		{0x8CD4, 0, 0, 0, 2, 30, 500, 20, 208, 0, 49, 96},
		{0x8CD5, 0, 0, 6, 0, 40, 500, 20, 229, 0, 85, 44},
		{0x8CE4, 0, -1, 4, 0, 20, 150, 15, 57, 94, 56, 81},
		{0x8CE5, 0, 0, 4, 0, 20, 150, 15, 122, 94, 87, 48},
		{0x8CE6, 0, 1, 4, 0, 20, 150, 15, 5, 109, 117, 59},
		{0x8CED, 0, 1, 3, 0, 20, 150, 15, 90, 51, 151, 59},
		{0x8CEE, 0, -1, 3, 0, 20, 150, 15, 109, 32, 92, 40},
		{0x8CF0, 0, 0, 3, 0, 20, 250, 25, 13, 67, 122, 49},
		{0x8CF1, 0, 1, 3, 0, 20, 250, 25, 25, 90, 160, 35},
		{0x8CDA, 0, -1, 6, 0, 80, 1000, 30, 115, 27, 49, 60},
		{0x8CDD, 0, 1, 6, 0, 80, 1000, 30, 95, 0, 48, 90},
		{0x8D27, 0, 2, 2, 7, 40, 150, 15, 34, 73, 66, 49},
		{0x8D28, 0, 2, 2, 5, 40, 150, 15, 32, 58, 71, 53},
		{0x8D29, 0, 2, 2, 3, 40, 150, 15, 32, 58, 71, 53},
		{0x8CB7, 0, 0, 0, 0, 30, 150, 15, 145, 1, 45, 61},
		{0x8CB8, 0, 0, 0, 0, 30, 150, 15, 154, 21, 57, 69},
		{0x8D20, 0, 2, 2, 9, 40, 1000, 30, 34, 73, 66, 49},
		{0x8D21, 0, 2, 2, 7, 40, 1000, 30, 32, 58, 71, 53},
		{0x8D22, 0, 2, 2, 5, 40, 1000, 30, 32, 58, 71, 53},
		{0x8D23, 0, 2, 2, 3, 40, 1000, 30, 32, 58, 71, 53},
		{0xFFFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};
	const AiRecord aiList[23] = {
		{0x09, 0x00, 0x0000, 0x0000, 0x0000, 10, 136, 0x0082, 0x0000, 0, 0, 0},
		{0x0A, 0x00, 0x0000, 0x0000, 0x0000, 10, 160, 0x0002, 0x0000, 0, 0, 0},
		{0x13, 0x00, 0x0000, 0x0000, 0x0000, 30, 240, 0x0008, 0x0000, 0, 0, 0},
		{0x09, 0x0A, 0x0000, 0x0000, 0x0000, 15, 160, 0x0082, 0x0000, 0, 0, 0},
		{0x11, 0x00, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000, 0, 0, 0},
		{0x14, 0x00, 0x0000, 0x0000, 0x0000, 15, 100, 0x0189, 0x0000, 0, 0, 0},
		{0x15, 0x00, 0x0000, 0x0000, 0x0000, 15, 220, 0x0103, 0x0000, 0, 0, 0},
		{0x16, 0x00, 0x0000, 0x0000, 0x0000, 10, 250, 0x000B, 0x0000, 0, 0, 0},
		{0x18, 0x00, 0x0000, 0x0000, 0x0000, 30, 217, 0x0220, 0x0000, 0, 0, 0},
		{0x13, 0x13, 0x0914, 0x0000, 0x0000, 18, 180, 0x0108, 0x0000, 0, 0, 0},
		{0x16, 0x15, 0x0013, 0x0000, 0x0000, 18, 240, 0x014B, 0x0000, 0, 0, 0},
		{0x04, 0x0F, 0x1716, 0x0007, 0x0008, 30, 300, 0x0063, 0x0000, 0, 0, 0},
		{0x14, 0x12, 0x0000, 0x0000, 0x0000, 15, 170, 0x0188, 0x0000, 0, 0, 0},
		{0x16, 0x17, 0x0000, 0x0008, 0x0000, 30, 250, 0x000F, 0x0000, 0, 0, 0},
		{0x12, 0x00, 0x0000, 0x0000, 0x0000, 10, 170, 0x0000, 0x0000, 0, 0, 0},
		{0x04, 0x0E, 0x001A, 0x0707, 0x0000, 15, 250, 0x0464, 0x00C0, 0, 0, 0},
		{0x04, 0x0F, 0x001A, 0x0704, 0x0000, 15, 250, 0x0260, 0x00C1, 0, 0, 0},
		{0x04, 0x19, 0x0000, 0x000A, 0x0000, 15, 140, 0x0233, 0x0000, 0, 0, 0},
		{0x04, 0x0E, 0x0018, 0x0004, 0x0000, 30, 120, 0x0460, 0x0000, 0, 0, 0},
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
