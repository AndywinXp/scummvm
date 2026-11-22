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

#ifndef TOPGUN_TOPGUN_V1_2_1_H
#define TOPGUN_TOPGUN_V1_2_1_H

#include "topgun/libraries/audio/audio_v1_2_1.h"
#include "topgun/libraries/resource/resource_v1_2_1.h"
#include "topgun/libraries/sprite/sprite_v1_2_1.h"
#include "topgun/topgun_v1_2.h"

// Variables and operands
#define TG_HOST_V1_2_1_NUM_VARIABLES               0x13FE
#define TG_HOST_V1_2_1_TABLE_SIZE                  0x159F
#define TG_HOST_V1_2_1_LAST_OPERAND_INDEX          0x159E
#define TG_HOST_V1_2_1_FIRST_ARGUMENT_SLOT         0x13FE
#define TG_HOST_V1_2_1_LITERAL_BIAS                0x7531
#define TG_HOST_V1_2_1_FIRST_FRAME_INDEX           0x159D
#define TG_HOST_V1_2_1_FIRST_SYSTEM_VARIABLE       5001
#define TG_HOST_V1_2_1_NUM_SYSTEM_VARIABLES        117
#define TG_HOST_V1_2_1_VARIABLE_MOUSE_BUTTON       5001
#define TG_HOST_V1_2_1_VARIABLE_LOMEM              5002
#define TG_HOST_V1_2_1_VARIABLE_SCREEN_WIDTH       5003
#define TG_HOST_V1_2_1_VARIABLE_SCREEN_HEIGHT      5004
#define TG_HOST_V1_2_1_VARIABLE_MOUSE_X            5005
#define TG_HOST_V1_2_1_VARIABLE_MOUSE_Y            5006
#define TG_HOST_V1_2_1_VARIABLE_CLICK_X            5007
#define TG_HOST_V1_2_1_VARIABLE_CLICK_Y            5008
#define TG_HOST_V1_2_1_VARIABLE_DROP_X             5009
#define TG_HOST_V1_2_1_VARIABLE_DROP_Y             5010
#define TG_HOST_V1_2_1_VARIABLE_LAUNCH_MODE        5011
#define TG_HOST_V1_2_1_VARIABLE_AUDIO_PRESENT      5012
#define TG_HOST_V1_2_1_VARIABLE_PROCESSOR_TYPE     5013
#define TG_HOST_V1_2_1_VARIABLE_OVER_256_COLORS    5014
#define TG_HOST_V1_2_1_VARIABLE_SCREEN_SAVER       5015
#define TG_HOST_V1_2_1_VARIABLE_JOYSTICKS          5016
#define TG_HOST_V1_2_1_VARIABLE_EVENT_PARAMETER    5017

// Scenes

// Palette
#define TG_HOST_V1_2_1_PALETTE_SIZE                1024

// Messages
#define TG_HOST_V1_2_1_MESSAGE_FIRST_FLUSHED       0x4C8
#define TG_HOST_V1_2_1_MESSAGE_LAST_FLUSHED        0x4CB

// Resource objects
#define TG_HOST_V1_2_1_SPRITE_DATA_ORIGINAL_COUNT  2
#define TG_HOST_V1_2_1_SPRITE_DATA_MEMBER_COUNT    4
#define TG_HOST_V1_2_1_SPRITE_DATA_PALETTE_COUNT   0x0A
#define TG_HOST_V1_2_1_SPRITE_DATA_LEVEL           0x0C
#define TG_HOST_V1_2_1_SPRITE_DATA_WHOLE_BOX       0x0F
#define TG_HOST_V1_2_1_SPRITE_DATA_DEFER_FLAG      0x10
#define TG_HOST_V1_2_1_SPRITE_DATA_PALETTE_FLAG    0x12
#define TG_HOST_V1_2_1_SPRITE_DATA_PICK_INVISIBLE  0x13
#define TG_HOST_V1_2_1_SPRITE_DATA_MEMBERS         0x14
#define TG_HOST_V1_2_1_SPRITE_DATA_PALETTE_ENTRIES 0x1C
#define TG_HOST_V1_2_1_PALETTE_EXTENSION_END       0xF6
#define TG_HOST_V1_2_1_BITMAP_HEADER_SIZE          0x14
#define TG_HOST_V1_2_1_BITMAP_HOT_X                0x0A
#define TG_HOST_V1_2_1_BITMAP_HOT_Y                0x0C
#define TG_HOST_V1_2_1_BITMAP_RESOURCE_ID          0x0E
#define TG_HOST_V1_2_1_GROUP_X                     2
#define TG_HOST_V1_2_1_GROUP_Y                     4
#define TG_HOST_V1_2_1_MAX_FRAME_BATCH             50

