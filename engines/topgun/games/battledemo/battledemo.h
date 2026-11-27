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

#ifndef TOPGUN_GAMES_BATTLEDEMO_BATTLEDEMO_H
#define TOPGUN_GAMES_BATTLEDEMO_BATTLEDEMO_H

#include "topgun/games/battle/battle.h"

namespace Topgun {

#define TG_BATTLE_V1_2_3_1_LITERAL_BIAS 0x7D01
#define TG_BATTLE_V1_2_3_1_LAST_OPERAND_INDEX 0x5FE

#define TG_BATTLE_V1_2_3_1_FIGHTER_TYPE_COUNT 2
#define TG_BATTLE_V1_2_3_1_KEY_TABLE_LAST 0x80
#define TG_BATTLE_V1_2_3_1_INPUT_TIMEOUT 275
#define TG_BATTLE_V1_2_3_1_WIDE_VALUE 800
#define TG_BATTLE_V1_2_3_1_AI_WEIGHT_ROWS 27
#define TG_BATTLE_V1_2_3_1_AI_WEIGHT_ROW_SIZE 8
// The last row reads 4 bytes past the weight table
#define TG_BATTLE_V1_2_3_1_AI_WEIGHTS_SIZE (TG_BATTLE_V1_2_3_1_AI_WEIGHT_ROWS * TG_BATTLE_V1_2_3_1_AI_WEIGHT_ROW_SIZE + 4)
// On a tie the original reads lastUsedSeq from a null record and gets this value
#define TG_BATTLE_V1_2_3_1_AI_NULL_RECORD_LAST_USED 0x0000505A
#define TG_BATTLE_V1_2_3_1_HIT_POINT_FIRST_SPRITE 0x8830
#define TG_BATTLE_V1_2_3_1_HIT_POINT_LAST_SPRITE 0x8839

#define TG_BATTLE_V1_2_3_1_MESSAGE_CLOSE 0x10
#define TG_BATTLE_V1_2_3_1_MESSAGE_COMMAND 0x111
#define TG_BATTLE_V1_2_3_1_MESSAGE_GOTO_SCENE 0x4C9
#define TG_BATTLE_V1_2_3_1_MESSAGE_5F4 0x5F4

#define TG_BATTLE_V1_2_3_1_TIMER_FIGHTER0 3
#define TG_BATTLE_V1_2_3_1_TIMER_FIGHTER1 4
#define TG_BATTLE_V1_2_3_1_TIMER_KEY0 5
#define TG_BATTLE_V1_2_3_1_TIMER_KEY1 6
#define TG_BATTLE_V1_2_3_1_TIMER_INPUT0 7
#define TG_BATTLE_V1_2_3_1_TIMER_INPUT1 8

// Scripts handled by the plug-in
#define TG_BATTLE_V1_2_3_1_HOOK_MOVE_DONE 0x8333
#define TG_BATTLE_V1_2_3_1_HOOK_MOVES_REPOSITION 0x8334
#define TG_BATTLE_V1_2_3_1_HOOK_HIT_ATTACK 0x8335
#define TG_BATTLE_V1_2_3_1_HOOK_MOVES_DIE 0x8336
#define TG_BATTLE_V1_2_3_1_HOOK_HIT_BUTTON 0x8337
#define TG_BATTLE_V1_2_3_1_HOOK_STUB 0x8338
#define TG_BATTLE_V1_2_3_1_HOOK_CONTACT_FLIP 0x8339
#define TG_BATTLE_V1_2_3_1_HOOK_WEAPONS_FIRE 0x833A
#define TG_BATTLE_V1_2_3_1_HOOK_WEAPONS_HIT 0x833B
#define TG_BATTLE_V1_2_3_1_HOOK_WEAPONS_SPECIAL_HIT 0x833C
#define TG_BATTLE_V1_2_3_1_HOOK_REGISTER_BUTTON 0x833E
#define TG_BATTLE_V1_2_3_1_HOOK_HIT_FIGHTER 0x833F
#define TG_BATTLE_V1_2_3_1_HOOK_INIT 0x89BC
#define TG_BATTLE_V1_2_3_1_HOOK_KEY_TIMEOUT0 0x89BD
#define TG_BATTLE_V1_2_3_1_HOOK_KEY_TIMEOUT1 0x89BE
#define TG_BATTLE_V1_2_3_1_HOOK_INPUT_TIMEOUT0 0x89BF
#define TG_BATTLE_V1_2_3_1_HOOK_INPUT_TIMEOUT1 0x89C0
#define TG_BATTLE_V1_2_3_1_HOOK_DELAY_TIMEOUT0 0x89C1
#define TG_BATTLE_V1_2_3_1_HOOK_DELAY_TIMEOUT1 0x89C2
#define TG_BATTLE_V1_2_3_1_HOOK_SET_BOUNDS 0x89D0

// Scripts run by the plug-in
#define TG_BATTLE_V1_2_3_1_SCRIPT_HIT_POINT 0x881C
#define TG_BATTLE_V1_2_3_1_SCRIPT_FIGHT_START 0x8321
#define TG_BATTLE_V1_2_3_1_SCRIPT_FIGHT_OVER 0x8322
#define TG_BATTLE_V1_2_3_1_SCRIPT_FIGHT_BEGIN 0x832A
#define TG_BATTLE_V1_2_3_1_SCRIPT_FIGHT_OVER_SPRITE 0x8844
#define TG_BATTLE_V1_2_3_1_SCRIPT_BOTH_ENTERED 0x8847
#define TG_BATTLE_V1_2_3_1_SCRIPT_SET_CHANGED 0x88FF
#define TG_BATTLE_V1_2_3_1_SCRIPT_COMBO 0x8902
#define TG_BATTLE_V1_2_3_1_SCRIPT_CHANGE_ENERGY 0x8903
#define TG_BATTLE_V1_2_3_1_SCRIPT_WEAPON_SELECT 0x8958
#define TG_BATTLE_V1_2_3_1_SCRIPT_AMMO_DISPLAY 0x895B
#define TG_BATTLE_V1_2_3_1_SCRIPT_FIGHT_END 0x89B9
#define TG_BATTLE_V1_2_3_1_SCRIPT_SET_SPRITE_CEL 0x89BB

#define TG_BATTLE_V1_2_3_1_VAR_ARGUMENT_7 7
#define TG_BATTLE_V1_2_3_1_VAR_ARGUMENT_8 8
#define TG_BATTLE_V1_2_3_1_VAR_1003 1003
#define TG_BATTLE_V1_2_3_1_VAR_JOYSTICKS 1016
#define TG_BATTLE_V1_2_3_1_VAR_HUMAN_PLAYERS 1028
#define TG_BATTLE_V1_2_3_1_VAR_FIGHTER_TYPE 1029
#define TG_BATTLE_V1_2_3_1_VAR_WINNER 1031
#define TG_BATTLE_V1_2_3_1_VAR_PLAYER 1032
// The energy of player p is in variable 1033 + p
#define TG_BATTLE_V1_2_3_1_VAR_ENERGY 1033
#define TG_BATTLE_V1_2_3_1_VAR_WEAPON 1035
#define TG_BATTLE_V1_2_3_1_VAR_LOSER_ARGUMENT_7 1048
#define TG_BATTLE_V1_2_3_1_VAR_LOSER_ARGUMENT_8 1049
#define TG_BATTLE_V1_2_3_1_VAR_AMMO_WEAPON1 1050
#define TG_BATTLE_V1_2_3_1_VAR_AMMO_WEAPON2 1052
#define TG_BATTLE_V1_2_3_1_VAR_AMMO_WEAPON4 1054
#define TG_BATTLE_V1_2_3_1_VAR_AMMO_WEAPON8 1056
#define TG_BATTLE_V1_2_3_1_VAR_AMMO_WEAPON16 1058
#define TG_BATTLE_V1_2_3_1_VAR_AMMO_WEAPON32 1060
#define TG_BATTLE_V1_2_3_1_VAR_AMMO_WEAPON64 1062
#define TG_BATTLE_V1_2_3_1_VAR_AMMO_WEAPON128 1064
#define TG_BATTLE_V1_2_3_1_VAR_DIFFICULTY 1070
#define TG_BATTLE_V1_2_3_1_VAR_PLAYER_OPTIONS 1071
#define TG_BATTLE_V1_2_3_1_VAR_STAT_ATTACKS0 1085
#define TG_BATTLE_V1_2_3_1_VAR_STAT_HITS0 1086
#define TG_BATTLE_V1_2_3_1_VAR_STAT_RATING0 1087
#define TG_BATTLE_V1_2_3_1_VAR_STAT_ATTACKS1 1088
#define TG_BATTLE_V1_2_3_1_VAR_STAT_HITS1 1089
#define TG_BATTLE_V1_2_3_1_VAR_STAT_RATING1 1090
#define TG_BATTLE_V1_2_3_1_VAR_INPUT_TYPE 1105
#define TG_BATTLE_V1_2_3_1_VAR_AUTO_TURN 1107
// The maximum energy of player p is in variable 1109 + p
#define TG_BATTLE_V1_2_3_1_VAR_MAX_ENERGY 1109
#define TG_BATTLE_V1_2_3_1_VAR_EFFECTS_OFF 1114
#define TG_BATTLE_V1_2_3_1_VAR_BUSY_COUNTER 1115

class BattleDemoPlugin : public BattlePlugin {
public:
	BattleDemoPlugin(PluginHost *host);

