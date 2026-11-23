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

void BattlePlugin::initTablesType4() {
	FighterTypeData *t = &_typeData[4];

	const uint16 varList[kVarListSize] = {
		0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
		0x0080, 0x0081, 0x99DC, 0x99DD, 0x99E1, 0x99E2, 0x0083
	};
	const Common::Rect rects[kRectCount] = {
		{-41, -27, 44, 15},
		{-44, -27, 41, 15},
		{-45, -60, 60, 43},
		{-60, -60, 45, 43},
		{-17, -7, 28, 70},
		{-28, -7, 17, 70}
	};
	const int16 offsetsX[kRectCount] = {0, 0, 0, 0, -17, 17};
	const WalkStep walkSteps[kWalkStepCount] = {{92, 30}, {70, 70}, {40, 40}};
	const uint16 knockDistances[kSetCount] = {62, 74};
	const int16 offsetsY[kSetCount] = {155, 119};
	const uint16 sprites[kSpriteTableSize] = {0x980A, 0x9A08, 0x97BE, 0x9803, 0x97DB};
	const uint16 animations[kAnimationTableSize] = {
		0x97B2, 0x9811, 0x9814, 0x0084, 0x9819, 0x0085, 0x981A, 0x980D, 0x0086, 0x981D, 0x981E, 0x981F,
		0x982E, 0x982F, 0x9835, 0x9834, 0x9831, 0x9830, 0x9833, 0x982B, 0x9865, 0x9837, 0x9836, 0x9838,
		0x0000, 0x0000, 0x983A, 0x982C, 0x983E, 0x9841, 0x9842, 0x983D, 0x9856, 0x9857, 0x9852, 0x9853,
		0x9847, 0x9843, 0x9858, 0x9845, 0x9849, 0x9848, 0x984B, 0x984C, 0x984D, 0x9844, 0x9847, 0x9846,
		0x984E, 0x9850, 0x984F, 0x9868, 0x9859, 0x985A, 0x985B, 0x985C, 0x985D, 0x9841, 0x9842, 0x9826,
		0x9827, 0x9826, 0x9827, 0x985F, 0x9860, 0x9861, 0x9862, 0x9839, 0x9839, 0x985E, 0x985E, 0x9839,
		0x985E, 0x982A, 0x9863, 0x983B, 0x9864, 0x980F, 0x980F, 0x983C, 0x9810, 0x97BF, 0x97C0, 0x99EE,
		0x9A17, 0x9A09, 0x980E, 0x9832, 0x984A, 0x0000, 0x9820, 0x9821, 0x9822, 0x9823, 0x9824, 0x9825,
		0x9854, 0x9855, 0x9828, 0x982D, 0x9851, 0x9804, 0x9829
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
		{0x0A091515, 0xFFFFFFFF},
		{0x00141214, 0x00FFFFFF},
		{0x00001911, 0x0000FFFF},
		{0x00001617, 0x0000FFFF},
		{0x0000090A, 0x0000FFFF},
		{0xFFFFFFFF, 0x00000000}
	};
	const MoveEntry moves0[kMoveCount] = {
		{&BattlePlugin::moveIdle, 0x0001, 1, nullptr, 0},
		{&BattlePlugin::moveWalk, 0x0002, 1, _specialMoves.mortiePetWalkTurn, 0},
		{&BattlePlugin::moveWalk, 0x0003, 1, nullptr, 0},
		{&BattlePlugin::moveTurn, 0x0004, 1, nullptr, 0},
		{&BattlePlugin::moveJumpUp, 0x0005, 1, _specialMoves.mortiePetJumpSteer, 0},
		{&BattlePlugin::moveWallBounce, 0x0006, 0, nullptr, 0},
		{&BattlePlugin::moveWallBounce, 0x0007, 0, nullptr, 0},
		{&BattlePlugin::moveCrouch, 0x000B, 0, nullptr, 0},
		{&BattlePlugin::moveGuard, 0x000C, 1, nullptr, 0},
		{&BattlePlugin::moveAttackHoldAnyButton, 0x0008, 1, nullptr, 0x0007},
		{&BattlePlugin::moveAttack, 0x0009, 1, nullptr, 0},
		{&BattlePlugin::moveKnockOut, 0x000D, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0031, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0032, 1, nullptr, 0},
		{&BattlePlugin::moveJumpAttack, 0x0033, 1, nullptr, 0x9880},
		{&BattlePlugin::moveJumpAttack, 0x0034, 1, nullptr, 0x9880},
		{&BattlePlugin::moveGuard, 0x004E, 1, nullptr, 0},
		{&BattlePlugin::moveSidestep, 0x0043, 1, nullptr, 0},
		{&BattlePlugin::moveSidestep, 0x0044, 1, nullptr, 0},
		{&BattlePlugin::moveTransform, 0x000A, 1, nullptr, 0},
		{&BattlePlugin::moveGrab, 0x0045, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0047, 0, nullptr, 0}
	};
	const MoveEntry moves1[kMoveCount] = {
		{&BattlePlugin::moveIdle, 0x0014, 1, nullptr, 0},
		{&BattlePlugin::moveWalk, 0x0015, 1, _specialMoves.mortieBeastWalkTurn, 0},
		{&BattlePlugin::moveWalk, 0x0016, 1, nullptr, 0},
		{&BattlePlugin::moveTurn, 0x0017, 1, nullptr, 0},
		{&BattlePlugin::moveJumpUp, 0x0019, 1, _specialMoves.mortieBeastJumpSteerAndDive, 0x993E},
		{&BattlePlugin::moveWallBounce, 0x001A, 0, nullptr, 0},
		{&BattlePlugin::moveWallBounce, 0x001B, 0, _specialMoves.mortieBeastDiveAirAttack, 0},
		{&BattlePlugin::moveCrouch, 0x0025, 1, nullptr, 0},
		{&BattlePlugin::moveGuard, 0x0026, 1, nullptr, 0},
		{&BattlePlugin::moveAttackCloseRange, 0x001C, 1, nullptr, 0},
		{&BattlePlugin::moveAttackCloseRange, 0x001D, 1, nullptr, 0},
		{&BattlePlugin::moveKnockOut, 0x0029, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0035, 1, nullptr, 0},
		{&BattlePlugin::moveJump, 0x0036, 1, nullptr, 0},
		{&BattlePlugin::moveJumpAttack, 0x0037, 1, _specialMoves.mortieBeastJumpAttackBackBoost, 0x993E},
		{&BattlePlugin::moveJumpAttack, 0x0038, 1, _specialMoves.mortieBeastJumpAttackForwardBoost, 0x993E},
		{&BattlePlugin::moveGuard, 0x004F, 1, nullptr, 0},
		{&BattlePlugin::moveAttackHoldButton2, 0x001E, 1, nullptr, 0x0006},
		{&BattlePlugin::moveAttack, 0x001F, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0020, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0021, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0022, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x0023, 1, _specialMoves.mortieBeastSpinAttackChain, 0},
		{&BattlePlugin::moveAttack, 0x0024, 1, nullptr, 0},
		{&BattlePlugin::moveProjectile, 0x0048, 1, nullptr, 0},
		{&BattlePlugin::moveAttack, 0x004D, 1, _specialMoves.mortieBeastDiveAirAttack, 0},
		{&BattlePlugin::moveAttackBoost, 0x0000, 1, nullptr, 0},
		{&BattlePlugin::moveSelectWeapon, 0x0000, 1, nullptr, 0},
		{&BattlePlugin::moveAirAttack, 0x0050, 1, nullptr, 0}
	};
	const HitBox hits0[kHitBoxCount] = {
		{0x98AD, 0, 0, 1, 2, 0x000A, 0x0032, 5, 94, 2, 69, 52},
		{0x98AE, 0, 0, 1, 0, 0x000A, 0x0032, 5, 89, 1, 129, 89},
		{0x98B3, 0, 0, 0, 2, 0x0014, 0x0032, 5, 125, 0, 63, 36},
		{0x98B4, 0, 0, 0, 0, 0x0014, 0x0032, 5, 25, 1, 41, 109},
		{0x98B8, 0, 1, 4, 0, 0x0014, 0x0032, 5, 0, 47, 109, 89},
		{0x98B9, 0, 1, 4, 0, 0x0005, 0x0032, 5, 144, 1, 98, 98},
		{0x98BD, 0, 0, 2, 2, 0x0005, 0x0032, 5, 126, 19, 103, 63},
		{0x98C1, 0, 0, 2, 0, 0x0005, 0x0032, 5, 109, 92, 119, 43},
		{0x9921, 0, -1, 2, 0, 0x0005, 0x0032, 5, 93, 14, 91, 51},
		{0xFFFF}
	};
	const HitBox hits1[kHitBoxCount] = {
		{0x9929, 0, 1, 0, 0, 0x0014, 0x0096, 15, 1, 103, 130, 99},
		{0x992A, 0, 1, 0, 0, 0x0014, 0x0096, 15, 193, 0, 131, 118},
		{0x9965, 0, 0, 0, 0, 0x0014, 0x0064, 10, 202, 22, 79, 103},
		{0x992D, 0, 0, 0, 0, 0x0014, 0x0096, 15, 179, 1, 125, 105},
		{0x9932, 0, 0, 0, 0, 0x0014, 0x0064, 10, 195, 1, 103, 78},
		{0x9935, 0, 1, 1, 0, 0x0014, 0x0064, 10, 180, 60, 92, 62},
		{0x9936, 0, 1, 1, 0, 0x0014, 0x0064, 10, 194, 0, 74, 111},
		{0x9944, 0, 0, 1, 0, 0x001E, 0x01F4, 20, 0, 0, 58, 64},
		{0x9945, 0, 0, 6, 0, 0x0028, 0x01F4, 20, 136, 0, 131, 91},
		{0x9955, 0, 1, 4, 0, 0x0014, 0x00C8, 20, 161, 0, 115, 79},
		{0x9956, 0, 1, 4, 0, 0x0014, 0x00C8, 20, 116, 28, 92, 117},
		{0x995E, 0, 1, 3, 0, 0x0014, 0x0064, 10, 137, 29, 171, 50},
		{0x995F, 0, 1, 3, 0, 0x0014, 0x0064, 10, 0, 76, 130, 59},
		{0x9962, 0, 2, 3, 0, 0x0014, 0x0096, 15, 1, 35, 129, 38},
		{0x994E, 0, 2, 6, 4, 0x0050, 0x03E8, 30, 176, 0, 163, 53},
		{0x9952, 0, 2, 6, 0, 0x0050, 0x03E8, 30, 331, 0, 103, 104},
		{0x99A2, 0, 2, 2, 7, 0x0028, 0x00C8, 20, 9, 160, 136, 64},
		{0x99A3, 0, 2, 2, 5, 0x0028, 0x00C8, 20, 0, 167, 145, 61},
		{0x99A4, 0, 2, 2, 3, 0x0028, 0x00C8, 20, 0, 167, 145, 61},
		{0x9928, 0, 0, 0, 0, 0x0014, 0x0096, 15, 195, 1, 103, 78},
		{0x9927, 0, 0, 0, 0, 0x0014, 0x00FA, 25, 193, 0, 131, 118},
		{0x999B, 0, 2, 2, 9, 0x0028, 0x03E8, 30, 9, 160, 136, 64},
		{0x999C, 0, 2, 2, 7, 0x0028, 0x03E8, 30, 0, 167, 145, 61},
		{0x999D, 0, 2, 2, 5, 0x0028, 0x03E8, 30, 0, 167, 145, 61},
		{0x999E, 0, 2, 2, 3, 0x0028, 0x03E8, 30, 0, 167, 145, 61},
		{0xFFFF}
	};
	const AiRecord aiList[kAiListCount] = {
		{0x09, 0x00, 0x0000, 0x0000, 0x0000, 10, 174, 0x0080, 0x0000},
		{0x0A, 0x00, 0x0000, 0x0000, 0x0000, 20, 174, 0x0002, 0x0000},
		{0x13, 0x00, 0x0000, 0x0000, 0x0000, 30, 175, 0x0008, 0x0000},
		{0x09, 0x0A, 0x0000, 0x0000, 0x0000, 17, 174, 0x018A, 0x0000},
		{0x11, 0x00, 0x0000, 0x0000, 0x0000, 30, 600, 0x0040, 0x0000},
		{0x14, 0x00, 0x0000, 0x0000, 0x0000, 10, 158, 0x0189, 0x0000},
		{0x15, 0x00, 0x0000, 0x0000, 0x0000, 20, 176, 0x0103, 0x0000},
		{0x16, 0x00, 0x0000, 0x0000, 0x0000, 10, 192, 0x000B, 0x0000},
		{0x18, 0x00, 0x0000, 0x0000, 0x0000, 30, 333, 0x0023, 0x0000},
		{0x12, 0x12, 0x1514, 0x0000, 0x0000, 13, 190, 0x010B, 0x0000},
		{0x11, 0x16, 0x0000, 0x0000, 0x0000, 20, 300, 0x0063, 0x0000},
		{0x16, 0x17, 0x0000, 0x0008, 0x0000, 15, 192, 0x000F, 0x0000},
		{0x12, 0x00, 0x0000, 0x0000, 0x0000, 5, 190, 0x0000, 0x0000},
		{0x04, 0x0E, 0x001A, 0x0707, 0x0000, 15, 250, 0x0464, 0x00BC},
		{0x04, 0x0F, 0x001A, 0x0704, 0x0000, 25, 250, 0x0260, 0x00BD},
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
