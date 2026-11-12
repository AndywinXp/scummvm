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

#ifndef GRIM_SPUTCOSTUME_H
#define GRIM_SPUTCOSTUME_H

#include "common/scummsys.h"

#include "engines/grim/chore/cos.h"
#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/res.h"
#include "engines/grim/costume/sputcostume_components.h"

namespace Grim {

class GrimEngine;
class Lua_V1;
class SputRender;
class SputResource;
class SputActorModule;
class SputRoom;
class StdLbm;
class WuRegistry;
struct CostumeNode;
struct SputActor;

class SputCostume {
	friend class BitmapHandler;
	friend class AnimationHandler;
	friend class ColormapHandler;
	friend class MaterialHandler;
	friend class ThingHandler;
	friend class MeshHandler;
	friend class SpriteHandler;
	friend class KeyframeHandler;
	friend class DemoKeyframeHandler;
	friend class SoundHandler;
	friend class DemoSoundHandler;
	friend class SmushHandler;
	friend class DemoSmushHandler;
	friend class IMuseHandler;
	friend class LuaVarHandler;

public:
	SputCostume(Res *res, RenderDroid *rd, SputResource *sputResource, SputRender *sputRender, SputRoom *sputRoom, StdLbm *stdLbm, WuRegistry *wuRegistry);

	void setGrimEngine(GrimEngine *grim) { _grim = grim; }
	void setLua(Lua_V1 *lua) { _lua = lua; }
	void setActors(SputActor *actors) { _actors = actors; }
	void setSputActor(SputActorModule *sputActor) { _sputActor = sputActor; }
	Cos *getCos() { return &_cos; }
	RdColormap *getShadowColormap() const { return _shadowColormap; }
	void setShadowColormap(RdColormap *colormap) { _shadowColormap = colormap; }

	void setShadowColor(int red, int green, int blue);
	void printCostumeInfo(CostumeNode *node);
	void freeCostumeNodes(CostumeNode *node, int freeInstance);
	CostumeNode *popCostume(CostumeNode *node, int freeInstance);
	CostumeNode *setCostume(CostumeNode *node, int costume, int instance);
	CostumeNode *pushCostume(CostumeNode *prev, int costume, int instance);
	RdThing *getMainThing(int inst);
	void update(CostumeNode *node, float delta);
	void resetCostume(int inst);
	int initialize(SysStruct *hs);
	void terminate();
	int startup();
	int shutdown();
	int saveHook(const char *path, int costume, const char *header);
	int instanceHook(int inst);
	void puppetHook(ModelInst *model, int trackNum, int markerType);
	void fadeKeysIn(int inst, int choreIndex, int timeMs);
	void fadeKeysOut(int inst, int choreIndex, int timeMs);
	void getDataPath(const char *valueName, char *data, const char *defaultDirectory);
	void writeRegistryValue(const char *valueName, const char *data);
	void nullSub1();
	int stub();

	static void puppetHookCallback(void *userArg, int trackNum, int markerType);

private:
	Res *_res;
	RenderDroid *_rd;
	SputResource *_sputResource;
	SputRender *_sputRender;
	SputRoom *_sputRoom;
	StdLbm *_stdLbm;
	WuRegistry *_wuRegistry;
	GrimEngine *_grim;
	Lua_V1 *_lua = nullptr;
	SputActorModule *_sputActor;
	SputActor *_actors;
	SysStruct *_hs;
	Cos _cos;

	int _initialized;
	int _started;
	RdColormap *_shadowColormap;
	CosStateBitmapDesc *_lecBitmaps;
	void *_lastBitmap;
	CosInstanceHook _oldInstanceHook;
	CosSaveHook _oldSaveHook;
	char _keyframePath[128];
	char _colormapPath[128];
	char _materialPath[128];
	char _modelPath[128];
	char _soundPath[128];
	char _smushPath[128];
	char _bitmapPath[128];

	ThingHandler _thingHandler;
	ColormapHandler _colormapHandler;
	MeshHandler _meshHandler;
	MaterialHandler _materialHandler;
	KeyframeHandler _keyframeHandler;
	DemoKeyframeHandler _demoKeyframeHandler;
	BitmapHandler _bitmapHandler;
	AnimationHandler _animationHandler;
	SoundHandler _soundHandler;
	DemoSoundHandler _demoSoundHandler;
	SmushHandler _smushHandler;
	DemoSmushHandler _demoSmushHandler;
	SpriteHandler _spriteHandler;
	IMuseHandler _imuseHandler;
	LuaVarHandler _luaVarHandler;
};

} // End of namespace Grim

#endif
