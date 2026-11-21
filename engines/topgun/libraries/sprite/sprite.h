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

#ifndef TOPGUN_LIBRARIES_SPRITE_SPRITE_H
#define TOPGUN_LIBRARIES_SPRITE_SPRITE_H

#include "common/array.h"
#include "common/hashmap.h"
#include "common/ptr.h"
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
		kBitmapWidthOffset = 2,
		kBitmapHeightOffset = 6,
		kBitmapHotXOffset = 0xA,
		kBitmapHotYOffset = 0xC,
		kComposeBufferWidth = 256,
		kComposeBufferHeight = 254,
		kMaxSprites = 50,
		kNumSpriteParts = 3,
		kNumQueueCommands = 29,
		kQueueRecordCopySize = 0x12,
		kFrameNotification = 0x1C
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
	struct SpritePart {
		byte *frame;
		Common::Rect box;

		SpritePart() : frame(nullptr) {}
	};

	struct SpritePartBlock {
		SpritePart parts[kNumSpriteParts];
	};
	typedef Common::SharedPtr<SpritePartBlock> SpritePartBlockPtr;

	struct SpriteObject {
		int16 x;
		int16 y;
		Common::Rect boundingBox;
		Common::Rect previousBox;
		uint32 moveDue;
		uint32 frameDue;
		uint32 moveInterval;
		uint32 frameInterval;
		byte *currentFrame;
		SpritePartBlockPtr parts;
		Common::Array<byte *> frames;
		byte *queueStart;
		byte *queueCurrent;
		byte *queueEnd;
		uint16 id;
		uint16 currentFrameIndex;
		uint16 nextFrameIndex;
		uint16 rangeStart;
		uint16 rangeEnd;
		bool visible;
		bool notifyOnFrame;
		bool framePending;
		bool useUnionRefresh;
		bool forward;
		bool previousValid;
		byte timingMode;
		bool inCallback;
		bool loopsDisabled;
		byte scriptActive;
		byte partCount;

		SpriteObject() : x(0), y(0), moveDue(0), frameDue(0), moveInterval(0), frameInterval(0), currentFrame(nullptr),
			queueStart(nullptr), queueCurrent(nullptr), queueEnd(nullptr), id(0), currentFrameIndex(0),
			nextFrameIndex(0), rangeStart(0), rangeEnd(0), visible(false), notifyOnFrame(false),
			framePending(false), useUnionRefresh(false), forward(false), previousValid(false),
			timingMode(0), inCallback(false), loopsDisabled(false), scriptActive(0), partCount(0) {}
	};

	typedef Common::HashMap<uint16, SpriteObject *> SpriteMap;

	// Library
	void initQueueRecordSizeTable();

	// Sprite objects
	uint16 spriteObjectAlloc();
	void freeDistinctCelBitmaps(uint16 sprite);
	void cleanupSpriteData(uint16 sprite);
	void initSpriteFromFirstCel(uint16 sprite);
	bool duplicateSpriteData(const Common::Array<byte *> &frames, Common::Array<byte *> &duplicate);
	int16 findSpriteIndex(uint16 sprite);
	uint16 spriteObjectSetCelRange(uint16 sprite, uint16 startFrame, uint16 first, uint16 last);
	SpriteObject *findSprite(uint16 sprite, const char *caller);
	uint16 registerSprite(SpriteObject *object);

	// Animation
	bool processSpriteQueueCommand(uint16 sprite);
	bool processSpriteAnimationStep(uint16 sprite);
	byte *nextQueueRecordWithLoops(uint16 sprite);
	void computeInterpolatedMoveSteps(byte *command);
	void fireSpriteQueueCallbackMsg(uint16 sprite);
	void notifySpriteFrameChange(uint16 sprite);
	bool tickPlayRangeRecord(uint16 sprite);
	bool tickMultiCelRecord(uint16 sprite);
	bool tickQuadPathRecord(uint16 sprite);
	bool tickLinearMoveRecord(uint16 sprite);
	bool tickDelayedMoveRecord(uint16 sprite);
	bool tickWaitRecord(uint16 sprite);
	bool stepQuadPath(uint16 sprite);
	bool stepLinearMove(uint16 sprite);
	Common::Point computeQuadCurvePoint(uint16 step, uint16 steps, int16 x0, int16 y0, int16 viaX, int16 viaY, int16 endX, int16 endY);
	Common::Point computeLinePoint(uint16 step, uint16 total, int16 x0, int16 y0, int16 x1, int16 y1);
	void applyFrameChange(uint16 sprite);
	uint16 tickCursorSprite();

	// Rendering
	void renderSpriteToSurface(uint16 sprite);
	void repaintPreviousRect(uint16 sprite, bool excludeCurrent);
	void recomputeCelBoxArray(uint16 sprite, const byte *command);
	void releasePaletteResources();

	TopgunEngine *_engine;
	bool _initialized = false;
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
	int _tickCursor = 0;
	Common::Rect _clipRect;
	int16 _scrollX = 0;
	int16 _scrollY = 0;
	uint16 _queueCommandSizeTable[kNumQueueCommands];
	uint16 _spriteTable[kMaxSprites];
	SpriteMap _sprites;
	uint16 _nextSpriteHandle = 1;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_SPRITE_SPRITE_H
