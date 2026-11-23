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

#ifndef TOPGUN_TOPGUN_V1_2_3_H
#define TOPGUN_TOPGUN_V1_2_3_H

#include "topgun/libraries/audio/audio_v1_2_3.h"
#include "topgun/libraries/resource/resource_v1_2_3.h"
#include "topgun/libraries/sprite/sprite_v1_2_3.h"
#include "topgun/plugin.h"
#include "topgun/topgun_v1_2.h"

// Variables and operands
#define TG_HOST_V1_2_3_NUM_VARIABLES               0x13FE
#define TG_HOST_V1_2_3_TABLE_SIZE                  0x159F
#define TG_HOST_V1_2_3_LAST_OPERAND_INDEX          0x159E
#define TG_HOST_V1_2_3_FIRST_ARGUMENT_SLOT         0x13FE
#define TG_HOST_V1_2_3_LITERAL_BIAS                0x7531
#define TG_HOST_V1_2_3_FIRST_FRAME_INDEX           0x159D
#define TG_HOST_V1_2_3_FIRST_SYSTEM_VARIABLE       5001
#define TG_HOST_V1_2_3_NUM_SYSTEM_VARIABLES        117
#define TG_HOST_V1_2_3_VARIABLE_MOUSE_BUTTON       5001
#define TG_HOST_V1_2_3_VARIABLE_LOMEM              5002
#define TG_HOST_V1_2_3_VARIABLE_SCREEN_WIDTH       5003
#define TG_HOST_V1_2_3_VARIABLE_SCREEN_HEIGHT      5004
#define TG_HOST_V1_2_3_VARIABLE_MOUSE_X            5005
#define TG_HOST_V1_2_3_VARIABLE_MOUSE_Y            5006
#define TG_HOST_V1_2_3_VARIABLE_CLICK_X            5007
#define TG_HOST_V1_2_3_VARIABLE_CLICK_Y            5008
#define TG_HOST_V1_2_3_VARIABLE_DROP_X             5009
#define TG_HOST_V1_2_3_VARIABLE_DROP_Y             5010
#define TG_HOST_V1_2_3_VARIABLE_LAUNCH_MODE        5011
#define TG_HOST_V1_2_3_VARIABLE_AUDIO_PRESENT      5012
#define TG_HOST_V1_2_3_VARIABLE_PROCESSOR_TYPE     5013
#define TG_HOST_V1_2_3_VARIABLE_OVER_256_COLORS    5014
#define TG_HOST_V1_2_3_VARIABLE_SCREEN_SAVER       5015
#define TG_HOST_V1_2_3_VARIABLE_JOYSTICKS          5016
#define TG_HOST_V1_2_3_VARIABLE_EVENT_PARAMETER    5017

// Scenes

// Palette
#define TG_HOST_V1_2_3_PALETTE_SIZE                1024

// Messages
#define TG_HOST_V1_2_3_MESSAGE_FIRST_FLUSHED       0x4C8
#define TG_HOST_V1_2_3_MESSAGE_LAST_FLUSHED        0x4CB
#define TG_HOST_V1_2_3_MESSAGE_FIRST_PLUGIN        0x658
#define TG_HOST_V1_2_3_MESSAGE_LAST_PLUGIN         0x6BB
#define TG_HOST_V1_2_6_MESSAGE_FIRST_PLUGIN        0x6BC
#define TG_HOST_V1_2_6_MESSAGE_LAST_PLUGIN         0x71F

// Plug-ins
#define TG_PLUGIN_V1_2_3_STRING_COPY_SIZE          0x7F
#define TG_PLUGIN_V1_2_3_FRAME_IDS_SIZE            4

