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

#ifndef TOPGUN_TOPGUN_V1_2_H
#define TOPGUN_TOPGUN_V1_2_H

#include "common/array.h"
#include "common/formats/ini-file.h"
#include "common/hashmap.h"
#include "common/hash-str.h"
#include "common/path.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "topgun/libraries/audio/audio_v1_2.h"
#include "topgun/libraries/resource/resource_v1_2.h"
#include "topgun/libraries/sprite/sprite_v1_2.h"
#include "topgun/topgun_v1_1.h"

// Variables and operands
#define TG_HOST_V1_2_NUM_VARIABLES               1535
#define TG_HOST_V1_2_VARIABLE_LAST_INDEX         0x5FE
#define TG_HOST_V1_2_LITERAL_BIAS                0x7D01
#define TG_HOST_V1_2_ENTRY_SCRIPT                0x82FF
#define TG_HOST_V1_2_NUM_SCENE_VARIABLES         401
#define TG_HOST_V1_2_4_NUM_SCENE_VARIABLES       5001
#define TG_HOST_V1_2_4_SCENE_NAME_SIZE           14
#define TG_HOST_V1_2_6_SCENE_NAME_SIZE           16
#define TG_HOST_V1_2_FIRST_CLEARED_VARIABLE      401
#define TG_HOST_V1_2_NUM_CLEARED_VARIABLES       114
#define TG_HOST_V1_2_VARIABLE_MOUSE_BUTTON       401
#define TG_HOST_V1_2_VARIABLE_LOMEM              402
#define TG_HOST_V1_2_VARIABLE_SCREEN_WIDTH       403
#define TG_HOST_V1_2_VARIABLE_SCREEN_HEIGHT      404
#define TG_HOST_V1_2_VARIABLE_MOUSE_X            405
#define TG_HOST_V1_2_VARIABLE_MOUSE_Y            406
#define TG_HOST_V1_2_VARIABLE_LAUNCH_MODE        411
#define TG_HOST_V1_2_VARIABLE_AUDIO_PRESENT      412
#define TG_HOST_V1_2_VARIABLE_PROCESSOR_TYPE     413
#define TG_HOST_V1_2_VARIABLE_OVER_256_COLORS    414
#define TG_HOST_V1_2_1_NUM_CLEARED_VARIABLES     116
#define TG_HOST_V1_2_1_VARIABLE_SCREEN_SAVER     415
#define TG_HOST_V1_2_1_VARIABLE_JOYSTICKS        416
#define TG_HOST_V1_2_2_NUM_CLEARED_VARIABLES     117
#define TG_HOST_V1_2_2_VARIABLE_EVENT_PARAMETER  417
#define TG_HOST_V1_2_TABLE_SIZE                  0x159F
#define TG_HOST_V1_2_3_NUM_SCENE_VARIABLES       1001
#define TG_HOST_V1_2_3_FIRST_ARGUMENT_SLOT       0x45E
#define TG_HOST_V1_2_3_FIRST_FRAME_INDEX         0x5FD
#define TG_HOST_V1_2_3_FIRST_SYSTEM_VARIABLE     1001
#define TG_HOST_V1_2_3_NUM_SYSTEM_VARIABLES      117
#define TG_HOST_V1_2_3_VARIABLE_MOUSE_BUTTON     1001
#define TG_HOST_V1_2_3_VARIABLE_LOMEM            1002
#define TG_HOST_V1_2_3_VARIABLE_SCREEN_WIDTH     1003
#define TG_HOST_V1_2_3_VARIABLE_SCREEN_HEIGHT    1004
#define TG_HOST_V1_2_3_VARIABLE_MOUSE_X          1005
#define TG_HOST_V1_2_3_VARIABLE_MOUSE_Y          1006
#define TG_HOST_V1_2_3_VARIABLE_CLICK_X          1007
#define TG_HOST_V1_2_3_VARIABLE_CLICK_Y          1008
#define TG_HOST_V1_2_3_VARIABLE_DROP_X           1009
#define TG_HOST_V1_2_3_VARIABLE_DROP_Y           1010
#define TG_HOST_V1_2_3_VARIABLE_LAUNCH_MODE      1011
#define TG_HOST_V1_2_3_VARIABLE_AUDIO_PRESENT    1012
#define TG_HOST_V1_2_3_VARIABLE_PROCESSOR_TYPE   1013
#define TG_HOST_V1_2_3_VARIABLE_OVER_256_COLORS  1014
#define TG_HOST_V1_2_3_VARIABLE_SCREEN_SAVER     1015
#define TG_HOST_V1_2_3_VARIABLE_JOYSTICKS        1016
#define TG_HOST_V1_2_3_VARIABLE_EVENT_PARAMETER  1017

// Scenes and strings
#define TG_HOST_V1_2_STRING_SLOT_SIZE            128
#define TG_HOST_V1_2_STRING_ID_MASK              0x7FFF
#define TG_HOST_V1_2_STRING_ID_CONSTANT          0x8000
#define TG_HOST_V1_2_CONTAINER_TITLE_SIZE        0x4E
#define TG_HOST_V1_2_6_CONTAINER_TITLE_SIZE      0x52
#define TG_HOST_V1_2_12_CONTAINER_TITLE_SIZE     0x50
#define TG_HOST_V1_2_3_1_LIBRARY_NAME_SIZE       14
#define TG_HOST_V1_2_4_LIBRARY_NAME_SIZE         15
#define TG_HOST_V1_2_6_LIBRARY_NAME_SIZE         17

// Background record of the demo: the colour and the wipe follow the two bitmap ids
#define TG_HOST_V1_2_3_1_BACKGROUND_COLOR_OFFSET 8
#define TG_HOST_V1_2_3_1_BACKGROUND_WIPE_OFFSET  0x0C

// Script hook of a plug-in: it reads up to the sixth argument
#define TG_HOST_V1_2_3_1_HOOK_ARGUMENTS          6

// Device groups
#define TG_INPUT_V1_2_3_MAX_NATIVE_TIMERS          8
#define TG_DEVICE_V1_2_3_NUM_GROUPS                2
#define TG_DEVICE_V1_2_3_NUM_DEVICES               6
#define TG_DEVICE_V1_2_4_KEYBOARD_BASE             1
#define TG_DEVICE_V1_2_4_JOYSTICK_BASE             3
#define TG_DEVICE_V1_2_4_MOUSE                     5
#define TG_DEVICE_V1_2_3_TYPE_JOYSTICK             1
#define TG_DEVICE_V1_2_3_TYPE_KEYBOARD             2
#define TG_DEVICE_V1_2_4_TYPE_MOUSE                5
#define TG_DEVICE_V1_2_3_MAX_BINDINGS_A            50
#define TG_DEVICE_V1_2_3_MAX_BINDINGS_B            4
#define TG_DEVICE_V1_2_3_NUM_KEYBOARD_SETS         2
#define TG_DEVICE_V1_2_3_KEYBOARD_SET_SLOTS        6
#define TG_DEVICE_V1_2_6_KEYBOARD_SET_SLOTS        8
#define TG_DEVICE_V1_2_3_CODE_TABLE_SIZE           32
#define TG_DEVICE_V1_2_3_JOYSTICK_CODES_OFFSET     14 // 0x2A - 0x1C
#define TG_DEVICE_V1_2_3_JOYSTICK_STATE_SIZE       17
#define TG_DEVICE_V1_2_3_NO_DIRECTION              0x88
#define TG_DEVICE_V1_2_3_KEY_RELEASE_DELAY         200
#define TG_DEVICE_V1_2_3_COMBO_TIMEOUT             200

// Tables
#define TG_HOST_V1_2_KEY_TABLE_SIZE              128
#define TG_HOST_V1_2_MAX_CLICK_REGIONS           100
#define TG_INPUT_V1_2_4_KEY_TABLE_SIZE           256
#define TG_INPUT_V1_2_4_MAX_CLICK_REGIONS        5416 // 0xFDE8 / 12
#define TG_INPUT_V1_2_6_MAX_CLICK_REGIONS        5909 // 0xFDE8 / 11
#define TG_INPUT_V1_2_7_1_MAX_CLICK_REGIONS      5000 // 0xFDE8 / 13
#define TG_HOST_V1_2_MAX_HOVER_REGIONS           40
#define TG_HOST_V1_2_12_MAX_HOVER_REGIONS        80
#define TG_HOST_V1_2_MAX_SCHEDULED_TIMERS        20
#define TG_HOST_V1_2_MAX_COLLISION_PAIRS         50
#define TG_HOST_V1_2_1_MAX_COLLISION_PAIRS       100
#define TG_HOST_V1_2_MAX_MOVIES                  4

