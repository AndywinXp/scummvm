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

#ifndef TOPGUN_TOPGUN_V1_2_4_H
#define TOPGUN_TOPGUN_V1_2_4_H

#include "topgun/libraries/audio/audio_v1_2_4.h"
#include "topgun/libraries/resource/resource_v1_2_4.h"
#include "topgun/libraries/sprite/sprite_v1_2_4.h"
#include "topgun/plugin.h"
#include "topgun/topgun_v1_2.h"

// Variables and operands
#define TG_HOST_V1_2_4_NUM_VARIABLES               0x13FE
#define TG_HOST_V1_2_4_TABLE_SIZE                  0x159F
#define TG_HOST_V1_2_4_LAST_OPERAND_INDEX          0x159E
#define TG_HOST_V1_2_4_FIRST_ARGUMENT_SLOT         0x13FE
#define TG_HOST_V1_2_4_LITERAL_BIAS                0x7531
#define TG_HOST_V1_2_4_FIRST_FRAME_INDEX           0x159D
#define TG_HOST_V1_2_4_FIRST_SYSTEM_VARIABLE       5001
#define TG_HOST_V1_2_4_NUM_SYSTEM_VARIABLES        117
#define TG_HOST_V1_2_4_VARIABLE_MOUSE_BUTTON       5001
#define TG_HOST_V1_2_4_VARIABLE_LOMEM              5002
#define TG_HOST_V1_2_4_VARIABLE_SCREEN_WIDTH       5003
#define TG_HOST_V1_2_4_VARIABLE_SCREEN_HEIGHT      5004
#define TG_HOST_V1_2_4_VARIABLE_MOUSE_X            5005
#define TG_HOST_V1_2_4_VARIABLE_MOUSE_Y            5006
#define TG_HOST_V1_2_4_VARIABLE_CLICK_X            5007
#define TG_HOST_V1_2_4_VARIABLE_CLICK_Y            5008
#define TG_HOST_V1_2_4_VARIABLE_DROP_X             5009
#define TG_HOST_V1_2_4_VARIABLE_DROP_Y             5010
#define TG_HOST_V1_2_4_VARIABLE_LAUNCH_MODE        5011
#define TG_HOST_V1_2_4_VARIABLE_AUDIO_PRESENT      5012
#define TG_HOST_V1_2_4_VARIABLE_PROCESSOR_TYPE     5013
#define TG_HOST_V1_2_4_VARIABLE_OVER_256_COLORS    5014
#define TG_HOST_V1_2_4_VARIABLE_SCREEN_SAVER       5015
#define TG_HOST_V1_2_4_VARIABLE_JOYSTICKS          5016
#define TG_HOST_V1_2_4_VARIABLE_EVENT_PARAMETER    5017

// Scenes

// Palette
#define TG_HOST_V1_2_4_PALETTE_SIZE                1024

// Messages
#define TG_HOST_V1_2_4_MESSAGE_FIRST_FLUSHED       0x4C8
#define TG_HOST_V1_2_4_MESSAGE_LAST_FLUSHED        0x4CB
#define TG_HOST_V1_2_4_MESSAGE_FIRST_PLUGIN        0x658
#define TG_HOST_V1_2_4_MESSAGE_LAST_PLUGIN         0x6BB
#define TG_HOST_V1_2_9_MESSAGE_FIRST_PLUGIN        0x6BC
#define TG_HOST_V1_2_9_MESSAGE_LAST_PLUGIN         0x71F

// Plug-ins
#define TG_PLUGIN_V1_2_4_STRING_COPY_SIZE          0x7F
#define TG_PLUGIN_V1_2_4_FRAME_IDS_SIZE            4
#define TG_PLUGIN_V1_2_12_KEY_LIBRARY_NAMES         10
#define TG_PLUGIN_V1_2_12_KEY_FUNCTION_NAMES        11
#define TG_PLUGIN_V1_2_12_KEY_FUNCTION_LIBRARIES    12
#define TG_PLUGIN_V1_2_12_RUN_MESSAGE_FRAME_OFFSET  0x20
#define TG_PLUGIN_V1_2_12_RUN_MESSAGE_MAX_ARGUMENTS 8
#define TG_PLUGIN_V1_2_12_RUN_MESSAGE_HEADER_SIZE   6