// Resource objects
#define TG_HOST_V1_2_3_SPRITE_DATA_ORIGINAL_COUNT  2
#define TG_HOST_V1_2_3_SPRITE_DATA_MEMBER_COUNT    4
#define TG_HOST_V1_2_3_SPRITE_DATA_PALETTE_COUNT   0x0A
#define TG_HOST_V1_2_3_SPRITE_DATA_LEVEL           0x0C
#define TG_HOST_V1_2_3_SPRITE_DATA_WHOLE_BOX       0x0F
#define TG_HOST_V1_2_3_SPRITE_DATA_DEFER_FLAG      0x10
#define TG_HOST_V1_2_3_SPRITE_DATA_PALETTE_FLAG    0x12
#define TG_HOST_V1_2_3_SPRITE_DATA_PICK_INVISIBLE  0x13
#define TG_HOST_V1_2_3_SPRITE_DATA_MEMBERS         0x14
#define TG_HOST_V1_2_3_SPRITE_DATA_PALETTE_ENTRIES 0x1C
#define TG_HOST_V1_2_6_SPRITE_DATA_SHIFT           2    // every field after the click script moves up by this much
#define TG_HOST_V1_2_6_SPRITE_DATA_PALETTE_ENTRIES_SHIFT 0x0E
#define TG_HOST_V1_2_3_PALETTE_EXTENSION_END       0xF6
#define TG_HOST_V1_2_3_BITMAP_HEADER_SIZE          0x14
#define TG_HOST_V1_2_3_BITMAP_HOT_X                0x0A
#define TG_HOST_V1_2_3_BITMAP_HOT_Y                0x0C
#define TG_HOST_V1_2_3_BITMAP_RESOURCE_ID          0x0E
#define TG_HOST_V1_2_3_GROUP_X                     2
#define TG_HOST_V1_2_3_GROUP_Y                     4
#define TG_HOST_V1_2_3_MAX_FRAME_BATCH             50

// Strings
#define TG_STRING_V1_2_3_FORMAT_ARGUMENT_SIZE      4

// Movies
#define TG_MOVIE_V1_2_3_FRAME_TABLE_RESOURCE_OFFSET  2
#define TG_MOVIE_V1_2_3_STREAM_RESOURCE_OFFSET       1
#define TG_MOVIE_V1_2_3_TIMELINE_AUDIO_SIZE          0x1E
#define TG_MOVIE_V1_2_3_TIMELINE_AUDIO_STATE         2
#define TG_MOVIE_V1_2_3_TIMELINE_AUDIO_STATE_SIZE    4

// Interpreter built-ins
#define TG_BUILTIN_V1_2_3_FIXED_POINT_SCALE        10000
#define TG_BUILTIN_V1_2_3_MAX_STRING_INDEX         0x7F

// Interpreter
#define TG_SCRIPT_V1_2_3_MAX_DEPTH                30
#define TG_SCRIPT_V1_2_3_TABLE_ADDRESS             0x1520
#define TG_SCRIPT_V1_2_3_MAX_POSTED_RUNS           40
#define TG_SCRIPT_V1_2_3_POSTED_RUN_SIZE           18
#define TG_SCRIPT_V1_2_3_CALL_HEADER_SIZE          6
#define TG_SCRIPT_V1_2_3_LAST_OPCODE               119
#define TG_SCRIPT_V1_2_5_LAST_OPCODE               143
#define TG_SCRIPT_V1_2_3_POSTED_RECORD_BUFFER_SIZE 20
#define TG_SCRIPT_V1_2_3_GOTO_RECORD_SIZE          18
#define TG_SCRIPT_V1_2_5_GOTO_RECORD_SIZE          20
#define TG_SCRIPT_V1_2_3_GOTO_NAME_OFFSET          2
#define TG_SCRIPT_V1_2_3_GOTO_RELEASE_OFFSET       0x10
#define TG_SCRIPT_V1_2_5_GOTO_RELEASE_OFFSET       0x12
#define TG_SCRIPT_V1_2_3_GOTO_BACK_OFFSET          0x11
#define TG_SCRIPT_V1_2_5_GOTO_BACK_OFFSET          0x13
#define TG_SCRIPT_V1_2_5_QUIT_SCENE_RECORD_SIZE    18
#define TG_SCRIPT_V1_2_5_RESERVED_RECORD_SIZE      6
#define TG_SCRIPT_V1_2_3_SWITCH_ENTRY_SIZE         6
#define TG_SCRIPT_V1_2_3_LAST_BUILTIN_CODE         0x8D
#define TG_SCRIPT_V1_2_5_LAST_BUILTIN_CODE         0x8F
#define TG_SCRIPT_V1_2_6_LAST_BUILTIN_CODE         0xB2
#define TG_SCRIPT_V1_2_6_SET_CLICK_SCRIPT_SIZE     20
#define TG_SCRIPT_V1_2_6_SET_BACKGROUND_SIZE       16
#define TG_SCRIPT_V1_2_6_SET_HOVER_REGION_SIZE     22
#define TG_SCRIPT_V1_2_6_GET_ELEMENT_IDS_SIZE      24
#define TG_SCRIPT_V1_2_6_WINDOWS_VERSION           95
#define TG_SCRIPT_V1_2_6_IDLE_SECONDS_TO_MS        1000