// Resources
#define TG_HOST_V1_2_RESOURCE_RECORD_SIZE        8
#define TG_HOST_V1_2_MAX_FRAME_BATCH             50
#define TG_HOST_V1_2_SPRITE_DATA_MEMBER_COUNT    2
#define TG_HOST_V1_2_SPRITE_DATA_LEVEL           8
#define TG_HOST_V1_2_SPRITE_DATA_PICK_FLAG       0x0B
#define TG_HOST_V1_2_SPRITE_DATA_DEFER_FLAG      0x0C
#define TG_HOST_V1_2_SPRITE_DATA_MEMBERS         0x0E
#define TG_HOST_V1_2_SPRITE_DATA_MEMBER_BASE     0x16
#define TG_HOST_V1_2_BITMAP_RESOURCE_ID          0x0E

// Palette
#define TG_HOST_V1_2_NUM_SYSTEM_COLORS           10
#define TG_HOST_V1_2_NUM_FADE_COLORS             0xDD
#define TG_HOST_V1_2_1_PALETTE_LAST_COLOR        0xF6
#define TG_HOST_V1_2_1_FADE_LAST_COLOR           0xEC
#define TG_HOST_V1_2_SOLID_COLOR_ENTRY           0xE6
#define TG_HOST_V1_2_TRANS_PALETTE_FIRST         0xE7
#define TG_HOST_V1_2_NUM_TRANS_PALETTE_COLORS    0x0F

// Transitions
#define TG_HOST_V1_2_FADE_STEPS                  100
#define TG_HOST_V1_2_WIPE_RINGS_BASE             50
#define TG_HOST_V1_2_WIPE_RINGS_FACTOR           30
#define TG_HOST_V1_2_WIPE_RINGS_DIVISOR          100
#define TG_HOST_V1_2_WIPE_MIN_RINGS              2

// Startup
#define TG_HOST_V1_2_MIN_COLORS                  256
#define TG_HOST_V1_2_1_FILE_HEADER_SIZE          0x7C
#define TG_HOST_V1_2_1_HEADER_MAX_FADE_COLORS    0x5A
#define TG_HOST_V1_2_1_HEADER_MAX_TRANS_COLORS   0x5C
#define TG_HOST_V1_2_2_FILE_HEADER_SIZE          0x8A

// Sprite object data of a type 12 resource
#define TG_HOST_V1_2_SPRITE_DATA_CLICK_SCRIPT    0
#define TG_HOST_V1_2_SPRITE_DATA_DRAG_SCRIPT     4
#define TG_HOST_V1_2_SPRITE_DATA_DROP_SCRIPT     6
#define TG_HOST_V1_2_SPRITE_DATA_DISABLED        0x0A
#define TG_HOST_V1_2_SPRITE_DATA_DRAGGABLE       0x0D

// Mouse and keyboard
#define TG_HOST_V1_2_VARIABLE_CLICK_X            407
#define TG_HOST_V1_2_VARIABLE_CLICK_Y            408
#define TG_HOST_V1_2_VARIABLE_DROP_X             409
#define TG_HOST_V1_2_VARIABLE_DROP_Y             410
#define TG_HOST_V1_2_MOUSE_EVENT_DOWN            1
#define TG_HOST_V1_2_MOUSE_EVENT_UP              2
#define TG_HOST_V1_2_MOUSE_EVENT_MOVE            4
#define TG_HOST_V1_2_MOUSE_BUTTON_LEFT           1
#define TG_HOST_V1_2_MOUSE_BUTTON_RIGHT          2
#define TG_HOST_V1_2_KEY_STATE_SIZE              256
#define TG_HOST_V1_2_VK_TAB                      0x09
#define TG_HOST_V1_2_VK_SHIFT                    0x10
#define TG_HOST_V1_2_VK_CONTROL                  0x11
#define TG_HOST_V1_2_VK_ESCAPE                   0x1B
#define TG_HOST_V1_2_VK_ADD                      0x6B
#define TG_HOST_V1_2_VK_SUBTRACT                 0x6D
#define TG_HOST_V1_2_NO_KEY                      0xFFFF
#define TG_HOST_V1_2_CHAR_BACKSPACE              0x08
#define TG_HOST_V1_2_CHAR_RETURN                 0x0D
#define TG_HOST_V1_2_CHAR_ESCAPE                 0x1B
#define TG_HOST_V1_2_CHAR_FIRST_PRINTABLE        0x20
#define TG_INPUT_V1_2_1_VK_PAUSE                 0x13

// Joystick
#define TG_JOYSTICK_V1_2_1_NUM_STICKS            2
#define TG_JOYSTICK_V1_2_1_NUM_SCRIPTS           34 // 2 sticks * 17 slots
#define TG_JOYSTICK_V1_2_1_AXIS_CENTER           0x8000
#define TG_JOYSTICK_V1_2_1_DISTANCE_THRESHOLD    0x1F40
#define TG_JOYSTICK_V1_2_1_EIGHT_WAY_THRESHOLD   0x1F40
#define TG_JOYSTICK_V1_2_1_STICK_OFFSET          0x11
#define TG_JOYSTICK_V1_2_1_NO_INDEX              0xFFFF

// Cursors
#define TG_HOST_V1_2_CURSOR_MODE_ARROW           1
#define TG_HOST_V1_2_CURSOR_MODE_BUSY            3
#define TG_HOST_V1_2_CURSOR_MODE_HIDDEN          4
#define TG_HOST_V1_2_NUM_CURSOR_IMAGES           6
#define TG_HOST_V1_2_4_NUM_CURSOR_IMAGES         7
#define TG_HOST_V1_2_4_CURSOR_MODE_INVISIBLE     9

// Movies
#define TG_MOVIE_V1_2_HEADER_SIZE                8
#define TG_MOVIE_V1_2_HEADER_FRAME_COUNT         2
#define TG_MOVIE_V1_2_HEADER_TABLE_OFFSET        4
#define TG_MOVIE_V1_2_FRAME_ENTRY_SIZE           8
#define TG_MOVIE_V1_2_FRAME_ENTRY_STATE          1
#define TG_MOVIE_V1_2_FRAME_ENTRY_CHUNK_SIZE     2
#define TG_MOVIE_V1_2_FRAME_ENTRY_FILE_OFFSET    4
#define TG_MOVIE_V1_2_3_FRAME_TABLE_RESOURCE_OFFSET 1
#define TG_MOVIE_V1_2_NUM_PREFETCH_BUFFERS       10
#define TG_MOVIE_V1_2_PREFETCH_BUFFER_SIZE       0xFDE8
#define TG_MOVIE_V1_2_AUDIO_BUFFER_SIZE          0x18000
#define TG_MOVIE_V1_2_LATE_THRESHOLD             500
#define TG_MOVIE_V1_2_SKIPPED_FRAMES             10
#define TG_MOVIE_V1_2_SKIP_STEP                  1
#define TG_MOVIE_V1_2_MAX_FRAME_TABLE_ENTRIES    0x1FFF
#define TG_MOVIE_V1_2_BITMAP_KIND                9
#define TG_MOVIE_V1_2_6_BITMAP_KIND              8
#define TG_MOVIE_V1_2_BITMAP_KIND_DELTA          1
#define TG_MOVIE_V1_2_BITMAP_KIND_MERGE          2
#define TG_MOVIE_V1_2_BITMAP_BLIT_DIFF           1
#define TG_MOVIE_V1_2_BITMAP_BLIT_MERGE          2
#define TG_MOVIE_V1_2_TIMELINE_REPEAT            2
#define TG_MOVIE_V1_2_TIMELINE_COUNTER           4
#define TG_MOVIE_V1_2_TIMELINE_DELAY             6
#define TG_MOVIE_V1_2_TIMELINE_BITMAP_SIZE       8
#define TG_MOVIE_V1_2_TIMELINE_WAIT_SIZE         4
#define TG_MOVIE_V1_2_TIMELINE_SKIP_SIZE         4
#define TG_MOVIE_V1_2_TIMELINE_AUDIO_SIZE        0x1A
#define TG_MOVIE_V1_2_TIMELINE_SCRIPT_PREFIX     2
#define TG_MOVIE_V1_2_COMMAND_RESOURCE           2
#define TG_MOVIE_V1_2_COMMAND_INDEX              4
#define TG_MOVIE_V1_2_COMMAND_FRAME_READY        0x0A
#define TG_MOVIE_V1_2_6_COMMAND_FRAME_READY      6
#define TG_MOVIE_V1_2_7_2_DEFAULT_SPEED          100

