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

#include "engines/grim/grim.h"
#include "engines/grim/lua_v1.h"
#include "engines/grim/sputprimitive.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputsmush.h"
#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_bitmap.h"
#include "engines/grim/std/std_display.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

static int getScreenColor(int32 color) {
	const StdColorFormat *pFormat = g_grim->getColorInfo();
	uint value = (uint)color;
	uint red = (value & 0xFF) >> pFormat->redLoss << pFormat->redShift;
	uint green = ((value >> 8) & 0xFF) >> pFormat->greenLoss << pFormat->greenShift;
	uint blue = ((value >> 16) & 0xFF) >> pFormat->blueLoss << pFormat->blueShift;

	return (int)((uint16)green | (uint16)blue | (uint16)red);
}

bool Lua_V1::isImage(lua_Object obj) {
	return lua_isuserdata(obj) && lua_tag(obj) == MKTAG('F','U','B','V');
}

int Lua_V1::findImage(lua_Object obj) {
	int handle = lua_getuserdata(obj);

	for (int i = 0; i < kMaxImages; i++) {
		if (_images[i] && _imageHandles[i] == handle)
			return i;
	}

	warning("Lua_V1::findImage(): invalid image handle %d", handle);

	return -1;
}

void Lua_V1::pushImage(StdVBuffer *pImage) {
	for (int i = 0; i < kMaxImages; i++) {
		if (!_images[i]) {
			_images[i] = pImage;
			_imageHandles[i] = _nextImageHandle++;
			lua_pushusertag(_imageHandles[i], MKTAG('F','U','B','V'));
			return;
		}
	}

	warning("Lua_V1::pushImage(): too many images");

	g_grim->_pStdDisplay->vBufferFree(pImage);
	lua_pushnil();
}

void Lua_V1::GetImage() {
	lua_Object nameObj = lua_getparam(1);

	if (lua_isstring(nameObj)) {
		Common::String path(lua_getstring(nameObj));
		StdBitmapStruct *pBitmap = g_grim->_pStdBitmap->loadImage(path, 0, 0);

		if (pBitmap) {
			if (!pBitmap->apFrames || pBitmap->numFrames == 0 || !pBitmap->apFrames[0]) {
				warning("Lua_V1::GetImage(): the image has no frames");
				g_grim->_pStdBitmap->unloadBitmap(pBitmap);
				lua_pushnil();
				return;
			}

			StdVBuffer *pImage = pBitmap->apFrames[0];

			pImage->colorKey = 63519;
			pBitmap->apFrames[0] = nullptr;
			g_grim->_pStdBitmap->unloadBitmap(pBitmap);

			if (pImage->tex.format.bpp == 16) {
				g_grim->_pSputRender->colorCorrect((uint16 *)pImage->pPixels, pImage->tex.width * pImage->tex.height);
				g_grim->_pSputRender->colorCorrect((uint16 *)&pImage->colorKey, 1);
			} else {
				warning("Lua_V1::GetImage(): the image is not 16 bpp");
			}

			pushImage(pImage);
			return;
		}
	}

	lua_pushnil();
}

void Lua_V1::FreeImage() {
	lua_Object imageObj = lua_getparam(1);

	if (isImage(imageObj)) {
		int slot = findImage(imageObj);

		if (slot >= 0) {
			g_grim->_pStdDisplay->vBufferFree(_images[slot]);
			_imageHandles[slot] = 0;
		}
	}
}

void Lua_V1::BlastImage() {
	lua_Object imageObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object modeObj = lua_getparam(4);
	char bColorKey = (char)(lua_isnil(modeObj) == 0);

	if (isImage(imageObj) && lua_isnumber(xObj) && lua_isnumber(yObj)) {
		int slot = findImage(imageObj);

		if (slot >= 0) {
			int x = luaFtol(lua_getnumber(xObj));
			int y = luaFtol(lua_getnumber(yObj));
			StdVBuffer *pDst = g_grim->_pSputRender->drawBuffer();

			if (!pDst) {
				warning("Lua_V1::BlastImage(): the display is not set");
				return;
			}

			g_grim->_pStdDisplay->vBufferCopy(pDst, _images[slot], x, y, nullptr, bColorKey);
		}
	}
}