// Interpreter operations: arithmetic and conditions
#define TG_SCRIPT_V1_2_3_ARITHMETIC_OPERANDS_OFFSET 6
#define TG_SCRIPT_V1_2_3_ARITHMETIC_OPERAND_SIZE   6
#define TG_SCRIPT_V1_2_3_ARITHMETIC_OPERATION      2
#define TG_SCRIPT_V1_2_3_ARITHMETIC_NEGATE         4
#define TG_SCRIPT_V1_2_3_CONDITION_EQUAL           0x01
#define TG_SCRIPT_V1_2_3_CONDITION_LESS            0x02
#define TG_SCRIPT_V1_2_3_CONDITION_GREATER         0x04
#define TG_SCRIPT_V1_2_3_CONDITION_OR              0x08
#define TG_SCRIPT_V1_2_3_CONDITION_AND             0x10
#define TG_SCRIPT_V1_2_3_CONDITION_MODULO          0x20
#define TG_SCRIPT_V1_2_3_CONDITION_NEGATE          0x80
#define TG_SCRIPT_V1_2_3_CONDITIONAL_SIZE          0x0E
#define TG_SCRIPT_V1_2_3_SKIP_TAKEN_SIZE           0x0A
#define TG_SCRIPT_V1_2_3_NUM_SCRIPT_CHOICES        6

// Interpreter operations: audio and palette
#define TG_SCRIPT_V1_2_3_MIDI_PARAMETERS_SIZE      10
#define TG_SCRIPT_V1_2_3_FIRST_PALETTE_ENTRY       10
#define TG_SCRIPT_V1_2_3_MAX_PALETTE_ENTRIES       0xEC
#define TG_SCRIPT_V1_2_3_PALETTE_RESOURCE_SIZE     0x3B0 // 0xEC entries * 4 bytes
#define TG_SCRIPT_V1_2_3_PALETTE_ENTRIES_OFFSET    8

// Transitions
#define TG_HOST_V1_2_6_WIPE_PARAMETER_COUNT        2
#define TG_HOST_V1_2_6_WIPE_PARAMETER_DURATION     4
#define TG_HOST_V1_2_6_WIPE_ROWS_DEFAULT_COUNT     10
#define TG_HOST_V1_2_6_WIPE_SLIDE_DEFAULT_STEPS    0x3C

// Text objects
#define TG_TEXT_V1_2_3_HEADER_COLOR                0
#define TG_TEXT_V1_2_3_HEADER_FACE                 2
#define TG_TEXT_V1_2_3_HEADER_TEXT                 4
#define TG_TEXT_V1_2_3_HEADER_HEIGHT               6
#define TG_TEXT_V1_2_3_HEADER_HOT_X                8
#define TG_TEXT_V1_2_3_HEADER_HOT_Y                0x0A
#define TG_TEXT_V1_2_3_HEADER_BOX_WIDTH            0x0C
#define TG_TEXT_V1_2_3_HEADER_BOX_HEIGHT           0x0E
#define TG_TEXT_V1_2_3_HEADER_FLAGS                0x10
#define TG_TEXT_V1_2_3_EDIT_NUMERIC                11
#define TG_TEXT_V1_2_3_MAX_FONTS                   10
#define TG_TEXT_V1_2_3_FLAG_CENTER                 0x0001
#define TG_TEXT_V1_2_3_FLAG_RIGHT                  0x0002
#define TG_TEXT_V1_2_3_FLAG_VERTICAL_CENTER        0x0004
#define TG_TEXT_V1_2_3_FLAG_BOTTOM                 0x0008
#define TG_TEXT_V1_2_3_FLAG_WORD_BREAK             0x0010
#define TG_TEXT_V1_2_3_FLAG_SINGLE_LINE            0x0020
#define TG_TEXT_V1_2_3_FLAG_NO_PREFIX              0x0800
#define TG_TEXT_V1_2_3_UNLIMITED_WIDTH             0x7FFF