	void free() override;
	int findFunction(const Common::String &name) const override;
	uint16 callFunction(int function, const uint16 *arguments, uint16 count) override;
	uint16 messageFilter(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) override;
	bool scriptHook(uint16 script, const uint16 *arguments) override;

private:
	InputRecord _verminBeastInputRecords[kInputRecordCount] = {};
	byte _aiWeights[TG_BATTLE_V1_2_3_1_AI_WEIGHTS_SIZE] = {};
	uint16 _weaponProjectile128[kFighterCount] = {};

	// Tables
	void initDemoTablesType0();
	void initDemoTablesType1();
	void initDemoTablesShared();

	// Lifecycle
	void beastInit() override;
	void emptyFunction();

	// Input
	void initInputTimers() override;
	uint16 identity(uint16 player, uint16 device) override;
	uint16 inputMatch(uint16 player, uint16 timeout) override;
	uint16 keyDown(uint16 virtualKey) override;
	uint16 keyUp(uint16 virtualKey) override;
	void keyboardMap(uint16 player, uint16 enable) override;

	// AI
	void aiSetupSkill(Fighter *f) override;
	void aiEvent(Fighter *f, uint16 event) override;
	void aiReact(Fighter *self, Fighter *opp) override;
	void aiStartTimer(Fighter *f) override;
	void aiTimerStep(Fighter *f) override;
	void aiExecuteMove(Fighter *f) override;
	AiRecord *aiPickMove(Fighter *f, AiRecord *list, byte mask, int32 *best) override;
	AiRecord *aiPickReaction(Fighter *f, AiRecord *list, int32 *best);
	AiRecord *aiPickMove2(Fighter *f, AiRecord *list, uint16 flag, int32 *best) override;
	void aiChooseNext(Fighter *f) override;

