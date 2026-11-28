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

#ifndef TOPGUN_TOPGUN_V2_0_H
#define TOPGUN_TOPGUN_V2_0_H

#include "topgun/topgun_v1_2_4.h"

// Variables and operands
#define TG_HOST_V2_0_FIRST_FRAME_INDEX             0x159E

// Interpreter
#define TG_SCRIPT_V2_0_TABLE_ADDRESS               0x1328

// Call record
#define TG_SCRIPT_V2_0_CALL_SCRIPT_FLAG            4
#define TG_SCRIPT_V2_0_CALL_ARGUMENT_FLAGS         5
#define TG_SCRIPT_V2_0_CALL_FRAME_SLOTS            6
#define TG_SCRIPT_V2_0_CALL_COUNT                  7
#define TG_SCRIPT_V2_0_CALL_HEADER_SIZE            8
#define TG_SCRIPT_V2_0_CALL_MAX_ARGUMENTS          8
#define TG_SCRIPT_V2_0_CALL_CLEARED_WORD           0x14
#define TG_SCRIPT_V2_0_CALL_MASK_BIT               0x17
#define TG_SCRIPT_V2_0_FORWARDED_RECORD_SIZE       0x16
#define TG_SCRIPT_V2_0_HOST_FRAME_SLOTS            0x20

// Interpreter operations
#define TG_SCRIPT_V2_0_NUM_ELEMENT_VARIABLES       8
#define TG_SCRIPT_V2_0_ARITHMETIC_MAX_OPERANDS     3
#define TG_SCRIPT_V2_0_ARITHMETIC_VARIABLE         5
#define TG_SCRIPT_V2_0_SWITCH_ENTRY_SIZE           8
#define TG_SCRIPT_V2_0_SWITCH_ENTRY_VARIABLE       6
#define TG_SCRIPT_V2_0_WAVE_BLOCK_SIZE             0x16
#define TG_SCRIPT_V2_0_KEY_CTRL                    0x04
#define TG_SCRIPT_V2_0_KEY_SHIFT                   0x08
#define TG_SCRIPT_V2_0_KEY_ENABLE                  0x10
#define TG_SCRIPT_V2_0_KEY_DISABLE                 0x20
#define TG_SCRIPT_V2_0_KEY_UP                      0x40
#define TG_SCRIPT_V2_0_CLICK_ALL                   0x01
#define TG_SCRIPT_V2_0_CLICK_ENABLE                0x02
#define TG_SCRIPT_V2_0_CLICK_DISABLE               0x04
#define TG_SCRIPT_V2_0_HOVER_CLEAR                 0x01
#define TG_SCRIPT_V2_0_LIST_RUN_FAIL_SCRIPT        0x01
#define TG_SCRIPT_V2_0_LIST_ADVANCE                0x02

// Built-ins
#define TG_SCRIPT_V2_0_LAST_BUILTIN_CODE           0x12C
#define TG_SCRIPT_V2_0_WAVE_HAS_VOLUME             0x01
#define TG_SCRIPT_V2_0_WAVE_NOTIFY                 0x02
#define TG_SCRIPT_V2_0_WAVE_NO_FLUSH               0x04

// Text objects
#define TG_TEXT_V2_0_HEADER_HEIGHT_VARIABLE        0x13
#define TG_TEXT_V2_0_EDIT_SCRIPT                   4
#define TG_TEXT_V2_0_EDIT_STRING                   6
#define TG_TEXT_V2_0_EDIT_FLAG_NUMERIC             0x01

// Movies
#define TG_MOVIE_V2_0_COMMAND_RESOLVE_SIZE         8

