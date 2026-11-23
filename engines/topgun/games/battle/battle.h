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

#ifndef TOPGUN_GAMES_BATTLE_BATTLE_H
#define TOPGUN_GAMES_BATTLE_BATTLE_H

#include "common/rect.h"
#include "common/scummsys.h"
#include "common/str.h"

#include "topgun/plugin.h"

namespace Topgun {

enum {
	kFighterCount = 2,
	kFighterTypeCount = 7,
	kSetCount = 2,
	kMoveCount = 50,
	kRemapSize = 0x51,
	kAnimationTableSize = 103,
	kVarListSize = 15,
	kRectCount = 6,
	kWalkStepCount = 3,
	kSpriteTableSize = 5,
	kComboCount = 6,
	kHitBoxCount = 31,
	kAiListCount = 23,
	kInputRecordCount = 27,
	kReactionTableCount = 3,
	kReactionTableSize = 28,
	kBoundsCount = 2,
	kButtonCount = 10,
	kKeyTableSize = 257,
	kKeyBindingSize = 24,
	kJoystickFlagCount = 0x22,
	kEventSlotCount = 0x25,
	kAiStrategyCount = 7,
	kDamageClassCount = 7,
	kWeaponTypeCount = 7,
	kWeaponPoolSize = 6
};

enum FighterType {
	kFighterSparky = 0,
	kFighterVermin = 1,
	kFighterNicky = 2,
	kFighterKuli = 3,
	kFighterMortie = 4,
	kFighterTor = 5,
	kFighterEvilToadMan = 6
};

enum {
	kMessageKeyDown = 0x100,
	kMessageKeyUp = 0x101,
	kMessageJoystick1Move = 0x3A0,
	kMessageJoystick2Move = 0x3A1,
	kMessageJoystick1ButtonDown = 0x3B5,
	kMessageJoystick2ButtonDown = 0x3B6,
	kMessageJoystick1ButtonUp = 0x3B7,
	kMessageJoystick2ButtonUp = 0x3B8
};

// Windows virtual-key numbers: scripts and the profile store and use them
enum {
	kKey0 = 0x30,
	kKey1 = 0x31,
	kKey2 = 0x32,
	kKey9 = 0x39,
	kKeyB = 0x42,
	kKeyC = 0x43,
	kKeyF = 0x46,
	kKeyV = 0x56,
	kKeyLeft = 0x25,
	kKeyUp = 0x26,
	kKeyRight = 0x27,
	kKeyDown = 0x28
};

enum {
	kLiteralBias = 0x7531,
	kLastOperandIndex = 0x159E
};

enum {
	kTimerFighter0 = 0x0E,
	kTimerFighter1 = 0x0F,
	kTimerKey0 = 0x10,
	kTimerKey1 = 0x11,
	kTimerInput0 = 0x12,
	kTimerInput1 = 0x13
};

enum {
	kScriptHitAttack = 0x9DE1,
	kScriptWeaponsHit = 0x9DE2,
	kScriptWeaponsSpecialHit = 0x9DE3,
	kScriptKeyTimer0 = 0x9DE5,
	kScriptKeyTimer1 = 0x9DE6,
	kScriptInputTimer0 = 0x9DE7,
	kScriptInputTimer1 = 0x9DE8,
	kScriptFighterTimer0 = 0x9DE9,
	kScriptFighterTimer1 = 0x9DEA,
	kScriptChangeEnergy = 0x9D33,
	kScriptAddScore = 0x9B0F,
	kScriptCombo = 0x9D32,
	kScriptAmmoDisplay = 0x9D83,
	kScriptWeaponSelect = 0x9D80,
	kScriptTableSetChanged = 0x9D2D,
	kScriptDoorEntered = 0x9DD6,
	kScriptSetSpriteCel = 0x9DDA,
	kScriptEndRoundBonus = 0x9D36,
	kScriptHitPointHit = 0xB035
};

// Encoded handle of the first hit point sprite
enum {
	kHitPointSpriteBase = 0xB048
};

enum {
	kSpeedNone = 0,
	kSpeedWalk = 1,
	kSpeedSlow = 2,
	kSpeedPushing = 4,
	kMotionSpeedNormal = 0x7D,
	kMotionSpeedSlow = 0x3E,
	kProjectileDistance = 0x320,
	kPushFromPositions = 0x3E7,
	kNoDoor = 0x270F,
	kLevelNone = 0x63
};

// Index into Fighter::moveTab
enum MoveSlot {
	kMoveSlotIdle = 0,
	kMoveSlotWalkForward = 1,
	kMoveSlotWalkBack = 2,
	kMoveSlotTurn = 3,
	kMoveSlotJumpUp = 4,
	kMoveSlotLevelUp = 5,
	kMoveSlotLevelDown = 6,
	kMoveSlotCrouch = 7,
	kMoveSlotGuardDown = 8,
	kMoveSlotKnockOut = 0x0B,
	kMoveSlotJumpBack = 0x0C,
	kMoveSlotJumpForward = 0x0D,
	kMoveSlotJumpAttackBack = 0x0E,
	kMoveSlotJumpAttackForward = 0x0F,
	kMoveSlotGuardDownForward = 0x10,
	kMoveSlotSidestepMinus = 0x11,
	kMoveSlotSidestepPlus = 0x12,
	kMoveSlotWeaponHold = 0x11,
	kMoveSlotTransform = 0x13,
	kMoveSlotProjectile = 0x18,
	kMoveSlotAttackBoost = 0x1A,
	kMoveSlotSelectWeapon = 0x1B,
	kMoveSlotAirAttack = 0x1C
};

// Animation roles used by the reaction code
enum ReactionAnim {
	kReactKnockBackSet0 = 0x0E,
	kReactKnockBackSet1 = 0x2D,
	kReactPushBackSet0 = 0x41,
	kReactPushBackSet1 = 0x42
};

enum AiEvent {
	kAiEventMoveEnded = 0,
	kAiEventOpponentAttacking = 1,
	kAiEventOpponentVulnerable = 2,
	kAiEventOpponentRepositioned = 3,
	kAiEventHitBlocked = 4,
	kAiEventHurt = 5,
	kAiEventHitLanded = 6,
	kAiEventAttackMissed = 7,
	kAiEventFightWon = 8,
	kAiEventFightLost = 9,
	kAiEventPushed = 10,
	kAiEventOpponentStunned = 11
};

enum AiFlag {
	kAiFlagTurn = 0x0004,
	kAiFlagWalkForward = 0x0200,
	kAiFlagWalkBack = 0x0400,
	kAiFlagWalkMask = 0x0600,
	kAiFlagLevelUp = 0x0800,
	kAiFlagLevelDown = 0x1000,
	kAiFlagDirectionMask = 0x1E00,
	kAiFlagImmediateMask = 0x6000
};

enum InputCode {
	kDirNone = 0,
	kDirUp = 3,
	kDirDown = 5,
	kDirBack = 6,
	kDirForward = 7,
	kDirUpBack = 9,
	kDirUpForward = 0x0A,
	kDirDownBack = 0x0B,
	kDirDownForward = 0x0C,
	kDirNoCode = 0x88,
	kInputButton1 = 1,
	kInputButton2 = 2,
	kInputButton3 = 4,
	kInputButton4 = 8,
	kInputRelease1 = 0xFE,
	kInputRelease2 = 0xFD,
	kInputRelease3 = 0xFB,
	kInputRelease4 = 0xF7,
	kInputUp = 0x10,
	kInputDown = 0x20,
	kInputRight = 0x40,
	kInputLeft = 0x80
};

enum InputDevice {
	kInputDeviceNone = 0,
	kInputDeviceJoystick = 1,
	kInputDeviceKeyboard = 2,
	kInputDeviceDisable = 3,
	kInputDeviceEnable = 4
};

enum {
	kVarStanceFlags = 2519,
	kVarHitPointCount = 2563,
	kVarHumanPlayers = 5028,
	kVarFighterType = 5029,
	kVarWinner = 5031,
	kVarEnergy = 5033,
	kVarWeapon = 5035,
	kVarMode = 5043,
	kVarAmmoWeapon1 = 5050,
	kVarAmmoWeapon2 = 5052,
	kVarAmmoWeapon4 = 5054,
	kVarAmmoWeapon8 = 5056,
	kVarAmmoWeapon16 = 5058,
	kVarAmmoWeapon32 = 5062,
	kVarAmmoWeapon64 = 5064,
	kVarDifficulty = 5070,
	kVarPlayerOptions = 5071,
	kVarOptions1 = 5081,
	kVarOptions2 = 5082,
	kVarWins = 5083,
	kVarStatAttacks0 = 5085,
	kVarStatHits0 = 5086,
	kVarStatAttacks1 = 5088,
	kVarStatHits1 = 5089,
	kVarStatPercentage1 = 5090,
	kVarRemotePlayer = 5091,
	kVarNetworkGame = 5092,
	kVarAmmoCounter = 5094,
	kVarStatPercentage0 = 5097,
	kVarLocalPlayer = 5098,
	kVarInputType = 5105,
	kVarAutoTurn = 5107,
	kVarMaxEnergy = 5109,
	kVarEffectsOff = 5114,
	kVarBusyCounter = 5115,
	kVarWeaponPower = 872
};

enum FighterState {
	kStateNormal = 0,
	kStateGuard = 1,
	kStateAttack = 2,
	kStateFinal = 3,
	kStateDying = 4,
	kStateWinner = 5,
	kStateKnockedDown = 6
};

enum FighterFlag {
	kFlagKnockOutSeen = 1,
	kFlagInfiniteAmmo = 2,
	kFlagNoKnockOut = 4,
	kFlagSidestep = 8,
	kFlagTypeInit = 0x10,
	kFlagProjectile = 0x20,
	kFlagLevelChange = 0x40,
	kFlagResetOnEnd = 0x80
};

enum FightPhase {
	kPhaseNone = 0,
	kPhaseWalkIn = 1,
	kPhaseFight = 2
};

enum {
	kAiNullRecordLastUsed = 0x1A348AE2
};

struct WalkStep {
	uint16 fullStep;
	uint16 shortStep;
};

struct ComboRecord {
	uint32 pattern;
	uint32 mask;
};

struct AiStrategyRecord {
	int16 unk_00;
	int16 successDelta;
	int16 failureDelta;
	int16 opponentStunnedDelta;
};

struct SpecialMove {
	uint16 value;
	uint16 mask;
	uint16 frameLo;
	uint16 frameHi;
	byte move;
	uint16 cel;
};

struct InputRecord {
	uint16 value;
	uint16 mask;
	byte move;
};

struct HitBox {
	uint16 frameId;
	byte baseFacing;
	int16 laneRel;
	byte damageClass;
	byte skipCount;
	uint16 force;
	uint16 scorePoints;
	int16 energyLoss;
	int16 x;
	int16 y;
	int16 width;
	int16 height;
};

struct ReactionRecord {
	uint16 limit;
	byte reactionMove;
};

struct AiRecord {
	byte move;
	byte move2;
	uint16 moves34;
	uint16 delays12;
	uint16 delays34;
	uint32 weight;
	int16 maxDistance;
	uint16 flags;
	uint16 cel;
	uint32 lastUsedSeq;
	uint32 missCount;
	uint32 hitCount;
};

struct PendingMove {
	byte active;
	byte type;
	byte player;
	byte move;
	uint16 lastFrame;
	uint16 animIndex;
	uint16 handlerParam;
	uint16 cel;
	uint16 buttons;
	uint16 dirCode;
	uint32 dueTime;
};

class BattlePlugin;
struct Fighter;

typedef uint16 (BattlePlugin::*MoveHandler)(Fighter *f, uint16 move, uint16 anim);
typedef void (BattlePlugin::*Callback)(Fighter *f);
typedef uint16 (BattlePlugin::*Hook)(Fighter *f, uint16 code);

struct MoveEntry {
	MoveHandler handler;
	uint16 animIndex;
	uint16 available;
	const SpecialMove *specialList;
	uint16 handlerParam;
};

struct ArenaBounds {
	int16 left;
	int16 right;
	int16 y;
	int16 sidestepMinusLimit;
	int16 sidestepPlusLimit;
};

struct ButtonRecord {
	uint16 spriteId;
	uint16 script;
	Common::Rect rect;
};

struct KeyEntry {
	byte player;
	byte armed;
	byte code;
};

struct EventSlot {
	uint16 callback;
	uint16 script;
	uint16 scriptArg0;
	uint16 scriptArg1;
	byte armed;
	byte fired;
};

struct AiBlock {
	uint16 maxReactionDelay = 0;
	uint16 reactionDelay = 0;
	uint16 reactionDelayStep = 0;
	uint16 reuseCooldown = 0;
	int16 confidence = 0;
	int16 highConfidenceLimit = 0;
	int16 lowConfidenceLimit = 0;
	byte strategy = 0;
	uint16 repeatCount = 0;
	AiRecord *list = nullptr;
	uint32 selectionCount = 0;
	uint16 outcomeHistory = 0;
	uint16 windowBit = 0;
	uint16 windowHits = 0;
	byte timerArmed = 0;
	byte waiting = 0;
	byte unk_1f = 0;
	AiRecord *currentRecord = nullptr;
	AiRecord *nextRecord = nullptr;
	uint32 moveQueue = 0;
	uint32 delayQueue = 0;
	uint16 currentFlags = 0;
	int32 switchScore = 0;
};

struct Fighter {
	uint16 type = 0;
	uint16 sprite = 0;
	uint16 spriteId = 0;
	byte player = 0;
	byte ai = 0;
	Fighter *opponent = nullptr;
	byte set = 0;
	byte posture = 0;
	byte state = 0;
	byte speed = 0;
	const uint16 *spriteTab = nullptr;
	int16 x = 0;
	int16 y = 0;
	int16 lane = 0;
	const int16 *yTab = nullptr;
	const Common::Rect *rectTab = nullptr;
	const int16 *xTab = nullptr;
	const uint16 *knockTab = nullptr;
	byte level = 0;
	uint16 z = 0;
	uint16 anim = 0;
	byte move = 0;
	uint16 handlerParam = 0;
	uint16 moveCel = 0;
	const WalkStep *walkTab = nullptr;
	int16 walkStep = 0;
	const uint16 *varList = nullptr;
	uint32 combo = 0;
	const ComboRecord *comboTab = nullptr;
	uint32 comboEnd = 0;
	uint16 weapon = 0;
	uint16 *ammo = nullptr;
	uint16 holdCount = 0;
	byte holdFlag = 0;
	byte inFight = 0;
	byte attackActive = 0;
	uint16 lastAnim = 0;
	byte side = 0;
	byte autoTurn = 0;
	byte contact = 0;
	byte crouching = 0;
	byte flags = 0;
	byte joyLast = 0;
	byte keyDirs = 0;
	uint16 keyTimerId = 0;
	uint16 keyTimerScript = 0;
	byte keyTimerArmed = 0;
	byte inputDevice = 0;
	byte inputEnabled = 0;
	byte facing = 0;
	uint16 history = 0;
	uint16 dirCode = 0;
	uint16 buttons = 0;
	uint16 inputTimerId = 0;
	uint16 inputTimerScript = 0;
	uint32 inputTimeout = 0;
	Hook hook = nullptr;
	const SpecialMove *specialTab = nullptr;
	uint16 lastFrame = 0;
	uint16 queueGate = 0;
	byte queued = 0;
	const InputRecord *inputTab[kSetCount] = {};
	MoveEntry moveTab[kSetCount][kMoveCount] = {};
	const HitBox *hitTab[kSetCount] = {};
	const ReactionRecord *reactTab[kReactionTableCount] = {};
	const uint16 *animTab = nullptr;
	uint16 remap[kRemapSize] = {};
	Callback doneCallback = nullptr;
	Callback timerCallback = nullptr;
	uint16 timerId = 0;
	uint16 timerScript = 0;
	uint16 damageMultiplier = 0;
	byte beyondOuterWall = 0;
	byte atWall = 0;
	AiBlock aiBlock;
	uint32 fightAttacks = 0;
	uint32 fightHits = 0;
	uint32 totalAttacks = 0;
	uint32 totalHits = 0;
};

struct BattleContext {
	uint16 enabled = 0;
	Fighter fighters[kFighterCount];
	ArenaBounds bounds[kBoundsCount] = {};
	int16 outerLeft = 0;
	int16 outerRight = 0;
	uint16 buttonCount = 0;
	ButtonRecord buttons[kButtonCount] = {};
	KeyEntry keyTable[kKeyTableSize] = {};
	int16 doorX = 0;
	int16 doorY = 0;
	Common::Rect doorRect;
	byte joystickFlags[kJoystickFlagCount] = {};
	int16 edgeGap = 0;
	int16 gapMinusMargins = 0;
	int16 centerDistance = 0;
	byte phase = 0;
	byte enteredCount = 0;
	byte contact = 0;
	byte effectsOff = 0;
	byte aiEngaged = 0;
	byte lastScorer = 0;
	byte fightOver = 0;
	byte networkGame = 0;
	uint16 networkActive = 0;
	EventSlot eventSlots[kEventSlotCount] = {};
	Fighter *networkFighter = nullptr;
	PendingMove pendingMoves[kFighterCount] = {};
	Hook moveHook = nullptr;
	byte inputQueueLength = 0;
};

struct FighterTypeData {
	uint16 varList[kVarListSize] = {};
	Common::Rect rects[kRectCount];
	int16 offsetsX[kRectCount] = {};
	WalkStep walkSteps[kWalkStepCount] = {};
	uint16 knockDistances[kSetCount] = {};
	int16 offsetsY[kSetCount] = {};
	uint16 sprites[kSpriteTableSize] = {};
	uint16 animations[kAnimationTableSize] = {};
	uint16 remap[kRemapSize] = {};
	ComboRecord combos[kComboCount] = {};
	MoveEntry moves[kSetCount][kMoveCount] = {};
	HitBox hits[kSetCount][kHitBoxCount] = {};
	AiRecord aiList[kAiListCount] = {};
};

struct FighterInitParams {
	uint16 sprite;
	uint16 spriteId;
	byte facing;
	byte flags;
	uint16 script;
};

struct SpecialMoveLists {
	SpecialMove sparkyPetWalkTurn[4] = {};
	SpecialMove sparkyBeastWalkTurn[2] = {};
	SpecialMove sparkyBeastSpinAttackChain[3] = {};
	SpecialMove sparkyPetJumpSteer[5] = {};
	SpecialMove sparkyBeastJumpSteerAndDive[10] = {};
	SpecialMove sparkyBeastDiveAirAttack[2] = {};
	SpecialMove sparkyBeastJumpAttackBackBoost[3] = {};
	SpecialMove sparkyBeastJumpAttackForwardBoost[3] = {};
	SpecialMove nickyPetWalkTurn[4] = {};
	SpecialMove nickyBeastWalkTurn[2] = {};
	SpecialMove nickyBeastSpinAttackChain[3] = {};
	SpecialMove nickyPetJumpSteer[5] = {};
	SpecialMove nickyBeastJumpSteerAndDive[10] = {};
	SpecialMove nickyBeastDiveAirAttack[2] = {};
	SpecialMove nickyBeastJumpAttackBackBoost[3] = {};
	SpecialMove nickyBeastJumpAttackForwardBoost[3] = {};
	SpecialMove kuliPetWalkTurn[4] = {};
	SpecialMove kuliBeastWalkTurn[2] = {};
	SpecialMove kuliBeastSpinAttackChain[3] = {};
	SpecialMove kuliPetJumpSteer[5] = {};
	SpecialMove kuliBeastJumpSteerAndDive[10] = {};
	SpecialMove kuliBeastDiveAirAttack[2] = {};
	SpecialMove kuliBeastJumpAttackBackBoost[3] = {};
	SpecialMove kuliBeastJumpAttackForwardBoost[3] = {};
	SpecialMove verminPetWalkTurn[4] = {};
	SpecialMove verminBeastWalkTurn[2] = {};
	SpecialMove verminBeastSpinAttackChain[3] = {};
	SpecialMove verminPetJumpSteer[5] = {};
	SpecialMove verminBeastJumpSteerAndDive[10] = {};
	SpecialMove verminBeastDiveAirAttack[2] = {};
	SpecialMove verminBeastJumpAttackBackBoost[3] = {};
	SpecialMove verminBeastJumpAttackForwardBoost[3] = {};
	SpecialMove torPetWalkTurn[4] = {};
	SpecialMove torBeastWalkTurn[2] = {};
	SpecialMove torBeastSpinAttackChain[3] = {};
	SpecialMove torPetJumpSteer[5] = {};
	SpecialMove torBeastJumpSteerAndDive[10] = {};
	SpecialMove torBeastDiveAirAttack[2] = {};
	SpecialMove torBeastJumpAttackBackBoost[3] = {};
	SpecialMove torBeastJumpAttackForwardBoost[3] = {};
	SpecialMove evilToadManBeastSpinAttackChain[3] = {};
	SpecialMove evilToadManBeastJumpSteerAndDive[9] = {};
	SpecialMove evilToadManBeastJumpAttackBackBoost[3] = {};
	SpecialMove evilToadManBeastJumpAttackForwardBoost[3] = {};
	SpecialMove mortiePetWalkTurn[4] = {};
	SpecialMove mortieBeastWalkTurn[2] = {};
	SpecialMove mortieBeastSpinAttackChain[3] = {};
	SpecialMove mortiePetJumpSteer[5] = {};
	SpecialMove mortieBeastJumpSteerAndDive[10] = {};
	SpecialMove mortieBeastDiveAirAttack[2] = {};
	SpecialMove mortieBeastJumpAttackBackBoost[3] = {};
	SpecialMove mortieBeastJumpAttackForwardBoost[3] = {};
};

class BattlePlugin : public Plugin {
public:
	BattlePlugin(PluginHost *host);
	~BattlePlugin();