// Text objects
#define TG_TEXT_V1_2_FONT_FULL_NAME              4
#define TG_TEXT_V1_2_DATA_SIZE                   17
#define TG_TEXT_V1_2_DATA_COLOR                  4
#define TG_TEXT_V1_2_DATA_FACE                   6
#define TG_TEXT_V1_2_DATA_TEXT                   8
#define TG_TEXT_V1_2_DATA_HEIGHT                 0x0A
#define TG_TEXT_V1_2_DATA_X                      0x0C
#define TG_TEXT_V1_2_DATA_Y                      0x0E
#define TG_TEXT_V1_2_DATA_ALIGNMENT              0x10
#define TG_TEXT_V1_2_ALIGN_CENTER                1
#define TG_TEXT_V1_2_ALIGN_RIGHT                 2
#define TG_TEXT_V1_2_EDIT_RECORD_SIZE            11
#define TG_TEXT_V1_2_EDIT_STRING                 4
#define TG_TEXT_V1_2_EDIT_SCRIPT                 6
#define TG_TEXT_V1_2_EDIT_MAX_LENGTH             10
#define TG_TEXT_V1_2_TEXT_SIZE                   0x80
#define TG_TEXT_V1_2_DEFAULT_MAX_LENGTH          0x7F
#define TG_TEXT_V1_2_NUM_SPRITES                 8
#define TG_TEXT_V1_2_SOLID_COLOR                 0xFF
#define TG_TEXT_V1_2_DEFAULT_HEIGHT              12
#define TG_TEXT_V1_2_1_HEADER_SIZE               22
#define TG_TEXT_V1_2_1_HEADER_BOX_WIDTH          0x10
#define TG_TEXT_V1_2_1_HEADER_BOX_HEIGHT         0x12
#define TG_TEXT_V1_2_1_HEADER_FLAGS              0x14
#define TG_TEXT_V1_2_3_RESOURCE_TYPE             14
#define TG_TEXT_V1_2_4_HEADER_SIZE               20
#define TG_TEXT_V1_2_4_SHORT_HEADER_SIZE         18
#define TG_TEXT_V1_2_4_EDIT_RECORD_SIZE          12
#define TG_TEXT_V1_2_4_FACE_SIZE                 32

// INI file
#define TG_INI_V1_2_BACKUP_LAST                  999
#define TG_INI_V1_2_COPY_CHUNK                   0x400
#define TG_INI_V1_2_BUFFER_SIZE                  0xFF
#define TG_INI_V1_2_4_STRING_SIZE                260
#define TG_INI_V1_2_COPY_SIZE                    0x7F
#define TG_INI_V1_2_CIPHER_BASE                  0x30
#define TG_INI_V1_2_CIPHER_SHIFT                 4
#define TG_INI_V1_2_SCREEN_SAVER_KEY             0x50
#define TG_INI_V1_2_RECORD_VALUE                 2
#define TG_INI_V1_2_RECORD_KEY                   4
#define TG_INI_V1_2_RECORD_SECTION               6
#define TG_INI_V1_2_RECORD_OBFUSCATION           8
#define TG_INI_V1_2_RECORD_STRING_MODE           9

// Interpreter
#define TG_SCRIPT_V1_2_MAX_DEPTH                 30
#define TG_SCRIPT_V1_2_ARGUMENT_FRAME_VARIABLE   515
#define TG_SCRIPT_V1_2_ARGUMENT_STACK_VARIABLE   535
#define TG_SCRIPT_V1_2_ARGUMENT_STACK_SIZE       200
#define TG_SCRIPT_V1_2_1_ARGUMENT_FRAME_VARIABLE 517
#define TG_SCRIPT_V1_2_1_ARGUMENT_FRAME_SIZE     8
#define TG_SCRIPT_V1_2_2_ARGUMENT_FRAME_VARIABLE 518
#define TG_SCRIPT_V1_2_CALL_HEADER_SIZE          5
#define TG_SCRIPT_V1_2_3_CALL_HEADER_SIZE        6
#define TG_SCRIPT_V1_2_3_TABLE_ADDRESS           0xF03
#define TG_SCRIPT_V1_2_3_1_TABLE_ADDRESS         0x1187
#define TG_SCRIPT_V1_2_3_1_LAST_BUILTIN_CODE     0x60
#define TG_SCRIPT_V1_2_3_1_POSTED_RECORD_BUFFER_SIZE 20
#define TG_SCRIPT_V1_2_3_1_MIDI_PARAMETERS_SIZE  10
#define TG_SCRIPT_V1_2_EXPRESSION_STACK_SIZE     100 // (0x7D6 - 0x70E) / 2
#define TG_SCRIPT_V1_2_NUM_ELEMENT_VARIABLES     4
#define TG_SCRIPT_V1_2_7_1_NUM_ELEMENT_VARIABLES 10
#define TG_SCRIPT_V1_2_3_SWITCH_ENTRY_SIZE       4
#define TG_SCRIPT_V1_2_NUM_SCRIPT_CHOICES        6
#define TG_SCRIPT_V1_2_ROUND_ROBIN_NESTED_OPCODE 23
#define TG_SCRIPT_V1_2_STRING_COPY_SIZE          0x7F
#define TG_SCRIPT_V1_2_FORMAT_BUFFER_SIZE        0x400
#define TG_SCRIPT_V1_2_FORMAT_MAX_ARGUMENTS      6
#define TG_SCRIPT_V1_2_FORMAT_ARGUMENTS_OFFSET   8
#define TG_SCRIPT_V1_2_FORMAT_ARGUMENT_SIZE      3
#define TG_SCRIPT_V1_2_HOST_EVENT_SCENE_END      0x13
#define TG_SCRIPT_V1_2_HOST_EVENT_OPERATION      0x14
#define TG_SCRIPT_V1_2_HOST_EVENT_OPERATION_OFFSET 4
#define TG_SCRIPT_V1_2_3_1_HOST_EVENT_POSTED_RUN 0x15
#define TG_SCRIPT_V1_2_ARITHMETIC_OPERANDS_OFFSET 6
#define TG_SCRIPT_V1_2_ARITHMETIC_OPERAND_SIZE   4
#define TG_SCRIPT_V1_2_ARITHMETIC_OPERATION_MASK 0x3F
#define TG_SCRIPT_V1_2_ARITHMETIC_NEGATE_FLAG    0x80
#define TG_SCRIPT_V1_2_CONDITION_EQUAL           0x01
#define TG_SCRIPT_V1_2_CONDITION_LESS            0x02
#define TG_SCRIPT_V1_2_CONDITION_GREATER         0x04
#define TG_SCRIPT_V1_2_CONDITION_NEGATE          0x80
#define TG_SCRIPT_V1_2_1_CONDITION_OR            0x08
#define TG_SCRIPT_V1_2_1_CONDITION_AND           0x10
#define TG_SCRIPT_V1_2_1_CONDITION_MODULO        0x20
#define TG_SCRIPT_V1_2_SKIP_TAKEN_SIZE           9
#define TG_SCRIPT_V1_2_CONDITIONAL_SIZE          0x0C
#define TG_SCRIPT_V1_2_MAX_ZOOM_MODE             4
#define TG_SCRIPT_V1_2_ANGLE_RESULT_SCALE        10000
#define TG_SCRIPT_V1_2_ANGLE_ROUNDING            5000

namespace Graphics {
class Cursor;
class Font;
class ManagedSurface;
}

