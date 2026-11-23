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

#ifndef TOPGUN_LIBRARIES_SPRITE_SPRITE_V1_2_H
#define TOPGUN_LIBRARIES_SPRITE_SPRITE_V1_2_H

#include "topgun/libraries/sprite/sprite.h"

// Limits
#define TG_SPR_V1_2_MAX_SPRITES                100
#define TG_SPR_V1_2_NUM_SPRITE_PARTS           4
#define TG_SPR_V1_2_NUM_QUEUE_OPCODES          22
#define TG_SPR_V1_2_CEL_CACHE_SIZE             30
#define TG_SPR_V1_2_EVENT_RECORD_SIZE          256
#define TG_SPR_V1_2_NOTIFICATION_RECORD_SIZE   0x1E
#define TG_SPR_V1_2_FRAME_NOTIFICATION         0x12
#define TG_SPR_V1_2_UNION_REFRESH_LIMIT        32
#define TG_SPR_V1_2_QUEUE_BLOCK_ROUNDING       0xFFC0
#define TG_SPR_V1_2_QUEUE_BLOCK_SLACK          0x3F

// Limits of the versions after this one
#define TG_SPR_V1_2_1_CEL_CACHE_SIZE           20
#define TG_SPR_V1_2_2_FLIP_RECORD_SIZE         0x0C
#define TG_SPR_V1_2_3_MAX_SPRITES              250
#define TG_SPR_V1_2_3_LAST_QUEUE_OPCODE        22
#define TG_SPR_V1_2_3_FLIP_FLAG_MASK           0xFF00
#define TG_SPR_V1_2_3_VARIABLE_LAST_INDEX      0x159E
// A literal n is stored as n + 0x8ACF, so adding 0x7531 (0x10000 - 0x8ACF) recovers it
#define TG_SPR_V1_2_3_LITERAL_BIAS             0x7531

// Bitmap header
#define TG_SPR_V1_2_BITMAP_HEADER_SIZE         0x14
#define TG_SPR_V1_2_BITMAP_PITCH_OFFSET        4
#define TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET  8

// A literal n is stored as n + 0x82FF, so adding 0x7D01 (0x10000 - 0x82FF) recovers it
#define TG_SPR_V1_2_VARIABLE_LAST_INDEX        0x5FE
#define TG_SPR_V1_2_LITERAL_BIAS               0x7D01

#define TG_SPR_V1_2_LEVEL_TOPMOST              0xFFFF
#define TG_SPR_V1_2_FILL_NONE                  0xFFFF
#define TG_SPR_V1_2_INTERVAL_KEEP              0xFFFF
#define TG_SPR_V1_2_OPAQUE_FILL_BYTE           0xFF

namespace Topgun {

class TopgunEngine_v1_2;

enum SpriteQueueOpcode_v1_2 {
	kSprV1_2OpPlayRange = 1,
	kSprV1_2OpMultiCel,
	kSprV1_2OpReserved3,
	kSprV1_2OpCompositeBackground,
	kSprV1_2OpQuadPath,
	kSprV1_2OpLoop,
	kSprV1_2OpFlip,
	kSprV1_2OpHide,
	kSprV1_2OpLinearMove,
	kSprV1_2OpDelayedMove,
	kSprV1_2OpWait,
	kSprV1_2OpSetPosition,
	kSprV1_2OpTimingMode,
	kSprV1_2OpFrameInterval,
	kSprV1_2OpSetRange,
	kSprV1_2OpMoveInterval,
	kSprV1_2OpShowFrame,
	kSprV1_2OpReserved18,
	kSprV1_2OpHostEvent,
	kSprV1_2OpHostEventPayload,
	kSprV1_2OpMovieFrame
};

enum SpriteQueueOpcode_v1_2_3 {
	kSprV1_2_3OpPlayRange = 1,
	kSprV1_2_3OpMultiCel,
	kSprV1_2_3OpReserved3,
	kSprV1_2_3OpCompositeBackground,
	kSprV1_2_3OpQuadPath,
	kSprV1_2_3OpLoop,
	kSprV1_2_3OpFlip,
	kSprV1_2_3OpHide,
	kSprV1_2_3OpLinearMove,
	kSprV1_2_3OpDelayedMove,
	kSprV1_2_3OpWait,
	kSprV1_2_3OpSetPosition,
	kSprV1_2_3OpTimingMode,
	kSprV1_2_3OpFrameInterval,
	kSprV1_2_3OpSetRange,
	kSprV1_2_3OpMoveInterval,
	kSprV1_2_3OpShowFrame,
	kSprV1_2_3OpReserved18,
	kSprV1_2_3OpHostEvent,
	kSprV1_2_3OpHostEventPayload,
	kSprV1_2_3OpHostEventFixed,
	kSprV1_2_3OpMovieFrame
};

struct CelReference {
	byte *bitmap;
	uint16 id;