// Strings
#define TG_STRING_V1_2_1_FORMAT_ARGUMENT_SIZE      4

// Movies
#define TG_MOVIE_V1_2_1_FRAME_TABLE_RESOURCE_OFFSET  2
#define TG_MOVIE_V1_2_1_STREAM_RESOURCE_OFFSET       1
#define TG_MOVIE_V1_2_1_TIMELINE_AUDIO_SIZE          0x1E
#define TG_MOVIE_V1_2_1_TIMELINE_AUDIO_STATE         2
#define TG_MOVIE_V1_2_1_TIMELINE_AUDIO_STATE_SIZE    4

// Interpreter built-ins
#define TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE        10000
#define TG_BUILTIN_V1_2_1_MAX_STRING_INDEX         0x7F

// Interpreter
#define TG_SCRIPT_V1_2_1_MAX_DEPTH                30
#define TG_SCRIPT_V1_2_1_TABLE_ADDRESS             0x1520
#define TG_SCRIPT_V1_2_1_EXPRESSION_STACK_SIZE     100 // (0x7D6 - 0x70E) / 2
#define TG_SCRIPT_V1_2_1_MAX_POSTED_RUNS           40
#define TG_SCRIPT_V1_2_1_POSTED_RUN_SIZE           18
#define TG_SCRIPT_V1_2_1_CALL_HEADER_SIZE          6
#define TG_SCRIPT_V1_2_1_LAST_OPCODE               119
#define TG_SCRIPT_V1_2_1_POSTED_RECORD_BUFFER_SIZE 20
#define TG_SCRIPT_V1_2_1_GOTO_RECORD_SIZE          18
#define TG_SCRIPT_V1_2_1_GOTO_NAME_OFFSET          2
#define TG_SCRIPT_V1_2_1_GOTO_RELEASE_OFFSET       0x10
#define TG_SCRIPT_V1_2_1_GOTO_BACK_OFFSET          0x11
#define TG_SCRIPT_V1_2_1_SWITCH_ENTRY_SIZE         6
#define TG_SCRIPT_V1_2_1_LAST_BUILTIN_CODE         0x8D

// Interpreter operations: arithmetic and conditions
#define TG_SCRIPT_V1_2_1_ARITHMETIC_OPERANDS_OFFSET 6
#define TG_SCRIPT_V1_2_1_ARITHMETIC_OPERAND_SIZE   6
#define TG_SCRIPT_V1_2_1_ARITHMETIC_OPERATION      2
#define TG_SCRIPT_V1_2_1_ARITHMETIC_NEGATE         4
#define TG_SCRIPT_V1_2_1_CONDITION_EQUAL           0x01
#define TG_SCRIPT_V1_2_1_CONDITION_LESS            0x02
#define TG_SCRIPT_V1_2_1_CONDITION_GREATER         0x04
#define TG_SCRIPT_V1_2_1_CONDITION_OR              0x08
#define TG_SCRIPT_V1_2_1_CONDITION_AND             0x10
#define TG_SCRIPT_V1_2_1_CONDITION_MODULO          0x20
#define TG_SCRIPT_V1_2_1_CONDITION_NEGATE          0x80
#define TG_SCRIPT_V1_2_1_CONDITIONAL_SIZE          0x0E
#define TG_SCRIPT_V1_2_1_SKIP_TAKEN_SIZE           0x0A
#define TG_SCRIPT_V1_2_1_NUM_SCRIPT_CHOICES        6
#define TG_SCRIPT_V1_2_1_NUM_ELEMENT_VARIABLES     4

// Interpreter operations: audio and palette
#define TG_SCRIPT_V1_2_1_MIDI_PARAMETERS_SIZE      10
#define TG_SCRIPT_V1_2_1_FIRST_PALETTE_ENTRY       10
#define TG_SCRIPT_V1_2_1_MAX_PALETTE_ENTRIES       0xEC
#define TG_SCRIPT_V1_2_1_PALETTE_RESOURCE_SIZE     0x3B0 // 0xEC entries * 4 bytes
#define TG_SCRIPT_V1_2_1_PALETTE_ENTRIES_OFFSET    8