namespace Topgun {

class EnginePluginHost;
class Plugin;
class PluginHost;

enum MessageId_v1_2 {
	kMsgV1_2ActivateApp = 0x1C,
	kMsgV1_2Char = 0x102,
	kMsgV1_2SysKeyDown = 0x104,
	kMsgV1_2MouseMove = 0x200,
	kMsgV1_2LButtonUp = 0x202,
	kMsgV1_2RButtonUp = 0x205,
	kMsgV1_2MemoryError = 0x464,
	kMsgV1_2SceneEnd = 0x4C8,
	kMsgV1_2GotoScene = 0x4C9,
	kMsgV1_2RunScript = 0x4CA,
	kMsgV1_2SoundFinished = 0x52C,
	kMsgV1_2Mute = 0x781,
	kMsgV1_2CompanionWindow = 0x783
};

enum MessageId_v1_2_1 {
	kMsgV1_2_1Joystick1Move = 0x3A0,
	kMsgV1_2_1Joystick2Move = 0x3A1,
	kMsgV1_2_1Joystick1ButtonDown = 0x3B5,
	kMsgV1_2_1Joystick2ButtonDown = 0x3B6,
	kMsgV1_2_1Joystick1ButtonUp = 0x3B7,
	kMsgV1_2_1Joystick2ButtonUp = 0x3B8,
	kMsgV1_2_1IoError = 0x590
};

enum ResourceType_v1_2_1 {
	kResV1_2_1Bitmap = 1,
	kResV1_2_1Wave = 4,
	kResV1_2_1OpList = 5,
	kResV1_2_1BitmapGroup = 6,
	kResV1_2_1MixedGroup = 7,
	kResV1_2_1Movie = 8,
	kResV1_2_1Palette = 9,
	kResV1_2_1SpriteQueueData = 10,
	kResV1_2_1Script = 11,
	kResV1_2_1Sprite = 12,
	kResV1_2_1Text = 13
};

enum ResourceType_v1_2_3 {
	kResV1_2_3Bitmap = 1,
	kResV1_2_3Wave = 4,
	kResV1_2_3Midi = 5,
	kResV1_2_3Movie = 6,
	kResV1_2_3OpList = 7,
	kResV1_2_3BitmapGroup = 8,
	kResV1_2_3MixedGroup = 9,
	kResV1_2_3Palette = 10,
	kResV1_2_3SpriteQueueData = 11,
	kResV1_2_3Script = 12,
	kResV1_2_3Sprite = 13,
	kResV1_2_3Text = 14
};

enum ResourceType_v1_2_4 {
	kResV1_2_4Bitmap = 1,
	kResV1_2_4Midi = 4,
	kResV1_2_4Wave = 7,
	kResV1_2_4Movie = 8,
	kResV1_2_4OpList = 9,
	kResV1_2_4BitmapGroup = 10,
	kResV1_2_4MixedGroup = 11,
	kResV1_2_4Palette = 12,
	kResV1_2_4SpriteQueueData = 13,
	kResV1_2_4Script = 14,
	kResV1_2_4Sprite = 15,
	kResV1_2_4Text = 16
};

enum ResourceType_v1_2 {
	kResV1_2Bitmap = 1,
	kResV1_2Blob = 2,
	kResV1_2Wave = 3,
	kResV1_2SpriteQueue = 5,
	kResV1_2Movie = 6,
	kResV1_2OpList = 7,
	kResV1_2Text = 8,
	kResV1_2Group = 9,
	kResV1_2TransPalette = 10,
	kResV1_2Script = 11,
	kResV1_2Sprite = 12
};

enum ScriptOpcode_v1_2 {
	kOpV1_2CallScript = 1,
	kOpV1_2LoadResource,
	kOpV1_2UnloadResource,
	kOpV1_2SetClickScript,
	kOpV1_2SetSpriteQueue,
	kOpV1_2SetBackground,
	kOpV1_2SetSpriteObjectProperties,
	kOpV1_2PlayWave,
	kOpV1_2PlayCDTrack,
	kOpV1_2FadeIn,
	kOpV1_2FadeOut,
	kOpV1_2BindKeyScript,
	kOpV1_2Quit,
	kOpV1_2StopCD,
	kOpV1_2StopWave,
	kOpV1_2Arithmetic,
	kOpV1_2ConditionalRunScript,
	kOpV1_2GotoScene,
	kOpV1_2SetScriptTimer,
	kOpV1_2CancelScriptTimer,
	kOpV1_2SetSceneEndScript,
	kOpV1_2RandomRunScript,
	kOpV1_2RoundRobinRunScript,
	kOpV1_2TransferSprite,
	kOpV1_2StopSprite,
	kOpV1_2SetSpriteQueueIfLoaded,
	kOpV1_2DebugString,
	kOpV1_2SetSpriteLevel,
	kOpV1_2SetActivationScript,
	kOpV1_2PostScript,
	kOpV1_2CursorMode,
	kOpV1_2RunScriptIfLoaded,
	kOpV1_2AddCollisionPair,
	kOpV1_2ListRunScript,
	kOpV1_2LaunchProgram,
	kOpV1_2SetPauseScript = kOpV1_2LaunchProgram,
	kOpV1_2MoveSprite,
	kOpV1_2Jump,
	kOpV1_2ConditionalSkip,
	kOpV1_2SetDisplayZoom,
	kOpV1_2GetSpriteInfo,
	kOpV1_2FormatTextObject,
	kOpV1_2SetSpriteRect,
	kOpV1_2Switch,
	kOpV1_2Stop,
	kOpV1_2Nop,
	kOpV1_2Random,
	kOpV1_2GetKeyState,
	kOpV1_2GetDate,
	kOpV1_2GetAngle,
	kOpV1_2ClipLineToRect,
	kOpV1_2Unused,
	kOpV1_2SetWallpaper,
	kOpV1_2SetMute,
	kOpV1_2BeginTextEdit,
	kOpV1_2GetSpriteRect,
	kOpV1_2FormatString,
	kOpV1_2CompareStrings,
	kOpV1_2CompareStringsIgnoreCase,
	kOpV1_2IniRead,
	kOpV1_2IniWrite,
	kOpV1_2SetHoverRegion,
	kOpV1_2GetIntersect,
	kOpV1_2SetSpritePosition,
	kOpV1_2SetMouseEventScript,
	kOpV1_2Cosine,
	kOpV1_2Sine,
	kOpV1_2FlushIni,
	kOpV1_2IsSpriteBusy,
	kOpV1_2BackupIni,
	kOpV1_2GetTime,
	kOpV1_2SpriteCompositeToBackground,
	kOpV1_2SetJoystickScript,
	kOpV1_2SetCursorPosition,
	kOpV1_2GetSpriteElementIds = 75,
	kOpV1_2Expression,
	kOpV1_2ConditionalJumpExpression,
	kOpV1_2SwitchExpression,
	kOpV1_2ConditionalJumpExpressionCopy,
	kOpV1_2PlayMidi,
	kOpV1_2StopMidi,
	kOpV1_2SetDeviceGroupBinding,
	kOpV1_2DeviceGroupButtonState,
	kOpV1_2SetDeviceGroupScript,
	kOpV1_2DeviceGroupLatch,
	kOpV1_2SetDeviceGroupBindingB,
	kOpV1_2SetDeviceGroupFlag,
	kOpV1_2GetDeviceGroupState,
	kOpV1_2ReturnExpression,
	kOpV1_2_3_1SetMidiParams = 92,
	kOpV1_2_3_1SetVolume,
	kOpV1_2_3_1Unused94,
	kOpV1_2_3_1Unused95,
	kOpV1_2_3_1SetDebugScriptId
};

struct FormatArgument_v1_2 {
	uint16 value;
	const char *text;

	FormatArgument_v1_2() : value(0), text(nullptr) {}
};

enum MovieChunk_v1_2 {
	kMovieChunkKeyFrame = 0,
	kMovieChunkDeltaFrame,
	kMovieChunkAudio
};

enum MovieTimeline_v1_2 {
	kMovieTimelineBitmap = 1,
	kMovieTimelineWait,
	kMovieTimelineAudio,
	kMovieTimelineSkip,
	kMovieTimelineScript
};

struct PrefetchBuffer_v1_2 {
	byte *data;
	uint16 filled;
	uint16 cursor;
	uint16 count;
	bool ready;

	PrefetchBuffer_v1_2() : data(nullptr), filled(0), cursor(0), count(0), ready(false) {}
};

struct Movie_v1_2 {
	uint16 handle;
	byte *timeline;
	byte *frameTable;
	byte *frameTableBlock;
	byte *bitmap;
	byte *audioBuffer;
	byte *data;
	uint32 deadline;
	uint32 runOffset;
	uint32 audioBuffered;
	uint16 timelineSize;
	uint16 frameCount;
	uint32 frameTableOffset;
	uint32 streamBase;
	uint16 audioStream;
	uint16 delay;
	uint16 prefetchFrame;
	uint16 runSize;
	uint16 frame;
	uint16 id;
	uint16 index;
	uint16 sprite;
	uint16 timelineRemaining;
	bool frameUpdated;
	bool prefetchDone;
	bool stalled;
	bool skipping;
	bool late;
	bool prefetchExhausted;
	bool started;
	bool audioStarted;
	uint16 numBuffers;
	uint16 fillBuffer;
	uint16 consumeBuffer;
	uint16 speed;
	PrefetchBuffer_v1_2 buffers[TG_MOVIE_V1_2_NUM_PREFETCH_BUFFERS];

	Movie_v1_2() : handle(0), timeline(nullptr), frameTable(nullptr), frameTableBlock(nullptr), bitmap(nullptr), audioBuffer(nullptr), data(nullptr),
		deadline(0), runOffset(0), audioBuffered(0), timelineSize(0), frameCount(0), frameTableOffset(0), streamBase(0),
		audioStream(0), delay(0), prefetchFrame(0), runSize(0), frame(0), id(0), index(0), sprite(0),
		timelineRemaining(0), frameUpdated(false), prefetchDone(false), stalled(false), skipping(false),
		late(false), prefetchExhausted(false), started(false), audioStarted(false), numBuffers(0),
		fillBuffer(0), consumeBuffer(0), speed(TG_MOVIE_V1_2_7_2_DEFAULT_SPEED) {}
};

struct TextObject_v1_2 {
	uint16 handle;
	uint16 id;
	const Graphics::Font *font;
	bool fontOwned;
	uint16 fontHandle;
	uint16 color;
	byte edit[TG_TEXT_V1_2_4_EDIT_RECORD_SIZE];
	bool leadByteBuffered;
	byte leadByte;
	byte data[TG_TEXT_V1_2_1_HEADER_SIZE];
	char text[TG_TEXT_V1_2_TEXT_SIZE];
	char undo[TG_TEXT_V1_2_TEXT_SIZE];
	uint16 sprites[TG_TEXT_V1_2_NUM_SPRITES];

