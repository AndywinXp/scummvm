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

#ifndef TOPGUN_LIBRARIES_SPRITELIBRARY_H
#define TOPGUN_LIBRARIES_SPRITELIBRARY_H

#include "common/rect.h"
#include "common/scummsys.h"

namespace Topgun {

class TopgunEngine;

struct SpriteInfo {
	const byte *currentCommand;
	byte *frames[3];
	uint16 scriptActive;
	uint16 visible;
	uint16 frameCount;
	uint16 id;
	int16 x;
	int16 y;
	Common::Rect boundingBox;

	SpriteInfo() : currentCommand(nullptr), scriptActive(0), visible(0), frameCount(0), id(0), x(0), y(0) {
		frames[0] = nullptr;
		frames[1] = nullptr;
		frames[2] = nullptr;
	}
};

class SpriteLibrary {
public:
	enum {
		kBitmapHeaderSize = 16,
		kComposeBufferWidth = 256,
		kComposeBufferHeight = 254,
		kNumQueueCommands = 29
	};

	SpriteLibrary(TopgunEngine *engine);
	~SpriteLibrary();

	void spriteAnimate();
	bool spriteAppendBitmap(uint16 sprite, byte *frame);
	bool spriteAppendBitmaps(uint16 sprite, byte **frames, uint16 count);
	void spriteBreakLoops(uint16 sprite);
	bool spriteChangePalette(uint16 first, uint16 count, const byte *entries);
	void spriteClearUpdate(uint16 sprite);
	uint16 spriteCopy(uint16 sprite);
	void spriteCopyRect(Common::Rect &rect, const byte *source, byte *destination);
	uint16 spriteCreate(byte *frame);
	byte *spriteDibCreate(uint16 width, uint16 height, uint16 flags);
	void spriteEmptyQueue(uint16 sprite);
	void spriteEnable(uint16 sprite, uint16 enable);
	void spriteExitLibrary();
	uint16 spriteDestroy(uint16 sprite, bool hideRefresh, bool freeFrames);
	void spriteDestroyAll(bool hideRefresh);
	bool spriteFadePalette(uint16 mult, uint16 divisor, uint16 first, uint16 count);
	void spriteDibFree(byte *bitmap);
	uint16 spriteGetId(uint16 sprite);
	void spriteGetInfo(uint16 sprite, SpriteInfo &info);
	void spriteGetPalette(byte *destination, uint16 first, uint16 count);
	void spriteHide(uint16 sprite, bool refresh);
	bool spriteInitLibrary();
	void spritePaint(uint16 sprite);
	void spritePaintRect(const Common::Rect &rect, bool backgroundCoordinates);
	uint16 spritePicked(int16 x, int16 y);
	bool spriteRealizePalette();
	void spriteSetBackground(byte *background, Common::Rect *outClipRect);
	void spriteSetBkgTransparent(bool flag);
	void spriteSetDebugProc(uint32 value);
	void spriteSetId(uint16 sprite, uint16 id);
	void spriteSetMessageProc();
	void spriteSetMotion(uint16 sprite, uint32 milliseconds);
	bool spriteSetPalette(const byte *entries);
	void spriteSetPosition(uint16 sprite, int16 x, int16 y);
	bool spriteSetQueue(uint16 sprite, const byte *queue, uint16 size);
	void spriteSetServiceProc();
	void spriteSetSpeed(uint16 sprite, uint32 milliseconds);
	void spriteSetTopMost(uint16 sprite);
	void spriteShow(uint16 sprite, uint16 frame, bool refresh);
	bool spriteSwapOrder(uint16 spriteA, uint16 spriteB, bool onlyIfBAbove, bool refresh);
	void spriteTransfer(uint16 source, uint16 destination);
	void spriteCallback(uint16 sprite, bool notifyOnFrame);

private:
	void initQueueRecordSizeTable();

	TopgunEngine *_engine;
	bool _messageProcSet = false;
	bool _serviceProcSet = false;
	bool _inTick = false;
	bool _bkgTransparent = false;
	uint32 _debugProc = 0;
	byte *_composeBuffer = nullptr;
	uint32 _composeBufferCapacity = 0;
	byte *_backgroundBitmap = nullptr;
	uint16 _topmostSprite = 0;
	uint16 _spriteCount = 0;
	uint16 _tickCursor = 0;
	uint16 _spriteTableEnd = 0;
	Common::Rect _clipRect;
	int16 _scrollX = 0;
	int16 _scrollY = 0;
	uint16 _queueCommandSizeTable[kNumQueueCommands];
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_SPRITELIBRARY_H