	CelReference() : bitmap(nullptr), id(0) {}
	CelReference(byte *celBitmap, uint16 celId) : bitmap(celBitmap), id(celId) {}
};

struct SpriteInfo_v1_2 {
	const byte *currentCommand;
	byte *frames[TG_SPR_V1_2_NUM_SPRITE_PARTS];
	uint16 scriptActive;
	uint16 visible;
	uint16 frameCount;
	uint16 id;
	int16 x;
	int16 y;
	Common::Rect boundingBox;

	SpriteInfo_v1_2() : currentCommand(nullptr), scriptActive(0), visible(0), frameCount(0), id(0), x(0), y(0) {
		for (int i = 0; i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++)
			frames[i] = nullptr;
	}
};

class SpriteLibrary_v1_2 : public SpriteLibrary {
public:
	SpriteLibrary_v1_2(TopgunEngine_v1_2 *engine);
	~SpriteLibrary_v1_2() override;

	// Library
	bool spriteInitLibrary() override;
	virtual void spriteResetLibrary();
	void spriteAnimate() override;

	// Sprites
	uint16 spriteCreate(int16 level, const CelReference &cel);
	uint16 spriteCopy(uint16 sprite) override;
	uint16 spriteDestroy(uint16 sprite, bool hideRefresh, bool freeFrames) override;
	bool spriteAppendBitmap(uint16 sprite, const CelReference &cel);
	bool spriteAppendBitmaps(uint16 sprite, const CelReference *cels, uint16 count);
	virtual void spriteReplaceBitmap(uint16 sprite, const CelReference &oldCel, const CelReference &newCel);
	void spriteShow(uint16 sprite, uint16 frame, bool refresh) override;
	void spriteSetPosition(uint16 sprite, int16 x, int16 y, uint16 relative);
	virtual void spriteOffset(uint16 sprite, int16 deltaX, int16 deltaY);
	void spriteSetPriority(uint16 sprite, uint16 mode);
	void spriteSetPickMode(uint16 sprite, uint16 flag);
	void spriteSetTopMost(uint16 sprite) override;
	void spriteSetLevel(uint16 sprite, int16 level);
	void spriteCompToBackground(uint16 sprite, uint16 flag = 0);
	bool spriteSwapOrder(uint16 spriteA, uint16 spriteB, bool onlyIfBAbove, bool refresh) override;
	void spriteTransfer(uint16 source, uint16 destination) override;
	void spriteEmptyQueue(uint16 sprite) override;
	uint16 spritePicked(int16 x, int16 y) override;
	uint16 spriteIsPicked(uint16 sprite, int16 x, int16 y);
	uint16 spriteHitDetect(uint16 spriteA, uint16 spriteB);

	// Sprite information
	void spriteGetInfo(uint16 sprite, SpriteInfo_v1_2 &info);
	bool spriteGetBounds(uint16 sprite, Common::Rect &bounds);
	bool spriteGetPosition(uint16 sprite, int16 &x, int16 &y);
	uint16 spriteIsVisible(uint16 sprite);

	// Bitmaps and drawing
	byte *spriteDibCreate(uint16 width, uint16 height, uint16 flags) override;
	void spriteDibBlit(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 mode);
	void spriteDibDiff(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY);
	void spriteDibMerge(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY);
	void spriteDibSet(byte *bitmap, int16 x, int16 y, uint16 width, uint16 height, uint16 color);
	void spriteCopyRect(Common::Rect &rect, byte *destination, const byte *source);
	void spriteSetBackground(byte *background, bool noClear, uint16 colorWord, Common::Rect *outClipRect);
	void spriteSetClipBox(const Common::Rect *box);
	virtual bool spriteSetZoom(uint16 factor, int16 panX, int16 panY);
	void spriteSetStretch(uint16 flag);