	TextObject_v1_2();
	~TextObject_v1_2();
};

struct TextFont_v1_2 {
	uint16 handle;
	const Graphics::Font *font;
	bool owned;
	uint16 refCount;
	int16 height;
	char face[TG_TEXT_V1_2_4_FACE_SIZE];

	// Set for a substitute with other metrics; a zero line height stands for the height of the font
	int lineHeight;
	int lineOffset;

	TextFont_v1_2() : handle(0), font(nullptr), owned(false), refCount(0), height(0), lineHeight(0), lineOffset(0) {
		for (uint16 i = 0; i < TG_TEXT_V1_2_4_FACE_SIZE; i++)
			face[i] = 0;
	}
};

struct SceneEntry_v1_2 {
	char name[TG_HOST_V1_2_6_SCENE_NAME_SIZE];
	uint16 variables[TG_HOST_V1_2_4_NUM_SCENE_VARIABLES];
	uint16 stringCount;
	byte *strings;

	SceneEntry_v1_2() : stringCount(0), strings(nullptr) {
		for (int i = 0; i < TG_HOST_V1_2_6_SCENE_NAME_SIZE; i++)
			name[i] = 0;

		for (int i = 0; i < TG_HOST_V1_2_4_NUM_SCENE_VARIABLES; i++)
			variables[i] = 0;
	}
};

struct KeyEntry_v1_2 {
	uint16 plain;
	uint16 shift;
	uint16 ctrl;
	uint16 keyUp;
	byte disabled;

	KeyEntry_v1_2() : plain(0), shift(0), ctrl(0), keyUp(0), disabled(0) {}
};

struct ClickRegion_v1_2 {
	int16 left;
	int16 top;
	int16 right;
	int16 bottom;
	uint16 script;
	uint16 argument;
	byte disabled;

	ClickRegion_v1_2() : left(0), top(0), right(0), bottom(0), script(0), argument(0), disabled(0) {}
};

struct HoverRegion_v1_2 {
	int16 left;
	int16 top;
	int16 right;
	int16 bottom;
	uint16 enterScript;
	uint16 leaveScript;
	uint16 enterArgument;
	uint16 leaveArgument;
	uint16 sprite;

	HoverRegion_v1_2() : left(0), top(0), right(0), bottom(0), enterScript(0), leaveScript(0),
		enterArgument(0), leaveArgument(0), sprite(0) {}
};

struct ScheduledTimer_v1_2 {
	uint32 expiry;
	uint32 delay;
	uint16 id;
	uint16 script;
	byte repeat;

	ScheduledTimer_v1_2() : expiry(0), delay(0), id(0), script(0), repeat(0) {}
};

struct CollisionPair_v1_2 {
	uint16 first;
	uint16 second;
	uint16 script;
	uint16 argument1;
	uint16 argument2;
	uint16 repeat;
	byte sameLevelOnly;
	byte allowInvisibleFirst;
	byte allowInvisibleSecond;

	CollisionPair_v1_2() : first(0), second(0), script(0), argument1(0), argument2(0), repeat(0),
		sameLevelOnly(0), allowInvisibleFirst(0), allowInvisibleSecond(0) {}
};

enum NativeTimerProc_v1_2_4 {
	kNativeTimerV1_2_4ComboHistory = 1,
	kNativeTimerV1_2_4KeyRelease
};

struct KeyRoute_v1_2_4 {
	byte device;
	byte armed;
	byte code;

	KeyRoute_v1_2_4() : device(0), armed(0), code(0) {}
};

struct DeviceBinding_v1_2_4 {
	uint16 id;
	uint16 script;
	uint16 priority;
	uint16 mask;
	byte active;

	DeviceBinding_v1_2_4() : id(0), script(0), priority(0), mask(0), active(0) {}
};

struct DeviceGroup_v1_2_4 {
	uint16 history;
	uint16 lastCode;
	uint16 pressedMask;
	uint16 stateScript;
	uint16 numBindingsA;
	uint16 numBindingsB;
	DeviceBinding_v1_2_4 bindingsA[TG_DEVICE_V1_2_3_MAX_BINDINGS_A];
	DeviceBinding_v1_2_4 bindingsB[TG_DEVICE_V1_2_3_MAX_BINDINGS_B];
	DeviceBinding_v1_2_4 latch;
	byte type;
	byte remap;
	byte latchMode;
	byte device;
	byte attached;

	DeviceGroup_v1_2_4() : history(0), lastCode(0), pressedMask(0), stateScript(0), numBindingsA(0), numBindingsB(0),
		type(0), remap(0), latchMode(0), device(0), attached(0) {}
};

struct NativeTimer_v1_2_4 {
	uint32 expiry;
	uint32 interval;
	uint16 id;
	uint16 proc;
	byte repeat;

	NativeTimer_v1_2_4() : expiry(0), interval(0), id(0), proc(0), repeat(0) {}
};

struct PluginModule_v1_2_12 {
	Common::String name;
	Plugin *plugin;
	uint16 refCount;

	PluginModule_v1_2_12() : plugin(nullptr), refCount(0) {}
};

struct PluginFunction_v1_2_4 {
	uint16 library;
	int function;

	PluginFunction_v1_2_4() : library(0), function(0) {}
};

class TopgunEngine_v1_2 : public TopgunEngine_v1_1 {
	friend class AudioLibrary;
	friend class AudioLibrary_v1_2;
	friend class AudioLibrary_v1_2_4;
	friend class EnginePluginHost;
	friend class ResourceLibrary_v1_2;
	friend class SpriteLibrary_v1_2;
	friend class SpriteLibrary_v1_2_4;

public:
	TopgunEngine_v1_2(OSystem *syst, const ADGameDescription *gameDesc, int version = kVersionV1_2);
	~TopgunEngine_v1_2() override;

protected:
	// Libraries
	AudioLibrary_v1_2 *audioLib() { return static_cast<AudioLibrary_v1_2 *>(audioLibrary()); }
	ResourceLibrary_v1_2 *resourceLib() { return static_cast<ResourceLibrary_v1_2 *>(resourceLibrary()); }
	SpriteLibrary_v1_2 *spriteLib() { return static_cast<SpriteLibrary_v1_2 *>(spriteLibrary()); }
	void syncSoundSettings() override;

	// Operands
	uint16 variableIndex(uint16 operand) const;
	uint16 operandValue(uint16 operand) const;
	uint16 readVariable(uint16 index) const;
	void writeVariable(uint16 index, uint16 value);
	uint16 operandAddress(uint16 operand) const;
	uint16 readAddress(uint16 address) const;
	void writeAddress(uint16 address, uint16 value);

	bool isRecordIndexValid(uint16 index, const char *caller) const {
		if (!_records || index >= _numResources) {
			warning("TopgunEngine_v1_2::%s(): resource id %d is out of range", caller, index);
			return false;
		}

		return true;
	}

	// Hooks of the libraries
	bool postMessage(uint16 id, uint16 param, uint32 data) override;
	void resourceLoadProgressCallback() override;
	uint16 audioCreateWave(const byte *header, uint16 waveInfo);
	uint16 audioCreateMidiEvent(const byte *header, uint16 midiInfo);
	virtual uint32 audioSourceRead(uint16 proc, uint16 handle, byte *destination, uint32 count);
	virtual uint32 audioSourceRewind(uint16 proc, uint16 handle);
	virtual void audioSourceClose(uint16 proc, uint16 handle);
	void resourceSendMessage(uint16 window, uint16 message);
	bool resourceEnumCallback(uint16 type, uint32 offset, uint32 size);
	bool resourceVariableCallback(uint16 id, uint16 value, const char *name);
	bool resourceVariablePairCallback(uint16 id, uint16 value);
	void spriteServiceProc(uint16 word);
	byte *spriteBitmapProvider(uint16 id, bool release);
	void spriteMessageService(byte *record) override;
	virtual void spriteMessageService(byte *record, uint16 spriteId);
	void spriteQueueDoneService(uint16 sprite, uint16 tag);
	uint16 spriteMovieService(uint16 sprite, byte *command, byte **frame);
	uint32 movieAudioFill(uint16 movie, byte *destination, uint32 count);

	// Startup
	bool rtStartup(bool runChecks) override;
	bool runStartup(const Common::String &title, const Common::String &firstContainer);
	bool initApplication(const Common::String &title);
	bool registerViewClassAndCreateMainWindow();
	bool initEngineLibraries() override;
	bool checkDisplayCapabilities();
	bool applyLaunchModeSetup();
	void notifyPythonizerWindow();
	bool locateResourceFile(const Common::String &name);
	uint16 getPrivateProfileInt(const char *section, const char *key, uint16 defaultValue);
	void saveScreenSaverTimes();
	bool loadCursorResources();
	void destroyCursorResources();

	// Main loop
	uint16 mainLoop() override;
	void serviceScheduledMsgTimerQueue() override;
	void serviceCollisionPairs();
	void serviceMovies();
	bool peekPostedMessage(uint16 id, Message &message);