void Lua_V1::CleanBuffer() {
	g_grim->_pSputRender->cleanDrawBuffer();
}

void Lua_V1::StartFullscreenMovie() {
	lua_Object nameObj = lua_getparam(1);
	lua_Object loopObj = lua_getparam(2);

	if (lua_isstring(nameObj) && g_grim->playMovie(lua_getstring(nameObj), 1, !lua_isnil(loopObj), -1, -1) == 1)
		pushbool(true);
	else
		pushbool(false);
}

void Lua_V1::StartMovie() {
	lua_Object nameObj = lua_getparam(1);
	lua_Object loopObj = lua_getparam(2);
	lua_Object xObj = lua_getparam(3);
	lua_Object yObj = lua_getparam(4);

	if (lua_isstring(nameObj)) {
		g_imuse->soundsFloodMusicBuffer();

		int y = lua_isnumber(yObj) ? luaFtol(lua_getnumber(yObj)) : -1;
		int x = lua_isnumber(xObj) ? luaFtol(lua_getnumber(xObj)) : -1;

		if (g_grim->playMovie(lua_getstring(nameObj), 0, !lua_isnil(loopObj), x, y) == 1) {
			pushbool(true);
			return;
		}
	}

	pushbool(false);
}

void Lua_V1::StartFullscreenMovieDemo() {
	lua_Object nameObj = lua_getparam(1);

	if (!lua_isstring(nameObj)) {
		warning("StartMovieFullScreen: Argument must be a string.");
		lua_pushnil();
		return;
	}

	if (g_grim->playMovie(lua_getstring(nameObj), 1, 0, -1, -1) != 1)
		warning("Start Movie:%s FAIL", lua_getstring(nameObj));

	lua_pushnumber(1.0f);
}

void Lua_V1::StartMovieDemo() {
	lua_Object nameObj = lua_getparam(1);

	if (!lua_isstring(nameObj)) {
		warning("StartMovie: Argument must be a string.");
		lua_pushnil();
		return;
	}

	if (g_grim->playMovie(lua_getstring(nameObj), 0, 0, -1, -1) != 1)
		warning("Start Movie:%s FAIL", lua_getstring(nameObj));

	lua_pushnumber(1.0f);
}

void Lua_V1::IsFullscreenMoviePlaying() {
	pushbool(g_grim->_pSputSmush->moviePlaying() != 0);
}

void Lua_V1::IsFullscreenMoviePlayingDemo() {
	pushbool(g_grim->getLibMode() == 3);
}

void Lua_V1::IsMoviePlaying() {
	pushbool(g_grim->_pSputSmush->moviePlaying() != 0);
}

void Lua_V1::StopMovie() {
	g_grim->_pSputSmush->stopMovie();
}

void Lua_V1::PauseMovie() {
	g_grim->_pSputSmush->pause(lua_isnil(lua_getparam(1)));
}

void Lua_V1::PurgePrimitiveQueue() {
	PrimitiveQueue *pQueue = g_grim->_pSputPrimitive->getCurrent();

	pQueue->purge();
	pQueue->resetId();
}