	// Messages
	virtual bool spriteSendMessage(uint16 sprite, uint16 count, const uint16 *words);

	// Palette
	bool spriteChangePalette(uint16 first, uint16 count, const byte *entries, bool animate, bool replicate = false);
	uint16 spriteGetRgbPalette(byte *destination, uint16 first, uint16 count);
	uint16 spriteSelectPalette();

	// Host procedures and settings
	void spriteSetMessageProc(bool set);
	void spriteSetServiceProc(bool set, uint16 serviceWord);
	void spriteSetMovieProc(bool set);
	void spriteSetStreamProc(bool set);
	void spriteSetDebugProc(bool set);
	uint16 spriteSetSendMsg(uint16 flag);
	void spriteSetVarTable(const uint16 *table);

protected:
	struct BlitSetup_v1_2 {
		byte *destination;
		const byte *source;
		int32 destinationSkip;
		int32 sourceSkip;

		BlitSetup_v1_2() : destination(nullptr), source(nullptr), destinationSkip(0), sourceSkip(0) {}
	};

	struct CelCacheBlock {
		CelReference entries[TG_SPR_V1_2_CEL_CACHE_SIZE];
	};
	typedef Common::SharedPtr<CelCacheBlock> CelCacheBlockPtr;

	struct SpriteObject_v1_2 : public SpriteObject {
		int16 level;
		bool mirrorX;
		bool flipY;
		bool pickWholeBox;
		bool advancePending;
		bool animates;
		int16 pivotX;
		int16 pivotY;
		bool anchorX;
		bool anchorY;
		Common::Array<uint16> frameIds;
		CelCacheBlockPtr celCache;
		uint16 celCacheCount;
		byte *movieFrame;
		uint32 pauseTime;
		uint16 extraId;
		bool renderPending;
		bool multipartValid;
		bool paused;
		bool pickInvisible;
		byte *queueCapacityStart;
		uint32 queueCapacity;

		SpriteObject_v1_2() : level(0), mirrorX(false), flipY(false), pickWholeBox(false), advancePending(false),
			animates(false), pivotX(0), pivotY(0), anchorX(false), anchorY(false), celCacheCount(0), movieFrame(nullptr), pauseTime(0), extraId(0), renderPending(false),
			multipartValid(false), paused(false), pickInvisible(false), queueCapacityStart(nullptr), queueCapacity(0) {}
	};

	// Sprite objects
	SpriteObject_v1_2 *findSprite(uint16 sprite, const char *caller) override;
	SpriteObject_v1_2 *lookupSprite(uint16 sprite);
	virtual uint16 spriteObjectAlloc(int16 level);
	void appendFrame(SpriteObject_v1_2 *object, const CelReference &cel);
	void freeDistinctCelBitmaps(uint16 sprite) override;
	bool initSpriteFromFirstCel(uint16 sprite);
	uint16 spriteObjectSetCelRange(uint16 sprite, uint16 startFrame, uint16 first, uint16 last) override;
	virtual void recomputeSpriteBoundingBox(uint16 sprite);
	virtual void recomputeCelBoxArray(uint16 sprite);
	virtual void updateSpritePosition(uint16 sprite, int16 x, int16 y, uint16 relative, bool refresh = true);
	uint16 flipFlags(const SpriteObject_v1_2 *object) const;
	uint16 currentTickSprite() const;

	// Library
	bool createComposeBuffer();
	void computeClipRect();
	void initQueueRecordSizeTable() override;
	virtual uint16 decodeOperand(uint16 raw) const;
	void callServiceProc();
	uint16 callDebugProc(uint16 sprite);
	uint16 callMovieProc(uint16 sprite);
	virtual byte *callStreamProc(uint16 sprite, uint16 id, uint16 release);

	// Cel cache
	byte *getCelBitmap(uint16 id, uint16 sprite);
	byte *findBitmapInCelCache(uint16 id, uint16 sprite);
	virtual bool addBitmapToCelCache(uint16 sprite, byte *bitmap, uint16 id);
	void releaseCachedBitmap(uint16 sprite, uint16 id);
	void releaseAllCachedBitmaps(uint16 sprite);