	// Messages
	void dispatchMessage(const Message &message) override;
	void handlePaintMessage() override;
	void fillBackground() override;
	void handleAppShutdown() override;
	void handleActivateMessage(uint16 param) override;
	void handleSceneEndMessage() override;
	void handleRunScriptMessage(uint16 script, uint16 argument1, uint16 argument2);
	void handleMouseButtonMessage(const Message &message);
	void handleMouseButtonUpMessage(const Message &message);
	void handleMouseMoveMessage(const Message &message);
	void handleCharMessage(uint16 character);
	void handleDeleteKeyMessage();
	virtual void handleKeyUpMessage(Common::KeyCode keycode);
	void handleSysKeyDownAltTab(const Message &message);
	void handleKeyDownMessage(Common::KeyCode keycode, bool shift, bool ctrl) override;
	void flushPostedScriptMessages();
	void drainMessage(uint16 id);
	bool fatalError(const Common::String &message);
	bool fatalError(uint16 code, const Common::String &message);
	void showShutdownErrorBox();

	// Cursor
	void setCursorMode(byte mode) override;
	void applyCursorFromRegister() override;
	void forceShowOrHideCursor(bool show);

	// Scenes
	bool loadInitialScene(const Common::String &name) override;
	bool loadSceneContainerAndRunEntryScript(const Common::String &name, bool withProgressCallback) override;
	void switchToNextScene(const char *name) override;
	void cleanupCurrentSceneAndContainer() override;
	bool openResourceFileAndInit(const Common::String &name) override;
	SceneEntry_v1_2 *findOrCreateSceneEntry(const Common::String &name);
	void buildResourceNameOffsetTable();
	void releaseCurrentBackgroundRecord();
	void refreshGamePalette();
	virtual char *getStringFromId(uint16 id);

	// Resources
	void freeBitmapBlobIfUnreferenced(uint16 id) override;
	bool destroyGroupResourceObject(uint16 id) override;
	bool destroyMovieResourceObject(uint16 id);
	void destroyMidiResourceObject(uint16 id);
	bool ensureMidiResourceLoaded(uint16 id);
	bool isResourceObjectLoaded(uint16 id, uint16 type);
	bool destroySpriteResourceObject(uint16 id) override;
	bool releaseSceneSpriteRecord(uint16 id);
	bool destroyTextObjectResource(uint16 id);
	void destroyWaveResourceObject(uint16 id) override;
	bool ensureBitmapResourceLoaded(uint16 id) override;
	bool loadOrGetResourceData(uint16 id) override;
	bool ensureGroupResourceLoaded(uint16 id) override;
	bool ensureMovieResourceLoaded(uint16 id);
	bool loadTransPaletteResource(uint16 id) override;
	bool ensureSpriteResourceLoaded(uint16 id) override;
	bool attachResourceListToSprite(uint16 spriteHandle, const byte *idList, uint16 count, bool deferBitmaps);
	bool ensureTextObjectResourceLoaded(uint16 id);
	bool ensureWaveResourceLoaded(uint16 id) override;
	bool destroyResourceObjectByType(uint16 id, uint16 type) override;
	bool validateResourceType(uint16 id, uint16 type);
	void destroyBitmapGroupResourceObject(uint16 id);
	bool ensureBitmapGroupResourceLoaded(uint16 id);
	bool ensureBitmapOrGroupResourceLoaded(uint16 id);
	void applySpritePaletteExtension(const byte *header);
	void resetResourcesAndTransientState() override;
	void destroySceneMainObjects(bool release) override;
	bool validateAndGetResourceByType(uint16 id, uint16 type) override;
	bool isResourceObjectLoaded(uint16 id) override;
	byte *loadResourceWithProcessingPause(uint16 id, uint32 *sizeOut) override;
	virtual bool ensureResourceDataLoaded(uint16 id);
	bool registerTextObjectSpriteUse(uint16 object, uint16 sprite);
	void unregisterTextObjectSpriteUse(uint16 object, uint16 sprite);
	void stopMovieByHandle(uint16 object);

	// Sprite transfer
	bool transferSpriteContents(uint16 sourceId, uint16 destinationId, bool freeSource);
	void hideSourceSpriteAndAddFrameRefs(const SpriteInfo_v1_2 &info, uint16 sourceId);
	void destroySourceSpriteAfterTransfer(const SpriteInfo_v1_2 &info, uint16 sourceId);
	void decrementResourceRefCountAndMaybeDestroy(uint16 id) override;
	bool appendQueueBitmapsToSprite(const SpriteInfo_v1_2 &info, uint16 destinationId);
	void trimSpriteQueueBitmaps(uint16 id);

	// Backgrounds
	void syncBackgroundDimensions(const byte *bitmap);
	void commitBackgroundChangeAndRepaint(const Common::Rect &rect) override;
	void applyBackgroundToSpriteLib(Common::Rect *rectOut);
	void clearBackground() override;
	void setBackgroundFromBitmapResource(const byte *record);
	void setSolidBackgroundColorPalette(uint32 color);
	void setSolidBackgroundColorBitmap(uint32 color);
	bool setBackgroundWithWipeEffect(const byte *record);

	// Transitions
	void executeWipeEffect(const byte *params, byte *newBitmap, byte *oldBitmap);
	void copyAndPaintBackgroundRect(Common::Rect &rect) override;
	void paintFrameBandsLeftRight(const Common::Rect &outer, const Common::Rect &inner);
	void paintFrameBandsTopBottom(const Common::Rect &outer, const Common::Rect &inner) override;
	void wipeEffectFrameRingsInsideOut(uint16 rings, bool insetSides);
	void wipeEffectFrameRingsOutsideIn(uint16 rings, bool insetSides);
	void computeInsetRingRect(uint16 ring, Common::Rect *out, uint16 rings, bool insetSides);
	void advanceSceneFadeStep() override;
	using TopgunEngine_v1_1::sceneTransitionIn;
	using TopgunEngine_v1_1::sceneTransitionOut;
	void sceneTransitionIn(uint16 steps);
	void sceneTransitionOut(uint16 steps);

	// Scripts
	bool runScriptResource(uint16 scriptId) override;
	bool runScriptBytecodeLoop(uint16 scriptId) override;
	virtual bool executeScriptOp(byte *ip, uint16 *advance);
	virtual uint16 getScriptOpSize(const byte *ip);
	bool runPostedScriptRecord(const byte *record);
	bool callScriptHook(uint16 script);
	bool pushScriptArguments(uint16 count, const byte *arguments);
	void popScriptArguments();
	uint16 countScriptListEntries(uint16 listId);
	bool runScriptListEntry(uint16 listId, int16 index);
	bool evaluateCondition(byte condition, uint16 operandA, uint16 operandB);
	int32 applyArithmeticOperand(int32 accumulator, const byte *operand);
	uint16 lookupSwitchOffset(const byte *record);
	uint16 evaluateExpression(const byte *expression);
	void pushExpressionValue(uint16 value);
	bool exprCallScript(const byte *token);
	void exprCallBuiltinFunction(const byte *token);
	virtual void evaluateBuiltinFunction(uint16 code, uint16 argc);
	static int32 divideSigned32(int32 dividend, int32 divisor);
	static int32 roundedDivideBy10000(int32 value);
	Common::Point computeRayRectIntersection(int16 angle, int16 x, int16 y, int16 left, int16 top, int16 right, int16 bottom);
	static Common::String formatWindowsString(const char *format, const FormatArgument_v1_2 *arguments, uint16 count);

	// Plug-ins
	bool loadPluginLibrary(const Common::String &name);
	void unloadPluginLibrary();
	bool loadPluginFunctionTable();
	void exprCallPluginFunction(uint16 function, uint16 count);
	bool callMessageFilter(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh);