namespace Topgun {

enum ScriptOpcode_v2_0 {
	kOpV2_0CallScript = 1,
	kOpV2_0Nop = 2,
	kOpV2_0SetActivationScript = 4,
	kOpV2_0GetAngle = 11,
	kOpV2_0StopSprite = 13,
	kOpV2_0SetHoverRegion = 14,
	kOpV2_0SetClickScript = 16,
	kOpV2_0SetSpriteClipBox = 17,
	kOpV2_0SpriteCompositeToBackground = 20,
	kOpV2_0AddCollisionPair = 23,
	kOpV2_0Cosine = 26,
	kOpV2_0CursorMode = 27,
	kOpV2_0SetCursorPosition = 28,
	kOpV2_0DebugString = 30,
	kOpV2_0FlushIni = 35,
	kOpV2_0SetSpriteObjectProperties = 43,
	kOpV2_0BeginTextEdit = 44,
	kOpV2_0GotoScene = 54,
	kOpV2_0Quit = 57,
	kOpV2_0SetQuitSceneName = 58,
	kOpV2_0FadeIn = 60,
	kOpV2_0FadeOut = 61,
	kOpV2_0SetDebugScriptId = 62,
	kOpV2_0GetSpriteRect = 63,
	kOpV2_0GetSpriteElementIds = 64,
	kOpV2_0GetDate = 66,
	kOpV2_0GetIntersect = 72,
	kOpV2_0GetSpritePosition = 77,
	kOpV2_0GetTime = 83,
	kOpV2_0ConditionalRunScript = 89,
	kOpV2_0ConditionalSkip = 90,
	kOpV2_0ConditionalJumpExpression = 91,
	kOpV2_0RunScriptIfLoaded = 92,
	kOpV2_0SetDeviceGroupBinding = 94,
	kOpV2_0DeviceGroupButtonState = 96,
	kOpV2_0SetDeviceGroupScript = 97,
	kOpV2_0GetDeviceGroupState = 99,
	kOpV2_0SetJoystickScript = 101,
	kOpV2_0Jump = 104,
	kOpV2_0GetKeyState = 106,
	kOpV2_0BindKeyScript = 107,
	kOpV2_0CancelScriptTimer = 109,
	kOpV2_0SetSpriteLevel = 113,
	kOpV2_0SetReservedWord = 114,
	kOpV2_0SetSceneEndScript = 116,
	kOpV2_0LoadResource = 117,
	kOpV2_0SetMouseEventScript = 120,
	kOpV2_0SetMute = 122,
	kOpV2_0MoveSprite = 124,
	kOpV2_0SetPauseScript = 138,
	kOpV2_0PlayCDTrack = 142,
	kOpV2_0PlayMidi = 143,
	kOpV2_0PlayWave = 145,
	kOpV2_0PostScript = 149,
	kOpV2_0Random = 153,
	kOpV2_0RandomRunScript = 154,
	kOpV2_0IniRead = 156,
	kOpV2_0ReturnExpression = 161,
	kOpV2_0Stop = 162,
	kOpV2_0BackupIni = 163,
	kOpV2_0SetBackground = 164,
	kOpV2_0RoundRobinRunScript = 167,
	kOpV2_0Arithmetic = 169,
	kOpV2_0Expression = 175,
	kOpV2_0SetPaletteEntries = 179,
	kOpV2_0LoadPaletteResource = 180,
	kOpV2_0SetWallpaper = 183,
	kOpV2_0SetSpritePosition = 184,
	kOpV2_0SetSpriteQueue = 185,
	kOpV2_0FormatString = 192,
	kOpV2_0FormatTextObject = 194,
	kOpV2_0SetTextParam = 195,
	kOpV2_0SetScriptTimer = 196,
	kOpV2_0SetVolume = 198,
	kOpV2_0Sine = 202,
	kOpV2_0StopCD = 210,
	kOpV2_0StopMidi = 211,
	kOpV2_0StopWave = 212,
	kOpV2_0CompareStrings = 213,
	kOpV2_0CompareStringsIgnoreCase = 214,
	kOpV2_0ListRunScript = 217,
	kOpV2_0Switch = 218,
	kOpV2_0SwitchExpression = 219,
	kOpV2_0TransferSprite = 222,
	kOpV2_0UnloadResource = 223,
	kOpV2_0ClipLineToRect = 224,
	kOpV2_0IsSpriteBusy = 225,
	kOpV2_0ConditionalJumpExpressionCopy = 227,
	kOpV2_0IniWrite = 228,
	kOpV2_0SetDisplayZoom = 230
};

class TopgunEngine_v2_0 : public TopgunEngine_v1_2_4 {
	friend class EnginePluginHost;
	friend class TopgunEngine_v1_2;
	friend class TopgunEngine_v1_2_4;

public:
	TopgunEngine_v2_0(OSystem *syst, const ADGameDescription *gameDesc, EngineVersion version = kVersionV2_0);

protected:
	// Interpreter
	bool executeScriptOp(byte *ip, uint16 *advance) override;
	uint16 getScriptOpSize(const byte *ip) override;
	bool callScriptWithArguments(const byte *record);
	uint16 resolveScriptOperand(uint16 value, byte flag) const;
	void resolveScriptRect(byte *rect, byte flags) const;
	void resolveRecordOperandWord2(byte *record) const;

