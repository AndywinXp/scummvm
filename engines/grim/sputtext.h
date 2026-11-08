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

#ifndef GRIM_SPUTTEXT_H
#define GRIM_SPUTTEXT_H

#include "common/scummsys.h"

#include "engines/grim/blasttext.h"
#include "engines/grim/lua/lua.h"
#include "engines/grim/std/std_display.h"
#include "engines/grim/std/std_sys.h"

namespace Common {
class ReadStream;
class WriteStream;
}

namespace Grim {

class GrimEngine;
class LegacyImporter;
class Lua_V1;
class NormalText;
class Polyglot;
class RenderDroid;
class Res;
class SputRender;
class SputRoom;
class SputTime;
class Text;
struct SputActor;

struct SputTextEvent {
	SputTextEvent *pNext;
	SputTextEvent *pPrev;
	char text[512];
	TextParams params;
	int x;
	int y;
	StdRect clip;
	int duration;
	SputActor *pActor;
	int flags;
	int serial;
};

class SputText {
public:
	SputText(Res *pRes, RenderDroid *pRd, NormalText *pNormalText, Text *pText, BlastText *pBlastText, SputTime *pSputTime, Polyglot *pPolyglot);

	void setSputRender(SputRender *pSputRender) { _pSputRender = pSputRender; }
	void setSputRoom(SputRoom *pSputRoom) { _pSputRoom = pSputRoom; }
	void setGrimEngine(GrimEngine *pGrim) { _pGrim = pGrim; }
	void setLua(Lua_V1 *pLua) { _pLua = pLua; }

	void killEvent(SputTextEvent *pEvent);
	void update(int bForce);
	int initialize(SysStruct *pHS);
	void pause();
	void resume();
	SputTextEvent *createActorText(SputActor *pActor, const char *pText, int x, int y);
	SputTextEvent *addToQueue(SputTextEvent *pEvent, int duration, int flags);
	int release();
	int startup();
	int shutdown();
	SputTextEvent *allocEvent(const TextParams *pParams, const char *pText);
	const char *extractSayLineData(const char *msgId, char *translation);
	void drawTextEvent(SputTextEvent *pEvent, int bMarkDirty);
	SputTextEvent *findSerialNumber(int serial);
	int getTextObjectParams(TextParams *pParams, lua_Object tableObj, SputTextEvent *pEvent);
	void parseSayLineTable(lua_Object tableObj, int *pBackground, int *pVolume, int *pPan, int *pX, int *pY);
	TextParams *getDefaultTextParams() { return &_defaultTextParams; }
	int save(Common::WriteStream *pStream);
	int restore(Common::ReadStream *pStream);

private:
	friend class Lua_V1;
	friend class LegacyImporter;

	Res *_pRes;
	RenderDroid *_pRd;
	NormalText *_pNormalText;
	Text *_pText;
	BlastText *_pBlastText;
	SputTime *_pSputTime;
	Polyglot *_pPolyglot;
	SputRender *_pSputRender;
	SputRoom *_pSputRoom;
	GrimEngine *_pGrim;
	Lua_V1 *_pLua = nullptr;

	SysStruct *_pHS;
	int _bInitialized;
	int _bStarted;
	int _bLocked;

	SputTextEvent *_pTextQueue;
	BlastTextFont *_pFont;
	BlastTextFont *_pEmergencyFont;
	int _translationMode;

	int _centerValue;
	int _offscreenTextPos;
	int _textPosX;
	int _textPosY;

	StdRect _printLineClip;
	TextParams _printLineParams;
	TextParams _defaultTextParams;
	TextParams _blastTextParams;

	int _luaTextObjXRef;
	int _luaTextObjYRef;
	int _luaTextObjFontRef;
	int _luaTextObjWidthRef;
	int _luaTextObjHeightRef;
	int _luaTextObjFGColorRef;
	int _luaTextObjBGColorRef;
	int _luaTextObjFXColorRef;
	int _luaTextObjHIColorRef;
	int _luaTextObjDurationRef;
	int _luaTextObjCenterRef;
	int _luaTextObjLJustifyRef;
	int _luaTextObjRJustifyRef;
	int _luaTextObjVolumeRef;
	int _luaTextObjBufferRef;
	int _luaTextObjBackgroundRef;
};

} // End of namespace Grim

#endif