	// Opcodes
	virtual bool o_callScript(const byte *record); // 1
	bool o_setClickScript(const byte *record); // 4
	bool o_setSpriteQueue(uint16 sprite, uint16 data, bool flag); // 5
	void o_setBackground(const byte *record); // 6
	bool o_setSpriteObjectProperties(const byte *record); // 7
	bool o_playWave(uint16 waveId, const byte *params); // 8
	void o_playCDTrack(uint16 track) override; // 9
	void o_fadeInOut(const byte *record); // 10, 11
	void o_bindKeyScript(const byte *record); // 12
	void o_stopCD(); // 14
	void o_stopWave(uint16 waveId, const byte *block); // 15
	void o_arithmetic(const byte *record); // 16
	bool o_conditionalRunScript(const byte *record, uint16 &size); // 17
	void o_gotoScene(const byte *record); // 18
	void o_setScriptTimer(const byte *record); // 19
	void o_cancelScriptTimer(const byte *record); // 20
	bool o_randomRunScript(byte *record) override; // 22
	bool o_roundRobinRunScript(byte *record) override; // 23
	bool o_transferSprite(const byte *record); // 24
	void o_stopSprite(const byte *record); // 25
	void o_debugString(const byte *record); // 27
	void o_setSpriteLevel(uint16 spriteId, uint16 level); // 28
	bool o_addCollisionPair(const byte *record); // 33
	bool o_listRunScript(const byte *record); // 34
	void o_moveSprite(const byte *record); // 36
	uint16 o_conditionalSkip(const byte *record); // 38
	void o_setDisplayZoom(const byte *record); // 39
	void o_getSpriteInfo(const byte *record); // 40
	bool o_formatTextObject(const byte *record); // 41
	void o_setSpriteRect(const byte *record); // 42
	void o_random(const byte *record); // 46
	void o_getKeyState(const byte *record); // 47
	void o_getDate(const byte *record); // 48
	void o_getAngle(const byte *record); // 49
	void o_clipLineToRect(const byte *record); // 50
	bool o_beginTextEdit(const byte *record); // 54
	void o_getSpriteRect(const byte *record); // 55
	void o_formatString(const byte *record); // 56
	void o_compareStrings(const byte *record); // 57, 58
	void o_iniAccess(const byte *record); // 59, 60
	bool o_setHoverRegion(const byte *record); // 61
	void o_getIntersect(const byte *record); // 62
	bool o_setSpritePosition(const byte *record); // 63
	void o_sineCosine(const byte *record); // 65, 66
	void o_flushIni(const byte *record); // 67
	void o_isSpriteBusy(const byte *record); // 68
	void o_backupIni(); // 69
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

	// Device groups
	void clearDeviceState();
	uint16 insertDeviceBindingSorted(uint16 count, DeviceBinding_v1_2_4 *list, const byte *record);
	void setAllBindingScripts(uint16 script, uint16 count, DeviceBinding_v1_2_4 *list);
	void setAllBindingActive(byte active, uint16 count, DeviceBinding_v1_2_4 *list);
	void fillJoystickStateBytes(byte value, byte stick);
	uint16 deleteDeviceBinding(uint16 index, uint16 count, DeviceBinding_v1_2_4 *list);
	uint16 updateDeviceBindingList(uint16 maxEntries, uint16 count, DeviceBinding_v1_2_4 *list, const byte *record);
	void comboHistoryTimeoutCallback(byte timerId);
	void callDeviceGroupScript(uint16 group, uint16 argument, uint16 script);
	void scanDeviceBindingsListB(byte group);
	void deviceGroupInputEvent(byte code, byte device);
	bool keyboardRoutedKeyDown(uint16 vk);
	bool keyboardRoutedKeyUp(uint16 vk);
	void keyReleaseDelayTimerCallback(byte device);
	void bindKeyboardSetToDevice(byte device, byte value);

	// Input
	bool translate(const Common::Event &event, Message &message) override;
	bool peekInputMessage(Message &message) override;
	uint16 keyCodeToScriptKey(Common::KeyCode keycode);
	void adjustMasterVolumeByKey(Common::KeyCode keycode) override;
	void transformPointForDisplayZoom(Common::Point &point);
	virtual bool postRunScript(uint16 script, uint16 argument1, uint16 argument2);
	uint16 postMouseEventScript(uint16 code);
	bool appendClickRegion();
	void addOrUpdateClickRegion(const byte *record);
	void setAllSpriteObjectsCursor(uint16 value);
	void setAllClickRegionsAndSpriteObjectsEnabled(byte disabled);
	bool addHoverRegion(const byte *record);
	bool removeHoverRegion(const byte *record);
	void updateHoverRegionFromPoint(int16 x, int16 y);
	void dropDraggedSprite(int16 x, int16 y);
	uint16 getTextObjectFromId(uint16 id);
	void togglePauseState();

	// Joystick
	void clearJoystickScripts();
	uint16 joystickGetScriptForDirectionCode(uint16 offset, uint16 flags);
	uint16 joystickButtonIndex(uint16 flags);
	uint16 joystickDirectionIndex(uint16 offset, const uint16 *center, uint16 x, uint16 y);
	uint16 joystickDirectionByDistance(const uint16 *center, uint16 x, uint16 y);
	uint16 joystickDirection8Way(const uint16 *center, uint16 x, uint16 y);
	uint16 joystickGetScriptForPosition(uint16 offset, const uint16 *center, uint16 x, uint16 y);
	void handleJoystickMessage(const Message &message);
	void handleJoystickButtonMessage(const Message &message);
	uint16 joystickMessageToIndex(const Message &message);
	void joystickInit();
	void joystickRelease();

	// Timers and collisions
	uint16 removeNativeTimer(uint16 index);
	void findNativeTimer(uint16 id);
	void addNativeTimer(uint16 id, uint32 interval, uint16 proc, byte repeat);
	void serviceNativeTimerQueue();
	uint16 removeScheduledTimer(uint16 index);
	bool appendCollisionPair();
	bool addCollisionPair(const byte *record);
	uint16 removeCollisionPair(uint16 index);

	// Movies
	uint16 createMovieObject(uint16 id, uint16 argument, byte *data);
	uint16 destroyMovieObject(uint16 handle);
	Movie_v1_2 *movieFromHandle(uint16 handle);
	bool movieBegin(byte *command, uint16 sprite);
	bool movieCheckStartLeadIn(Movie_v1_2 *movie, uint32 now);
	void setMoviesPaused(bool paused);
	void movieMarkFramesSkippable(Movie_v1_2 *movie, uint16 count, uint16 step);
	void movieBlitDeltaRect(byte *movieBitmap, byte *frame);
	static void movieComputeFrameRect(const byte *bitmap, Common::Rect &rect);
	void movieFreePrefetchBuffers(Movie_v1_2 *movie);
	uint16 movieCountContiguousChunks(Movie_v1_2 *movie);
	bool moviePrefetchGroup(Movie_v1_2 *movie);
	bool movieAllocPrefetchBuffers(Movie_v1_2 *movie);
	bool movieLoadFrameTable(Movie_v1_2 *movie);
	bool movieDecodeBitmapFrame(Movie_v1_2 *movie, uint16 *advance);
	bool movieStartAudioStream(Movie_v1_2 *movie);
	bool movieRunScriptEvent(Movie_v1_2 *movie, uint16 *advance);
	void movieAdvanceTimelinePtr(Movie_v1_2 *movie, uint16 advance);
	bool movieStepFrame(Movie_v1_2 *movie);
	bool movieCopyAudioChunk(Movie_v1_2 *movie);
	uint16 movieReadNextBlock(Movie_v1_2 *movie);
	void movieResetPlaybackState(Movie_v1_2 *movie);
	bool movieRestart(Movie_v1_2 *movie);
	uint16 movieStop(Movie_v1_2 *movie);
	void destroyAllMovieObjects();

	// Text objects and their edit
	uint16 createTextObject(uint16 id, const byte *data);
	uint16 destroyTextObject(uint16 handle);
	TextObject_v1_2 *textObjectFromHandle(uint16 handle);
	void createTextObjectFont(TextObject_v1_2 *object);
	uint16 createTextObjectFont(int16 height, const char *face);
	void deleteTextObjectFont(uint16 font);
	void refreshTextObjectFont(TextObject_v1_2 *object);
	void loadGameFonts();
	Graphics::Font *createGameFont(const Common::String &face, int size, bool cell);
	Graphics::ManagedSurface *renderTextToBitmap(uint16 color, const char *text, const Graphics::Font *font);
	Graphics::ManagedSurface *renderTextToBitmap(const TextObject_v1_2 *object);
	byte *buildTextObjectBitmap(TextObject_v1_2 *object);
	byte *createDIBFromBitmapHandle(const Graphics::ManagedSurface *bitmap, bool remap, byte color);
	void positionTextObject(byte *bitmap, const TextObject_v1_2 *object);
	void replaceTextBitmapInSprites(byte *oldBitmap, byte *newBitmap, const TextObject_v1_2 *object);
	void destroyAllTextObjects();
	bool beginTextEdit(const byte *record);
	void commitTextEdit(uint16 id);
	void textEditAppendCaret(uint16 object);
	bool endTextEdit(uint16 object, bool cancel);
	bool textEditBackspace(uint16 object, bool clearAll);
	bool textEditAppendChar(uint16 object, byte character);
	bool refreshTextObjectBitmap(uint16 object);

	// INI file
	Common::String getIniFileName() const;
	void loadIniFile();
	void saveIniFile();
	Common::String makeUniqueBackupFileName() const;
	static bool copyFileBlocks(Common::SeekableReadStream *source, Common::WriteStream *destination);
	uint16 getPrivateProfileString(const char *section, const char *key, char *buffer, uint16 size);
	void writePrivateProfileString(const char *section, const char *key, const char *value);
	static void decodeIniString(byte key, char *string);
	static void encodeIniString(byte key, char *string);
	static uint16 parseIniInteger(const char *text);
	static bool parseIniHexadecimal(const char *text, uint32 &value);
	static void unescapeIniString(char *destination, const char *source);
	static void escapeIniString(char *destination, uint16 size, const char *source);
	void iniReadString(byte obfuscation, uint16 stringId, const char *key, const char *section);
	void iniReadInt(byte obfuscation, uint16 variable, const char *key, const char *section);
	void iniWriteStringEncoded(byte obfuscation, uint16 stringId, const char *key, const char *section);
	void iniWriteInt(byte obfuscation, uint16 variable, const char *key, const char *section);

