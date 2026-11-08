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
#include "engines/grim/sputcostume_components.h"

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
	SputCostume(Res *pRes, RenderDroid *pRd, SputResource *pSputResource, SputRender *pSputRender, SputRoom *pSputRoom, StdLbm *pStdLbm, WuRegistry *pWuRegistry);

	void setGrimEngine(GrimEngine *pGrim) { _pGrim = pGrim; }
	void setLua(Lua_V1 *pLua) { _pLua = pLua; }
	void setActors(SputActor *aActors) { _aActors = aActors; }
	void setSputActor(SputActorModule *pSputActor) { _pSputActor = pSputActor; }
	Cos *getCos() { return &_cos; }
	RdColormap *getShadowColormap() const { return _pShadowColormap; }
	void setShadowColormap(RdColormap *pColormap) { _pShadowColormap = pColormap; }

	void setShadowColor(int red, int green, int blue);
	void printCostumeInfo(CostumeNode *pNode);
	void freeCostumeNodes(CostumeNode *pNode, int bFreeInstance);
	CostumeNode *popCostume(CostumeNode *pNode, int bFreeInstance);
	CostumeNode *setCostume(CostumeNode *pNode, int hCostume, int hInstance);
	CostumeNode *pushCostume(CostumeNode *pPrev, int hCostume, int hInstance);
	RdThing *getMainThing(int hInst);
	void update(CostumeNode *pNode, float delta);
	int initialize(SysStruct *pHS);
	void terminate();
	int startup();
	int shutdown();
	int saveHook(const char *path, int hCostume, const char *header);
	int instanceHook(int hInst);
	void puppetHook(ModelInst *pModel, int trackNum, int markerType);
	void fadeKeysIn(int hInst, int choreIndex, int timeMs);
	void fadeKeysOut(int hInst, int choreIndex, int timeMs);
	void getDataPath(const char *valueName, char *pData, const char *defaultDirectory);
	void writeRegistryValue(const char *valueName, const char *pData);
	void nullSub1();
	int stub();

	static void puppetHookCallback(void *userArg, int trackNum, int markerType);

private:
	Res *_pRes;
	RenderDroid *_pRd;
	SputResource *_pSputResource;
	SputRender *_pSputRender;
	SputRoom *_pSputRoom;
	StdLbm *_pStdLbm;
	WuRegistry *_pWuRegistry;
	GrimEngine *_pGrim;
	Lua_V1 *_pLua = nullptr;
	SputActorModule *_pSputActor;
	SputActor *_aActors;
	SysStruct *_pHS;
	Cos _cos;

	int _bInitialized;
	int _bStarted;
	RdColormap *_pShadowColormap;
	CosStateBitmapDesc *_aLECBitmaps;
	void *_pLastBitmap;
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