// Text objects
#define TG_TEXT_V1_2_1_HEADER_COLOR                0
#define TG_TEXT_V1_2_1_HEADER_FACE                 2
#define TG_TEXT_V1_2_1_HEADER_TEXT                 4
#define TG_TEXT_V1_2_1_HEADER_HEIGHT               6
#define TG_TEXT_V1_2_1_HEADER_HOT_X                8
#define TG_TEXT_V1_2_1_HEADER_HOT_Y                0x0A
#define TG_TEXT_V1_2_1_HEADER_BOX_WIDTH            0x0C
#define TG_TEXT_V1_2_1_HEADER_BOX_HEIGHT           0x0E
#define TG_TEXT_V1_2_1_HEADER_FLAGS                0x10
#define TG_TEXT_V1_2_1_EDIT_NUMERIC                11
#define TG_TEXT_V1_2_1_MAX_FONTS                   10
#define TG_TEXT_V1_2_1_FLAG_CENTER                 0x0001
#define TG_TEXT_V1_2_1_FLAG_RIGHT                  0x0002
#define TG_TEXT_V1_2_1_FLAG_VERTICAL_CENTER        0x0004
#define TG_TEXT_V1_2_1_FLAG_BOTTOM                 0x0008
#define TG_TEXT_V1_2_1_FLAG_WORD_BREAK             0x0010
#define TG_TEXT_V1_2_1_FLAG_SINGLE_LINE            0x0020
#define TG_TEXT_V1_2_1_FLAG_NO_PREFIX              0x0800
#define TG_TEXT_V1_2_1_UNLIMITED_WIDTH             0x7FFF

// INI file

// Input and devices
#define TG_INPUT_V1_2_1_VK_PAUSE                   0x13
#define TG_INPUT_V1_2_1_MAX_COLLISION_PAIRS        4640 // arrays grow by 10 elements: largest n with n * 14 <= 0xFDE8
#define TG_INPUT_V1_2_1_MAX_NATIVE_TIMERS          8
#define TG_INPUT_V1_2_1_NUM_JOYSTICK_SCRIPTS       34 // 2 sticks * 17 slots
#define TG_INPUT_V1_2_1_OBJECT_CLICK_SCRIPT        0
#define TG_INPUT_V1_2_1_OBJECT_DRAG_SCRIPT         6
#define TG_INPUT_V1_2_1_OBJECT_DROP_SCRIPT         8
#define TG_INPUT_V1_2_1_OBJECT_DISABLED            0x0E
#define TG_INPUT_V1_2_1_OBJECT_DRAGGABLE           0x11

// Device groups
#define TG_DEVICE_V1_2_1_NUM_GROUPS                2
#define TG_DEVICE_V1_2_1_NUM_DEVICES               6
#define TG_DEVICE_V1_2_1_KEYBOARD_BASE             1
#define TG_DEVICE_V1_2_1_JOYSTICK_BASE             3
#define TG_DEVICE_V1_2_1_MOUSE                     5
#define TG_DEVICE_V1_2_1_TYPE_JOYSTICK             1
#define TG_DEVICE_V1_2_1_TYPE_KEYBOARD             2
#define TG_DEVICE_V1_2_1_TYPE_MOUSE                5
#define TG_DEVICE_V1_2_1_MAX_BINDINGS_A            50
#define TG_DEVICE_V1_2_1_MAX_BINDINGS_B            4
#define TG_DEVICE_V1_2_1_NUM_KEYBOARD_SETS         2
#define TG_DEVICE_V1_2_1_KEYBOARD_SET_SLOTS        6
#define TG_DEVICE_V1_2_1_CODE_TABLE_SIZE           32
#define TG_DEVICE_V1_2_1_JOYSTICK_CODES_OFFSET     14 // 0x2A - 0x1C
#define TG_DEVICE_V1_2_1_JOYSTICK_STATE_SIZE       17
#define TG_DEVICE_V1_2_1_NO_DIRECTION              0x88
#define TG_DEVICE_V1_2_1_KEY_RELEASE_DELAY         200
#define TG_DEVICE_V1_2_1_STICK_DEAD_INFLATE        0x14
#define TG_DEVICE_V1_2_1_STICK_OUTER_INFLATE       0x28
#define TG_DEVICE_V1_2_1_STICK_RAY_MARGIN          2
#define TG_DEVICE_V1_2_1_STICK_ANGLE_ROTATION      450  // 45.0 degrees in tenths
#define TG_DEVICE_V1_2_1_STICK_FULL_TURN           3600 // 360.0 degrees in tenths

// Joystick
#define TG_JOYSTICK_V1_2_1_NUM_STICKS              2
#define TG_JOYSTICK_V1_2_1_AXIS_CENTER             0x8000
#define TG_JOYSTICK_V1_2_1_DISTANCE_THRESHOLD      0x1F40
#define TG_JOYSTICK_V1_2_1_EIGHT_WAY_THRESHOLD     0x3E80
#define TG_JOYSTICK_V1_2_1_STICK_OFFSET            0x11
#define TG_JOYSTICK_V1_2_1_NO_INDEX                0xFFFF