	void free() override;
	int findFunction(const Common::String &name) const override;
	uint16 callFunction(int function, const uint16 *arguments, uint16 count) override;
	uint16 messageFilter(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) override;
	uint32 windowMessage(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) override;

private:
	PluginHost *_host;
	BattleContext *_ctx;
	byte _initFlags;
	bool _networkMessageShown = false;

	FighterTypeData _typeData[kFighterTypeCount];
	FighterInitParams _fighterInitParams[kFighterTypeCount] = {};
	SpecialMoveLists _specialMoves;

	InputRecord _inputRecords[kSetCount][kInputRecordCount] = {};
	ReactionRecord _reactionTables[kReactionTableCount][kReactionTableSize] = {};
	AiRecord _aiList0166[kFighterCount] = {};
	AiRecord _aiList01A2[kFighterCount] = {};
	AiRecord _aiLists01DE[kFighterCount][10] = {};
	AiRecord _aiList0436[2] = {};
	AiRecord _aiLists0472[kFighterCount][5] = {};
	AiStrategyRecord _aiStrategyTable[kAiStrategyCount] = {};
	byte _clashDefenderAnimations[6] = {};
	byte _clashSourceAnimations[3] = {};
	byte _knockAnimations[2] = {};
	byte _directionMirror[13] = {};
	byte _joystickCodes[17] = {};
	byte _keyBindings[kKeyBindingSize] = {};
	byte _directionCombine[16] = {};
	uint16 _contactMargins[kSetCount] = {};
	int16 _jumpOffsets[kSetCount][4] = {};
	byte _idleAnimations[3] = {};
	uint16 _hitSparkAnimations[kDamageClassCount] = {};
	uint16 _weaponLauncher1[kFighterCount] = {};
	uint16 _weaponPool1[kFighterCount * kWeaponPoolSize] = {};
	uint16 _weaponLauncher2[kFighterCount] = {};
	uint16 _weaponProjectile2[kFighterCount] = {};
	uint16 _weaponProjectile4[kFighterCount] = {};
	uint16 _weaponProjectile8[kFighterCount] = {};
	uint16 _weaponProjectile16[kFighterCount] = {};
	uint16 _weaponProjectile32[kFighterCount] = {};
	uint16 _weaponProjectile64[kFighterCount] = {};
	byte _weaponDamage[kFighterTypeCount][kWeaponTypeCount] = {};
	uint16 _weaponRing[kFighterCount] = {};