// INI file

// Input and devices
#define TG_INPUT_V1_2_3_MAX_COLLISION_PAIRS        4640 // arrays grow by 10 elements: largest n with n * 14 <= 0xFDE8
#define TG_INPUT_V1_2_3_MAX_NATIVE_TIMERS          8
#define TG_INPUT_V1_2_3_OBJECT_CLICK_SCRIPT        0
#define TG_INPUT_V1_2_3_OBJECT_DRAG_SCRIPT         6
#define TG_INPUT_V1_2_3_OBJECT_DROP_SCRIPT         8
#define TG_INPUT_V1_2_3_OBJECT_DISABLED            0x0E
#define TG_INPUT_V1_2_3_OBJECT_DRAGGABLE           0x11
#define TG_INPUT_V1_2_6_OBJECT_CLICK_ARGUMENT      2
#define TG_INPUT_V1_2_6_OBJECT_DRAG_SCRIPT         8
#define TG_INPUT_V1_2_6_OBJECT_DROP_SCRIPT         0x0A
#define TG_INPUT_V1_2_6_OBJECT_DISABLED            0x10
#define TG_INPUT_V1_2_6_OBJECT_DRAGGABLE           0x13

// Device groups
#define TG_DEVICE_V1_2_3_NUM_GROUPS                2
#define TG_DEVICE_V1_2_3_NUM_DEVICES               6
#define TG_DEVICE_V1_2_3_KEYBOARD_BASE             1
#define TG_DEVICE_V1_2_3_JOYSTICK_BASE             3
#define TG_DEVICE_V1_2_3_MOUSE                     5
#define TG_DEVICE_V1_2_3_TYPE_JOYSTICK             1
#define TG_DEVICE_V1_2_3_TYPE_KEYBOARD             2
#define TG_DEVICE_V1_2_3_TYPE_MOUSE                5
#define TG_DEVICE_V1_2_3_MAX_BINDINGS_A            50
#define TG_DEVICE_V1_2_3_MAX_BINDINGS_B            4
#define TG_DEVICE_V1_2_3_NUM_KEYBOARD_SETS         2
#define TG_DEVICE_V1_2_3_KEYBOARD_SET_SLOTS        6
#define TG_DEVICE_V1_2_5_KEYBOARD_SET_SLOTS        8
#define TG_DEVICE_V1_2_3_CODE_TABLE_SIZE           32
#define TG_DEVICE_V1_2_3_JOYSTICK_CODES_OFFSET     14 // 0x2A - 0x1C
#define TG_DEVICE_V1_2_3_JOYSTICK_STATE_SIZE       17
#define TG_DEVICE_V1_2_3_NO_DIRECTION              0x88
#define TG_DEVICE_V1_2_3_KEY_RELEASE_DELAY         200
#define TG_DEVICE_V1_2_3_STICK_DEAD_INFLATE        0x14
#define TG_DEVICE_V1_2_3_STICK_OUTER_INFLATE       0x28
#define TG_DEVICE_V1_2_3_STICK_RAY_MARGIN          2
#define TG_DEVICE_V1_2_3_STICK_ANGLE_ROTATION      450  // 45.0 degrees in tenths
#define TG_DEVICE_V1_2_3_STICK_FULL_TURN           3600 // 360.0 degrees in tenths

// Joystick
#define TG_JOYSTICK_V1_2_3_EIGHT_WAY_THRESHOLD     0x3E80