namespace Topgun {

enum MessageId_v1_2_1 {
	kMsgV1_2_1KeyUp = 0x101,
	kMsgV1_2_1IoError = 0x590
};

enum ResourceType_v1_2_1 {
	kResV1_2_1Bitmap = 1,
	kResV1_2_1Midi = 4,
	kResV1_2_1Wave = 7,
	kResV1_2_1Movie = 8,
	kResV1_2_1OpList = 9,
	kResV1_2_1BitmapGroup = 10,
	kResV1_2_1MixedGroup = 11,
	kResV1_2_1Palette = 12,
	kResV1_2_1SpriteQueueData = 13,
	kResV1_2_1Script = 14,
	kResV1_2_1Sprite = 15,
	kResV1_2_1Text = 16
};

enum ScriptOpcode_v1_2_1 {
	kOpV1_2_1CallScript = 1,
	kOpV1_2_1LoadResource = 2,
	kOpV1_2_1UnloadResource = 3,
	kOpV1_2_1SetClickScript = 4,
	kOpV1_2_1SetSpriteQueue = 5,
	kOpV1_2_1SetBackground = 6,
	kOpV1_2_1SetSpriteObjectProperties = 7,
	kOpV1_2_1PlayWave = 8,
	kOpV1_2_1PlayCDTrack = 9,
	kOpV1_2_1FadeIn = 10,
	kOpV1_2_1FadeOut = 11,
	kOpV1_2_1BindKeyScript = 12,
	kOpV1_2_1Quit = 13,
	kOpV1_2_1StopCD = 14,
	kOpV1_2_1StopWave = 15,
	kOpV1_2_1Arithmetic = 16,
	kOpV1_2_1ConditionalRunScript = 17,
	kOpV1_2_1GotoScene = 18,
	kOpV1_2_1SetScriptTimer = 19,
	kOpV1_2_1CancelScriptTimer = 20,
	kOpV1_2_1SetSceneEndScript = 21,
	kOpV1_2_1RandomRunScript = 22,
	kOpV1_2_1RoundRobinRunScript = 23,
	kOpV1_2_1TransferSprite = 24,
	kOpV1_2_1StopSprite = 25,
	kOpV1_2_1SetSpriteQueueIfLoaded = 26,
	kOpV1_2_1DebugString = 27,
	kOpV1_2_1SetSpriteLevel = 28,
	kOpV1_2_1SetActivationScript = 29,
	kOpV1_2_1PostScript = 30,
	kOpV1_2_1CursorMode = 31,
	kOpV1_2_1RunScriptIfLoaded = 32,
	kOpV1_2_1AddCollisionPair = 33,
	kOpV1_2_1ListRunScript = 34,
	kOpV1_2_1SetPauseScript = 35,
	kOpV1_2_1MoveSprite = 36,
	kOpV1_2_1Jump = 37,
	kOpV1_2_1ConditionalSkip = 38,
	kOpV1_2_1SetDisplayZoom = 39,
	kOpV1_2_1GetSpritePosition = 40,
	kOpV1_2_1FormatTextObject = 41,
	kOpV1_2_1SetSpriteClipBox = 42,
	kOpV1_2_1Switch = 43,
	kOpV1_2_1Stop = 44,
	kOpV1_2_1Nop = 45,
	kOpV1_2_1Random = 46,
	kOpV1_2_1GetKeyState = 47,
	kOpV1_2_1GetDate = 48,
	kOpV1_2_1GetAngle = 49,
	kOpV1_2_1ClipLineToRect = 50,
	kOpV1_2_1SetWallpaper = 52,
	kOpV1_2_1SetMute = 53,
	kOpV1_2_1BeginTextEdit = 54,
	kOpV1_2_1GetSpriteRect = 55,
	kOpV1_2_1FormatString = 56,
	kOpV1_2_1CompareStrings = 57,
	kOpV1_2_1CompareStringsIgnoreCase = 58,
	kOpV1_2_1IniRead = 59,
	kOpV1_2_1IniWrite = 60,
	kOpV1_2_1SetHoverRegion = 61,
	kOpV1_2_1GetIntersect = 62,
	kOpV1_2_1SetSpritePosition = 63,
	kOpV1_2_1SetMouseEventScript = 64,
	kOpV1_2_1Cosine = 65,
	kOpV1_2_1Sine = 66,
	kOpV1_2_1FlushIni = 67,
	kOpV1_2_1IsSpriteBusy = 68,
	kOpV1_2_1BackupIni = 69,
	kOpV1_2_1GetTime = 70,
	kOpV1_2_1SpriteCompositeToBackground = 71,
	kOpV1_2_1SetJoystickScript = 72,
	kOpV1_2_1SetCursorPosition = 73,
	kOpV1_2_1GetSpriteElementIds = 75,
	kOpV1_2_1Expression = 76,
	kOpV1_2_1ConditionalJumpExpression = 77,
	kOpV1_2_1SwitchExpression = 78,
	kOpV1_2_1ConditionalJumpExpressionCopy = 79,
	kOpV1_2_1PlayMidi = 80,
	kOpV1_2_1StopMidi = 81,
	kOpV1_2_1SetDeviceGroupBinding = 82,
	kOpV1_2_1DeviceGroupButtonState = 83,
	kOpV1_2_1SetDeviceGroupScript = 84,
	kOpV1_2_1DeviceGroupLatch = 85,
	kOpV1_2_1SetDeviceGroupBindingB = 86,
	kOpV1_2_1SetDeviceGroupFlag = 87,
	kOpV1_2_1GetDeviceGroupState = 88,
	kOpV1_2_1ReturnExpression = 89,
	kOpV1_2_1SetMidiParams = 92,
	kOpV1_2_1SetVolume = 93,
	kOpV1_2_1SetTextParam = 94,
	kOpV1_2_1LoadPaletteResource = 96,
	kOpV1_2_1SetPaletteEntries = 97,
	kOpV1_2_1SetDebugScriptId = 106,
	kOpV1_2_1SetQuitSceneName = 119
};

enum JoystickMessage_v1_2_1 {
	kMsgV1_2_1Joystick1Move = 0x3A0,
	kMsgV1_2_1Joystick2Move = 0x3A1,
	kMsgV1_2_1Joystick1ButtonDown = 0x3B5,
	kMsgV1_2_1Joystick2ButtonDown = 0x3B6,
	kMsgV1_2_1Joystick1ButtonUp = 0x3B7,
	kMsgV1_2_1Joystick2ButtonUp = 0x3B8
};

enum NativeTimerProc_v1_2_1 {
	kNativeTimerV1_2_1ComboHistory = 1,
	kNativeTimerV1_2_1KeyRelease
};

struct KeyRoute_v1_2_1 {
	byte device;
	byte armed;
	byte code;