	// Tables
	void initTablesType0();
	void initTablesType1();
	void initTablesType2();
	void initTablesType3();
	void initTablesType4();
	void initTablesType5();
	void initTablesType6();
	void initTablesSpecial();
	void initTablesShared();

	// Lifecycle
	void beastInit();
	void beastEnable(uint16 enable);
	void beastFinalFight();
	void beastSaveRestoreDlg(uint16 mode);
	uint16 beastNetwork(uint16 op, uint16 a, uint16 b);
	BattleContext *allocCtx();
	uint16 freeCtx(BattleContext *context);
	void delayTimeout(uint16 index);
	void kbdTimeout(byte player);
	void inputTimeout(byte player);
	void moveDone(uint16 id);
	uint16 identity(uint16 ignored, uint16 value);

	// Input
	void initInputTimers();
	uint16 inputEvent(byte code, byte player);
	uint16 inputMatch(uint16 player, uint16 timeout);
	uint16 setQueueGate(uint16 player, uint16 gate, uint16 run);
	void setMoveHook(Fighter *f, Hook on);
	void setFacing(uint16 player, uint16 facing);
	void setInputDevice(uint16 player, uint16 type);
	uint16 joyMessage(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh);
	void joyReset(uint16 player, uint16 value);
	uint16 keyDown(uint16 virtualKey);
	uint16 keyUp(uint16 virtualKey);
	void keyDirTimeout(uint16 player);
	void keyboardMap(uint16 player, uint16 enable);
	void keyboardSetKey(uint16 player, uint16 function, uint16 virtualKey);