// Resource objects
#define TG_HOST_V1_2_4_SPRITE_DATA_ORIGINAL_COUNT  2
#define TG_HOST_V1_2_4_SPRITE_DATA_MEMBER_COUNT    4
#define TG_HOST_V1_2_4_SPRITE_DATA_PALETTE_COUNT   0x0A
#define TG_HOST_V1_2_4_SPRITE_DATA_LEVEL           0x0C
#define TG_HOST_V1_2_4_SPRITE_DATA_WHOLE_BOX       0x0F
#define TG_HOST_V1_2_4_SPRITE_DATA_DEFER_FLAG      0x10
#define TG_HOST_V1_2_4_SPRITE_DATA_PALETTE_FLAG    0x12
#define TG_HOST_V1_2_4_SPRITE_DATA_PICK_INVISIBLE  0x13
#define TG_HOST_V1_2_4_SPRITE_DATA_MEMBERS         0x14
#define TG_HOST_V1_2_4_SPRITE_DATA_PALETTE_ENTRIES 0x1C
#define TG_HOST_V1_2_7_1_SPRITE_DATA_SHIFT         2    // every field after the click script moves up by this much
#define TG_HOST_V1_2_7_1_SPRITE_DATA_PALETTE_ENTRIES_SHIFT 0x0E
#define TG_HOST_V1_2_12_SPRITE_DATA_SCROLLABLE     0x16
#define TG_HOST_V1_2_12_SPRITE_DATA_MEMBERS        0x18
#define TG_HOST_V1_2_12_SPRITE_DATA_PALETTE_ENTRIES_SHIFT 0x10
#define TG_HOST_V1_2_4_PALETTE_EXTENSION_END       0xF6
#define TG_HOST_V1_2_4_BITMAP_HEADER_SIZE          0x14
#define TG_HOST_V1_2_4_BITMAP_HOT_X                0x0A
#define TG_HOST_V1_2_4_BITMAP_HOT_Y                0x0C
#define TG_HOST_V1_2_4_BITMAP_RESOURCE_ID          0x0E
#define TG_HOST_V1_2_4_GROUP_X                     2
#define TG_HOST_V1_2_4_GROUP_Y                     4
#define TG_HOST_V1_2_12_BITMAP_FLAGS_KEPT_MASK     0xCF
#define TG_HOST_V1_2_12_CELL_FLAGS                 2
#define TG_HOST_V1_2_12_CELL_FLAGS_MASK            0x30
#define TG_HOST_V1_2_12_CELL_X                     4
#define TG_HOST_V1_2_12_CELL_Y                     6
#define TG_HOST_V1_2_12_TABLE_RESOURCE_ID          8
#define TG_HOST_V1_2_12_TILE_HEADER_SIZE           12
#define TG_HOST_V1_2_12_TILE_WIDTH                 0
#define TG_HOST_V1_2_12_TILE_HEIGHT                2
#define TG_HOST_V1_2_12_TILE_COLUMNS               4
#define TG_HOST_V1_2_12_TILE_ROWS                  6
#define TG_HOST_V1_2_12_TILE_LAYOUT                8
#define TG_HOST_V1_2_4_MAX_FRAME_BATCH             50

// Strings
#define TG_STRING_V1_2_4_FORMAT_ARGUMENT_SIZE      4

// Movies
#define TG_MOVIE_V1_2_4_FRAME_TABLE_RESOURCE_OFFSET  2
#define TG_MOVIE_V1_2_4_STREAM_RESOURCE_OFFSET       1
#define TG_MOVIE_V1_2_3_2_TIMELINE_AUDIO_SIZE        0x1C // 2 + the 0x1A-byte wave header
#define TG_MOVIE_V1_2_4_TIMELINE_AUDIO_SIZE          0x1E
#define TG_MOVIE_V1_2_4_TIMELINE_AUDIO_STATE         2
#define TG_MOVIE_V1_2_4_TIMELINE_AUDIO_STATE_SIZE    4

// Interpreter built-ins
#define TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE        10000
#define TG_BUILTIN_V1_2_4_MAX_STRING_INDEX         0x7F