	KeyRoute_v1_2_1() : device(0), armed(0), code(0) {}
};

struct DeviceBinding_v1_2_1 {
	uint16 id;
	uint16 script;
	uint16 priority;
	uint16 mask;
	byte active;

	DeviceBinding_v1_2_1() : id(0), script(0), priority(0), mask(0), active(0) {}
};

struct DeviceGroup_v1_2_1 {
	uint16 history;
	uint16 lastCode;
	uint16 pressedMask;
	uint16 stateScript;
	uint16 numBindingsA;
	uint16 numBindingsB;
	DeviceBinding_v1_2_1 bindingsA[TG_DEVICE_V1_2_1_MAX_BINDINGS_A];
	DeviceBinding_v1_2_1 bindingsB[TG_DEVICE_V1_2_1_MAX_BINDINGS_B];
	DeviceBinding_v1_2_1 latch;
	byte type;
	byte remap;
	byte latchMode;
	byte device;

	DeviceGroup_v1_2_1() : history(0), lastCode(0), pressedMask(0), stateScript(0), numBindingsA(0), numBindingsB(0),
		type(0), remap(0), latchMode(0), device(0) {}
};

struct NativeTimer_v1_2_1 {
	uint32 expiry;
	uint32 interval;
	uint16 id;
	uint16 proc;
	byte repeat;

	NativeTimer_v1_2_1() : expiry(0), interval(0), id(0), proc(0), repeat(0) {}
};

class TopgunEngine_v1_2_1 : public TopgunEngine_v1_2 {
	friend class AudioLibrary_v1_2_1;
	friend class ResourceLibrary_v1_2;
	friend class ResourceLibrary_v1_2_1;
	friend class SpriteLibrary_v1_2_1;
	friend class TopgunEngine_v1_2;

public:
	TopgunEngine_v1_2_1(OSystem *syst, const ADGameDescription *gameDesc);
	~TopgunEngine_v1_2_1() override;

protected:
	// Libraries
	AudioLibrary_v1_2_1 *audioLib() { return static_cast<AudioLibrary_v1_2_1 *>(audioLibrary()); }
	ResourceLibrary_v1_2_1 *resourceLib() { return static_cast<ResourceLibrary_v1_2_1 *>(resourceLibrary()); }
	SpriteLibrary_v1_2_1 *spriteLib() { return static_cast<SpriteLibrary_v1_2_1 *>(spriteLibrary()); }
	void syncSoundSettings() override;

	// Operands
	uint16 variableIndex(uint16 operand) const;
	uint16 operandValue(uint16 operand) const override;
	uint16 readVariable(uint16 index) const override;
	void writeVariable(uint16 index, uint16 value) override;

