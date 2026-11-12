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
#include "engines/grim/render/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/std/std_bitmap.h"

namespace Grim {

ObjectStateImage::ObjectStateImage(Res *res, SputResource *sputResource, StdDisplay *stdDisplay, StdBitmap *stdBitmap, SputRender *sputRender, bool isDemo, bool isZ, int *frameCount) :
		demo(isDemo), z(isZ), frameCount(frameCount), res(res), sputResource(sputResource), stdDisplay(stdDisplay), stdBitmap(stdBitmap), sputRender(sputRender) {
	memset(&rect, 0, sizeof(rect));
	memset(fileNameArray, 0, sizeof(fileNameArray));
	memset(&bitmapRef, 0, sizeof(bitmapRef));
	vBufferPointer = nullptr;
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
		res->removeReference(&bitmapRef);

	if (vBufferPointer)
		stdDisplay->vBufferFree(vBufferPointer);

	curFrame = -1;
	fileNameArray[0] = 0;
	copyMode = 0;
}

void ObjectStateImage::setFile(const char *fileName) {
	clear();

	if (fileName) {
		if (strlen(fileName) >= sizeof(fileNameArray))
			warning("ObjectStateImage::setFile(): file name is too long");

		Common::strlcpy(fileNameArray, fileName, sizeof(fileNameArray));
	}
}

void ObjectStateImage::loadDemo() {
	if (bitmapRef.valid) {
		res->touch(bitmapRef.handle);
	} else {
		void *handle = sputResource->alloc(kSputTagBknd, fileNameArray);

		if (handle) {
			StdBitmapStruct *bitmap = (StdBitmapStruct *)handle;

			res->setReference(&bitmapRef, handle);

			if (bitmap->frames[bitmap->numFrames - 1]->type != 0) {
				StdVBuffer *clean = stdDisplay->vBufferNew(&bitmap->frames[0]->tex, 0, 0, nullptr);

				if (!*frameCount)
					*frameCount = (int)bitmap->numFrames + 1;

				StdVBuffer **frames = new StdVBuffer *[*frameCount]();

				for (uint i = 0; i < bitmap->numFrames && (int)i < *frameCount; i++)
					frames[i] = bitmap->frames[i];

				delete[] bitmap->frames;
				bitmap->frames = frames;
				bitmap->numFrames = *frameCount;
				cleanSaved = 0;
				stdBitmap->addFrame(bitmap, bitmap->numFrames - 1, clean);
			}

			if (z) {
				for (uint i = 0; i < bitmap->numFrames; i++)
					bitmap->frames[i]->colorKey = 63519;
			}
		}
	}

	if (z)
		return;

	if (bitmapRef.valid) {
		StdBitmapStruct *bitmap = (StdBitmapStruct *)bitmapRef.handle;

		bitmap->curFrame = -1;
		curFrame = -1;

		for (uint i = 0; i < bitmap->numFrames; i++)
			bitmap->frames[i]->colorKey = 63519;

		rect.x = bitmap->xPos;
		rect.y = bitmap->yPos;
		rect.width = (*bitmap->frames)->tex.width;
		rect.height = (*bitmap->frames)->tex.height;
	} else {
		warning("ObjectStateImage::loadDemo(): unable to load object state file: %s", fileNameArray);
	}
}

void ObjectStateImage::load(int load) {
	if (demo && load && fileNameArray[0]) {
		loadDemo();
		return;
	}

	if (load && fileNameArray[0]) {
		if (!bitmapRef.valid) {
			void *handle = sputResource->alloc(kSputTagBknd, fileNameArray);
			StdBitmapStruct *bitmap = (StdBitmapStruct *)handle;

			if (handle) {
				res->setReference(&bitmapRef, handle);
				rect.x = bitmap->xPos;
				rect.y = bitmap->yPos;
				rect.width = (*bitmap->frames)->tex.width;
				rect.height = (*bitmap->frames)->tex.height;
				bitmap->curFrame = curFrame;
				memUsage += stdBitmap->memUsage(bitmap);
			}
		}
	} else {
		if (bitmapRef.valid)
			res->removeReference(&bitmapRef);

		if (vBufferPointer)
			stdDisplay->vBufferFree(vBufferPointer);
	}
}

StdVBuffer *ObjectStateImage::vBuffer() {
	if (curFrame < 0 && !demo)
		return vBufferPointer;

	if (!bitmapRef.valid)
		error("ObjectStateImage::vBuffer(): Bad Bitmap Reference");

	StdBitmapStruct *bitmap = (StdBitmapStruct *)bitmapRef.handle;

	// The demo keeps the clean art in the last frame of the bitmap
	if (curFrame < 0)
		return bitmap->frames[bitmap->numFrames - 1];

	if (curFrame >= (int)bitmap->numFrames)
		curFrame = (int)bitmap->numFrames - 1;

	return bitmap->frames[curFrame];
}

void ObjectStateImage::setClean(StdVBuffer *src) {
	if (demo) {
		if (!cleanSaved && bitmapRef.valid) {
			StdBitmapStruct *bitmap = (StdBitmapStruct *)bitmapRef.handle;
			StdVBuffer *last = bitmap->frames[bitmap->numFrames - 1];
			StdRect srcRect;

			srcRect.x = bitmap->xPos;
			srcRect.y = bitmap->yPos;
			srcRect.width = last->tex.width;
			srcRect.height = last->tex.height;
			stdDisplay->vBufferCopy(last, src, 0, 0, &srcRect, 0);
			cleanSaved = 1;
		}

		return;
	}

	if (vBufferPointer)
		stdDisplay->vBufferFree(vBufferPointer);

	if (bitmapRef.valid) {
		StdBitmapStruct *bitmap = (StdBitmapStruct *)bitmapRef.handle;

		vBufferPointer = stdDisplay->vBufferNew(&bitmap->frames[0]->tex, 0, 0, nullptr);

		if (vBufferPointer)
			stdDisplay->vBufferCopy(vBufferPointer, src, 0, 0, &rect, 0);
	}
}

void ObjectStateImage::draw(StdVBuffer *dst) {
	if (fileNameArray[0]) {
		if (dst) {
			int mode = copyMode;

			if (bitmapRef.valid) {
				StdBitmapStruct *bitmap = (StdBitmapStruct *)bitmapRef.handle;

				curFrame = bitmap->curFrame;
				if (curFrame == -1)
					mode = 0;

				if (vBuffer()) {
					if (vBuffer()->type == 2) {
						sputRender->fillDrawBuffer(vBuffer());
						stdDisplay->vBufferCopy(dst, sputRender->getCleanBuffer(), bitmap->xPos, bitmap->yPos, nullptr, (char)mode);
					} else {
						stdDisplay->vBufferCopy(dst, vBuffer(), bitmap->xPos, bitmap->yPos, nullptr, (char)mode);
					}

					if (mode && !demo)
						sputRender->markDirtyRect(&rect, 1);
				}
			}
		}
	}
}

int ObjectStateImage::inSync() {
	if (!fileNameArray[0])
		return 1;

	if (bitmapRef.valid)
		return ((StdBitmapStruct *)bitmapRef.handle)->curFrame == curFrame;

	// The demo skips the entries without a bitmap
	return demo ? 1 : 0;
}

SputObjectState::SputObjectState(SputObjectState *head, const char *colorFile, const char *zFile, int objectType, Res *res, SputResource *sputResource, StdDisplay *stdDisplay, StdBitmap *stdBitmap, SputRender *sputRender, bool isDemo) :
		colorImage(res, sputResource, stdDisplay, stdBitmap, sputRender, isDemo, false, &frameCount),
		zImage(res, sputResource, stdDisplay, stdBitmap, sputRender, isDemo, true, &frameCount),
		sputRender(sputRender), demo(isDemo) {
	setupIndex = -1;
	luaHandle = 0;
	frameCount = 0;
	type = objectType;
	colorImage.setFile(colorFile);
	zImage.setFile(zFile);
	next = nullptr;
	prev = nullptr;

	if (head) {
		head->prev = this;
		next = head;
	}

	size = 252;
}

SputObjectState::~SputObjectState() {
	if (prev)
		prev->next = next;

	if (next)
		next->prev = prev;
}

void SputObjectState::loadResources() {
	colorImage.load(1);
	zImage.load(1);
}

void SputObjectState::unLoadResources() {
	colorImage.load(0);
	zImage.load(0);
}

void SputObjectState::reset(StdVBuffer *color, StdVBuffer *z) {
	if (type == 3) {
		colorImage.setClean(color);
		colorImage.curFrame = -1;
		zImage.setClean(z);
		zImage.curFrame = -1;
	}
}

void SputObjectState::update(int updateType, StdVBuffer *color, StdVBuffer *z) {
	if (type != updateType)
		return;

	if (type == 3) {
		int page = 0;

		if (!colorImage.inSync()) {
			colorImage.draw(color);

			if (zImage.bitmapRef.valid) {
				if (z && !(demo && (sputRender->getRenderFlags() & 2))) {
					((StdBitmapStruct *)zImage.bitmapRef.handle)->curFrame = colorImage.curFrame;
					zImage.draw(z);
					page = 1;
				}
			}

			sputRender->markAllPagesDirty(&colorImage.rect, page);
		}
	} else {
		int colorFrame;

		if (colorImage.bitmapRef.valid)
			colorFrame = ((StdBitmapStruct *)colorImage.bitmapRef.handle)->curFrame;
		else
			colorFrame = -1;

		if (colorFrame == -1 && colorImage.curFrame != -1)
			sputRender->markAllPagesDirty(&colorImage.rect, 1);

		colorImage.draw(color);
		zImage.draw(z);
	}
}

void SputObjectState::setCopyMode(int mode) {
	colorImage.copyMode = mode;
	zImage.copyMode = mode;
}

SputObjectState *SputObjectState::moveToBack(SputObjectState *other) {
	if (other != this) {
		if (!other) {
			warning("SputObjectState::moveToBack(): no list given");
			return this;
		}

		if (prev)
			prev->next = next;

		if (next)
			next->prev = prev;

		other->prev = this;
		prev = nullptr;
		next = other;
	}

	return this;
}

SputObjectState *SputObjectState::moveToFront(SputObjectState *other) {
	SputObjectState *last = other;
	SputObjectState *head = other;

	if (!other) {
		warning("SputObjectState::moveToFront(): no list given");
		return nullptr;
	}

	if (this == other) {
		if (!other->next)
			return other;

		head = next;
		head->prev = nullptr;
	}

	for (SputObjectState *state = other->next; state; state = state->next)
		last = state;

	if (next)
		next->prev = prev;

	if (prev)
		prev->next = next;

	last->next = this;
	prev = last;
	next = nullptr;

	return head;
}

} // End of namespace Grim
