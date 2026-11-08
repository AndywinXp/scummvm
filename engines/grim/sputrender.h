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

#ifndef GRIM_SPUTRENDER_H
#define GRIM_SPUTRENDER_H

#include "common/scummsys.h"

#include "engines/grim/res.h"
#include "engines/grim/sputcontrol.h"
#include "engines/grim/std/std_display.h"

namespace Common {
class ReadStream;
class WriteStream;
}

namespace Grim {

class BlastText;
class GrimEngine;
class LegacyImporter;
class Lua_V1;
class RenderDroid;
class SputActorModule;
class SputPrimitive;
class SputResource;
class SputRoom;
class SputSmush;
class SputText;
class SputTime;
class Std3D;
class StdControl;
class WalkSystem;
struct RdCanvas;
struct Room;
struct RoomCamera;
struct RoomSetup;
struct SputActor;

typedef int (*SputRenderPaletteFunc)(byte *, int);
typedef int (*SputRenderPageFlipFunc)();

struct DirtyRect {
	DirtyRect *pNext;
	DirtyRect *pPrev;
	int page;
	int serial;
	uint pageIndex;
	StdRect rect;
	DirtyRect *pNextFree;
};

struct CacheRect {
	int magic;
	StdRect rect;
	StdVBuffer *pColor;
	StdVBuffer *pZ;
	CacheRect *pNext;
};

struct ActorSortInfo {
	char kind;
	float depth;
	int actorIndex;
};

class SputRender : public SputControlCallback {
	friend class LegacyImporter;

public:
	SputRender(RenderDroid *pRd, StdDisplay *pStdDisplay, StdBitmap *pStdBitmap, Res *pRes, SputResource *pSputResource,
			SputPrimitive *pSputPrimitive, WalkSystem *pWalkSystem, SputRoom *pSputRoom, StdVBuffer *pBackBuffer, StdVBuffer *pMainGfxBuffer,
			SputTime *pSputTime, SputControl *pSputControl);

	void setActors(SputActor *aActors) { _aActors = aActors; }
	void setSputActor(SputActorModule *pSputActor) { _pSputActor = pSputActor; }
	void setSputText(SputText *pSputText) { _pSputText = pSputText; }
	void setBlastText(BlastText *pBlastText) { _pBlastText = pBlastText; }
	void setSputSmush(SputSmush *pSputSmush) { _pSputSmush = pSputSmush; }
	void setStdControl(StdControl *pStdControl) { _pStdControl = pStdControl; }
	void setStd3D(Std3D *pStd3D) { _pStd3D = pStd3D; }
	void setGrimEngine(GrimEngine *pGrim) { _pGrim = pGrim; }
	void setLua(Lua_V1 *pLua) { _pLua = pLua; }

	int initialize(SysStruct *pHS);
	int terminate();
	int startup();
	int shutdown();
	int save(Common::WriteStream *pStream);
	int restore(Common::ReadStream *pStream);
	int open();
	void close();
	int setDisplay(StdVBuffer *pVBuffer, StdVBuffer *pZBuffer, int xStart, int yStart, int xEnd, int yEnd, int *pOptions);
	int queryDisplayState();
	void using3DHardware(int b3D);
	int is3DHardware() const { return _b3DAcceleration; }
	void pageCountIs(int numPages);
	void setPaletteCallback(SputRenderPaletteFunc pFunc);
	void setPageFlipCallback(SputRenderPageFlipFunc pFunc);
	SputRenderPaletteFunc getPaletteFunc();
	void setPageFlip(int enable);
	int pageFlip();
	int setRenderOptions(const int *pOptions);
	int controlCallback(float frameTime) override;
	void forceRefresh();

	void markDirtyRect(const StdRect *pRect, int page);
	void markAllPagesDirty(const StdRect *pRect, int page);
	void freeCacheRects();
	CacheRect *createCacheRect(int x, int y, int width, int height);
	void restoreCacheRect();
	void freeBuffers();

	StdVBuffer *drawBuffer();
	StdVBuffer *cleanArtBuffer();
	StdVBuffer *cleanZBuffer();
	int copyGFXBuffer();
	int cleanDrawBuffer();
	void fillDrawBuffer(StdVBuffer *pSrc);
	StdVBuffer *screenShot(int width, int height);
	void saveScreen(const char *path);
	void saveZBuffer(const char *path);
	void saveDrawBuffer(const char *path);

	void buildGammaTable(const StdColorFormat *pFormat, float gamma);
	void colorCorrect(uint16 *pColors, int count);
	void setGamma(float gamma);
	void dim(int bForce, char shift);
	void dimRegion(int x, int y, int width, int height, float scale);

