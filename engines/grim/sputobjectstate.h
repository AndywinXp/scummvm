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

#ifndef GRIM_SPUTOBJECTSTATE_H
#define GRIM_SPUTOBJECTSTATE_H

#include "common/scummsys.h"

#include "engines/grim/res.h"
#include "engines/grim/std/std_display.h"

namespace Grim {

class SputRender;
class SputResource;

struct ObjectStateImage {
	ObjectStateImage(Res *pRes, SputResource *pSputResource, StdDisplay *pStdDisplay, StdBitmap *pStdBitmap, SputRender *pSputRender);
	~ObjectStateImage();

	void clear();
	void setFile(const char *fileName);
	void load(int bLoad);
	StdVBuffer *vBuffer();
	void setClean(StdVBuffer *pSrc);
	void draw(StdVBuffer *pDst);
	int inSync();

	int memUsage;
	StdRect rect;
	char aFileName[32];
	ResRef bitmapRef;
	StdVBuffer *pVBuffer;
	int curFrame;
	int copyMode;

private:
	Res *_pRes;
	SputResource *_pSputResource;
	StdDisplay *_pStdDisplay;
	StdBitmap *_pStdBitmap;
	SputRender *_pSputRender;
};

struct SputObjectState {
	SputObjectState(SputObjectState *pHead, const char *colorFile, const char *zFile, int objectType, Res *pRes, SputResource *pSputResource, StdDisplay *pStdDisplay, StdBitmap *pStdBitmap, SputRender *pSputRender);
	~SputObjectState();

	void loadResources();
	void unLoadResources();
	void reset(StdVBuffer *pColor, StdVBuffer *pZ);
	void update(int updateType, StdVBuffer *pColor, StdVBuffer *pZ);
	void setCopyMode(int mode);
	SputObjectState *moveToBack(SputObjectState *pOther);
	SputObjectState *moveToFront(SputObjectState *pOther);

	int size;
	int type;
	int setupIndex;
	ObjectStateImage colorImage;
	ObjectStateImage zImage;
	SputObjectState *pPrev;
	SputObjectState *pNext;

private:
	SputRender *_pSputRender;
};

} // End of namespace Grim

#endif
