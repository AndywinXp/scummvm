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

#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

CDrawContext *CDrawContext::context = nullptr;

CDrawContext::CDrawContext() : drawListA(5000), drawListB(5000) {
	displayMode.width = 800;
	displayMode.height = 600;
	displayMode.bpp = 16;
	displayMode.reserved12[0] = 2;
	displayMode.reserved12[1] = 16;
	displayMode.flags = 0;
	defaultTexture = nullptr;
	frameCounter = 0;
	trianglesDrawn = 0;
	counter8 = 0;
	counter12 = 0;
	counter16 = 0;
	counter20 = 0;
	counter24 = 0;
	polyOptions = 0;
	blendMode = 0;
	textureFlag = true;
	pixelCenter = false;
	sceneBegun = false;
	hasGammaCtl = false;

	for (int i = 0; i < kNumCachedTextures; i++)
		textureCache[i] = nullptr;
}

CDrawContext::~CDrawContext() {
	if (context == this)
		context = nullptr;

	delete defaultTexture;
	defaultTexture = nullptr;

	for (int i = 0; i < kNumCachedTextures; i++) {
		delete textureCache[i];
		textureCache[i] = nullptr;
	}
}

void *CDrawContext::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void CDrawContext::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

bool CDrawContext::hasGammaControl() {
	return hasGammaCtl;
}

bool CDrawContext::getTextureFlag() {
	return textureFlag;
}

void CDrawContext::setTextureFlag(bool value) {
	textureFlag = value;
}

bool CDrawContext::getPixelCenter() {
	return pixelCenter;
}

void CDrawContext::setPixelCenter(bool value) {
	pixelCenter = value;
}

Graphics::PixelFormat CDrawContext::getMovieFormat() {
	return Graphics::PixelFormat::createFormatRGBA32();
}

bool CDrawContext::beginScene() {
	if (sceneBegun) {
		if (gpConsoleServices)
			gpConsoleServices->logWarning("CDrawContext::BeginScene -- BeginScene already called this frame!\n");
		else
			warning("CDrawContext::beginScene(): beginScene already called this frame");

		return false;
	}

	trianglesDrawn = 0;
	counter8 = 0;
	counter12 = 0;
	counter16 = 0;
	counter20 = 0;
	counter24 = 0;
	frameCounter++;
	sceneBegun = true;

	return true;
}

bool CDrawContext::endScene() {
	if (!sceneBegun) {
		if (gpConsoleServices)
			gpConsoleServices->logWarning("CDrawContext::EndScene -- No previous call to BeginScene!\n");
		else
			warning("CDrawContext::endScene(): no previous call to beginScene");

		return false;
	}

	sceneBegun = false;

	return true;
}

int CDrawContext::flushDrawLists() {
	int total = 0;
	int countA = drawListA.count;

	for (int i = 0; i < countA; i++) {
		total += outputDrawPoly(drawListA.polys[i]);
		drawListA.recyclePoly(drawListA.polys[i]);
	}

	int countB = drawListB.count;

	drawListB.sort();

	for (int i = 0; i < countB; i++) {
		total += outputDrawPoly(drawListB.polys[i]);
		drawListB.recyclePoly(drawListB.polys[i]);
	}

	afterFlush();
	drawListA.reset();
	drawListB.reset();

	return total;
}

int CDrawContext::outputDrawList(CDrawList *list) {
	int total = 0;
	int count = list->count;

	list->sort();

	for (int i = 0; i < count; i++)
		total += outputDrawPoly(list->polys[i]);

	list->~CDrawList();
	gpMemoryServices->freeImpl(list);

	return total;
}

int *CDrawContext::getViewportRect(int *rect) {
	rect[0] = 0;
	rect[1] = 0;
	rect[2] = displayMode.width;
	rect[3] = displayMode.height;

	return rect;
}

LColorFormat CDrawContext::getTextureFormat(int first, int second) {
	static const LColorFormat defaultFormat;

	return defaultFormat;
}

void CDrawContext::freeTexture(CTexture *tex) {
	if (tex == defaultTexture)
		return;

	for (int i = 0; i < kNumCachedTextures; i++) {
		if (tex == textureCache[i])
			return;
	}

	delete tex;
}

int CDrawContext::getTextureCost(CTexture *tex) {
	if (!tex || tex == defaultTexture)
		return 0;

	return sizeof(CTexture);
}

CDrawList *CDrawContext::getDrawList(int index) {
	if (index)
		return &drawListB;

	return &drawListA;
}

bool CDrawContext::setCachedTexture(int slot, int width, int height, const LColorFormat &format, byte *pixels, const char *name) {
	if (slot < 0 || slot >= kNumCachedTextures)
		return false;

	if (textureCache[slot]) {
		LABitmap bitmap(width, height, format, pixels, false);

		textureCache[slot]->setup(&bitmap);
	} else {
		textureCache[slot] = createTexture(width, height, format, pixels, name);
	}

	return textureCache[slot] != nullptr;
}

bool CDrawContext::setCachedTexture(int slot, LABitmap *bitmap) {
	if (slot < 0 || slot >= kNumCachedTextures)
		return false;

	if (textureCache[slot]) {
		textureCache[slot]->setup(bitmap);

		return textureCache[slot] != nullptr;
	}

	textureCache[slot] = createTexture(bitmap);

	return textureCache[slot] != nullptr;
}

CTexture *CDrawContext::getCachedTexture(int slot) {
	if (slot < 0 || slot >= kNumCachedTextures)
		return defaultTexture;

	CTexture *texture = textureCache[slot];

	if (!texture)
		return defaultTexture;

	return texture;
}

} // End of namespace Grim
