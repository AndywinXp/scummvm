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

#ifndef TOPGUN_TOPGUN_V1_1_H
#define TOPGUN_TOPGUN_V1_1_H

#include "topgun/topgun_v1_0.h"

#define TG_HOST_V1_1_MAX_SCRIPT_DEPTH 30

namespace Topgun {

enum ScriptOpcodeV1_1 {
	kOpV1_1CursorMode = 31,
	kOpV1_1RunScriptIfLoaded = 32,
	kOpV1_1SetSpriteCollision = 33,
	kOpV1_1CallPluginFunction = 34,
	kOpV1_1RunListEntry = 35,
	kOpV1_1IniVariable = 36
};

class TopgunEngine_v1_1 : public TopgunEngine_v1_0 {
public:
	TopgunEngine_v1_1(OSystem *syst, const ADGameDescription *gameDesc, int version = kVersionV1_1);
	~TopgunEngine_v1_1() override;

protected:
	// Main loop
	void initCDAudio() override;
	uint16 mainLoop() override;
	void resourceLoadProgressCallback() override;

	// Messages
	void handleActivateMessage(uint16 param) override;
	void handleSceneEndMessage() override;

	// Input
	void handleKeyDownMessage(Common::KeyCode keycode, bool shift, bool ctrl) override;
	void handleMouseButtonDown(int16 x, int16 y) override;

	// Cursor
	void applyCursorFromRegister() override;
	virtual void setCursorMode(byte mode);

	// Scenes
	void switchToNextScene(const char *name) override;
	bool loadInitialScene(const Common::String &name) override;
	void cleanupCurrentSceneAndContainer() override;
	virtual void destroySceneMainObjects(bool release);

	// Resources
	void freeBitmapBlobIfUnreferenced(uint16 id) override;
	bool destroySpriteResourceObject(uint16 id) override;
	bool loadTransPaletteResource(uint16 id) override;
	byte *loadResourceWithProcessingPause(uint16 id, uint32 *sizeOut) override;
	bool validateAndGetResourceByType(uint16 id, uint16 type) override;
	bool isResourceObjectLoaded(uint16 id) override;

	// Transitions
	void paintFrameBandsTopBottom(const Common::Rect &outer, const Common::Rect &inner) override;
	void advanceSceneFadeStep() override;
	void sceneTransitionIn() override;
	void sceneTransitionOut() override;

	// Scripts
	bool runScriptBytecodeLoop(uint16 scriptId) override;
	bool runScriptResource(uint16 scriptId) override;
	bool runOpcode(byte *ip, uint16 &size);
	uint16 *flagSlot(uint16 flag);
	uint16 readFlag(uint16 flag);

	// Opcodes
	void o_playCDTrack(uint16 track) override; // 9
	void o_setFlag(const byte *record); // 16
	bool o_testFlag(const byte *record); // 17
	bool o_randomRunScript(byte *record) override; // 22
	bool o_roundRobinRunScript(byte *record) override; // 23
	bool o_setSpriteCollision(const byte *record); // 33
	bool o_runListEntry(const byte *record); // 35
	void o_iniVariable(const byte *record); // 36

	byte _cursorMode = 1;
	bool _scriptsDisabled = false;
	bool _releaseOnPalette = false;
	bool _altTab = false;
};

} // End of namespace Topgun

#endif // TOPGUN_TOPGUN_V1_1_H