	// Hits
	uint16 hitAttack(uint16 id) override;
	uint16 hitFighter(uint16 a, uint16 b, uint16 id) override;
	uint16 hitStub(uint16 id);
	void startReaction(Fighter *f, uint16 anim) override;
	void pushBackAnim(Fighter *f, uint16 anim, uint16 amount, uint16 p, uint16 mode) override;
	void hitStunAnim(Fighter *f, uint16 anim) override;
	uint16 clashPush(Fighter *d, Fighter *s, uint16 v) override;
	uint16 hitPointTest(Fighter *a) override;

	// Moves
	void playAnim(Fighter *f, uint16 anim) override;
	void startKnockdown(Fighter *f) override;
	void resetFighterState(Fighter *f) override;
	void switchSprite(Fighter *f, uint16 k, uint16 anim, uint16 v) override;
	uint16 jumpMove(Fighter *f, uint16 anim, uint16 flip, int16 dx, int16 dy) override;
	uint16 moveWallBounce(Fighter *f, uint16 move, uint16 anim) override;
	uint16 moveSidestep(Fighter *f, uint16 move, uint16 anim) override;
	uint16 moveProjectile(Fighter *f, uint16 move, uint16 anim) override;
	uint16 moveKnockOut(Fighter *f, uint16 move, uint16 anim) override;
	void cbGetUp(Fighter *f) override;
	void cbFallAnim(Fighter *f) override;
	uint16 hookJump(Fighter *f, uint16 code) override;
	void initAiFlags(Fighter *f) override;
	void setStanceSet(Fighter *f, uint16 set, uint16 flag) override;

	// Geometry
	void movesReposition(uint16 id, uint16 mode) override;

	// Scene
	void storePlayer(Fighter *f);
	void cbEmpty(Fighter *f);
	void movesEnter(Fighter *f);
	void enterFinished(Fighter *f) override;
	void movesDie(uint16 flag, uint16 who) override;
	void fightOver(Fighter *f) override;
	void endFight(Fighter *f) override;
	void sceneRegisterButton(uint16 id, uint16 variable, uint16 remove) override;
	void initFightersFromHost() override;
	void initFighter(Fighter *f, uint16 player, uint16 type) override;

	// Weapons
	void weaponInit(Fighter *f) override;
	void weaponsFire(uint16 id, int16 dx, int16 dy) override;
	uint16 weaponsHit(uint16 shooter, uint16 sprite) override;
	uint16 weaponsSpecialHit(uint16 shooter, uint16 sprite) override;

	// Stats
	void loadStats(Fighter *f) override;
	void storeStats(Fighter *f) override;
	uint16 scoreAttack(Fighter *f, uint16 hit, uint16 move, uint16 value) override;
};

} // End of namespace Topgun

#endif // TOPGUN_GAMES_BATTLEDEMO_BATTLEDEMO_H