	// Ai
	void aiSetupSkill(Fighter *f);
	void aiEvent(Fighter *f, uint16 event);
	void aiUpdateStrategy(AiBlock *a);
	void aiReact(Fighter *self, Fighter *opp);
	void aiStartTimer(Fighter *f);
	void aiTimerStep(Fighter *f);
	void aiSelectMove(Fighter *f, AiRecord *rec);
	void aiAdvance(Fighter *f);
	void aiPoll(Fighter *f);
	void aiExecuteMove(Fighter *f);
	void aiRespond(Fighter *f);
	void aiFollowUp(Fighter *f);
	byte moveClass(byte move);
	AiRecord *aiPickMove(Fighter *f, AiRecord *list, byte mask, int32 *best);
	AiRecord *aiPickMove2(Fighter *f, AiRecord *list, uint16 flag, int32 *best);
	void aiChooseNext(Fighter *f);

	// Hits
	uint16 hitAttack(uint16 id);
	uint16 hitButton(uint16 id);
	uint16 hitFighter(uint16 a, uint16 b, uint16 id);
	void startReaction(Fighter *f, uint16 anim);
	void knockBackAnim(Fighter *f, uint16 anim, uint16 dir);
	void pushBackAnim(Fighter *f, uint16 anim, uint16 amount, uint16 p, uint16 mode);
	void hitStunAnim(Fighter *f, uint16 anim);
	uint16 clashPush(Fighter *d, Fighter *s, uint16 v);
	uint16 hitPointTest(Fighter *a);