	void getIris(float percent, int x, int y);
	void irisDown(int x, int y, int duration);
	void irisUp(int x, int y, int duration);
	int irisComplete();
	void clearIris();

	void drawProgressBar(float progress);
	void allowLoadImage(int enable);
	void drawLoadingImage();

	void drawDebuggingToggle();
	void syncCamera(Room *pRoom, int setup);
	void useSetup(Room *pRoom, int setup);
	void update();
	int loadSetupBackground(const StdColorFormat *pFormat);
	void preRendered(int bBitmap, int bZBuffer);
	void reverseProject(int x, int y, float *pOut);
	int buildShadowBitmap(float *pOut, float *pA, float *pB, float a, float b, float c, float d);
	void restoreDirtyRects(StdVBuffer *pDst, StdVBuffer *pZBuffer);
	void drawPrimitives(int layerMask);
	void sortActors();
	void resetActorSortList();
	ActorSortInfo *allocActorSortEntry();
	static int compareActorDepth(const void *pA, const void *pB);
	void drawActor(SputActor *pActor);
	void drawActorDebug(SputActor *pActor);
	void drawCameraDebug(Room *pRoom);
	void drawActorBoxes(Room *pRoom, uint setup);
	void drawActorBox(RoomSetup *pSetup);
	void drawBoundingBox();
	void drawWalkPlanes();

	StdVBuffer *getCleanBuffer() const { return _pCleanBuffer; }
	uint16 *getColors() { return _aColors; }
	void setSkipPrimitives(int bSkip) { _bSkipPrimitives = bSkip; }
	int getCamActorIndex() const { return _gCamActorIndex; }
	RoomCamera *getCamera() const { return _pCamera; }
	const uint16 *getColorLookup() const { return _colorLookup; }

private:
	void markDirtyRectInternal(StdRect *pRect, int page, uint pageIndex);

	RenderDroid *_pRd;
	StdDisplay *_pStdDisplay;
	StdBitmap *_pStdBitmap;
	Res *_pRes;
	SputResource *_pSputResource;
	SputPrimitive *_pSputPrimitive;
	WalkSystem *_pWalkSystem;
	SputRoom *_pSputRoom;
	StdVBuffer *_pBackBuffer;
	StdVBuffer *_pMainGfxBuffer;

	SputActorModule *_pSputActor;
	SputActor *_aActors;
	SputText *_pSputText;
	BlastText *_pBlastText;
	SputSmush *_pSputSmush;
	SputTime *_pSputTime;
	SputControl *_pSputControl;
	StdControl *_pStdControl;
	Std3D *_pStd3D = nullptr;
	GrimEngine *_pGrim;
	Lua_V1 *_pLua = nullptr;

	SysStruct *_pHS;
	int _bInitialized;
	int _bStarted;

	int _renderVars[7];
	uint16 _colorLookup[65536];
	float _gammaCorrection;
	StdColorFormat *_pColorInfo;
	int _b3DAcceleration;
	int _bAcceleratedZBuffers;

	StdVBuffer *_pDrawBuffer;
	StdVBuffer *_pZBuffer;
	RdCanvas *_pCanvas;
	StdVBuffer *_pCleanBuffer;
	StdVBuffer *_pCleanArtBuffer;
	StdVBuffer *_pCleanZBuffer;
	int _pDecompressionBuffer;

	int _numPages;
	DirtyRect *_pDirtyRectQueue;
	DirtyRect *_pFreeDirtyRects;
	int _numDirtyRects;
	int _nextSerialNumber;
	CacheRect *_pCacheRectList;
	int _reset;

	int _irisStart;
	float _irisDuration;
	int _irisCenterX;
	int _irisCenterY;
	int _renderFlags;
	int _bSkipPrimitives;

	SputRenderPaletteFunc _pSetPaletteFunc;
	SputRenderPageFlipFunc _pPageFlipFunc;
	int _pageFlipEnable;
	int _movieJustFinished;
	int _gCamActorIndex;

	int _debugFlags;
	int _bDebugKey1Down;
	int _bDebugKey2Down;
	int _bDebugKey4Down;
	int _bDebugKey8Down;
	int _bDebugKey10Down;

	int _renderEnable;
	int _loadImageAllowed;
	int _perfCounter;
	int _waitBitmapTimerMs;
	int _waitBitmapFrame;
	ResRef _waitBitmapRef;

	float _ambient;
	int _curView;
	Room *_pLastRoom;
	RoomCamera *_pCamera;
	uint16 _aColors[24];
	uint16 _dirtyRectColor;
	int _bPrintLuaStack;
	int _numSortActors;
	ActorSortInfo _aActorSortInfo[128];
	ActorSortInfo *_aSortedActors[128];
};

} // End of namespace Grim

#endif