	// Hooks of the libraries
	bool postMessage(uint16 id, uint16 param, uint32 data) override;
	uint16 audioCreateWaveEvent(const byte *header, uint16 waveInfo);
	uint16 audioCreateMidiEvent(const byte *header, uint16 midiInfo);
	uint32 audioSourceRead(uint16 proc, uint16 handle, byte *destination, uint32 count) override;
	uint32 audioSourceRewind(uint16 proc, uint16 handle) override;
	void audioSourceClose(uint16 proc, uint16 handle) override;
	void resourceSendMessage(uint16 window, uint16 message);
	bool resourceVariablePairCallback(uint16 id, uint16 value);
	void spriteMessageService(byte *record, uint16 spriteId) override;

	// Startup
	bool rtStartup(bool runChecks) override;
	bool runStartup(const Common::String &title, const Common::String &firstContainer);
	bool initApplication(const Common::String &title);
	bool registerViewClassAndCreateMainWindow();
	bool initEngineLibraries() override;
	uint16 getPrivateProfileInt(const char *section, const char *key, uint16 defaultValue);

	// Main loop
	uint16 mainLoop() override;

	// Messages
	void dispatchMessage(const Message &message) override;
	void fillBackground() override;
	void handleAppShutdown() override;
	void handleActivateMessage(uint16 param) override;
	void flushPostedScriptMessages();
	bool fatalError(uint16 code, const Common::String &message);
	void showShutdownErrorBox();

	// Resources
	void destroyBitmapGroupResourceObject(uint16 id);
	void destroyMidiResourceObject(uint16 id);
	bool ensureBitmapGroupResourceLoaded(uint16 id);
	bool ensureMidiResourceLoaded(uint16 id);
	void applySpritePaletteExtension(const byte *header);
	bool validateResourceType(uint16 id, uint16 type);
	bool destroyResourceObjectByType(uint16 id, uint16 type) override;
	bool validateAndGetResourceByType(uint16 id, uint16 type) override;
	bool isResourceObjectLoaded(uint16 id, uint16 type);
	byte *spriteBitmapProvider(uint16 id, bool release);

	// Backgrounds
	byte *loadBitmapResourceCopy(uint16 id);
	void setBackgroundByMode(const byte *record);
	bool setBackgroundWithWipeEffect(const byte *record);

	// Transitions
	void wipeEffectIrisRings(uint16 rings);
	void executeWipeEffect(const byte *params, byte *newBitmap, byte *oldBitmap);

	// Interpreter
	bool executeScriptOp(byte *ip, uint16 *advance) override;
	uint16 getScriptOpSize(const byte *ip) override;
	bool runPostedScriptRecord(const byte *record);
	bool enqueuePostedScriptRun(uint16 script, uint16 argument1, uint16 argument2);
	void servicePostedRunQueue();
	uint16 operandAddress(uint16 operand) const;
	uint16 readAddress(uint16 address) const;
	void writeAddress(uint16 address, uint16 value);
	int32 applyArithmeticOperand(int32 accumulator, const byte *operand);
	uint16 evaluateCondition(byte condition, uint16 operandA, uint16 operandB);

	// Expressions
	uint16 evaluateExpression(const byte *expression);
	void pushExpressionValue(uint16 value);
	void exprCallBuiltinFunction(const byte *token);
	bool exprCallScript(const byte *token);
	void evaluateBuiltinFunction(uint16 code, uint16 argc);

	// Opcodes
	bool o_callScript(const byte *record) override; // 1
	bool o_setSpriteQueue(uint16 sprite, uint16 data, bool flag) override; // 5
	void o_setBackground(const byte *record); // 6
	bool o_playWave(uint16 waveId, const byte *params); // 8
	void o_stopWave(uint16 waveId, const byte *block); // 15
	void o_arithmetic(const byte *record); // 16
	bool o_conditionalRunScript(const byte *record, uint16 &size); // 17
	void o_gotoScene(const byte *record); // 18
	bool o_randomRunScript(byte *record) override; // 22
	bool o_roundRobinRunScript(byte *record) override; // 23
	void o_stopSprite(const byte *record); // 25
	void o_setSpriteLevel(const byte *record); // 28
	void o_setCursorMode(const byte *record); // 31
	void o_listRunScript(const byte *record); // 34
	void o_moveSprite(const byte *record); // 36
	uint16 o_conditionalSkip(const byte *record); // 38
	void o_getSpritePosition(const byte *record); // 40
	uint16 o_switch(const byte *record); // 43
	void o_getKeyState(const byte *record); // 47
	bool o_beginTextEdit(const byte *record); // 54
	bool o_setSpritePosition(const byte *record); // 63
	void o_setMouseEventScript(const byte *record); // 64
	void o_getTime(const byte *record); // 70
	void o_spriteCompositeToBackground(const byte *record); // 71
	void o_setJoystickScript(const byte *record); // 72
	void o_setCursorPosition(const byte *record); // 73
	void o_getSpriteElementIds(const byte *record); // 75
	uint16 o_conditionalJumpExpr(const byte *record); // 77
	uint16 o_switchExpression(const byte *record); // 78
	uint16 o_conditionalJumpExprCopy(const byte *record); // 79
	void o_playMidi(const byte *record); // 80
	void o_stopMidi(const byte *record); // 81
	void o_setDeviceGroupBinding(const byte *record); // 82
	void o_deviceGroupButtonState(const byte *record); // 83
	void o_setDeviceGroupScript(const byte *record); // 84
	void o_deviceGroupLatch(const byte *record); // 85
	void o_setDeviceGroupBindingB(const byte *record); // 86
	void o_setDeviceGroupFlag(const byte *record); // 87
	void o_getDeviceGroupState(const byte *record); // 88
	void o_setMidiParams(const byte *record); // 92
	void o_setVolume(const byte *record); // 93
	void o_setTextParam(const byte *record); // 94
	void o_loadPaletteResource(const byte *record); // 96
	void o_setPaletteEntries(const byte *record); // 97