	// Moves
	void setAttackActive(Fighter *f, uint16 v);
	void playAnim(Fighter *f, uint16 anim);
	void startKnockdown(Fighter *f);
	uint16 forceMove(Fighter *f, uint16 move);
	void reactionDone(Fighter *f);
	void reactionDoneKnock(Fighter *f);
	void reactionDoneCombo(Fighter *f);
	void setInFight(Fighter *f, uint16 v);
	void resetFighterState(Fighter *f);
	void switchSprite(Fighter *f, uint16 k, uint16 anim, uint16 v);
	void endMove(Fighter *f);
	uint16 jumpMove(Fighter *f, uint16 anim, uint16 flip, int16 dx, int16 dy);
	uint16 moveGrab(Fighter *f, uint16 move, uint16 anim);
	uint16 moveJump(Fighter *f, uint16 move, uint16 anim);
	uint16 moveJumpUp(Fighter *f, uint16 move, uint16 anim);
	uint16 moveJumpAttack(Fighter *f, uint16 move, uint16 anim);
	uint16 moveWallBounce(Fighter *f, uint16 move, uint16 anim);
	uint16 moveIdle(Fighter *f, uint16 move, uint16 anim);
	uint16 moveTurn(Fighter *f, uint16 move, uint16 anim);
	uint16 moveWalk(Fighter *f, uint16 move, uint16 anim);
	uint16 moveSidestep(Fighter *f, uint16 move, uint16 anim);
	uint16 moveAttack(Fighter *f, uint16 move, uint16 anim);
	uint16 moveAttackCloseRange(Fighter *f, uint16 move, uint16 anim);
	uint16 moveAttackHoldButton2(Fighter *f, uint16 move, uint16 anim);
	uint16 moveAirAttack(Fighter *f, uint16 move, uint16 anim);
	uint16 moveProjectile(Fighter *f, uint16 move, uint16 anim);
	uint16 moveAttackBoost(Fighter *f, uint16 move, uint16 anim);
	uint16 moveAttackHoldAnyButton(Fighter *f, uint16 move, uint16 anim);
	uint16 moveGuard(Fighter *f, uint16 move, uint16 anim);
	uint16 moveCrouch(Fighter *f, uint16 move, uint16 anim);
	uint16 moveKnockOut(Fighter *f, uint16 move, uint16 anim);
	uint16 moveSelectWeapon(Fighter *f, uint16 move, uint16 anim);
	uint16 moveTransform(Fighter *f, uint16 move, uint16 anim);
	uint16 startDeath(Fighter *f, uint16 anim);
	void cbHoldCount(Fighter *f);
	void cbJumpEnd(Fighter *f);
	void cbReplayAnim(Fighter *f);
	void cbReplayAnimSync(Fighter *f);
	void cbWalkContinue(Fighter *f);
	void cbSidestepContinue(Fighter *f);
	void cbGetUp(Fighter *f);
	void cbFallAnim(Fighter *f);
	void cbTurnAround(Fighter *f);
	uint16 hookGuard(Fighter *f, uint16 code);
	uint16 hookEndAfterAnim(Fighter *f, uint16 code);
	uint16 hookRelease(Fighter *f, uint16 code);
	uint16 hookEndNow(Fighter *f, uint16 code);
	uint16 hookHoldTrack(Fighter *f, uint16 code);
	uint16 hookJump(Fighter *f, uint16 code);
	uint16 hookWalk(Fighter *f, uint16 code);
	uint16 netInputHook(Fighter *f, uint16 code);
	void initAiFlags(Fighter *f);
	void cbNull(Fighter *f);
	void setContact(Fighter *f, uint16 v);
	void setStanceSet(Fighter *f, uint16 set, uint16 flag);
	uint16 autoTurn(Fighter *f);