namespace Topgun {

enum MessageId_v1_2_3 {
	kMsgV1_2_3KeyUp = 0x101
};

enum ResourceType_v1_2_3 {
	kResV1_2_3Bitmap = 1,
	kResV1_2_3Midi = 4,
	kResV1_2_3Wave = 7,
	kResV1_2_3Movie = 8,
	kResV1_2_3OpList = 9,
	kResV1_2_3BitmapGroup = 10,
	kResV1_2_3MixedGroup = 11,
	kResV1_2_3Palette = 12,
	kResV1_2_3SpriteQueueData = 13,
	kResV1_2_3Script = 14,
	kResV1_2_3Sprite = 15,
	kResV1_2_3Text = 16
};

// The later numbering of the types from the wave on; the bitmap and the midi keep the numbers above
enum ResourceType_v1_2_5 {
	kResV1_2_5Wave = 9,
	kResV1_2_5Movie = 10,
	kResV1_2_5OpList = 11,
	kResV1_2_5BitmapGroup = 12,
	kResV1_2_5MixedGroup = 13,
	kResV1_2_5Palette = 14,
	kResV1_2_5SpriteQueueData = 15,
	kResV1_2_5Script = 16,
	kResV1_2_5Sprite = 17,
	kResV1_2_5Text = 18
};

enum ScriptOpcode_v1_2_3 {
	kOpV1_2_3CallScript = 1,
	kOpV1_2_3LoadResource = 2,
	kOpV1_2_3UnloadResource = 3,
	kOpV1_2_3SetClickScript = 4,
	kOpV1_2_3SetSpriteQueue = 5,
	kOpV1_2_3SetBackground = 6,
	kOpV1_2_3SetSpriteObjectProperties = 7,
	kOpV1_2_3PlayWave = 8,
	kOpV1_2_3PlayCDTrack = 9,
	kOpV1_2_3FadeIn = 10,
	kOpV1_2_3FadeOut = 11,
	kOpV1_2_3BindKeyScript = 12,
	kOpV1_2_3Quit = 13,
	kOpV1_2_3StopCD = 14,
	kOpV1_2_3StopWave = 15,
	kOpV1_2_3Arithmetic = 16,
	kOpV1_2_3ConditionalRunScript = 17,
	kOpV1_2_3GotoScene = 18,
	kOpV1_2_3SetScriptTimer = 19,
	kOpV1_2_3CancelScriptTimer = 20,
	kOpV1_2_3SetSceneEndScript = 21,
	kOpV1_2_3RandomRunScript = 22,
	kOpV1_2_3RoundRobinRunScript = 23,
	kOpV1_2_3TransferSprite = 24,
	kOpV1_2_3StopSprite = 25,
	kOpV1_2_3SetSpriteQueueIfLoaded = 26,
	kOpV1_2_3DebugString = 27,
	kOpV1_2_3SetSpriteLevel = 28,
	kOpV1_2_3SetActivationScript = 29,
	kOpV1_2_3PostScript = 30,
	kOpV1_2_3CursorMode = 31,
	kOpV1_2_3RunScriptIfLoaded = 32,
	kOpV1_2_3AddCollisionPair = 33,
	kOpV1_2_3ListRunScript = 34,
	kOpV1_2_3SetPauseScript = 35,
	kOpV1_2_3MoveSprite = 36,
	kOpV1_2_3Jump = 37,
	kOpV1_2_3ConditionalSkip = 38,
	kOpV1_2_3SetDisplayZoom = 39,
	kOpV1_2_3GetSpritePosition = 40,
	kOpV1_2_3FormatTextObject = 41,
	kOpV1_2_3SetSpriteClipBox = 42,
	kOpV1_2_3Switch = 43,
	kOpV1_2_3Stop = 44,
	kOpV1_2_3Nop = 45,
	kOpV1_2_3Random = 46,
	kOpV1_2_3GetKeyState = 47,
	kOpV1_2_3GetDate = 48,
	kOpV1_2_3GetAngle = 49,
	kOpV1_2_3ClipLineToRect = 50,
	kOpV1_2_3SetWallpaper = 52,
	kOpV1_2_3SetMute = 53,
	kOpV1_2_3BeginTextEdit = 54,
	kOpV1_2_3GetSpriteRect = 55,
	kOpV1_2_3FormatString = 56,
	kOpV1_2_3CompareStrings = 57,
	kOpV1_2_3CompareStringsIgnoreCase = 58,
	kOpV1_2_3IniRead = 59,
	kOpV1_2_3IniWrite = 60,
	kOpV1_2_3SetHoverRegion = 61,
	kOpV1_2_3GetIntersect = 62,
	kOpV1_2_3SetSpritePosition = 63,
	kOpV1_2_3SetMouseEventScript = 64,
	kOpV1_2_3Cosine = 65,
	kOpV1_2_3Sine = 66,
	kOpV1_2_3FlushIni = 67,
	kOpV1_2_3IsSpriteBusy = 68,
	kOpV1_2_3BackupIni = 69,
	kOpV1_2_3GetTime = 70,
	kOpV1_2_3SpriteCompositeToBackground = 71,
	kOpV1_2_3SetJoystickScript = 72,
	kOpV1_2_3SetCursorPosition = 73,
	kOpV1_2_3GetSpriteElementIds = 75,
	kOpV1_2_3Expression = 76,
	kOpV1_2_3ConditionalJumpExpression = 77,
	kOpV1_2_3SwitchExpression = 78,
	kOpV1_2_3ConditionalJumpExpressionCopy = 79,
	kOpV1_2_3PlayMidi = 80,
	kOpV1_2_3StopMidi = 81,
	kOpV1_2_3SetDeviceGroupBinding = 82,
	kOpV1_2_3DeviceGroupButtonState = 83,
	kOpV1_2_3SetDeviceGroupScript = 84,
	kOpV1_2_3DeviceGroupLatch = 85,
	kOpV1_2_3SetDeviceGroupBindingB = 86,
	kOpV1_2_3SetDeviceGroupFlag = 87,
	kOpV1_2_3GetDeviceGroupState = 88,
	kOpV1_2_3ReturnExpression = 89,
	kOpV1_2_3SetMidiParams = 92,
	kOpV1_2_3SetVolume = 93,
	kOpV1_2_3SetTextParam = 94,
	kOpV1_2_3LoadPaletteResource = 96,
	kOpV1_2_3SetPaletteEntries = 97,
	kOpV1_2_3SetDebugScriptId = 106,
	kOpV1_2_3SetQuitSceneName = 119
};

enum ScriptOpcode_v1_2_5 {
	kOpV1_2_5SetReservedWord = 141,
	kOpV1_2_5Reserved = 143
};

enum NativeTimerProc_v1_2_3 {
	kNativeTimerV1_2_3ComboHistory = 1,
	kNativeTimerV1_2_3KeyRelease
};

struct KeyRoute_v1_2_3 {
	byte device;
	byte armed;
	byte code;