	// Builtins
	uint16 builtinGetKeyState(uint16 vk); // 47
	uint16 builtinSetDraggedSpriteId(uint16 id); // 95
	uint16 builtinCopyResource(uint16 id); // 101
	uint16 builtinDeleteResource(uint16 id); // 102
	uint16 builtinSetKeyHookScript(uint16 script); // 105
	uint16 builtinSetKeyboardSetKey(uint16 set, uint16 kind, uint16 vk); // 107
	uint16 builtinPauseTimers(uint16 flag); // 109
	uint16 builtinSetJoystickDistanceMode(uint16 value); // 110
	uint16 builtinSetSpriteHoverScript(uint16 script); // 111
	uint16 builtinBlitBitmapOverlap(uint16 first, uint16 second); // 125
	uint16 builtinGetMouseEventScript(); // 129
	uint16 builtinSetJoystickButtonOffsetMode(uint16 value); // 130
	uint16 builtinReattachDeviceGroup(uint16 group, uint16 flag); // 137

	// Text objects and INI
	uint16 createTextObjectFont(int16 height, const char *face);
	void deleteTextObjectFont(uint16 font);
	void refreshTextObjectFont(TextObject_v1_2 *object);
	Graphics::ManagedSurface *renderTextToBitmap(const TextObject_v1_2 *object);
	void textEditAppendCaret(uint16 object);
	bool startTextEditOnObject(const byte *record);

	// Input messages
	void handleKeyUpMessage(Common::KeyCode keycode);
	void togglePauseState();
	uint16 postKeyHookScript(uint16 vk);

	// Click, hover and drag regions
	bool postRunScript(uint16 script, uint16 argument1, uint16 argument2) override;
	bool updateSpriteHoverFromPoint(int16 x, int16 y);
	void postSpriteLeaveScript();

	// Timers and collisions
	uint16 removeNativeTimer(uint16 index);
	void findNativeTimer(uint16 id);
	void addNativeTimer(uint16 id, uint32 interval, uint16 proc, byte repeat);
	void serviceNativeTimerQueue();
	void serviceCollisionPairs();

	// Device groups
	void clearJoystickScripts();
	void clearDeviceState();
	uint16 insertDeviceBindingSorted(uint16 count, DeviceBinding_v1_2_1 *list, const byte *record);
	void setAllBindingScripts(uint16 script, uint16 count, DeviceBinding_v1_2_1 *list);
	void setAllBindingActive(byte active, uint16 count, DeviceBinding_v1_2_1 *list);
	void fillJoystickStateBytes(byte value, byte stick);
	uint16 deleteDeviceBinding(uint16 index, uint16 count, DeviceBinding_v1_2_1 *list);
	uint16 updateDeviceBindingList(uint16 maxEntries, uint16 count, DeviceBinding_v1_2_1 *list, const byte *record);
	void fireDeviceGroupStateScript(byte group);
	void comboHistoryTimeoutCallback(byte timerId);
	void scanDeviceBindingsListB(byte group);
	void deviceGroupInputEvent(byte code, byte device);
	bool keyboardRoutedKeyDown(uint16 vk);
	bool keyboardRoutedKeyUp(uint16 vk);
	void keyReleaseDelayTimerCallback(byte device);
	void bindKeyboardSetToDevice(byte device, byte value);
	bool mouseAsStickMessage(Common::Point point, uint16 message);
	void resetMouseStick(byte value);

	// Joystick
	uint16 joystickGetScriptForDirectionCode(uint16 offset, uint16 flags);
	uint16 joystickButtonIndex(uint16 flags);
	uint16 joystickDirectionIndex(uint16 offset, const uint16 *center, uint16 x, uint16 y);
	uint16 joystickDirectionByDistance(const uint16 *center, uint16 x, uint16 y);
	uint16 joystickDirection8Way(const uint16 *center, uint16 x, uint16 y);
	uint16 joystickGetScriptForPosition(uint16 offset, const uint16 *center, uint16 x, uint16 y);
	void handleJoystickButtonMessage(const Message &message);
	void joystickInit();
	uint16 joystickMessageToIndex(const Message &message);
	void handleJoystickMessage(const Message &message);
	void joystickRelease();

