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

#include "common/textconsole.h"

#include "engines/grim/emi/graphics/cdrawcontext.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

CDrawContext::CDrawContext(EmiEngine *engine) : _engine(engine), _listCount(0), _drawListA(this, 5000), _drawListB(this, 5000) {
	_displayMode.width = 800;
	_displayMode.height = 600;
	_displayMode.bpp = 16;
	_displayMode.reserved12[0] = 2;
	_displayMode.reserved12[1] = 16;
	_displayMode.flags = 0;
	_defaultTexture = nullptr;
	_frameCounter = 0;
	_trianglesDrawn = 0;
	_counter8 = 0;
	_counter12 = 0;
	_counter16 = 0;
	_counter20 = 0;
	_counter24 = 0;
	_polyOptions = 0;
	_blendMode = 0;
	_textureFlag = true;
	_pixelCenter = false;
	_sceneBegun = false;
	_hasGammaCtl = false;

	for (int i = 0; i < kNumCachedTextures; i++)
		_textureCache[i] = nullptr;
}

CDrawContext::~CDrawContext() {
	if (_engine->getDrawContext() == this)
		_engine->setDrawContext(nullptr);

	delete _defaultTexture;
	_defaultTexture = nullptr;

	for (int i = 0; i < kNumCachedTextures; i++) {
		delete _textureCache[i];
		_textureCache[i] = nullptr;
	}
}

void *CDrawContext::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void CDrawContext::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

EmiEngine *CDrawContext::getEngine() const {
	return _engine;
}

bool CDrawContext::hasGammaControl() {
	return _hasGammaCtl;
}

bool CDrawContext::getTextureFlag() {
	return _textureFlag;
}

void CDrawContext::setTextureFlag(bool value) {
	_textureFlag = value;
}

bool CDrawContext::getPixelCenter() {
	return _pixelCenter;
}

void CDrawContext::setPixelCenter(bool value) {
	_pixelCenter = value;
}

Graphics::PixelFormat CDrawContext::getMovieFormat() {
	return Graphics::PixelFormat::createFormatRGBA32();
}

bool CDrawContext::beginScene() {
	if (_sceneBegun) {
		if (_engine->getConsoleServices())
			_engine->getConsoleServices()->logWarning("CDrawContext::BeginScene -- BeginScene already called this frame!\n");
		else
			warning("CDrawContext::beginScene(): beginScene already called this frame");

		return false;
	}

	_trianglesDrawn = 0;
	_counter8 = 0;
	_counter12 = 0;
	_counter16 = 0;
	_counter20 = 0;
	_counter24 = 0;
	_frameCounter++;
	_sceneBegun = true;

	return true;
}

bool CDrawContext::endScene() {
	if (!_sceneBegun) {
		if (_engine->getConsoleServices())
			_engine->getConsoleServices()->logWarning("CDrawContext::EndScene -- No previous call to BeginScene!\n");
		else
			warning("CDrawContext::endScene(): no previous call to beginScene");

		return false;
	}

	_sceneBegun = false;

	return true;
}

int CDrawContext::flushDrawLists() {
	int total = 0;
	int countA = _drawListA._count;

	for (int i = 0; i < countA; i++) {
		total += outputDrawPoly(_drawListA._polys[i]);
		_drawListA.recyclePoly(_drawListA._polys[i]);
	}

	int countB = _drawListB._count;

	_drawListB.sort();

	for (int i = 0; i < countB; i++) {
		total += outputDrawPoly(_drawListB._polys[i]);
		_drawListB.recyclePoly(_drawListB._polys[i]);
	}

	afterFlush();
	_drawListA.reset();
	_drawListB.reset();

	return total;
}

int CDrawContext::outputDrawList(CDrawList *list) {
	int total = 0;
	int count = list->_count;

	list->sort();

	for (int i = 0; i < count; i++)
		total += outputDrawPoly(list->_polys[i]);

	list->~CDrawList();
	_engine->getMemoryServices()->freeImpl(list);

	return total;
}

int *CDrawContext::getViewportRect(int *rect) {
	rect[0] = 0;
	rect[1] = 0;
	rect[2] = _displayMode.width;
	rect[3] = _displayMode.height;

	return rect;
}

LColorFormat CDrawContext::getTextureFormat(int first, int second) {
	static const LColorFormat defaultFormat;

	return defaultFormat;
}

void CDrawContext::freeTexture(CTexture *tex) {
	if (tex == _defaultTexture)
		return;

	for (int i = 0; i < kNumCachedTextures; i++) {
		if (tex == _textureCache[i])
			return;
	}

	delete tex;
}

int CDrawContext::getTextureCost(CTexture *tex) {
	if (!tex || tex == _defaultTexture)
		return 0;

	return sizeof(CTexture);
}

CDrawList *CDrawContext::getDrawList(int index) {
	if (index)
		return &_drawListB;

	return &_drawListA;
}

bool CDrawContext::setCachedTexture(int slot, int width, int height, const LColorFormat &format, byte *pixels, const char *name) {
	if (slot < 0 || slot >= kNumCachedTextures)
		return false;

	if (_textureCache[slot]) {
		LABitmap bitmap(_engine, width, height, format, pixels, false);

		_textureCache[slot]->setup(&bitmap);
	} else {
		_textureCache[slot] = createTexture(width, height, format, pixels, name);
	}

	return _textureCache[slot] != nullptr;
}

bool CDrawContext::setCachedTexture(int slot, LABitmap *bitmap) {
	if (slot < 0 || slot >= kNumCachedTextures)
		return false;

	if (_textureCache[slot]) {
		_textureCache[slot]->setup(bitmap);

		return _textureCache[slot] != nullptr;
	}

	_textureCache[slot] = createTexture(bitmap);

	return _textureCache[slot] != nullptr;
}

CTexture *CDrawContext::getCachedTexture(int slot) {
	if (slot < 0 || slot >= kNumCachedTextures)
		return _defaultTexture;

	CTexture *texture = _textureCache[slot];

	if (!texture)
		return _defaultTexture;

	return texture;
}

} // End of namespace Grim