// Interpreter
#define TG_SCRIPT_V1_2_4_MAX_DEPTH                30
#define TG_SCRIPT_V1_2_4_TABLE_ADDRESS             0x1520
#define TG_SCRIPT_V1_2_3_2_MAX_POSTED_RUNS         20
#define TG_SCRIPT_V1_2_4_MAX_POSTED_RUNS           40
#define TG_SCRIPT_V1_2_4_POSTED_RUN_SIZE           18
#define TG_SCRIPT_V2_0_POSTED_RUN_SIZE             20
#define TG_SCRIPT_V1_2_3_2_LAST_OPCODE             106
#define TG_SCRIPT_V1_2_3_3_LAST_OPCODE             120
#define TG_SCRIPT_V1_2_4_LAST_OPCODE               119
#define TG_SCRIPT_V1_2_3_2_STOP_SPRITE_SIZE        4
#define TG_SCRIPT_V1_2_3_2_ARITHMETIC_SIZE         18
#define TG_SCRIPT_V1_2_3_2_COLLISION_PAIR_SIZE     16
#define TG_SCRIPT_V1_2_3_2_DEVICE_STATE_SIZE       8
#define TG_SCRIPT_V1_2_3_3_QUEUE_DONE_SIZE         4
#define TG_SCRIPT_V1_2_6_LAST_OPCODE               143
#define TG_SCRIPT_V1_2_4_GOTO_RECORD_SIZE          18
#define TG_SCRIPT_V1_2_6_GOTO_RECORD_SIZE          20
#define TG_SCRIPT_V1_2_4_GOTO_NAME_OFFSET          2
#define TG_SCRIPT_V1_2_4_GOTO_RELEASE_OFFSET       0x10
#define TG_SCRIPT_V1_2_6_GOTO_RELEASE_OFFSET       0x12
#define TG_SCRIPT_V1_2_4_GOTO_BACK_OFFSET          0x11
#define TG_SCRIPT_V1_2_6_GOTO_BACK_OFFSET          0x13
#define TG_SCRIPT_V1_2_6_QUIT_SCENE_RECORD_SIZE    18
#define TG_SCRIPT_V1_2_6_RESERVED_RECORD_SIZE      6
#define TG_SCRIPT_V1_2_4_SWITCH_ENTRY_SIZE         6
#define TG_SCRIPT_V1_2_3_2_LAST_BUILTIN_CODE       0x70
#define TG_SCRIPT_V1_2_3_3_LAST_BUILTIN_CODE       0x78
#define TG_SCRIPT_V1_2_4_LAST_BUILTIN_CODE         0x8D
#define TG_SCRIPT_V1_2_6_LAST_BUILTIN_CODE         0x8F
#define TG_SCRIPT_V1_2_7_LAST_BUILTIN_CODE         0x90
#define TG_SCRIPT_V1_2_7_1_LAST_BUILTIN_CODE       0x91
#define TG_SCRIPT_V1_2_7_2_LAST_BUILTIN_CODE       0x96
#define TG_SCRIPT_V1_2_7_3_LAST_BUILTIN_CODE       0x98
#define TG_SCRIPT_V1_2_9_LAST_BUILTIN_CODE         0x9A
#define TG_SCRIPT_V1_2_10_LAST_BUILTIN_CODE        0x9C
#define TG_SCRIPT_V1_2_11_LAST_BUILTIN_CODE        0xB2
#define TG_SCRIPT_V1_2_12_LAST_BUILTIN_CODE        0xEC
#define TG_BUILTIN_V1_2_12_MAX_TABLE_DIMENSIONS    5    // (0x16 - 0x0C) / 2 stride words
#define TG_BUILTIN_V1_2_12_TABLE_HEADER_SIZE       0x20 // the elements follow the header
#define TG_BUILTIN_V1_2_12_TABLE_COUNT             4
#define TG_BUILTIN_V1_2_12_TABLE_DIMENSIONS        0x0A
#define TG_BUILTIN_V1_2_12_TABLE_STRIDES           0x0C
#define TG_BUILTIN_V1_2_12_TABLE_SIZES             0x16
#define TG_BUILTIN_V1_2_12_MAX_WAVE_VOLUME         0x10
#define TG_BUILTIN_V1_2_12_MAX_MIDI_CHANNEL        16
#define TG_BUILTIN_V1_2_12_MAX_COLOR_DISTANCE      1000
#define TG_BUILTIN_V1_2_12_NO_NEAREST_COLOR        0xFF
#define TG_SCRIPT_V1_2_12_TABLE_ADDRESS            0x131A
#define TG_SCRIPT_V1_2_7_1_SET_CLICK_SCRIPT_SIZE   20
#define TG_SCRIPT_V1_2_7_SET_BACKGROUND_SIZE       16
#define TG_SCRIPT_V1_2_7_1_SET_HOVER_REGION_SIZE   20
#define TG_SCRIPT_V1_2_9_SET_HOVER_REGION_SIZE     22
#define TG_SCRIPT_V1_2_7_1_GET_ELEMENT_IDS_SIZE    24
#define TG_SCRIPT_V1_2_7_WINDOWS_VERSION           95
#define TG_SCRIPT_V1_2_7_2_IDLE_SECONDS_TO_MS      1000