	KeyRoute_v1_2_3() : device(0), armed(0), code(0) {}
};

struct DeviceBinding_v1_2_3 {
	uint16 id;
	uint16 script;
	uint16 priority;
	uint16 mask;
	byte active;

	DeviceBinding_v1_2_3() : id(0), script(0), priority(0), mask(0), active(0) {}
};

struct DeviceGroup_v1_2_3 {
	uint16 history;
	uint16 lastCode;
	uint16 pressedMask;
	uint16 stateScript;
	uint16 numBindingsA;
	uint16 numBindingsB;
	DeviceBinding_v1_2_3 bindingsA[TG_DEVICE_V1_2_3_MAX_BINDINGS_A];
	DeviceBinding_v1_2_3 bindingsB[TG_DEVICE_V1_2_3_MAX_BINDINGS_B];
	DeviceBinding_v1_2_3 latch;
	byte type;
	byte remap;
	byte latchMode;
	byte device;
	byte attached;

	DeviceGroup_v1_2_3() : history(0), lastCode(0), pressedMask(0), stateScript(0), numBindingsA(0), numBindingsB(0),
		type(0), remap(0), latchMode(0), device(0), attached(0) {}
};

struct NativeTimer_v1_2_3 {
	uint32 expiry;
	uint32 interval;
	uint16 id;
	uint16 proc;
	byte repeat;

	NativeTimer_v1_2_3() : expiry(0), interval(0), id(0), proc(0), repeat(0) {}
};

class EnginePluginHost;

class TopgunEngine_v1_2_3 : public TopgunEngine_v1_2 {
	friend class AudioLibrary_v1_2_3;
	friend class EnginePluginHost;
	friend class ResourceLibrary_v1_2;
	friend class ResourceLibrary_v1_2_3;
	friend class SpriteLibrary_v1_2_3;
	friend class TopgunEngine_v1_2;

public:
	TopgunEngine_v1_2_3(OSystem *syst, const ADGameDescription *gameDesc, EngineVersion version);
	~TopgunEngine_v1_2_3() override;

protected:
	// Libraries
	AudioLibrary_v1_2_3 *audioLib() { return static_cast<AudioLibrary_v1_2_3 *>(audioLibrary()); }
	ResourceLibrary_v1_2_3 *resourceLib() { return static_cast<ResourceLibrary_v1_2_3 *>(resourceLibrary()); }
	SpriteLibrary_v1_2_3 *spriteLib() { return static_cast<SpriteLibrary_v1_2_3 *>(spriteLibrary()); }
	void syncSoundSettings() override;