	// Queue interpreter
	bool processSpriteQueueCommand(uint16 sprite) override;
	virtual bool processSpriteAnimationStep(uint16 sprite);
	byte *nextQueueRecordWithLoops(uint16 sprite);
	void computeInterpolatedMoveSteps(byte *command);
	bool fireSpriteQueueCallbackMsg(uint16 sprite, uint16 size);
	virtual void notifySpriteFrameChange(uint16 sprite);
	bool tickPlayRangeRecord(uint16 sprite);
	bool tickMultiCelRecord(uint16 sprite);
	bool tickQuadPathRecord(uint16 sprite);
	bool tickLinearMoveRecord(uint16 sprite);
	bool tickDelayedMoveRecord(uint16 sprite);
	bool tickWaitRecord(uint16 sprite);
	void tickFlipRecord(uint16 sprite);
	void tickSetPositionRecord(uint16 sprite, const byte *command);
	bool tickMovieFrameRecord(uint16 sprite);
	virtual bool stepQuadPath(uint16 sprite);
	bool stepLinearMove(uint16 sprite);
	void applyFrameChange(uint16 sprite) override;

	// Message builders
	byte *allocQueueRecord(uint16 opcode, uint16 sprite);
	byte *growQueueForRecord(uint16 opcode, uint16 sprite);
	void resetQueueForImmediate(uint16 sprite);
	void runImmediateRecord(byte *record, uint16 sprite);
	bool buildPlayRangeMessage(uint16 mode, const uint16 *params, uint16 sprite);
	bool buildQuadPathMessage(uint16 mode, const uint16 *params, uint16 sprite);
	bool buildLinearMoveMessage(uint16 mode, const uint16 *params, uint16 sprite);

	// Rendering
	bool renderRect(const Common::Rect &rect) override;
	void renderSpriteToSurface(uint16 sprite) override;
	bool repaintUnionOfCurrentAndPreviousRect(uint16 sprite) override;
	void compositeSpritesInRect(const Common::Rect &rect);
	void compositeSpritePartsIntoBuffer(uint16 sprite, const Common::Rect &rect);
	void compositeSpriteIntoBackground(uint16 sprite, uint16 flag = 0);
	void compositeMultipartIntoBackground(uint16 sprite, const Common::Rect &rect);
	void blitSpriteToScreen(int16 x, int16 y, const byte *bitmap) override;
	bool ensureComposeBufferSize(uint16 width, uint16 height);

	// Blitters
	byte *bitmapPixels(const byte *bitmap) const;
	virtual bool blitSetupRaw(byte *destination, const byte *source, uint16 &width, uint16 &height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags, BlitSetup_v1_2 &setup);
	virtual void blitOpaqueRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags);
	void blitDibDiffRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags);
	virtual void blitTransparentRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags);
	virtual void blitMaskedRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags);
	uint16 blitPixelOverlapTest(const byte *first, const byte *second, uint16 width, uint16 height, int16 firstX, int16 firstY, int16 secondX, int16 secondY, uint16 flags);
	virtual uint16 getBitmapPixel(const byte *bitmap, int16 x, int16 y, uint16 flip);
	void fillBitmapByte(byte *bitmap, int16 x, int16 y, uint16 width, uint16 height, uint16 color);

	// Palette
	bool createIdentityLogicalPalette() override;

	// Rectangles
	static bool intersectRectLocal(const Common::Rect &first, const Common::Rect &second, Common::Rect &result);
	static void unionRectLocal(const Common::Rect &first, const Common::Rect &second, Common::Rect &result);

	uint16 _tickNesting = 0;
	bool _debugProcSet = false;
	bool _movieProcSet = false;
	bool _streamProcSet = false;
	bool _inHostCallback = false;
	bool _hostEventsSuppressed = false;
	uint16 _serviceWord = 0;
	const uint16 *_variableTable = nullptr;
	Common::Rect _userClipBox;
	uint16 _fillColor = 0;
	bool _zoomActive = false;
	bool _stretch = false;
	uint16 _zoomFactor = 0;
	int16 _zoomOffsetX = 0;
	int16 _zoomOffsetY = 0;
	byte _eventRecord[TG_SPR_V1_2_EVENT_RECORD_SIZE];

private:
	TopgunEngine_v1_2 *engine() const;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_SPRITE_SPRITE_V1_2_H