// Interpreter operations: arithmetic and conditions
#define TG_SCRIPT_V1_2_4_ARITHMETIC_OPERANDS_OFFSET 6
#define TG_SCRIPT_V1_2_4_ARITHMETIC_OPERAND_SIZE   6
#define TG_SCRIPT_V1_2_4_ARITHMETIC_OPERATION      2
#define TG_SCRIPT_V1_2_4_ARITHMETIC_NEGATE         4
#define TG_SCRIPT_V1_2_4_CONDITION_EQUAL           0x01
#define TG_SCRIPT_V1_2_4_CONDITION_LESS            0x02
#define TG_SCRIPT_V1_2_4_CONDITION_GREATER         0x04
#define TG_SCRIPT_V1_2_4_CONDITION_OR              0x08
#define TG_SCRIPT_V1_2_4_CONDITION_AND             0x10
#define TG_SCRIPT_V1_2_4_CONDITION_MODULO          0x20
#define TG_SCRIPT_V1_2_4_CONDITION_NEGATE          0x80
#define TG_SCRIPT_V1_2_4_CONDITIONAL_SIZE          0x0E
#define TG_SCRIPT_V1_2_4_SKIP_TAKEN_SIZE           0x0A
#define TG_SCRIPT_V1_2_4_NUM_SCRIPT_CHOICES        6

// Interpreter operations: audio and palette
#define TG_SCRIPT_V1_2_4_FIRST_PALETTE_ENTRY       10
#define TG_SCRIPT_V1_2_4_MAX_PALETTE_ENTRIES       0xEC
#define TG_SCRIPT_V1_2_4_PALETTE_RESOURCE_SIZE     0x3B0 // 0xEC entries * 4 bytes
#define TG_SCRIPT_V1_2_4_PALETTE_ENTRIES_OFFSET    8

// Transitions
#define TG_HOST_V1_2_7_WIPE_PARAMETER_COUNT        2
#define TG_HOST_V1_2_7_WIPE_PARAMETER_DURATION     4
#define TG_HOST_V1_2_7_WIPE_ROWS_DEFAULT_COUNT     10
#define TG_HOST_V1_2_7_WIPE_SLIDE_DEFAULT_STEPS    0x3C

// Text objects
#define TG_TEXT_V1_2_4_HEADER_COLOR                0
#define TG_TEXT_V1_2_4_HEADER_FACE                 2
#define TG_TEXT_V1_2_4_HEADER_TEXT                 4
#define TG_TEXT_V1_2_4_HEADER_HEIGHT               6
#define TG_TEXT_V1_2_4_HEADER_HOT_X                8
#define TG_TEXT_V1_2_4_HEADER_HOT_Y                0x0A
#define TG_TEXT_V1_2_4_HEADER_BOX_WIDTH            0x0C
#define TG_TEXT_V1_2_4_HEADER_BOX_HEIGHT           0x0E
#define TG_TEXT_V1_2_4_HEADER_FLAGS                0x10
#define TG_TEXT_V1_2_4_EDIT_NUMERIC                11
#define TG_TEXT_V1_2_4_MAX_FONTS                   10
#define TG_TEXT_V1_2_4_FLAG_CENTER                 0x0001
#define TG_TEXT_V1_2_4_FLAG_RIGHT                  0x0002
#define TG_TEXT_V1_2_4_FLAG_VERTICAL_CENTER        0x0004
#define TG_TEXT_V1_2_4_FLAG_BOTTOM                 0x0008
#define TG_TEXT_V1_2_4_FLAG_WORD_BREAK             0x0010
#define TG_TEXT_V1_2_4_FLAG_SINGLE_LINE            0x0020
#define TG_TEXT_V1_2_4_FLAG_NO_PREFIX              0x0800
#define TG_TEXT_V1_2_4_UNLIMITED_WIDTH             0x7FFF