	// Hooks of the libraries
	bool postMessage(uint16 id, uint16 param, uint32 data) override;
	uint16 audioCreateWaveEvent(const byte *header, uint16 waveInfo);
	void spriteMessageService(byte *record, uint16 spriteId) override;

	// Startup
	bool rtStartup(bool runChecks) override;
	bool runStartup(const Common::String &title, const Common::String &firstContainer);
	bool initApplication(const Common::String &title);
	bool registerViewClassAndCreateMainWindow();
	bool initEngineLibraries() override;

	// Main loop
	uint16 mainLoop() override;

	// Messages
	void dispatchMessage(const Message &message) override;
	void fillBackground() override;
	void handleAppShutdown() override;
	void handleActivateMessage(uint16 param) override;
	void flushPostedScriptMessages();

	// Resources
	void applySpritePaletteExtension(const byte *header);
	bool destroyResourceObjectByType(uint16 id, uint16 type) override;
	bool validateAndGetResourceByType(uint16 id, uint16 type) override;

	// Backgrounds
	byte *loadBitmapResourceCopy(uint16 id);
	void setBackgroundByMode(const byte *record);

	// Transitions
	void wipeEffectIrisRings(uint16 rings);
	void wipeEffectInterlacedRows(uint16 duration, uint16 count);
	void wipeEffectSlideColumns(int16 direction, uint16 duration, uint16 steps);
	void wipeEffectSlideRows(int16 direction, uint16 duration, uint16 steps);

	// Interpreter
	bool executeScriptOp(byte *ip, uint16 *advance) override;
	uint16 getScriptOpSize(const byte *ip) override;
	bool runPostedScriptRecord(const byte *record);
	bool enqueuePostedScriptRun(uint16 script, uint16 argument1, uint16 argument2);
	void servicePostedRunQueue();
	int32 applyArithmeticOperand(int32 accumulator, const byte *operand);
	uint16 evaluateCondition(byte condition, uint16 operandA, uint16 operandB);

	// Plug-ins
	bool loadPluginLibrary(const Common::String &name);
	void unloadPluginLibrary();
	bool loadPluginFunctionTable();
	void exprCallPluginFunction(uint16 function, uint16 count);
	bool callMessageFilter(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh);

	// Expressions
	void exprCallBuiltinFunction(const byte *token);
	void evaluateBuiltinFunction(uint16 code, uint16 argc) override;

	// Opcodes
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
	void o_getKeyState(const byte *record); // 47
	bool o_setSpritePosition(const byte *record); // 63
	void o_setMouseEventScript(const byte *record); // 64
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

	// Input messages
	void handleKeyUpMessage(Common::KeyCode keycode) override;
	uint16 postKeyHookScript(uint16 vk);

	// Click, hover and drag regions
	bool postRunScript(uint16 script, uint16 argument1, uint16 argument2) override;
	void cancelMouseCapture();
	void idleRearm();
	void idleCheck();
	void callRubberBandScript();
	bool updateSpriteHoverFromPoint(int16 x, int16 y);
	void postSpriteLeaveScript();

	// Timers and collisions
	uint16 removeNativeTimer(uint16 index);
	void findNativeTimer(uint16 id);
	void addNativeTimer(uint16 id, uint32 interval, uint16 proc, byte repeat);
	void serviceNativeTimerQueue();
	void serviceCollisionPairs();