	// Geometry
	uint16 hurtBoxOverlap(Fighter *f, Common::Rect *destination, Common::Point *points, uint16 wide);
	void updateDistance(Fighter *f);
	int16 clampMove(Fighter *f, int16 dx, uint16 useSprite);
	void movesContactFlip();
	void contactPush(Fighter *f);
	void updateWallFlags(Fighter *f);
	void movesReposition(uint16 id, uint16 mode);
	void setPosition(Fighter *f, int16 x, uint16 level);
	void separateFighters(Fighter *f);

	// Scene
	void movesEnter(uint16 index);
	void cbEnterAnimDone(Fighter *f);
	void enterFinished(Fighter *f);
	void fightSetup(uint16 slot, uint16 flag);
	void movesDie(uint16 flag, uint16 who);
	void loserFall(Fighter *f);
	void fightOver(Fighter *f);
	void winnerPose(Fighter *f);
	void winnerWall(Fighter *f);
	void collapse(Fighter *f);
	void endFight(Fighter *f);
	void movesDieFinal(uint16 index);
	void fighterReset(Fighter *f);
	void eventTrigger15(Fighter *f);
	void eventTrigger16(Fighter *f);
	void eventTrigger18(Fighter *f);
	void eventSlotClear(byte index);
	void eventWait(uint16 index, uint16 function, uint16 variable);
	void eventTrigger(uint16 index, uint16 a, uint16 b);
	void sceneRegisterButton(uint16 id, uint16 variable, uint16 remove);
	void sceneSetBounds(uint16 index, uint16 left, uint16 right, uint16 y, uint16 f1, uint16 f2);
	void sceneSetDoor(uint16 w0, uint16 w1, uint16 w2, uint16 w3, uint16 w4, uint16 w5);
	void spawnShadow(Fighter *f);
	void spawnHitSpark(const Common::Rect *rect, byte damageClass);
	void initFightersFromHost();
	void initFighter(Fighter *f, uint16 player, uint16 type);

	// Weapons
	void weaponInit(Fighter *f);
	void weaponsFire(uint16 id, int16 dx, int16 dy);
	uint16 weaponsHit(uint16 shooter, uint16 sprite);
	uint16 weaponsSpecialHit(uint16 shooter, uint16 sprite);

	// Stats
	void loadStats(Fighter *f);
	void storeStats(Fighter *f);
	uint16 scoreAttack(Fighter *f, uint16 hit, uint16 move, uint16 value);
	void beastSaveRestore(uint16 save, uint16 nameVar);
};

} // End of namespace Topgun

#endif // TOPGUN_GAMES_BATTLE_BATTLE_H