// INI file

// Input and devices
#define TG_INPUT_V1_2_4_MAX_COLLISION_PAIRS        4640 // arrays grow by 10 elements: largest n with n * 14 <= 0xFDE8
#define TG_INPUT_V1_2_4_OBJECT_CLICK_SCRIPT        0
#define TG_INPUT_V1_2_4_OBJECT_DRAG_SCRIPT         6
#define TG_INPUT_V1_2_4_OBJECT_DROP_SCRIPT         8
#define TG_INPUT_V1_2_4_OBJECT_DISABLED            0x0E
#define TG_INPUT_V1_2_4_OBJECT_DRAGGABLE           0x11
#define TG_INPUT_V1_2_7_1_OBJECT_CLICK_ARGUMENT    2
#define TG_INPUT_V1_2_7_1_OBJECT_DRAG_SCRIPT       8
#define TG_INPUT_V1_2_7_1_OBJECT_DROP_SCRIPT       0x0A
#define TG_INPUT_V1_2_7_1_OBJECT_DISABLED          0x10
#define TG_INPUT_V1_2_7_1_OBJECT_DRAGGABLE         0x13

// Mouse used as a stick
#define TG_DEVICE_V1_2_4_STICK_DEAD_INFLATE        0x14
#define TG_DEVICE_V1_2_4_STICK_OUTER_INFLATE       0x28
#define TG_DEVICE_V1_2_4_STICK_RAY_MARGIN          2
#define TG_DEVICE_V1_2_4_STICK_ANGLE_ROTATION      450  // 45.0 degrees in tenths
#define TG_DEVICE_V1_2_4_STICK_FULL_TURN           3600 // 360.0 degrees in tenths

// Joystick
#define TG_JOYSTICK_V1_2_4_EIGHT_WAY_THRESHOLD     0x3E80

