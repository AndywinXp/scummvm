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

#include "common/endian.h"
#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/sputobjectstate.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/std/std_bitmap.h"

namespace Grim {

ObjectStateImage::ObjectStateImage(Res *pRes, SputResource *pSputResource, StdDisplay *pStdDisplay, StdBitmap *pStdBitmap, SputRender *pSputRender, bool isDemo, bool isZ, int *pFrameCount) :
		_bDemo(isDemo), _bZ(isZ), _pFrameCount(pFrameCount), _pRes(pRes), _pSputResource(pSputResource), _pStdDisplay(pStdDisplay), _pStdBitmap(pStdBitmap), _pSputRender(pSputRender) {
	memset(&rect, 0, sizeof(rect));
	memset(aFileName, 0, sizeof(aFileName));
	memset(&bitmapRef, 0, sizeof(bitmapRef));
	pVBuffer = nullptr;
	copyMode = 0;
	curFrame = -1;
	cleanSaved = 0;
	memUsage = 116;
}

ObjectStateImage::~ObjectStateImage() {
	clear();
}

void ObjectStateImage::clear() {
	if (bitmapRef.valid)
		_pRes->removeReference(&bitmapRef);

	if (pVBuffer)
		_pStdDisplay->vBufferFree(pVBuffer);

	curFrame = -1;
	aFileName[0] = 0;
	copyMode = 0;
}

void ObjectStateImage::setFile(const char *fileName) {
	clear();

	if (fileName) {
		if (strlen(fileName) >= sizeof(aFileName))
			warning("ObjectStateImage::setFile(): file name is too long");

		Common::strlcpy(aFileName, fileName, sizeof(aFileName));
	}
}

void ObjectStateImage::loadDemo() {
	if (bitmapRef.valid) {
		_pRes->touch(bitmapRef.handle);
	} else {
		void *handle = _pSputResource->alloc(kSputTagBknd, aFileName);

		if (handle) {
			StdBitmapStruct *pBitmap = (StdBitmapStruct *)handle;

			_pRes->setReference(&bitmapRef, handle);

			if (pBitmap->apFrames[pBitmap->numFrames - 1]->type != 0) {
				StdVBuffer *pClean = _pStdDisplay->vBufferNew(&pBitmap->apFrames[0]->tex, 0, 0, nullptr);

				if (!*_pFrameCount)
					*_pFrameCount = (int)pBitmap->numFrames + 1;

				StdVBuffer **apFrames = new StdVBuffer *[*_pFrameCount]();

				for (uint i = 0; i < pBitmap->numFrames && (int)i < *_pFrameCount; i++)
					apFrames[i] = pBitmap->apFrames[i];

				delete[] pBitmap->apFrames;
				pBitmap->apFrames = apFrames;
				pBitmap->numFrames = *_pFrameCount;
				cleanSaved = 0;
				_pStdBitmap->addFrame(pBitmap, pBitmap->numFrames - 1, pClean);
			}

			if (_bZ) {
				for (uint i = 0; i < pBitmap->numFrames; i++)
					pBitmap->apFrames[i]->colorKey = 63519;
			}
		}
	}

	if (_bZ)
		return;

	if (bitmapRef.valid) {
		StdBitmapStruct *pBitmap = (StdBitmapStruct *)bitmapRef.handle;

		pBitmap->curFrame = -1;
		curFrame = -1;

		for (uint i = 0; i < pBitmap->numFrames; i++)
			pBitmap->apFrames[i]->colorKey = 63519;

		rect.x = pBitmap->xPos;
		rect.y = pBitmap->yPos;
		rect.width = (*pBitmap->apFrames)->tex.width;
		rect.height = (*pBitmap->apFrames)->tex.height;
	} else {
		warning("ObjectStateImage::loadDemo(): unable to load object state file: %s", aFileName);
	}
}

void ObjectStateImage::load(int bLoad) {
	if (_bDemo && bLoad && aFileName[0]) {
		loadDemo();
		return;
	}

	if (bLoad && aFileName[0]) {
		if (!bitmapRef.valid) {
			void *handle = _pSputResource->alloc(kSputTagBknd, aFileName);
			StdBitmapStruct *pBitmap = (StdBitmapStruct *)handle;

			if (handle) {
				_pRes->setReference(&bitmapRef, handle);
				rect.x = pBitmap->xPos;
				rect.y = pBitmap->yPos;
				rect.width = (*pBitmap->apFrames)->tex.width;
				rect.height = (*pBitmap->apFrames)->tex.height;
				pBitmap->curFrame = curFrame;
				memUsage += _pStdBitmap->memUsage(pBitmap);
			}
		}
	} else {
		if (bitmapRef.valid)
			_pRes->removeReference(&bitmapRef);

		if (pVBuffer)
			_pStdDisplay->vBufferFree(pVBuffer);
	}
}

StdVBuffer *ObjectStateImage::vBuffer() {
	if (curFrame < 0 && !_bDemo)
		return pVBuffer;

	if (!bitmapRef.valid)
		error("ObjectStateImage::vBuffer(): Bad Bitmap Reference");

	StdBitmapStruct *pBitmap = (StdBitmapStruct *)bitmapRef.handle;

	// The demo keeps the clean art in the last frame of the bitmap
	if (curFrame < 0)
		return pBitmap->apFrames[pBitmap->numFrames - 1];

	if (curFrame >= (int)pBitmap->numFrames)
		curFrame = (int)pBitmap->numFrames - 1;

	return pBitmap->apFrames[curFrame];
}

void ObjectStateImage::setClean(StdVBuffer *pSrc) {
	if (_bDemo) {
		if (!cleanSaved && bitmapRef.valid) {
			StdBitmapStruct *pBitmap = (StdBitmapStruct *)bitmapRef.handle;
			StdVBuffer *pLast = pBitmap->apFrames[pBitmap->numFrames - 1];
			StdRect srcRect;

			srcRect.x = pBitmap->xPos;
			srcRect.y = pBitmap->yPos;
			srcRect.width = pLast->tex.width;
			srcRect.height = pLast->tex.height;
			_pStdDisplay->vBufferCopy(pLast, pSrc, 0, 0, &srcRect, 0);
			cleanSaved = 1;
		}

		return;
	}

	if (pVBuffer)
		_pStdDisplay->vBufferFree(pVBuffer);

	if (bitmapRef.valid) {
		StdBitmapStruct *pBitmap = (StdBitmapStruct *)bitmapRef.handle;

		pVBuffer = _pStdDisplay->vBufferNew(&pBitmap->apFrames[0]->tex, 0, 0, nullptr);

		if (pVBuffer)
			_pStdDisplay->vBufferCopy(pVBuffer, pSrc, 0, 0, &rect, 0);
	}
}

void ObjectStateImage::draw(StdVBuffer *pDst) {
	if (aFileName[0]) {
		if (pDst) {
			int mode = copyMode;

			if (bitmapRef.valid) {
				StdBitmapStruct *pBitmap = (StdBitmapStruct *)bitmapRef.handle;

				curFrame = pBitmap->curFrame;
				if (curFrame == -1)
					mode = 0;

				if (vBuffer()) {
					if (vBuffer()->type == 2) {
						_pSputRender->fillDrawBuffer(vBuffer());
						_pStdDisplay->vBufferCopy(pDst, _pSputRender->getCleanBuffer(), pBitmap->xPos, pBitmap->yPos, nullptr, (char)mode);
					} else {
						_pStdDisplay->vBufferCopy(pDst, vBuffer(), pBitmap->xPos, pBitmap->yPos, nullptr, (char)mode);
					}

					if (mode && !_bDemo)
						_pSputRender->markDirtyRect(&rect, 1);
				}
			}
		}
	}
}

int ObjectStateImage::inSync() {
	if (!aFileName[0])
		return 1;

	if (bitmapRef.valid)
		return ((StdBitmapStruct *)bitmapRef.handle)->curFrame == curFrame;

	// The demo skips the entries without a bitmap
	return _bDemo ? 1 : 0;
}

SputObjectState::SputObjectState(SputObjectState *pHead, const char *colorFile, const char *zFile, int objectType, Res *pRes, SputResource *pSputResource, StdDisplay *pStdDisplay, StdBitmap *pStdBitmap, SputRender *pSputRender, bool isDemo) :
		colorImage(pRes, pSputResource, pStdDisplay, pStdBitmap, pSputRender, isDemo, false, &frameCount),
		zImage(pRes, pSputResource, pStdDisplay, pStdBitmap, pSputRender, isDemo, true, &frameCount),
		_pSputRender(pSputRender), _bDemo(isDemo) {
	setupIndex = -1;
	luaHandle = 0;
	frameCount = 0;
	type = objectType;
	colorImage.setFile(colorFile);
	zImage.setFile(zFile);
	pNext = nullptr;
	pPrev = nullptr;

	if (pHead) {
		pHead->pPrev = this;
		pNext = pHead;
	}

	size = 252;
}

SputObjectState::~SputObjectState() {
	if (pPrev)
		pPrev->pNext = pNext;

	if (pNext)
		pNext->pPrev = pPrev;
}

void SputObjectState::loadResources() {
	colorImage.load(1);
	zImage.load(1);
}

void SputObjectState::unLoadResources() {
	colorImage.load(0);
	zImage.load(0);
}

void SputObjectState::reset(StdVBuffer *pColor, StdVBuffer *pZ) {
	if (type == 3) {
		colorImage.setClean(pColor);
		colorImage.curFrame = -1;
		zImage.setClean(pZ);
		zImage.curFrame = -1;
	}
}

void SputObjectState::update(int updateType, StdVBuffer *pColor, StdVBuffer *pZ) {
	if (type != updateType)
		return;

	if (type == 3) {
		int page = 0;

		if (!colorImage.inSync()) {
			colorImage.draw(pColor);

			if (zImage.bitmapRef.valid) {
				if (pZ && !(_bDemo && (_pSputRender->getRenderFlags() & 2))) {
					((StdBitmapStruct *)zImage.bitmapRef.handle)->curFrame = colorImage.curFrame;
					zImage.draw(pZ);
					page = 1;
				}
			}

			_pSputRender->markAllPagesDirty(&colorImage.rect, page);
		}
	} else {
		int colorFrame;

		if (colorImage.bitmapRef.valid)
			colorFrame = ((StdBitmapStruct *)colorImage.bitmapRef.handle)->curFrame;
		else
			colorFrame = -1;

		if (colorFrame == -1 && colorImage.curFrame != -1)
			_pSputRender->markAllPagesDirty(&colorImage.rect, 1);

		colorImage.draw(pColor);
		zImage.draw(pZ);
	}
}

void SputObjectState::setCopyMode(int mode) {
	colorImage.copyMode = mode;
	zImage.copyMode = mode;
}

SputObjectState *SputObjectState::moveToBack(SputObjectState *pOther) {
	if (pOther != this) {
		if (!pOther) {
			warning("SputObjectState::moveToBack(): no list given");
			return this;
		}

		if (pPrev)
			pPrev->pNext = pNext;

		if (pNext)
			pNext->pPrev = pPrev;

		pOther->pPrev = this;
		pPrev = nullptr;
		pNext = pOther;
	}

	return this;
}

SputObjectState *SputObjectState::moveToFront(SputObjectState *pOther) {
	SputObjectState *pLast = pOther;
	SputObjectState *pHead = pOther;

	if (!pOther) {
		warning("SputObjectState::moveToFront(): no list given");
		return nullptr;
	}

	if (this == pOther) {
		if (!pOther->pNext)
			return pOther;

		pHead = pNext;
		pHead->pPrev = nullptr;
	}

	for (SputObjectState *pState = pOther->pNext; pState; pState = pState->pNext)
		pLast = pState;

	if (pNext)
		pNext->pPrev = pPrev;

	if (pPrev)
		pPrev->pNext = pNext;

	pLast->pNext = this;
	pPrev = pLast;
	pNext = nullptr;

	return pHead;
}

} // End of namespace Grim