void Lua_V1::DrawPolygon() {
	PrimitiveQueue *pQueue = g_grim->_pSputPrimitive->getCurrent();
	int layer = 2;
	int color = 0;
	lua_Object pointsObj = lua_getparam(1);
	lua_Object tableObj = lua_getparam(2);

	if (!lua_istable(pointsObj)) {
		lua_pushnil();
		return;
	}

	if (lua_istable(tableObj)) {
		lua_pushobject(tableObj);
		lua_pushstring("color");
		lua_Object colorObj = lua_gettable();
		if (lua_isuserdata(colorObj) && lua_tag(colorObj) == MKTAG('R','L','O','C'))
			color = getScreenColor(lua_getuserdata(colorObj));

		lua_pushobject(tableObj);
		lua_pushstring("layer");
		lua_Object layerObj = lua_gettable();
		if (lua_isnumber(layerObj))
			layer = luaFtol(lua_getnumber(layerObj));
	}

	int numPoints = 0;
	int index = 1;

	while (numPoints < 250) {
		lua_pushobject(pointsObj);
		lua_pushnumber((float)index);

		if (lua_isnil(lua_gettable()))
			break;

		index += 2;
		numPoints++;
	}

	if (numPoints <= 0) {
		lua_pushnil();
		return;
	}

	int *pPoints = new int[numPoints * 2]();

	index = 1;
	for (int i = 0; i < numPoints; i++) {
		lua_pushobject(pointsObj);
		lua_pushnumber((float)index);
		index++;
		pPoints[i * 2] = luaFtol(lua_getnumber(lua_gettable()));

		lua_pushobject(pointsObj);
		lua_pushnumber((float)index);
		index++;
		lua_Object yObj = lua_gettable();
		if (!lua_isnil(yObj))
			pPoints[i * 2 + 1] = luaFtol(lua_getnumber(yObj));
		else
			pPoints[i * 2 + 1] = 0;
	}

	int id = pQueue->addPolygon(numPoints, pPoints, color, layer);

	delete[] pPoints;
	pPoints = nullptr;

	lua_pushnumber((float)(uint)id);
}

void Lua_V1::DrawLine() {
	PrimitiveQueue *pQueue = g_grim->_pSputPrimitive->getCurrent();
	int layer = 2;
	int color = 0;
	lua_Object x1Obj = lua_getparam(1);
	lua_Object y1Obj = lua_getparam(2);
	lua_Object x2Obj = lua_getparam(3);
	lua_Object y2Obj = lua_getparam(4);
	lua_Object tableObj = lua_getparam(5);

	if (!lua_isnumber(x1Obj) || !lua_isnumber(y1Obj) || !lua_isnumber(x2Obj) || !lua_isnumber(y2Obj)) {
		lua_pushnil();
		return;
	}

	int x1 = luaFtol(lua_getnumber(x1Obj));
	int y1 = luaFtol(lua_getnumber(y1Obj));
	int x2 = luaFtol(lua_getnumber(x2Obj));
	int y2 = luaFtol(lua_getnumber(y2Obj));

	if (lua_istable(tableObj)) {
		lua_pushobject(tableObj);
		lua_pushstring("color");
		lua_Object colorObj = lua_gettable();
		if (lua_isuserdata(colorObj) && lua_tag(colorObj) == MKTAG('R','L','O','C'))
			color = getScreenColor(lua_getuserdata(colorObj));

		lua_pushobject(tableObj);
		lua_pushstring("layer");
		lua_Object layerObj = lua_gettable();
		if (lua_isnumber(layerObj))
			layer = luaFtol(lua_getnumber(layerObj));
	}

	lua_pushnumber((float)(uint)pQueue->addLine(x1, y1, x2, y2, color, layer));
}