namespace Topgun {

enum MessageId_v1_2_4 {
	kMsgV1_2_4KeyUp = 0x101
};

enum MessageId_v1_2_12 {
	kMsgV1_2_12FreePluginModules = 0x5F4
};

// The later numbering of the types from the wave on; the bitmap and the midi keep the numbers above
enum ResourceType_v1_2_6 {
	kResV1_2_6Wave = 9,
	kResV1_2_6Movie = 10,
	kResV1_2_6OpList = 11,
	kResV1_2_6BitmapGroup = 12,
	kResV1_2_6MixedGroup = 13,
	kResV1_2_6Palette = 14,
	kResV1_2_6SpriteQueueData = 15,
	kResV1_2_6Script = 16,
	kResV1_2_6Sprite = 17,
	kResV1_2_6Text = 18
};

// The numbering of the types from the movie on moves up by one to make room for the table; the tile is new
enum ResourceType_v1_2_12 {
	kResV1_2_12Bitmap = 1,
	kResV1_2_12Midi = 4,
	kResV1_2_12Wave = 9,
	kResV1_2_12Table = 10,
	kResV1_2_12Movie = 11,
	kResV1_2_12OpList = 12,
	kResV1_2_12Cell = 13,
	kResV1_2_12Group = 14,
	kResV1_2_12Palette = 15,
	kResV1_2_12SpriteQueueData = 16,
	kResV1_2_12Script = 17,
	kResV1_2_12Sprite = 18,
	kResV1_2_12Text = 19,
	kResV1_2_12Tile = 20
};

enum ScriptOpcode_v1_2_4 {
	kOpV1_2_4CallScript = 1,
	kOpV1_2_4LoadResource = 2,
	kOpV1_2_4UnloadResource = 3,
	kOpV1_2_4SetClickScript = 4,
	kOpV1_2_4SetSpriteQueue = 5,
	kOpV1_2_4SetBackground = 6,
	kOpV1_2_4SetSpriteObjectProperties = 7,
	kOpV1_2_4PlayWave = 8,
	kOpV1_2_4PlayCDTrack = 9,
	kOpV1_2_4FadeIn = 10,
	kOpV1_2_4FadeOut = 11,
	kOpV1_2_4BindKeyScript = 12,
	kOpV1_2_4Quit = 13,
	kOpV1_2_4StopCD = 14,
	kOpV1_2_4StopWave = 15,
	kOpV1_2_4Arithmetic = 16,
	kOpV1_2_4ConditionalRunScript = 17,
	kOpV1_2_4GotoScene = 18,
	kOpV1_2_4SetScriptTimer = 19,
	kOpV1_2_4CancelScriptTimer = 20,
	kOpV1_2_4SetSceneEndScript = 21,
	kOpV1_2_4RandomRunScript = 22,
	kOpV1_2_4RoundRobinRunScript = 23,
	kOpV1_2_4TransferSprite = 24,
	kOpV1_2_4StopSprite = 25,
	kOpV1_2_4SetSpriteQueueIfLoaded = 26,
	kOpV1_2_4DebugString = 27,
	kOpV1_2_4SetSpriteLevel = 28,
	kOpV1_2_4SetActivationScript = 29,
	kOpV1_2_4PostScript = 30,
	kOpV1_2_4CursorMode = 31,
	kOpV1_2_4RunScriptIfLoaded = 32,
	kOpV1_2_4AddCollisionPair = 33,
	kOpV1_2_4ListRunScript = 34,
	kOpV1_2_4SetPauseScript = 35,
	kOpV1_2_4MoveSprite = 36,
	kOpV1_2_4Jump = 37,
	kOpV1_2_4ConditionalSkip = 38,
	kOpV1_2_4SetDisplayZoom = 39,
	kOpV1_2_4GetSpritePosition = 40,
	kOpV1_2_4FormatTextObject = 41,
	kOpV1_2_4SetSpriteClipBox = 42,
	kOpV1_2_4Switch = 43,
	kOpV1_2_4Stop = 44,
	kOpV1_2_4Nop = 45,
	kOpV1_2_4Random = 46,
	kOpV1_2_4GetKeyState = 47,
	kOpV1_2_4GetDate = 48,
	kOpV1_2_4GetAngle = 49,
	kOpV1_2_4ClipLineToRect = 50,
	kOpV1_2_4SetWallpaper = 52,
	kOpV1_2_4SetMute = 53,
	kOpV1_2_4BeginTextEdit = 54,
	kOpV1_2_4GetSpriteRect = 55,
	kOpV1_2_4FormatString = 56,
	kOpV1_2_4CompareStrings = 57,
	kOpV1_2_4CompareStringsIgnoreCase = 58,
	kOpV1_2_4IniRead = 59,
	kOpV1_2_4IniWrite = 60,
	kOpV1_2_4SetHoverRegion = 61,
	kOpV1_2_4GetIntersect = 62,
	kOpV1_2_4SetSpritePosition = 63,
	kOpV1_2_4SetMouseEventScript = 64,
	kOpV1_2_4Cosine = 65,
	kOpV1_2_4Sine = 66,
	kOpV1_2_4FlushIni = 67,
	kOpV1_2_4IsSpriteBusy = 68,
	kOpV1_2_4BackupIni = 69,
	kOpV1_2_4GetTime = 70,
	kOpV1_2_4SpriteCompositeToBackground = 71,
	kOpV1_2_4SetJoystickScript = 72,
	kOpV1_2_4SetCursorPosition = 73,
	kOpV1_2_4GetSpriteElementIds = 75,
	kOpV1_2_4Expression = 76,
	kOpV1_2_4ConditionalJumpExpression = 77,
	kOpV1_2_4SwitchExpression = 78,
	kOpV1_2_4ConditionalJumpExpressionCopy = 79,
	kOpV1_2_4PlayMidi = 80,
	kOpV1_2_4StopMidi = 81,
	kOpV1_2_4SetDeviceGroupBinding = 82,
	kOpV1_2_4DeviceGroupButtonState = 83,
	kOpV1_2_4SetDeviceGroupScript = 84,
	kOpV1_2_4DeviceGroupLatch = 85,
	kOpV1_2_4SetDeviceGroupBindingB = 86,
	kOpV1_2_4SetDeviceGroupFlag = 87,
	kOpV1_2_4GetDeviceGroupState = 88,
	kOpV1_2_4ReturnExpression = 89,
	kOpV1_2_4SetMidiParams = 92,
	kOpV1_2_4SetVolume = 93,
	kOpV1_2_4SetTextParam = 94,
	kOpV1_2_4LoadPaletteResource = 96,
	kOpV1_2_4SetPaletteEntries = 97,
	kOpV1_2_4SetDebugScriptId = 106,
	kOpV1_2_4SetQuitSceneName = 119
};

enum ScriptOpcode_v1_2_3_3 {
	kOpV1_2_3_3SetQueueDoneScript = 120
};

enum ScriptOpcode_v1_2_6 {
	kOpV1_2_6SetReservedWord = 141,
	kOpV1_2_6Reserved = 143
};

struct ScriptCursor_v1_2_12 {
	uint16 id;
	uint16 sequentialIndex;
	uint16 randomIndex;