	// Opcodes
	bool o_callScript(const byte *record) override; // 1
	void o2_setActivationScript(const byte *record); // 4
	void o2_getAngle(const byte *record); // 11
	void o2_stopSprite(const byte *record); // 13
	void o2_setHoverRegion(const byte *record); // 14
	bool o2_setClickScript(const byte *record); // 16
	void o2_spriteCompositeToBackground(const byte *record); // 20
	bool o2_addCollisionPair(const byte *record); // 23
	void o2_sineCosine(const byte *record); // 26, 202
	void o2_setCursorPosition(const byte *record); // 28
	void o2_debugString(const byte *record); // 30
	bool o2_setSpriteObjectProperties(const byte *record); // 43
	bool o2_beginTextEdit(const byte *record); // 44
	void o2_getSpriteRect(const byte *record); // 63
	void o2_getSpriteElementIds(const byte *record); // 64
	void o2_getIntersect(const byte *record); // 72
	void o2_getSpritePosition(const byte *record); // 77
	bool o2_conditionalRunScript(const byte *record, uint16 &size); // 89
	uint16 o2_conditionalSkip(const byte *record); // 90
	bool o2_runScriptIfLoaded(const byte *record); // 92
	void o2_setDeviceGroupBinding(const byte *record); // 94
	void o2_deviceGroupButtonState(const byte *record); // 96
	void o2_setDeviceGroupScript(const byte *record); // 97
	void o2_getDeviceGroupState(const byte *record); // 99
	void o2_setJoystickScript(const byte *record); // 101
	void o2_getKeyState(const byte *record); // 106
	void o2_bindKeyScript(const byte *record); // 107
	void o2_setSpriteLevel(const byte *record); // 113
	void o2_setSceneEndScript(const byte *record); // 116
	bool o2_loadUnloadResource(const byte *record); // 117, 223
	void o2_setMouseEventScript(const byte *record); // 120
	void o2_moveSprite(const byte *record); // 124
	void o2_setPauseScript(const byte *record); // 138
	void o2_playMidi(const byte *record); // 143
	bool o2_playWave(const byte *record); // 145
	void o2_postScript(const byte *record); // 149
	void o2_random(const byte *record); // 153
	bool o2_randomRunScript(byte *record); // 154
	void o2_setBackground(const byte *record); // 164
	bool o2_roundRobinRunScript(byte *record); // 167
	void o2_arithmetic(const byte *record); // 169
	void o2_setPaletteEntries(const byte *record); // 179
	void o2_loadPaletteResource(const byte *record); // 180
	bool o2_setSpritePosition(const byte *record); // 184
	bool o2_setSpriteQueue(const byte *record); // 185
	void o2_formatString(const byte *record); // 192
	bool o2_formatTextObject(const byte *record); // 194
	void o2_setTextParam(const byte *record); // 195
	void o2_setScriptTimer(const byte *record); // 196
	void o2_stopMidi(const byte *record); // 211
	void o2_stopWave(const byte *record); // 212
	void o2_listRunScript(const byte *record); // 217
	void o2_transferSprite(const byte *record); // 222
	void o2_clipLineToRect(const byte *record); // 224
	void o2_isSpriteBusy(const byte *record); // 225
	void o2_setDisplayZoom(const byte *record); // 230
};

} // End of namespace Topgun

#endif // TOPGUN_TOPGUN_V2_0_H