void Lua_V1::ChangePrimitive() {
	PrimitiveQueue *pQueue = g_grim->_pSputPrimitive->getCurrent();
	lua_Object idObj = lua_getparam(1);
	lua_Object tableObj = lua_getparam(2);

	if (!lua_isnumber(idObj) || !lua_istable(tableObj))
		return;

	int id = luaFtol(lua_getnumber(idObj));

	lua_pushobject(tableObj);
	lua_pushstring("color");
	lua_Object colorObj = lua_gettable();
	if (lua_isuserdata(colorObj) && lua_tag(colorObj) == MKTAG('R','L','O','C'))
		pQueue->changeColor(id, getScreenColor(lua_getuserdata(colorObj)));

	lua_pushobject(tableObj);
	lua_pushstring("layer");
	lua_Object layerObj = lua_gettable();
	if (lua_isnumber(layerObj))
		pQueue->changeLayer(id, luaFtol(lua_getnumber(layerObj)));

	lua_pushobject(tableObj);
	lua_pushstring("xoffset");
	lua_Object xObj = lua_gettable();
	lua_pushobject(tableObj);
	lua_pushstring("yoffset");
	lua_Object yObj = lua_gettable();
	if (lua_isnumber(xObj) || lua_isnumber(yObj)) {
		int x = 0;
		int y = 0;
		if (lua_isnumber(xObj))
			x = luaFtol(lua_getnumber(xObj));
		if (lua_isnumber(yObj))
			y = luaFtol(lua_getnumber(yObj));
		pQueue->move(id, x, y);
	}

	lua_pushobject(tableObj);
	lua_pushstring("x");
	xObj = lua_gettable();
	lua_pushobject(tableObj);
	lua_pushstring("y");
	yObj = lua_gettable();
	if (lua_isnumber(xObj) || lua_isnumber(yObj)) {
		int x = -1;
		int y = -1;
		if (lua_isnumber(xObj))
			x = luaFtol(lua_getnumber(xObj));
		if (lua_isnumber(yObj))
			y = luaFtol(lua_getnumber(yObj));
		pQueue->moveTo(id, x, y);
	}

	lua_pushobject(tableObj);
	lua_pushstring("x2");
	xObj = lua_gettable();
	lua_pushobject(tableObj);
	lua_pushstring("y2");
	yObj = lua_gettable();
	if (lua_isnumber(xObj) || lua_isnumber(yObj)) {
		int x = -1;
		int y = -1;
		if (lua_isnumber(xObj))
			x = luaFtol(lua_getnumber(xObj));
		if (lua_isnumber(yObj))
			y = luaFtol(lua_getnumber(yObj));
		pQueue->moveEndpoint(id, x, y);
	}

	lua_pushobject(tableObj);
	lua_pushstring("width");
	lua_Object width = lua_gettable();
	lua_pushobject(tableObj);
	lua_pushstring("height");
	lua_Object height = lua_gettable();
	if (lua_isnumber(width) || lua_isnumber(height)) {
		int w = -1;
		int h = -1;
		if (lua_isnumber(width))
			w = luaFtol(lua_getnumber(width));
		if (lua_isnumber(height))
			h = luaFtol(lua_getnumber(height));
		pQueue->changeDimensions(id, w, h);
	}
}

void Lua_V1::DrawRectangle() {
	PrimitiveQueue *pQueue = g_grim->_pSputPrimitive->getCurrent();
	int layer = 2;
	int color = 0;
	int filled = 0;
	lua_Object x1Obj = lua_getparam(1);
	lua_Object y1Obj = lua_getparam(2);
	lua_Object x2Obj = lua_getparam(3);
	lua_Object y2Obj = lua_getparam(4);
	lua_Object tableObj = lua_getparam(5);

	if (!lua_isnumber(x1Obj) || !lua_isnumber(y1Obj) || !lua_isnumber(x2Obj) || !lua_isnumber(y2Obj)) {
		lua_pushnil();
		return;
	}

	int x1 = luaFtol(lua_getnumber(x1Obj));
	int y1 = luaFtol(lua_getnumber(y1Obj));
	int x2 = luaFtol(lua_getnumber(x2Obj));
	int y2 = luaFtol(lua_getnumber(y2Obj));

	if (lua_istable(tableObj)) {
		lua_pushobject(tableObj);
		lua_pushstring("color");
		lua_Object colorObj = lua_gettable();
		if (lua_isuserdata(colorObj) && lua_tag(colorObj) == MKTAG('R','L','O','C'))
			color = getScreenColor(lua_getuserdata(colorObj));

		lua_pushobject(tableObj);
		lua_pushstring("filled");
		lua_Object filledObj = lua_gettable();
		if (!lua_isnil(filledObj))
			filled = 1;

		lua_pushobject(tableObj);
		lua_pushstring("layer");
		lua_Object layerObj = lua_gettable();
		if (lua_isnumber(layerObj))
			layer = luaFtol(lua_getnumber(layerObj));
	}

	lua_pushnumber((float)(uint)pQueue->addRect(x1, y1, x2, y2, color, filled, layer));
}