	ScriptCursor_v1_2_12() : id(0), sequentialIndex(0), randomIndex(0) {}
};

class TopgunEngine_v1_2_4 : public TopgunEngine_v1_2 {
	friend class AudioLibrary_v1_2_4;
	friend class EnginePluginHost;
	friend class ResourceLibrary_v1_2;
	friend class ResourceLibrary_v1_2_4;
	friend class SpriteLibrary_v1_2_4;
	friend class TopgunEngine_v1_2;

public:
	TopgunEngine_v1_2_4(OSystem *syst, const ADGameDescription *gameDesc, EngineVersion version);
	~TopgunEngine_v1_2_4() override;

protected:
	// Libraries
	AudioLibrary_v1_2_4 *audioLib() { return static_cast<AudioLibrary_v1_2_4 *>(audioLibrary()); }
	ResourceLibrary_v1_2_4 *resourceLib() { return static_cast<ResourceLibrary_v1_2_4 *>(resourceLibrary()); }
	SpriteLibrary_v1_2_4 *spriteLib() { return static_cast<SpriteLibrary_v1_2_4 *>(spriteLibrary()); }

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
	bool ensureTableResourceLoaded(uint16 id);
	void destroyTableResourceObject(uint16 id);
	bool ensureTileResourceLoaded(uint16 id);
	void destroyTileResourceObject(uint16 id);
	bool loadBitmapListToPointers(uint16 count, const byte *list, byte **cells);
	void releaseTileResourceCells(uint16 count, const byte *list);

	// Backgrounds
	byte *loadBitmapResourceCopy(uint16 id);
	void setBackgroundByMode(const byte *record);
	void setTileScreenRegion(bool repaint, TileBackground *tileMap);
	void setScrollBox(const Common::Rect &box);

	// Transitions
	void wipeEffectIrisRings(uint16 rings);
	void wipeEffectInterlacedRows(uint16 duration, uint16 count);
	void wipeEffectSlideColumns(int16 direction, uint16 duration, uint16 steps);
	void wipeEffectSlideRows(int16 direction, uint16 duration, uint16 steps);

	// Interpreter
	bool executeScriptOp(byte *ip, uint16 *advance) override;
	uint16 getScriptOpSize(const byte *ip) override;
	bool enqueuePostedScriptRun(uint16 script, uint16 argument1, uint16 argument2);
	void servicePostedRunQueue();
	int32 applyArithmeticOperand(int32 accumulator, const byte *operand);
	uint16 evaluateCondition(byte condition, uint16 operandA, uint16 operandB);

