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
#include "common/util.h"

#include "engines/grim/grim.h"
#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/sputprimitive.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputsmush.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/sputtime.h"
#include "engines/grim/std/std_bitmap.h"

namespace Grim {

void SputRender::buildGammaTable(const StdColorFormat *pFormat, float gamma) {
	if (!(gamma >= 0.0f)) {
		warning("SputRender::buildGammaTable(): invalid gamma");
		return;
	}

	if (pFormat->redBits < 0 || pFormat->redBits > 30 || pFormat->greenBits < 0 || pFormat->greenBits > 30 ||
			pFormat->blueBits < 0 || pFormat->blueBits > 30 || pFormat->redShift < 0 || pFormat->redShift > 31 ||
			pFormat->greenShift < 0 || pFormat->greenShift > 31 || pFormat->blueShift < 0 || pFormat->blueShift > 31) {
		warning("SputRender::buildGammaTable(): invalid color format");
		return;
	}

	for (int i = 0; i < 0x10000; i++) {
		float red = (float)((double)((i >> 11) & 0x1F) / 31.0);
		float green = (float)((double)((i >> 5) & 0x3F) / 63.0);
		float blue = (float)((double)(i & 0x1F) / 31.0);

		red = (float)pow((double)red, (double)gamma);
		green = (float)pow((double)green, (double)gamma);
		blue = (float)pow((double)blue, (double)gamma);

		red = (float)((double)((1 << pFormat->redBits) - 1) * (double)red);
		green = (float)((double)((1 << pFormat->greenBits) - 1) * (double)green);
		blue = (float)((double)((1 << pFormat->blueBits) - 1) * (double)blue);

		uint16 value = (uint16)((uint)(int)(int64)(double)red << pFormat->redShift);

		value |= (uint16)((uint)(int)(int64)(double)green << pFormat->greenShift);
		value |= (uint16)((uint)(int)(int64)(double)blue << pFormat->blueShift);
		_colorLookup[i] = value;
	}

	_gammaCorrection = gamma;
}

void SputRender::colorCorrect(uint16 *pColors, int count) {
	if (count < 0) {
		warning("SputRender::colorCorrect(): negative count");
		return;
	}

	for (int i = 0; i < count; i++)
		pColors[i] = _colorLookup[pColors[i]];
}

void SputRender::setGamma(float gamma) {
	if (_gammaCorrection != gamma) {
		_gammaCorrection = gamma;
		loadSetupBackground(_pColorInfo);

		Room *pRoom = _pSputRoom->getCurrentRoom();

		if (pRoom)
			useSetup(pRoom, pRoom->curSetup);
	}
}

void SputRender::dim(int bForce, char shift) {
	int mode = _pGrim->getLibMode();

	if (mode == 4 && !bForce)
		return;

	if (!_pCleanArtBuffer) {
		warning("SputRender::dim(): the display is not set");
		return;
	}

	if (mode == 3) {
		_pSputSmush->setTarget(_pBackBuffer);
	} else {
		_pSputText->pause();
		_pSputPrimitive->pause();
		_pSputTime->advanceManual(0);
		update();
		_pRd->finishFrame();
		_pSputText->resume();
		_pSputPrimitive->resume();
	}

	copyGFXBuffer();

	StdVBuffer *pBuffer = _pCleanArtBuffer;
	StdColorFormat *pFormat = &pBuffer->tex.format;

	if (shift < 0 || shift > 31 || pFormat->bpp < 16 || pFormat->greenBits < 1 || pFormat->greenBits > 32 ||
			pFormat->blueBits < 1 || pFormat->blueBits > 32 || pFormat->redShift < 0 || pFormat->redShift > 31 ||
			pFormat->greenShift < 0 || pFormat->greenShift > 31 || pFormat->redLoss < 0 || pFormat->redLoss > 31 ||
			pFormat->greenLoss < 0 || pFormat->greenLoss > 31 || pFormat->blueLoss < 0 || pFormat->blueLoss > 31) {
		warning("SputRender::dim(): unsupported shift or color format");
		return;
	}

	_pStdDisplay->vBufferLock(pBuffer);

	uint greenMask = 0xFFFFFFFF >> (32 - pFormat->greenBits);
	uint blueMask = 0xFFFFFFFF >> (32 - pFormat->blueBits);

	for (int y = 0; y < pBuffer->tex.height; y++) {
		uint16 *pPixel = (uint16 *)(pBuffer->pPixels + pBuffer->tex.pitchInBytes * y);

		for (int x = 0; x < pBuffer->tex.width; x++) {
			int bRoom = _pSputRoom->getCurrentRoom() != nullptr;
			uint pixel;

			if (bRoom)
				pixel = *pPixel;
			else
				pixel = 0;

			int red = (int)((pixel >> pFormat->redShift) << pFormat->redLoss);
			int green = (int)((greenMask & (pixel >> pFormat->greenShift)) << pFormat->greenLoss);
			int blue = (int)(blueMask & pixel);
			int gray = MAX(MAX(red, green), blue);

			gray = gray >> shift;
			*pPixel = (uint16)((gray >> pFormat->blueLoss) | ((gray >> pFormat->greenLoss) << pFormat->greenShift) |
					((gray >> pFormat->redLoss) << pFormat->redShift));
			pPixel++;
		}
	}

	_pStdDisplay->vBufferUnlock(pBuffer);

	StdRect rect;

	rect.x = 0;
	rect.y = 0;
	rect.width = pBuffer->tex.width;
	rect.height = pBuffer->tex.height;
	markAllPagesDirty(&rect, 0);
}

void SputRender::dimRegion(int x, int y, int width, int height, float scale) {
	StdColorFormat *pFormat = &_pBackBuffer->tex.format;

	if (pFormat->bpp != 16 || pFormat->redBits < 1 || pFormat->redBits > 32 || pFormat->greenBits < 1 || pFormat->greenBits > 32 ||
			pFormat->blueBits < 1 || pFormat->blueBits > 32 || pFormat->redShift < 0 || pFormat->redShift > 31 ||
			pFormat->greenShift < 0 || pFormat->greenShift > 31 || pFormat->blueShift < 0 || pFormat->blueShift > 31) {
		warning("SputRender::dimRegion(): unsupported color format");
		return;
	}

	if (x < 0 || y < 0 || width < 0 || height < 0 || x + width > _pBackBuffer->tex.width || y + height > _pBackBuffer->tex.height) {
		warning("SputRender::dimRegion(): region outside of the back buffer");
		return;
	}

	if (!(scale >= -1000000.0f && scale <= 1000000.0f)) {
		warning("SputRender::dimRegion(): invalid scale");
		return;
	}

	uint redMask = 0xFFFFFFFF >> (32 - pFormat->redBits);
	uint greenMask = 0xFFFFFFFF >> (32 - pFormat->greenBits);
	uint blueMask = 0xFFFFFFFF >> (32 - pFormat->blueBits);

	_pStdDisplay->vBufferLock(_pBackBuffer);

	byte *pRow = _pBackBuffer->pPixels + 2 * x + _pBackBuffer->tex.pitchInBytes * y;

	for (int row = 0; row < height; row++) {
		uint16 *pPixel = (uint16 *)pRow;

		for (int column = 0; column < width; column++) {
			uint pixel = *pPixel;
			int red = (int)(int64)((double)(redMask & (uint16)(pixel >> pFormat->redShift)) * (double)scale);
			int green = (int)(int64)((double)(greenMask & (uint16)(pixel >> pFormat->greenShift)) * (double)scale);
			int blue = (int)(int64)((double)(blueMask & (uint16)(pixel >> pFormat->blueShift)) * (double)scale);

			*pPixel = (uint16)(((uint)red << pFormat->redShift) | ((uint)green << pFormat->greenShift) | ((uint)blue << pFormat->blueShift));
			pPixel++;
		}

		pRow += _pBackBuffer->tex.pitchInBytes;
	}

	_pStdDisplay->vBufferUnlock(_pBackBuffer);
}

void SputRender::getIris(float percent, int x, int y) {
	if (!(percent >= 0.0f && percent <= 1.0f)) {
		warning("SputRender::getIris(): percent is not between 0 and 1");
		return;
	}

	if (x < 0 || x > 640 || y < 0 || y > 480 || _pBackBuffer->tex.pitchInBytes < 1280 || _pBackBuffer->tex.height < 480) {
		warning("SputRender::getIris(): iris center or back buffer out of range");
		return;
	}

	int top = (int)((double)y * (double)percent);
	int left = (int)((double)x * (double)percent);
	int bottom = (int)((double)(480 - y) * (double)percent);
	int right = (int)((double)(640 - x) * (double)percent);
	byte *pPixels = _pBackBuffer->pPixels;
	int pitch = _pBackBuffer->tex.pitchInBytes;

	_pStdDisplay->vBufferLock(_pBackBuffer);

	int row = 0;

	if (top > 0) {
		byte *pRow = pPixels;

		row = top;

		for (int i = 0; i < top; i++) {
			memset(pRow, 0, 1280);
			pRow += pitch;
		}
	}

	if (row < 480 - bottom) {
		byte *pLeft = pPixels + row * pitch;
		byte *pRight = pPixels + 2 * (640 - right) + row * pitch;
		int numRows = 480 - bottom - row;

		for (int i = 0; i < numRows; i++) {
			memset(pLeft, 0, 2 * left);
			pLeft += pitch;
			memset(pRight, 0, 2 * right);
			pRight += pitch;
		}

		row = 480 - bottom;
	}

	if (row < 480) {
		byte *pRow = pPixels + row * pitch;

		for (int i = 0; i < 480 - row; i++) {
			memset(pRow, 0, 1280);
			pRow += pitch;
		}
	}

	_pStdDisplay->vBufferUnlock(_pBackBuffer);
}

void SputRender::irisDown(int x, int y, int duration) {
	int gameTime = _pSputTime->getGameTime();

	_irisStart = gameTime;
	_irisCenterY = y;
	_irisDuration = (float)duration;
	_irisCenterX = x;
	_renderFlags |= 4;
}

void SputRender::irisUp(int x, int y, int duration) {
	int gameTime = _pSputTime->getGameTime();

	_irisStart = gameTime;
	_irisCenterY = y;
	_irisCenterX = x;
	_irisDuration = (float)(-(double)duration);
	_renderFlags |= 4;
}

int SputRender::irisComplete() {
	if ((_renderFlags & 8) != 0)
		return 1;

	return ((uint)~_renderFlags >> 2) & 1;
}

void SputRender::clearIris() {
	StdRect rect;

	_renderFlags &= ~0xC;
	rect.width = 640;
	rect.height = 480;
	rect.y = 0;
	rect.x = 0;
	markAllPagesDirty(&rect, 0);
}

void SputRender::drawProgressBar(float progress) {
	uint16 color = 0xFFE0;

	if (progress > 1.0f)
		progress = 1.0f;

	colorCorrect(&color, 1);

	if (_pMainGfxBuffer->tex.format.bpp != 16 || _pMainGfxBuffer->tex.width < 590 || _pMainGfxBuffer->tex.height < 468) {
		warning("SputRender::drawProgressBar(): unsupported buffer");
		return;
	}

	double barWidth = (double)progress * 540.0;
	int filled = 0;

	if (barWidth > 0.0)
		filled = (int)barWidth;

	_pStdDisplay->vBufferLock(_pMainGfxBuffer);

	byte *pRow = _pMainGfxBuffer->pPixels + 455 * _pMainGfxBuffer->tex.pitchInBytes;
	uint16 *pPixel = (uint16 *)(pRow + 100);

	for (int i = 0; i < 540; i++)
		*pPixel++ = color;

	for (int row = 0; row < 12; row++) {
		pRow += 2 * ((uint)_pMainGfxBuffer->tex.pitchInBytes >> 1);
		pPixel = (uint16 *)(pRow + 100);
		((uint16 *)pRow)[589] = 0xFFE0;
		((uint16 *)pRow)[50] = 0xFFE0;

		for (int i = 0; i < filled; i++)
			*pPixel++ = color;
	}

	pPixel = (uint16 *)(pRow + 100);

	for (int i = 0; i < 540; i++)
		*pPixel++ = color;

	_pStdDisplay->vBufferUnlock(_pMainGfxBuffer);
}

void SputRender::allowLoadImage(int enable) {
	_renderEnable = enable;
}

void SputRender::drawLoadingImage() {
	if (_loadImageAllowed && _renderEnable) {
		int perfCounterDiff = _pGrim->queryPerfCounterDiff(12);

		_pGrim->setPerfCounter(12);
		int elapsed = perfCounterDiff + _perfCounter;

		_perfCounter += perfCounterDiff;

		if (_perfCounter > 5000) {
			_perfCounter = 0;
			_waitBitmapTimerMs = 0;
			return;
		}

		if (elapsed > 3500) {
			_perfCounter = 3501;
			_waitBitmapTimerMs += perfCounterDiff;

			if (_waitBitmapTimerMs >= 250) {
				_waitBitmapTimerMs = 0;

				if (!_waitBitmapRef.valid) {
					_loadImageAllowed = 0;

					void *handle = _pSputResource->alloc(kSputTagBknd, "wait.bm");

					if (!handle)
						return;

					_pRes->setReference(&_waitBitmapRef, handle);
					_loadImageAllowed = 1;

					if (!_waitBitmapRef.valid)
						error("SputRender::drawLoadingImage(): Bad Bitmap Reference");
				}

				StdBitmapStruct *pBitmap = (StdBitmapStruct *)_waitBitmapRef.handle;

				// The original could divide by zero here, let's avoid that...
				if (pBitmap->numFrames == 0) {
					warning("SputRender::drawLoadingImage(): the loading bitmap has no frames");
					return;
				}

				_waitBitmapFrame = (int)((uint)(_waitBitmapFrame + 1) % pBitmap->numFrames);
				_pStdDisplay->vBufferCopy(_pMainGfxBuffer, pBitmap->apFrames[_waitBitmapFrame], pBitmap->xPos, pBitmap->yPos, nullptr, 1);

				StdRect rect;

				rect.x = pBitmap->xPos;
				rect.y = pBitmap->yPos;
				rect.width = pBitmap->apFrames[0]->tex.width;
				rect.height = pBitmap->apFrames[0]->tex.height;
				markAllPagesDirty(&rect, 1);
			}
		}
	}
}

} // End of namespace Grim
