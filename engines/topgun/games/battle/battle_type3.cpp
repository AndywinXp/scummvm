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

void BattlePlugin::initTablesType3() {
	FighterTypeData *t = &_typeData[3];

	const uint16 varList[kVarListSize] = {
		0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
		0x0060, 0x0061, 0x94E2, 0x94E3, 0x94E7, 0x94E8, 0x0063
	};
	const Common::Rect rects[kRectCount] = {
		{-21, -32, 41, 25},
		{-41, -32, 21, 25},
		{-43, -50, 61, 44},
		{-61, -50, 43, 44},
		{-18, -23, 45, 49},
		{-45, -23, 18, 49}
	};
	const int16 offsetsX[kRectCount] = {0, 0, 0, 0, -57, 57};
	const WalkStep walkSteps[kWalkStepCount] = {{94, 32}, {39, 39}, {32, 32}};
	const uint16 knockDistances[kSetCount] = {128, 108};
	const int16 offsetsY[kSetCount] = {139, 115};
	const uint16 sprites[kSpriteTableSize] = {0x9311, 0x9504, 0x92C8, 0x930A, 0x92E5};
	const uint16 animations[kAnimationTableSize] = {
		0x92BC, 0x9318, 0x931B, 0x0064, 0x9320, 0x0065, 0x9321, 0x9314, 0x0066, 0x9324, 0x9325, 0x9326,
		0x9333, 0x9334, 0x933A, 0x9339, 0x9336, 0x9335, 0x9338, 0x9330, 0x936C, 0x933C, 0x933B, 0x933D,
		0x0000, 0x0000, 0x933F, 0x9331, 0x9343, 0x9346, 0x9347, 0x9342, 0x935B, 0x935C, 0x9357, 0x9358,
		0x934C, 0x9348, 0x935D, 0x934A, 0x934E, 0x934D, 0x9350, 0x9351, 0x9352, 0x9349, 0x934C, 0x934B,
		0x9353, 0x9355, 0x9354, 0x936F, 0x935E, 0x935F, 0x9360, 0x9361, 0x9362, 0x9346, 0x9347, 0x932D,
		0x932E, 0x932D, 0x932E, 0x9364, 0x9365, 0x9366, 0x9367, 0x933E, 0x933E, 0x9363, 0x9363, 0x933E,
		0x9363, 0x932F, 0x936A, 0x9340, 0x936B, 0x9316, 0x9316, 0x9341, 0x9317, 0x92C9, 0x92CA, 0x94F4,
		0x9512, 0x9505, 0x9315, 0x9337, 0x934F, 0x0000, 0x9327, 0x9328, 0x9329, 0x932A, 0x932B, 0x932C,
		0x9359, 0x935A, 0x9368, 0x9332, 0x9356, 0x930B, 0x9369
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
		{0x1914160A, 0xFFFFFFFF},
		{0x00143112, 0x00FFFFFF},
		{0x00003115, 0x0000FFFF},
		{0x0000090A, 0x0000FFFF},
		{0x00001614, 0x0000FFFF},
		{0xFFFFFFFF, 0x00000000}
	};
	const MoveEntry moves0[kMoveCount] = {
		{&BattlePlugin::moveIdle, 0x0001, 1, nullptr, 0},
		{&BattlePlugin::moveWalk, 0x0002, 1, _specialMoves.kuliPetWalkTurn, 0},
		{&BattlePlugin::moveWalk, 0x0003, 1, nullptr, 0},
		{&BattlePlugin::moveTurn, 0x0004, 1, nullptr, 0},
		{&BattlePlugin::moveJumpUp, 0x0005, 1, _specialMoves.kuliPetJumpSteer, 0},
		{&BattlePlugin::moveWallBounce, 0x0006, 0, nullptr, 0},
		{&BattlePlugin::moveWallBounce, 0x0007, 0, nullptr, 0},
		{&BattlePlugin::moveCrouch, 0x000B, 0, nullptr, 0},
		{&BattlePlugin::moveGuard, 0x000C, 1, nullptr, 0},
		{&BattlePlugin::moveAttackHoldAnyButton, 0x0008, 1, nullptr, 0x0007},
		{&BattlePlugin::moveAttack, 0x0009, 1, nullptr, 0},
		{&BattlePlugin::moveKnockOut, 0x000D, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0031, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0032, 1, nullptr, 0},
		{&BattlePlugin::moveJumpAttack, 0x0033, 1, nullptr, 0x9387},
		{&BattlePlugin::moveJumpAttack, 0x0034, 1, nullptr, 0x9387},
		{&BattlePlugin::moveGuard, 0x004E, 1, nullptr, 0},
		{&BattlePlugin::moveSidestep, 0x0043, 1, nullptr, 0},
		{&BattlePlugin::moveSidestep, 0x0044, 1, nullptr, 0},
		{&BattlePlugin::moveTransform, 0x000A, 1, nullptr, 0},
		{&BattlePlugin::moveGrab, 0x0045, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0047, 0, nullptr, 0},
		{nullptr, 0x0000, 0, nullptr, 0}
	};
	const MoveEntry moves1[kMoveCount] = {
		{&BattlePlugin::moveIdle, 0x0014, 1, nullptr, 0},
		{&BattlePlugin::moveWalk, 0x0015, 1, _specialMoves.kuliBeastWalkTurn, 0},
		{&BattlePlugin::moveWalk, 0x0016, 1, nullptr, 0},
		{&BattlePlugin::moveTurn, 0x0017, 1, nullptr, 0},
		{&BattlePlugin::moveJumpUp, 0x0019, 1, _specialMoves.kuliBeastJumpSteerAndDive, 0x944D},
		{&BattlePlugin::moveWallBounce, 0x001A, 0, nullptr, 0},
		{&BattlePlugin::moveWallBounce, 0x001B, 0, _specialMoves.kuliBeastDiveAirAttack, 0},
		{&BattlePlugin::moveCrouch, 0x0025, 1, nullptr, 0},
		{&BattlePlugin::moveGuard, 0x0026, 1, nullptr, 0},
		{&BattlePlugin::moveAttackCloseRange, 0x001C, 1, nullptr, 0},
		{&BattlePlugin::moveAttackCloseRange, 0x001D, 1, nullptr, 0},
		{&BattlePlugin::moveKnockOut, 0x0029, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0035, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0036, 1, nullptr, 0},
		{&BattlePlugin::moveJumpAttack, 0x0037, 1, _specialMoves.kuliBeastJumpAttackBackBoost, 0x944D},
		{&BattlePlugin::moveJumpAttack, 0x0038, 1, _specialMoves.kuliBeastJumpAttackForwardBoost, 0x944D},
		{&BattlePlugin::moveGuard, 0x004F, 1, nullptr, 0},
		{&BattlePlugin::moveAttackHoldButton2, 0x001E, 1, nullptr, 0x0006},
		{&BattlePlugin::moveAttack, 0x001F, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0020, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0021, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0022, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0023, 1, _specialMoves.kuliBeastSpinAttackChain, 0},
		{&BattlePlugin::moveAttack, 0x0024, 1, nullptr, 0},
		{&BattlePlugin::moveProjectile, 0x0048, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x004D, 1, _specialMoves.kuliBeastDiveAirAttack, 0},
		{&BattlePlugin::moveAttackBoost, 0x0000, 1, nullptr, 0},
		{&BattlePlugin::moveSelectWeapon, 0x0000, 1, nullptr, 0},
		{&BattlePlugin::moveAirAttack, 0x0050, 1, nullptr, 0},
		{nullptr, 0x0000, 0, nullptr, 0}
	};
	const HitBox hits0[kHitBoxCount] = {
		{0x93B4, 0, 0, 1, 2, 0x000A, 0x0032, 5, 102, 14, 90, 51},
		{0x93B5, 0, 0, 1, 0, 0x000A, 0x0032, 5, 97, 0, 201, 146},
		{0x93BA, 0, 0, 0, 2, 0x0014, 0x0032, 5, 120, 0, 65, 69},
		{0x93BB, 0, 0, 0, 0, 0x0014, 0x0032, 5, 47, 0, 40, 141},
		{0x93BF, 0, 1, 4, 0, 0x0014, 0x0032, 5, 1, 78, 140, 82},
		{0x93C0, 0, 1, 4, 0, 0x0014, 0x0032, 5, 117, 24, 168, 40},
		{0x93C4, 0, 0, 2, 3, 0x0005, 0x0032, 5, 122, 42, 99, 102},
		{0x93C8, 0, 0, 2, 0, 0x0005, 0x0032, 5, 126, 102, 97, 102},
		{0x942C, 0, -1, 2, 2, 0x0005, 0x0032, 5, 96, 1, 78, 84},
		{0xFFFF}
	};
	const HitBox hits1[kHitBoxCount] = {
		{0x9434, 0, 1, 0, 0, 0x0014, 0x0064, 10, 49, 94, 50, 60},
		{0x9435, 0, 0, 0, 0, 0x0014, 0x0064, 10, 145, 56, 62, 76},
		{0x9470, 0, 0, 0, 0, 0x0014, 0x0096, 15, 135, 81, 61, 87},
		{0x9439, 0, 0, 0, 0, 0x0014, 0x0064, 10, 138, 59, 97, 39},
		{0x943C, 0, 0, 0, 0, 0x0014, 0x0064, 10, 138, 39, 76, 41},
		{0x9443, 0, 0, 0, 0, 0x0014, 0x0064, 10, 157, 37, 81, 56},
		{0x9445, 0, 1, 1, 0, 0x0014, 0x0096, 15, 101, 51, 110, 68},
		{0x9446, 0, 0, 1, 0, 0x0014, 0x0096, 15, 145, 0, 83, 94},
		{0x9454, 0, 0, 0, 4, 0x0014, 0x01F4, 20, 149, 40, 56, 50},
		{0x9455, 0, 0, 6, 0, 0x0028, 0x01F4, 20, 167, 10, 140, 71},
		{0x9461, 0, -1, 4, 0, 0x0014, 0x00C8, 20, 37, 112, 81, 42},
		{0x9462, 0, 0, 4, 0, 0x0014, 0x00C8, 20, 122, 68, 84, 64},
		{0x9463, 0, 1, 4, 0, 0x0014, 0x00C8, 20, 102, 123, 69, 72},
		{0x946A, 0, 1, 3, 0, 0x0014, 0x0064, 10, 72, 49, 215, 46},
		{0x946B, 0, -1, 3, 0, 0x0014, 0x0064, 10, 99, 31, 111, 51},
		{0x946E, 0, 0, 3, 0, 0x0014, 0x0096, 15, 33, 1, 82, 93},
		{0x946F, 0, 1, 3, 0, 0x0014, 0x0096, 15, 0, 17, 86, 80},
		{0x945A, 0, 2, 6, 6, 0x0050, 0x03E8, 30, 85, 2, 89, 164},
		{0x945B, 0, 2, 6, 4, 0x0050, 0x03E8, 30, 93, 0, 93, 150},
		{0x945C, 0, 2, 6, 2, 0x0050, 0x03E8, 30, 91, 15, 68, 134},
		{0x945D, 0, 2, 6, 0, 0x0050, 0x03E8, 30, 67, 14, 131, 119},
		{0x945E, 0, 2, 6, 0, 0x0050, 0x03E8, 30, 51, 0, 64, 206},
		{0x94A2, 0, 2, 2, 5, 0x0028, 0x00C8, 20, 25, 78, 140, 49},
		{0x94A3, 0, 2, 2, 3, 0x0028, 0x00C8, 20, 25, 78, 140, 49},
		{0x9432, 0, 0, 0, 0, 0x001E, 0x0096, 15, 145, 56, 62, 76},
		{0x9433, 0, 0, 0, 0, 0x001E, 0x0096, 15, 157, 37, 81, 56},
		{0x949B, 0, 2, 2, 7, 0x0028, 0x03E8, 30, 25, 78, 140, 49},
		{0x949C, 0, 2, 2, 5, 0x0028, 0x03E8, 30, 25, 78, 140, 49},
		{0x949D, 0, 2, 2, 3, 0x0028, 0x03E8, 30, 25, 78, 140, 49},
		{0xFFFF}
	};
	const AiRecord aiList[kAiListCount] = {
		{0x09, 0x00, 0x0000, 0x0000, 0x0000, 10, 172, 0x0082, 0x0000},
		{0x0A, 0x00, 0x0000, 0x0000, 0x0000, 10, 168, 0x0002, 0x0000},
		{0x13, 0x00, 0x0000, 0x0000, 0x0000, 30, 240, 0x0008, 0x0000},
		{0x09, 0x0A, 0x0000, 0x0000, 0x0000, 15, 170, 0x0082, 0x0000},
		{0x11, 0x00, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000},
		{0x14, 0x00, 0x0000, 0x0000, 0x0000, 15, 150, 0x0189, 0x0000},
		{0x15, 0x00, 0x0000, 0x0000, 0x0000, 20, 166, 0x0103, 0x0000},
		{0x16, 0x00, 0x0000, 0x0000, 0x0000, 10, 228, 0x000B, 0x0000},
		{0x18, 0x00, 0x0000, 0x0000, 0x0000, 30, 400, 0x0220, 0x0000},
		{0x14, 0x16, 0x000A, 0x0000, 0x0000, 12, 150, 0x0108, 0x0000},
		{0x14, 0x04, 0x120F, 0x0700, 0x0000, 10, 240, 0x014B, 0x0000},
		{0x04, 0x0F, 0x0015, 0x0007, 0x0000, 20, 300, 0x0063, 0x0000},
		{0x09, 0x0A, 0x0000, 0x0000, 0x0000, 15, 170, 0x0188, 0x0000},
		{0x16, 0x17, 0x0000, 0x0008, 0x0000, 12, 228, 0x000F, 0x0000},
		{0x16, 0x14, 0x0000, 0x0000, 0x0000, 12, 150, 0x0000, 0x0000},
		{0x04, 0x0E, 0x001A, 0x0707, 0x0000, 15, 250, 0x0464, 0x00C0},
		{0x04, 0x0F, 0x001A, 0x0704, 0x0000, 15, 250, 0x0260, 0x00C1},
		{0x04, 0x19, 0x0000, 0x000A, 0x0000, 20, 140, 0x0233, 0x0000},
		{0x04, 0x0E, 0x0018, 0x0004, 0x0000, 30, 120, 0x0460, 0x0000},
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