	// Plug-ins
	bool loadAndRegisterLibraries();
	PluginModule_v1_2_12 *loadPluginModule(const Common::String &fileName);
	void freePluginModule(PluginModule_v1_2_12 *module);
	bool registerExternalLibrary(PluginModule_v1_2_12 *module);
	void unloadAllPluginModules();
	void freeUnloadedPluginModules();
	void notifyPluginsFocusChanged(bool active);
	void notifyServicePause(bool paused);
	void notifyPluginsLifecycleEvent(uint16 mode, const char *name);
	void runRegisteredServiceCallbacks();
	uint16 runScriptHookWithArg(uint16 script, uint16 argument);

	// Expressions
	void evaluateBuiltinFunction(uint16 code, uint16 argc) override;

	// Opcodes
	void o_setBackground(const byte *record); // 6
	bool o_playWave(uint16 waveId, const byte *params); // 8
	void o_arithmetic(const byte *record); // 16
	bool o_conditionalRunScript(const byte *record, uint16 &size); // 17
	void o_gotoScene(const byte *record); // 18
	bool o_randomRunScript(byte *record) override; // 22
	bool o_roundRobinRunScript(byte *record) override; // 23
	void o_setCursorMode(const byte *record); // 31
	void o_listRunScript(const byte *record); // 34
	uint16 o_conditionalSkip(const byte *record); // 38
	void o_getSpritePosition(const byte *record); // 40
	void o_getKeyState(const byte *record); // 47
	void o_setMouseEventScript(const byte *record); // 64
	void o_setTextParam(const byte *record); // 94
	void o_loadPaletteResource(const byte *record); // 96
	void o_setPaletteEntries(const byte *record); // 97
	void o_setQueueDoneScript(const byte *record); // 120

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
	uint16 builtinBlitBitmapOverlap(uint16 first, uint16 second, int16 deltaX, int16 deltaY); // 125, 206
	uint16 builtinGetMouseEventScript(); // 129
	uint16 builtinSetJoystickButtonOffsetMode(uint16 value); // 130
	uint16 builtinReattachDeviceGroup(uint16 group, uint16 flag); // 137

	// Input messages
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
	void serviceCollisionPairs();

	// Device groups
	void fireDeviceGroupStateScript(byte group);
	void releaseOrRestoreDeviceGroups(bool release);
	void resetDeviceGroup(byte value, byte group);
	bool mouseAsStickMessage(Common::Point point, uint16 message);
	void resetMouseStick(byte value);

	// Startup state
	bool _debugMode = false;
	uint16 _activateScript = 0;
	bool _activateState = false;
	uint16 _numPostedScriptRuns = 0;

	// Scene state
	uint16 _numFileResources = 0;
	uint16 _driveType = 0;

	// Input state
	uint32 _timersPausedAt = 0;

	// Interpreter state
	uint16 _reservedScriptWord = 0;
	Common::Array<ScriptCursor_v1_2_12> _scriptCursors;
	Common::HashMap<uint16, TileBackground *> _tileBackgrounds;
	uint16 _waveStartedScript = 0;
	uint16 _idleScript = 0;
	uint32 _idleDelay = 0;
	uint32 _idleDeadline = 0;
	uint16 _rubberBandScript = 0;
	bool _wallpaperSelectionEnabled = true;
	byte _postedRuns[TG_SCRIPT_V1_2_4_MAX_POSTED_RUNS * TG_SCRIPT_V2_0_POSTED_RUN_SIZE] = {};
	char _quitSceneName[TG_HOST_V1_2_6_SCENE_NAME_SIZE] = {};
	char _gotoSceneName[TG_HOST_V1_2_6_SCENE_NAME_SIZE] = {};
	// The memory check is not emulated; the scripts compare this value with 53 as a signed number (53 * 65536 is about 3.4 MB), so it stands for plenty of memory
	uint16 _freeMemoryHigh = 0x7FFF;

	// Plug-in state
	Common::Array<PluginModule_v1_2_12 *> _pluginModules;
	Common::Array<PluginModule_v1_2_12 *> _pluginsPendingFree;
	uint16 _pluginTickProcCount = 0;

	// Palette and background state
	byte _palette[TG_HOST_V1_2_4_PALETTE_SIZE] = {};
};

} // End of namespace Topgun

#endif // TOPGUN_TOPGUN_V1_2_4_H