void Lua_V1::BlastRect() {
	lua_Object x1Obj = lua_getparam(1);
	lua_Object y1Obj = lua_getparam(2);
	lua_Object x2Obj = lua_getparam(3);
	lua_Object y2Obj = lua_getparam(4);
	lua_Object tableObj = lua_getparam(5);
	uint16 color = 0xFFFF;
	int filled = 0;

	if (!lua_isnumber(x1Obj) || !lua_isnumber(y1Obj) || !lua_isnumber(x2Obj) || !lua_isnumber(y2Obj)) {
		lua_pushnil();
		return;
	}

	int x1 = luaFtol(lua_getnumber(x1Obj));
	int y1 = luaFtol(lua_getnumber(y1Obj));
	int x2 = luaFtol(lua_getnumber(x2Obj));
	int y2 = luaFtol(lua_getnumber(y2Obj));

	if (lua_istable(tableObj)) {
		lua_pushobject(tableObj);
		lua_pushstring("color");
		lua_Object colorObj = lua_gettable();
		if (lua_isuserdata(colorObj) && lua_tag(colorObj) == MKTAG('R','L','O','C')) {
			uint value = (uint)lua_getuserdata(colorObj);

			color = (uint16)(((value >> 19) & 0x1F) | (((value >> 10) & 0x3F) << 5) | (((value >> 3) & 0x1F) << 11));
			g_grim->_pSputRender->colorCorrect(&color, 1);
		}

		lua_pushobject(tableObj);
		lua_pushstring("filled");
		lua_Object filledObj = lua_gettable();
		if (!lua_isnil(filledObj))
			filled = 1;
	}

	if (x2 < x1 || y2 < y1 || x1 < 0 || y1 < 0 || x2 > 639 || y2 > 479)
		return;

	StdVBuffer *pBuffer = g_grim->_pSputRender->drawBuffer();

	if (!pBuffer || pBuffer->tex.format.bpp != 16 || pBuffer->tex.width < 640 || pBuffer->tex.height < 480) {
		warning("Lua_V1::BlastRect(): unsupported draw buffer");
		return;
	}

	int width = x2 - x1 + 1;
	int height = y2 - y1 + 1;

	g_grim->_pStdDisplay->vBufferLock(pBuffer);

	if (!pBuffer->pPixels) {
		warning("Lua_V1::BlastRect(): the draw buffer could not be locked");
		g_grim->_pStdDisplay->vBufferUnlock(pBuffer);
		return;
	}

	int pitch = pBuffer->tex.pitchInBytes;
	byte *pRow = pBuffer->pPixels + pitch * y1 + x1 * 2;
	uint16 *pPixel = (uint16 *)pRow;

	for (int i = 0; i < width; i++)
		pPixel[i] = color;

	if (height > 2) {
		for (int row = 0; row < height - 2; row++) {
			pRow += pitch;
			pPixel = (uint16 *)pRow;

			int count = filled ? width - 1 : 1;

			for (int i = 0; i < count; i++)
				pPixel[i] = color;

			pPixel[width - 1] = color;
		}
	}

	if (height > 1) {
		pRow += pitch;
		pPixel = (uint16 *)pRow;

		for (int i = 0; i < width; i++)
			pPixel[i] = color;
	}

	g_grim->_pStdDisplay->vBufferUnlock(pBuffer);
}

void Lua_V1::KillPrimitive() {
	lua_Object idObj = lua_getparam(1);

	if (lua_isnumber(idObj))
		g_grim->_pSputPrimitive->getCurrent()->deletePrimitive(luaFtol(lua_getnumber(idObj)));
}