	// Variables
	uint16 _variableTable[TG_HOST_V1_2_1_TABLE_SIZE] = {};
	uint16 _argumentFrame = TG_HOST_V1_2_1_FIRST_FRAME_INDEX;

	// Startup state
	uint16 _fatalCode = 0;
	bool _debugMode = false;
	uint16 _maxJoySticks = 0;
	uint16 _inputTimeout = 0;
	byte _screenSaverOption = 1;
	uint16 _activateScript = 0;
	bool _activateState = false;
	uint16 _numPostedScriptRuns = 0;
	uint16 _numNativeTimers = 0;

	// Scene state
	uint16 _maxTransColors = 0;
	uint16 _numFileResources = 0;
	uint16 _driveType = 0;

	// Input state
	NativeTimer_v1_2_1 _nativeTimers[TG_INPUT_V1_2_1_MAX_NATIVE_TIMERS];
	uint32 _timersPausedAt = 0;
	bool _joystickRouted = false;
	bool _joystickDirectionFlag = false;
	bool _joystickButtonFlag = false;
	uint16 _pauseScript = 0;
	uint16 _numJoysticks = 0;

	// Device state
	KeyRoute_v1_2_1 _keyRoutes[TG_INPUT_V1_2_1_KEY_TABLE_SIZE];
	DeviceGroup_v1_2_1 _deviceGroups[TG_DEVICE_V1_2_1_NUM_GROUPS];
	byte _deviceOwners[TG_DEVICE_V1_2_1_NUM_DEVICES] = {};
	byte _keyboardDirections[TG_DEVICE_V1_2_1_NUM_KEYBOARD_SETS] = {};
	byte _joystickStates[TG_INPUT_V1_2_1_NUM_JOYSTICK_SCRIPTS] = {};
	byte _joystickLastIndex[TG_JOYSTICK_V1_2_1_NUM_STICKS] = {};
	uint16 _joystickScripts[TG_INPUT_V1_2_1_NUM_JOYSTICK_SCRIPTS] = {};
	Common::Rect _mouseStickDeadRect;
	Common::Rect _mouseStickOuterRect;
	uint16 _mouseStickCode = 0;

	byte _keyboardSetKeys[TG_DEVICE_V1_2_1_NUM_KEYBOARD_SETS][TG_DEVICE_V1_2_1_KEYBOARD_SET_SLOTS] = {
		{ 0x46, 0x56, 0x43, 0x42, 0x31, 0x32 },
		{ 0x26, 0x28, 0x25, 0x27, 0x39, 0x30 }
	};

	byte _keyboardSetCodes[TG_DEVICE_V1_2_1_NUM_KEYBOARD_SETS][TG_DEVICE_V1_2_1_KEYBOARD_SET_SLOTS] = {
		{ 0x10, 0x20, 0x80, 0x40, 0x01, 0x02 },
		{ 0x10, 0x20, 0x80, 0x40, 0x01, 0x02 }
	};

	// Joystick state
	bool _joystickCaptured = false;
	uint16 _joystickCenter[TG_JOYSTICK_V1_2_1_NUM_STICKS][2] = {};
	uint16 _joystickX = TG_JOYSTICK_V1_2_1_AXIS_CENTER;
	uint16 _joystickY = TG_JOYSTICK_V1_2_1_AXIS_CENTER;
	byte _joystickButtons = 0;

	// Interpreter state
	uint16 _expressionStack[TG_SCRIPT_V1_2_1_EXPRESSION_STACK_SIZE] = {};
	uint16 _expressionStackPointer = 0;
	uint16 _debugScriptId = 0;
	byte _postedRuns[TG_SCRIPT_V1_2_1_MAX_POSTED_RUNS * TG_SCRIPT_V1_2_1_POSTED_RUN_SIZE] = {};
	char _quitSceneName[TG_HOST_V1_2_1_SCENE_NAME_SIZE] = {};
	char _gotoSceneName[TG_HOST_V1_2_1_SCENE_NAME_SIZE] = {};
	// The memory check is not emulated; the scripts compare this value with 53 (53 * 65536 is about 3.4 MB), so it stands for plenty of memory
	uint16 _freeMemoryHigh = 0xFFFF;

	// Palette and background state
	byte _palette[TG_HOST_V1_2_1_PALETTE_SIZE] = {};
};

} // End of namespace Topgun

#endif // TOPGUN_TOPGUN_V1_2_1_H