	// Variables and scenes
	uint16 _variableTable[TG_HOST_V1_2_TABLE_SIZE] = {};
	uint16 _argumentFrame = 0;
	Common::Array<SceneEntry_v1_2> _sceneEntries;
	char _currentSceneName[TG_HOST_V1_2_6_SCENE_NAME_SIZE];
	uint16 _currentSceneIndex = 0;
	uint16 _previousSceneIndex = 0;
	Common::String _containerTitle;
	byte *_resourceNameOffsets = nullptr;
	bool _debugNameTable = false;

	// Startup state
	uint16 _launchMode = 0;
	bool _flagOver256Colors = false;
	byte _audioPresent = 0;
	bool _noFade = false;
	bool _memoryError = false;
	uint16 _fatalCode = 0;
	byte _screenSaverOption = 1;
	uint16 _maxJoySticks = 0;
	uint16 _numJoysticks = 0;
	bool _flagActivated = false;
	bool _cursorApplied = false;
	bool _entryScriptRunning = false;
	bool _deferBitmapLoading = false;
	bool _postSceneEndEnabled = false;
	uint16 _savedCursorMode = 0;
	uint32 _startTick = 0;
	Common::Rect _windowRect;
	Common::String _appTitle;
	Common::Path _resourcePath;

	// Resource state
	byte *_backgroundBitmap = nullptr;
	uint32 _backgroundColor = 0;
	bool _solidBackground = false;
	uint16 _maxFadeColors = 0;
	uint16 _maxTransColors = 0;
	uint16 _solidColorEntry = TG_HOST_V1_2_SOLID_COLOR_ENTRY;
	uint16 _sceneSpriteMidi = 0;
	bool _wipeEffectActive = false;
	byte *_wipeNewBitmap = nullptr;
	byte *_wipeOldBitmap = nullptr;
	uint16 _filePaletteEntryCount = 0;
	TileBackground *_tileMap = nullptr;
	bool _scrollBoxWindowBox = false;

	// Script state
	uint16 _scriptArgStackOffset = 0;
	uint16 _scriptArgCount = 0;
	uint16 _expressionStack[TG_SCRIPT_V1_2_EXPRESSION_STACK_SIZE] = {};
	uint16 _expressionStackPointer = 0;
	uint16 _currentScriptId = 0;
	uint16 _mouseEventScript = 0;
	uint16 _queueDoneScript = 0;
	uint16 _textEditObjectId = 0;
	uint16 _dragSpriteId = 0;
	uint16 _displayZoomMode = 0;
	bool _displayZoomActive = false;
	bool _leftButtonDown = false;
	bool _rightButtonDown = false;

	// Input state
	KeyEntry_v1_2 _keyTable[TG_INPUT_V1_2_4_KEY_TABLE_SIZE];
	Common::Array<ClickRegion_v1_2> _clickRegions;
	uint16 _numClickRegions = 0;
	bool _gamePaused = false;
	bool _keyboardRouted = false;
	bool _mouseAsStick = false;
	uint16 _hoverSpriteScript = 0;
	uint16 _hoverSprite = 0;
	uint16 _keyHookScript = 0;
	uint16 _scriptResult = 0;
	uint16 _pauseScript = 0;
	bool _joystickCaptured = false;
	bool _joystickRouted = false;
	bool _joystickDirectionFlag = false;
	bool _joystickButtonFlag = false;
	uint16 _joystickCenter[TG_JOYSTICK_V1_2_1_NUM_STICKS][2] = {};
	uint16 _joystickX = TG_JOYSTICK_V1_2_1_AXIS_CENTER;
	uint16 _joystickY = TG_JOYSTICK_V1_2_1_AXIS_CENTER;
	byte _joystickButtons = 0;
	uint16 _joystickScripts[TG_JOYSTICK_V1_2_1_NUM_SCRIPTS] = {};
	HoverRegion_v1_2 _hoverRegions[TG_HOST_V1_2_12_MAX_HOVER_REGIONS];
	uint16 _numHoverRegions = 0;
	uint16 _hoverRegionCurrent = 0;
	ScheduledTimer_v1_2 _scheduledTimers[TG_HOST_V1_2_MAX_SCHEDULED_TIMERS];
	uint16 _numScheduledTimers = 0;
	Common::Array<CollisionPair_v1_2> _collisionPairs;
	uint16 _numCollisionPairs = 0;
	int16 _scheduledTimerIndex = 0;
	bool _timersPaused = false;
	bool _moviesPaused = false;
	uint32 _moviePauseStartTime = 0;
	uint16 _numMovies = 0;
	uint16 _displayZoomX = 0;
	uint16 _displayZoomY = 0;
	int16 _dragStartX = 0;
	int16 _dragStartY = 0;
	uint16 _lastRandom = 0;
	bool _rubberBandMode = false;
	bool _captureDrag = false;
	Common::Point _captureDragStart;
	Common::Point _captureDragEnd;
	bool _keyStates[TG_HOST_V1_2_KEY_STATE_SIZE] = {};
	bool _hasPendingChar = false;
	Message _pendingChar;

	// Movie, text object and INI state
	Common::Array<Movie_v1_2 *> _movieObjects;
	uint16 _movieList[TG_HOST_V1_2_MAX_MOVIES] = {};
	Common::Array<TextObject_v1_2 *> _textObjects;
	Common::Array<TextFont_v1_2> _textFonts;
	Common::HashMap<Common::String, Common::Path, Common::IgnoreCase_Hash, Common::IgnoreCase_EqualTo> _gameFonts;
	bool _gameFontsLoaded = false;
	uint16 _nextTextFontHandle = 1;
	char _textEditUndo[TG_TEXT_V1_2_TEXT_SIZE] = {};
	Common::INIFile _iniFile;
	bool _iniFileLoaded = false;

	// Native timers and device groups
	NativeTimer_v1_2_4 _nativeTimers[TG_INPUT_V1_2_3_MAX_NATIVE_TIMERS];
	uint16 _numNativeTimers = 0;
	uint16 _inputTimeout = TG_DEVICE_V1_2_3_COMBO_TIMEOUT;
	KeyRoute_v1_2_4 _keyRoutes[TG_INPUT_V1_2_4_KEY_TABLE_SIZE];
	DeviceGroup_v1_2_4 _deviceGroups[TG_DEVICE_V1_2_3_NUM_GROUPS];
	byte _deviceOwners[TG_DEVICE_V1_2_3_NUM_DEVICES] = {};
	byte _keyboardDirections[TG_DEVICE_V1_2_3_NUM_KEYBOARD_SETS] = {};
	byte _joystickStates[TG_JOYSTICK_V1_2_1_NUM_SCRIPTS] = {};
	byte _joystickLastIndex[TG_JOYSTICK_V1_2_1_NUM_STICKS] = {};
	Common::Rect _mouseStickDeadRect;
	Common::Rect _mouseStickOuterRect;
	uint16 _mouseStickCode = 0;

	// The last two slots of each set are used only from the later version on
	byte _keyboardSetKeys[TG_DEVICE_V1_2_3_NUM_KEYBOARD_SETS][TG_DEVICE_V1_2_6_KEYBOARD_SET_SLOTS] = {
		{ 0x46, 0x56, 0x43, 0x42, 0x31, 0x32 },
		{ 0x26, 0x28, 0x25, 0x27, 0x39, 0x30 }
	};

	byte _keyboardSetCodes[TG_DEVICE_V1_2_3_NUM_KEYBOARD_SETS][TG_DEVICE_V1_2_6_KEYBOARD_SET_SLOTS] = {
		{ 0x10, 0x20, 0x80, 0x40, 0x01, 0x02 },
		{ 0x10, 0x20, 0x80, 0x40, 0x01, 0x02 }
	};

	// Plug-in state
	PluginHost *_pluginHost = nullptr;
	Plugin *_plugin = nullptr;
	Common::Array<PluginFunction_v1_2_4> _pluginFunctions;
	Plugin *_messageFilter = nullptr;
	Plugin *_windowMessageCallback = nullptr;
	Plugin *_scriptHook = nullptr;
	Common::Array<PluginModule_v1_2_12 *> _pluginLibraries;
	uint16 _debugScriptId = 0;

	// Cursor state
	Graphics::Cursor *_cursorImages[TG_HOST_V1_2_4_NUM_CURSOR_IMAGES] = {};
	bool _cursorCounterVisible = true;
	bool _cursorImageHidden = false;
};

} // End of namespace Topgun

#endif // TOPGUN_TOPGUN_V1_2_H