void Lua_V1::DimScreen() {
	// The demo dims to half, not to a quarter
	g_grim->_pSputRender->dim(0, (g_grim->getGameFlags() & ADGF_DEMO) ? 1 : 2);
}

void Lua_V1::DimRegion() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	lua_Object widthObj = lua_getparam(3);
	lua_Object heightObj = lua_getparam(4);
	lua_Object scaleObj = lua_getparam(5);
	int x = luaFtol(lua_getnumber(xObj));
	int y = luaFtol(lua_getnumber(yObj));
	int width = luaFtol(lua_getnumber(widthObj));
	int height = luaFtol(lua_getnumber(heightObj));

	g_grim->_pSputRender->dimRegion(x, y, width, height, lua_getnumber(scaleObj));
}

void Lua_V1::ScreenShot() {
	lua_Object widthObj = lua_getparam(1);
	lua_Object heightObj = lua_getparam(2);
	int width = 640;
	int height = 480;

	if (!lua_isnil(widthObj) && lua_isnumber(widthObj))
		width = luaFtol(lua_getnumber(widthObj));

	if (!lua_isnil(heightObj) && lua_isnumber(heightObj))
		height = luaFtol(lua_getnumber(heightObj));

	StdVBuffer *pImage = g_grim->_pSputRender->screenShot(width, height);

	if (pImage)
		pushImage(pImage);
	else
		lua_pushnil();
}

void Lua_V1::SetGamma() {
	lua_Object levelObj = lua_getparam(1);

	if (!lua_isnumber(levelObj))
		return;

	if (g_grim->getLibMode() != 4)
		g_grim->_pSputRender->setGamma(lua_getnumber(levelObj));
}

void Lua_V1::Display() {
	g_grim->_pSputRender->pageFlip();
}

void Lua_V1::EngineDisplay() {
	g_grim->_pSputRender->setPageFlip(luaFtol(lua_getnumber(lua_getparam(1))));
}

void Lua_V1::ForceRefresh() {
	g_grim->_pSputRender->forceRefresh();
}

void Lua_V1::RenderModeUser() {
	lua_Object modeObj = lua_getparam(1);

	if (lua_isnil(modeObj) && g_grim->getLibMode() == 4) {
		g_grim->_pSputSmush->pause(0);
		g_grim->_pSputRender->forceRefresh();
		g_grim->setLibMode(g_grim->getPrevMode());
	} else if ((!lua_isnil(modeObj) || (g_grim->getGameFlags() & ADGF_DEMO)) && g_grim->getLibMode() != 4) {
		// The demo enters the user mode also when the argument is nil
		g_grim->setPrevMode(g_grim->getLibMode());
		g_grim->_pSputSmush->pause(1);
		g_grim->setLibMode(4);
	}
}

void Lua_V1::IrisUp() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	lua_Object timeObj = lua_getparam(3);
	int time = luaFtol(lua_getnumber(timeObj));
	int y = luaFtol(lua_getnumber(yObj));
	int x = luaFtol(lua_getnumber(xObj));

	g_grim->_pSputRender->irisUp(x, y, time);
}

void Lua_V1::IrisDown() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	lua_Object timeObj = lua_getparam(3);
	int time = luaFtol(lua_getnumber(timeObj));
	int y = luaFtol(lua_getnumber(yObj));
	int x = luaFtol(lua_getnumber(xObj));

	g_grim->_pSputRender->irisDown(x, y, time);
}

void Lua_V1::IrisClear() {
	g_grim->_pSputRender->clearIris();
}

void Lua_V1::PreRender() {
	g_grim->_pSputRender->preRendered(getbool(1), getbool(2));
}

void Lua_V1::ResetTextures() {
#if defined(USE_OPENGL_SHADERS)
	if (g_grim->_pStd3D)
		g_grim->_pStd3D->purgeTextureCache();
#endif
}

} // end of namespace Grim
