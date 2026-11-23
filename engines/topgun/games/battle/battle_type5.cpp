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

#include "topgun/games/battle/battle.h"

namespace Topgun {

void BattlePlugin::initTablesType5() {
	FighterTypeData *t = &_typeData[5];

	const uint16 varList[kVarListSize] = {
		0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E,
		0x006F, 0x0070, 0x9755, 0x9756, 0x975A, 0x975B, 0x0072
	};
	const Common::Rect rects[kRectCount] = {
		{-31, -38, 40, 40},
		{-40, -38, 31, 26},
		{-41, -39, 57, 49},
		{-57, -39, 41, 49},
		{-12, -7, 39, 67},
		{-39, -7, 12, 67}
	};
	const int16 offsetsX[kRectCount] = {0, 0, 0, 0, -45, 45};
	const WalkStep walkSteps[kWalkStepCount] = {{73, 25}, {25, 25}, {40, 40}};
	const uint16 knockDistances[kSetCount] = {168, 103};
	const int16 offsetsY[kSetCount] = {132, 118};
	const uint16 sprites[kSpriteTableSize] = {0x9581, 0x9781, 0x9536, 0x9579, 0x9552};
	const uint16 animations[kAnimationTableSize] = {
		0x952A, 0x9588, 0x958B, 0x0073, 0x9590, 0x0074, 0x9591, 0x9584, 0x0075, 0x9594, 0x9595, 0x9596,
		0x95A5, 0x95A6, 0x95AC, 0x95AB, 0x95A8, 0x95A7, 0x95AA, 0x95A2, 0x95DC, 0x95AE, 0x95AD, 0x95AF,
		0x0000, 0x0000, 0x95B1, 0x95A3, 0x95B5, 0x95B8, 0x95B9, 0x95B4, 0x95CD, 0x95CE, 0x95C9, 0x95CA,
		0x95BE, 0x95BA, 0x95CF, 0x95BC, 0x95C0, 0x95BF, 0x95C2, 0x95C3, 0x95C4, 0x95BB, 0x95BE, 0x95BD,
		0x95C5, 0x95C7, 0x95C6, 0x95DF, 0x95D0, 0x95D1, 0x95D2, 0x95D3, 0x95D4, 0x95B8, 0x95B9, 0x959D,
		0x959E, 0x959D, 0x959E, 0x95D6, 0x95D7, 0x95D8, 0x95D9, 0x95B0, 0x95B0, 0x95D5, 0x95D5, 0x95B0,
		0x95D5, 0x95A1, 0x95DA, 0x95B2, 0x95DB, 0x9586, 0x9586, 0x95B3, 0x9587, 0x9537, 0x9538, 0x9767,
		0x978F, 0x9782, 0x9585, 0x95A9, 0x95C1, 0x97A2, 0x9597, 0x9598, 0x9599, 0x959A, 0x959B, 0x959C,
		0x95CB, 0x95CC, 0x959F, 0x95A4, 0x95C8, 0x957A, 0x95A0
	};
	const uint16 remap[kRemapSize] = {
		0x00, 0x01, 0x03, 0x05, 0x07, 0x08, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x12, 0x13, 0x14, 0x15, 0x16,
		0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
		0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
		0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x57, 0x58, 0x5C, 0x5D, 0x60, 0x61, 0x62, 0x63, 0x64,
		0x66
	};
	const ComboRecord combos[kComboCount] = {
		{0x09161415, 0xFFFFFFFF},
		{0x00121416, 0x00FFFFFF},
		{0x00001311, 0x0000FFFF},
		{0x00001617, 0x0000FFFF},
		{0x0000090A, 0x0000FFFF},
		{0xFFFFFFFF, 0x00000000}
	};
	const MoveEntry moves0[kMoveCount] = {
		{&BattlePlugin::moveIdle, 0x0001, 1, nullptr, 0},
		{&BattlePlugin::moveWalk, 0x0002, 1, _specialMoves.torPetWalkTurn, 0},
		{&BattlePlugin::moveWalk, 0x0003, 1, nullptr, 0},
		{&BattlePlugin::moveTurn, 0x0004, 1, nullptr, 0},
		{&BattlePlugin::moveJumpUp, 0x0005, 1, _specialMoves.torPetJumpSteer, 0},
		{&BattlePlugin::moveWallBounce, 0x0006, 0, nullptr, 0},
		{&BattlePlugin::moveWallBounce, 0x0007, 0, nullptr, 0},
		{&BattlePlugin::moveCrouch, 0x000B, 0, nullptr, 0},
		{&BattlePlugin::moveGuard, 0x000C, 1, nullptr, 0},
		{&BattlePlugin::moveAttackHoldAnyButton, 0x0008, 1, nullptr, 0x0007},
		{&BattlePlugin::moveAttack, 0x0009, 1, nullptr, 0},
		{&BattlePlugin::moveKnockOut, 0x000D, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0031, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0032, 1, nullptr, 0},
		{&BattlePlugin::moveJumpAttack, 0x0033, 1, nullptr, 0x95F7},
		{&BattlePlugin::moveJumpAttack, 0x0034, 1, nullptr, 0x95F7},
		{&BattlePlugin::moveGuard, 0x004E, 1, nullptr, 0},
		{&BattlePlugin::moveSidestep, 0x0043, 1, nullptr, 0},
		{&BattlePlugin::moveSidestep, 0x0044, 1, nullptr, 0},
		{&BattlePlugin::moveTransform, 0x000A, 1, nullptr, 0},
		{&BattlePlugin::moveGrab, 0x0045, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0047, 0, nullptr, 0}
	};
	const MoveEntry moves1[kMoveCount] = {
		{&BattlePlugin::moveIdle, 0x0014, 1, nullptr, 0},
		{&BattlePlugin::moveWalk, 0x0015, 1, _specialMoves.torBeastWalkTurn, 0},
		{&BattlePlugin::moveWalk, 0x0016, 1, nullptr, 0},
		{&BattlePlugin::moveTurn, 0x0017, 1, nullptr, 0},
		{&BattlePlugin::moveJumpUp, 0x0019, 1, _specialMoves.torBeastJumpSteerAndDive, 0x96B7},
		{&BattlePlugin::moveWallBounce, 0x001A, 0, nullptr, 0},
		{&BattlePlugin::moveWallBounce, 0x001B, 0, _specialMoves.torBeastDiveAirAttack, 0},
		{&BattlePlugin::moveCrouch, 0x0025, 1, nullptr, 0},
		{&BattlePlugin::moveGuard, 0x0026, 1, nullptr, 0},
		{&BattlePlugin::moveAttackCloseRange, 0x001C, 1, nullptr, 0},
		{&BattlePlugin::moveAttackCloseRange, 0x001D, 1, nullptr, 0},
		{&BattlePlugin::moveKnockOut, 0x0029, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0035, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0036, 1, nullptr, 0},
		{&BattlePlugin::moveJumpAttack, 0x0037, 1, _specialMoves.torBeastJumpAttackBackBoost, 0x96B7},
		{&BattlePlugin::moveJumpAttack, 0x0038, 1, _specialMoves.torBeastJumpAttackForwardBoost, 0x96B7},
		{&BattlePlugin::moveGuard, 0x004F, 1, nullptr, 0},
		{&BattlePlugin::moveAttackHoldButton2, 0x001E, 1, nullptr, 0x0006},
		{&BattlePlugin::moveAttack, 0x001F, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0020, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0021, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0022, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0023, 1, _specialMoves.torBeastSpinAttackChain, 0},
		{&BattlePlugin::moveAttack, 0x0024, 1, nullptr, 0},
		{&BattlePlugin::moveProjectile, 0x0048, 1, nullptr, 0x97A0},
		{&BattlePlugin::moveAttack, 0x004D, 1, _specialMoves.torBeastDiveAirAttack, 0},
		{&BattlePlugin::moveAttackBoost, 0x0000, 1, nullptr, 0},
		{&BattlePlugin::moveSelectWeapon, 0x0000, 1, nullptr, 0},
		{&BattlePlugin::moveAirAttack, 0x0050, 1, nullptr, 0}
	};
	const HitBox hits0[kHitBoxCount] = {
		{0x9625, 1, 0, 1, 2, 0x000A, 0x0032, 5, 0, 5, 43, 49},
		{0x9626, 1, 0, 1, 0, 0x000A, 0x0032, 5, 0, 0, 93, 76},
		{0x962B, 1, 0, 0, 2, 0x0014, 0x0032, 5, 6, 5, 40, 51},
		{0x962C, 1, 0, 0, 0, 0x0014, 0x0032, 5, 0, 7, 62, 61},
		{0x9631, 1, 1, 4, 0, 0x0014, 0x0032, 5, 0, 16, 133, 39},
		{0x9635, 1, 0, 2, 2, 0x0005, 0x0032, 5, 0, 15, 114, 41},
		{0x9638, 1, 0, 2, 0, 0x0005, 0x0032, 5, 1, 64, 107, 43},
		{0x969D, 1, -1, 2, 0, 0x0005, 0x0032, 5, 0, 24, 90, 57},
		{0xFFFF}
	};
	const HitBox hits1[kHitBoxCount] = {
		{0x96A5, 0, 1, 0, 0, 0x0014, 0x0064, 10, 104, 105, 60, 58},
		{0x96A6, 0, 0, 0, 0, 0x0014, 0x0064, 10, 154, 92, 114, 48},
		{0x96DD, 0, 0, 0, 0, 0x0014, 0x0064, 10, 114, 20, 100, 77},
		{0x96A7, 0, 0, 0, 0, 0x0014, 0x0064, 10, 168, 48, 160, 90},
		{0x96A8, 0, 1, 0, 0, 0x0014, 0x0064, 10, 0, 120, 179, 123},
		{0x96AD, 0, 0, 0, 0, 0x0014, 0x0096, 15, 152, 1, 115, 130},
		{0x96AF, 0, 0, 1, 0, 0x0014, 0x0064, 10, 126, 16, 95, 75},
		{0x96B0, 0, 0, 1, 0, 0x0014, 0x0064, 10, 89, 1, 80, 115},
		{0x96BE, 0, 0, 6, 0, 0x0028, 0x01F4, 20, 87, 0, 65, 119},
		{0x96BF, 0, 0, 6, 0, 0x0028, 0x01F4, 20, 148, 18, 138, 56},
		{0x96CE, 0, 2, 4, 0, 0x0014, 0x00C8, 20, 0, 21, 314, 83},
		{0x96CF, 0, 2, 4, 0, 0x0014, 0x00C8, 20, 1, 0, 129, 123},
		{0x96D0, 0, 2, 4, 0, 0x0014, 0x00C8, 20, 0, 0, 242, 97},
		{0x96D6, 0, 1, 3, 0, 0x0014, 0x00C8, 20, 41, 89, 64, 59},
		{0x96D7, 0, 1, 3, 0, 0x0014, 0x00C8, 20, 18, 22, 211, 32},
		{0x96D8, 0, -1, 3, 0, 0x0014, 0x00C8, 20, 8, 0, 66, 36},
		{0x96DB, 0, 0, 3, 0, 0x0014, 0x0064, 10, 0, 18, 108, 38},
		{0x96DC, 0, 1, 3, 0, 0x0014, 0x0064, 10, 16, 43, 72, 74},
		{0x9711, 0, 2, 2, 7, 0x0028, 0x00C8, 20, 58, 90, 87, 57},
		{0x9712, 0, 2, 2, 5, 0x0028, 0x00C8, 20, 45, 51, 151, 77},
		{0x9713, 0, 2, 2, 3, 0x0028, 0x00C8, 20, 45, 51, 151, 77},
		{0x96A4, 0, 0, 0, 0, 0x0014, 0x00C8, 20, 152, 1, 115, 130},
		{0x96A3, 0, 0, 0, 0, 0x0014, 0x0096, 15, 154, 92, 114, 48},
		{0x970A, 0, 2, 2, 9, 0x0028, 0x03E8, 30, 58, 90, 87, 57},
		{0x970B, 0, 2, 2, 7, 0x0028, 0x03E8, 30, 45, 51, 151, 77},
		{0x970C, 0, 2, 2, 5, 0x0028, 0x03E8, 30, 45, 51, 151, 77},
		{0x970D, 0, 2, 2, 3, 0x0028, 0x03E8, 30, 45, 51, 151, 77},
		{0xFFFF}
	};
	const AiRecord aiList[kAiListCount] = {
		{0x09, 0x00, 0x0000, 0x0000, 0x0000, 15, 221, 0x0080, 0x0000},
		{0x0A, 0x00, 0x0000, 0x0000, 0x0000, 10, 202, 0x0002, 0x0000},
		{0x13, 0x00, 0x0000, 0x0000, 0x0000, 30, 232, 0x0008, 0x0000},
		{0x09, 0x0A, 0x0000, 0x0000, 0x0000, 17, 200, 0x018A, 0x0000},
		{0x11, 0x00, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000},
		{0x14, 0x00, 0x0000, 0x0000, 0x0000, 10, 151, 0x0189, 0x0000},
		{0x15, 0x00, 0x0000, 0x0000, 0x0000, 20, 172, 0x0103, 0x0000},
		{0x16, 0x00, 0x0000, 0x0000, 0x0000, 10, 178, 0x000B, 0x0000},
		{0x18, 0x00, 0x0000, 0x0000, 0x0000, 30, 240, 0x0023, 0x0000},
		{0x09, 0x16, 0x1514, 0x0000, 0x0000, 13, 200, 0x010B, 0x0000},
		{0x13, 0x11, 0x0000, 0x0000, 0x0000, 30, 232, 0x010B, 0x0000},
		{0x12, 0x14, 0x0016, 0x0000, 0x0000, 12, 267, 0x0063, 0x0000},
		{0x16, 0x17, 0x0000, 0x0008, 0x0000, 14, 178, 0x000F, 0x0000},
		{0x12, 0x00, 0x0000, 0x0000, 0x0000, 10, 267, 0x0000, 0x0000},
		{0x04, 0x0E, 0x001A, 0x0707, 0x0000, 15, 250, 0x0464, 0x00C1},
		{0x04, 0x0F, 0x001A, 0x0704, 0x0000, 15, 250, 0x0260, 0x00C2},
		{0x04, 0x19, 0x0000, 0x000A, 0x0000, 20, 140, 0x0233, 0x0000},
		{0x04, 0x0E, 0x0018, 0x0004, 0x0000, 30, 120, 0x0463, 0x0000},
		{0x1B, 0x11, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000},
		{0x11, 0x0D, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000},
		{0x11, 0x0C, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000},
		{0xFF, 0xFF, 0xFFFF}
	};

	for (int i = 0; i < kVarListSize; i++)
		t->varList[i] = varList[i];
	for (int i = 0; i < kRectCount; i++) {
		t->rects[i] = rects[i];
		t->offsetsX[i] = offsetsX[i];
	}
	for (int i = 0; i < kWalkStepCount; i++)
		t->walkSteps[i] = walkSteps[i];
	for (int i = 0; i < kSetCount; i++) {
		t->knockDistances[i] = knockDistances[i];
		t->offsetsY[i] = offsetsY[i];
	}
	for (int i = 0; i < kSpriteTableSize; i++)
		t->sprites[i] = sprites[i];
	for (int i = 0; i < kAnimationTableSize; i++)
		t->animations[i] = animations[i];
	for (int i = 0; i < kRemapSize; i++)
		t->remap[i] = remap[i];
	for (int i = 0; i < kComboCount; i++)
		t->combos[i] = combos[i];
	for (int i = 0; i < kMoveCount; i++) {
		t->moves[0][i] = moves0[i];
		t->moves[1][i] = moves1[i];
	}
	for (int i = 0; i < kHitBoxCount; i++) {
		t->hits[0][i] = hits0[i];
		t->hits[1][i] = hits1[i];
	}
	for (int i = 0; i < kAiListCount; i++)
		t->aiList[i] = aiList[i];
}

} // End of namespace Topgun