	// Device groups
	void clearDeviceState();
	uint16 insertDeviceBindingSorted(uint16 count, DeviceBinding_v1_2_3 *list, const byte *record);
	void setAllBindingScripts(uint16 script, uint16 count, DeviceBinding_v1_2_3 *list);
	void setAllBindingActive(byte active, uint16 count, DeviceBinding_v1_2_3 *list);
	void fillJoystickStateBytes(byte value, byte stick);
	uint16 deleteDeviceBinding(uint16 index, uint16 count, DeviceBinding_v1_2_3 *list);
	uint16 updateDeviceBindingList(uint16 maxEntries, uint16 count, DeviceBinding_v1_2_3 *list, const byte *record);
	void fireDeviceGroupStateScript(byte group);
	void releaseOrRestoreDeviceGroups(bool release);
	void resetDeviceGroup(byte value, byte group);
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
	void handleJoystickButtonMessage(const Message &message);
	uint16 joystickMessageToIndex(const Message &message);

	// Startup state
	bool _debugMode = false;
	uint16 _inputTimeout = 0;
	uint16 _activateScript = 0;
	bool _activateState = false;
	uint16 _numPostedScriptRuns = 0;
	uint16 _numNativeTimers = 0;

	// Scene state
	uint16 _numFileResources = 0;
	uint16 _driveType = 0;

	// Input state
	NativeTimer_v1_2_3 _nativeTimers[TG_INPUT_V1_2_3_MAX_NATIVE_TIMERS];
	uint32 _timersPausedAt = 0;

	// Device state
	KeyRoute_v1_2_3 _keyRoutes[TG_INPUT_V1_2_3_KEY_TABLE_SIZE];
	DeviceGroup_v1_2_3 _deviceGroups[TG_DEVICE_V1_2_3_NUM_GROUPS];
	byte _deviceOwners[TG_DEVICE_V1_2_3_NUM_DEVICES] = {};
	byte _keyboardDirections[TG_DEVICE_V1_2_3_NUM_KEYBOARD_SETS] = {};
	byte _joystickStates[TG_JOYSTICK_V1_2_1_NUM_SCRIPTS] = {};
	byte _joystickLastIndex[TG_JOYSTICK_V1_2_1_NUM_STICKS] = {};
	Common::Rect _mouseStickDeadRect;
	Common::Rect _mouseStickOuterRect;
	uint16 _mouseStickCode = 0;

	// The last two slots of each set are used only from the later version on
	byte _keyboardSetKeys[TG_DEVICE_V1_2_3_NUM_KEYBOARD_SETS][TG_DEVICE_V1_2_5_KEYBOARD_SET_SLOTS] = {
		{ 0x46, 0x56, 0x43, 0x42, 0x31, 0x32 },
		{ 0x26, 0x28, 0x25, 0x27, 0x39, 0x30 }
	};

	byte _keyboardSetCodes[TG_DEVICE_V1_2_3_NUM_KEYBOARD_SETS][TG_DEVICE_V1_2_5_KEYBOARD_SET_SLOTS] = {
		{ 0x10, 0x20, 0x80, 0x40, 0x01, 0x02 },
		{ 0x10, 0x20, 0x80, 0x40, 0x01, 0x02 }
	};

	// Interpreter state
	uint16 _debugScriptId = 0;
	uint16 _reservedScriptWord = 0;
	uint16 _waveStartedScript = 0;
	uint16 _idleScript = 0;
	uint32 _idleDelay = 0;
	uint32 _idleDeadline = 0;
	uint16 _rubberBandScript = 0;
	bool _wallpaperSelectionEnabled = true;
	byte _postedRuns[TG_SCRIPT_V1_2_3_MAX_POSTED_RUNS * TG_SCRIPT_V1_2_3_POSTED_RUN_SIZE] = {};
	char _quitSceneName[TG_HOST_V1_2_5_SCENE_NAME_SIZE] = {};
	char _gotoSceneName[TG_HOST_V1_2_5_SCENE_NAME_SIZE] = {};
	// The memory check is not emulated; the scripts compare this value with 53 as a signed number (53 * 65536 is about 3.4 MB), so it stands for plenty of memory
	uint16 _freeMemoryHigh = 0x7FFF;

	// Plug-in state
	PluginHost *_pluginHost = nullptr;
	Plugin *_plugin = nullptr;
	Common::Array<int> _pluginFunctions;
	Plugin *_messageFilter = nullptr;
	Plugin *_windowMessageCallback = nullptr;

	// Palette and background state
	byte _palette[TG_HOST_V1_2_3_PALETTE_SIZE] = {};
};

} // End of namespace Topgun

#endif // TOPGUN_TOPGUN_V1_2_3_H
