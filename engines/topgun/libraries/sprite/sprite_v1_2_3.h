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

#ifndef TOPGUN_LIBRARIES_SPRITE_SPRITE_V1_2_3_H
#define TOPGUN_LIBRARIES_SPRITE_SPRITE_V1_2_3_H

#include "topgun/libraries/sprite/sprite_v1_2.h"

// Limits
#define TG_SPR_V1_2_3_NOTIFICATION_RECORD_SIZE 4
#define TG_SPR_V1_2_2_1_MAX_HOST_EVENT_WORDS   4
#define TG_SPR_V1_2_5_MAX_HOST_EVENT_WORDS     6
#define TG_SPR_V1_2_2_1_MAX_HOST_EVENT_COUNT   4
#define TG_SPR_V1_2_2_2_MAX_HOST_EVENT_COUNT   6
// Cel ids that fit in a multi-cel record: (0x0E - 2) / 2
#define TG_SPR_V1_2_2_2_MAX_MULTI_CEL_FRAMES   6

// Bitmap header: bit 7 of the byte at +9 marks a line-RLE bitmap
#define TG_SPR_V1_2_2_1_BITMAP_FLAGS_OFFSET    9
#define TG_SPR_V1_2_2_1_BITMAP_RLE_FLAG        0x80

// Bitmap header: the word at +8 holds the flags, 0x80 marks a line-RLE bitmap and 0x40 a top-down one
#define TG_SPR_V1_2_5_BITMAP_FLAGS_OFFSET      8
#define TG_SPR_V1_2_5_BITMAP_RLE_FLAG          0x80
#define TG_SPR_V1_2_5_BITMAP_TOP_DOWN_FLAG     0x40

// RLE rows: 0 ends the row, 1..0x7F is a run of one byte, 0x80..0xFE a literal of (0xFF - code) bytes,
// 0xFF a literal whose length is in the next byte and whose zero bytes are transparent
#define TG_SPR_V1_2_2_1_RLE_LAST_RUN_CODE      0x7F
#define TG_SPR_V1_2_2_1_RLE_TESTED_LITERAL_CODE 0xFF

#define TG_SPR_V1_2_2_1_MIRROR_FLAG_MASK       0x00FF

// Tile cell bitmaps: bits 4 and 5 of the flags word at +8 mirror and flip the cell
#define TG_SPR_V1_2_10_TILE_MIRROR_FLAG        0x10
#define TG_SPR_V1_2_10_TILE_FLIP_FLAG          0x20
#define TG_SPR_V1_2_10_TILE_STAGGERED_LAYOUT   1

namespace Topgun {

enum SpriteMessage_v1_2_3 {
	kSprV1_2_3MsgPlayRange = 1,
	kSprV1_2_3MsgMultiCel = 2,
	kSprV1_2_3MsgComposite = 4,
	kSprV1_2_3MsgQuadPath = 5,
	kSprV1_2_3MsgFlip = 7,
	kSprV1_2_3MsgHide = 8,
	kSprV1_2_3MsgLinearMove = 9,
	kSprV1_2_3MsgWait = 11,
	kSprV1_2_3MsgSetPosition = 12,
	kSprV1_2_3MsgTimingMode = 13,
	kSprV1_2_3MsgFrameInterval = 14,
	kSprV1_2_3MsgSetRange = 15,
	kSprV1_2_3MsgMoveInterval = 16,
	kSprV1_2_3MsgShowFrame = 17,
	kSprV1_2_3MsgHostEvent = 21
};

// Messages of the games that renumber them: 10 and 14 are new and the following ones move up by one
enum SpriteMessage_v1_2_6 {
	kSprV1_2_6MsgDelayedMove = 10,
	kSprV1_2_6MsgSetActive = 14,
	kSprV1_2_6MsgFrameInterval,
	kSprV1_2_6MsgSetRange,
	kSprV1_2_6MsgMoveInterval,
	kSprV1_2_6MsgShowFrame,
	kSprV1_2_6MsgHostEvent = 22
};

// Messages of the games that mark the start of a queue loop and jump back to it
enum SpriteMessage_v1_2_10 {
	kSprV1_2_10MsgMarkLoopStart = 3,
	kSprV1_2_10MsgLoopJump = 6
};

enum RleBlitMode_v1_2_3 {
	kSprV1_2_3BlitCopy,
	kSprV1_2_3BlitTransparent,
	kSprV1_2_3BlitMasked
};

struct SpriteInfo_v1_2_3 : public SpriteInfo_v1_2 {
	uint16 rangeStart;
	uint16 rangeEnd;
	uint16 nextFrame;
	uint16 forward;

	SpriteInfo_v1_2_3() : rangeStart(0), rangeEnd(0), nextFrame(0), forward(0) {}
};

class SpriteLibrary_v1_2_3 : public SpriteLibrary_v1_2 {
public:
	SpriteLibrary_v1_2_3(TopgunEngine_v1_2 *engine);
	~SpriteLibrary_v1_2_3() override;

	// Library
	void spriteScreenShake(int16 deltaX, int16 deltaY);
	void spriteScrollScreen(int16 deltaX, int16 deltaY);
	void spriteRubberBand(bool enable, int16 x, int16 y, bool move);
	void spriteSetScrollPos(int16 x, int16 y);
	Common::Point spriteGetScrollPos();
	void spriteScroll(int16 deltaX, int16 deltaY, bool useScrollDevice);
	void spriteSetScrollBox(const Common::Rect *box);
	Common::Rect spriteGetScrollBox();
	void spriteSetScrollable(uint16 sprite, uint16 flag);
	bool spriteSetTileBackground(TileBackground *tile, Common::Rect *outClipRect);
	void spriteBoxInBackground(byte *bitmap, uint16 steps, bool insetBoth);
	void spriteBoxOutBackground(byte *bitmap, uint16 steps, bool insetBoth);

	// Sprites
	void spriteHide(uint16 sprite, bool refresh) override;
	void spriteBreakLoops(uint16 sprite, uint16 flag);
	void spriteSetPickMode(uint16 sprite, uint16 wholeBox, uint16 pickInvisible);
	void spritePause(uint16 sprite, uint16 flag);
	void spriteSkipMessage(uint16 sprite, uint16 count);
	uint16 spriteGetExtraId(uint16 sprite);
	void spriteSetExtraId(uint16 sprite, uint16 extraId);
	uint16 spriteGetNumCels(uint16 sprite);
	void spriteSetSpriteClipBox(uint16 sprite, const Common::Rect *box);

	// Messages
	bool spritePostMessage(uint16 sprite, uint16 count, const uint16 *words);
	bool spriteSendMessage(uint16 sprite, uint16 count, const uint16 *words) override;

	// Paths
	Common::Point spriteLinePoint(uint16 step, uint16 total, int16 x0, int16 y0, int16 x1, int16 y1);
	Common::Point spriteCurvePoint(uint16 step, uint16 steps, int16 x0, int16 y0, int16 viaX, int16 viaY, int16 endX, int16 endY);

	// Sprite information
	void spriteGetInfo(uint16 sprite, SpriteInfo_v1_2_3 &info);
	uint16 spriteHitDetect(uint16 spriteA, uint16 spriteB, uint16 allowInvisibleA, uint16 allowInvisibleB, uint16 sameLevelOnly);
	uint16 spriteHitDetectRect(uint16 spriteA, uint16 spriteB, const Common::Rect &rect);
	uint16 spriteIsValid(uint16 sprite);

	// Bitmaps
	void spriteDibGetBox(const byte *bitmap, Common::Rect &box);

protected:
	struct RleBlitSetup {
		byte *destination;
		int32 destinationStride;
		int32 firstRow;
		uint16 firstColumn;

		RleBlitSetup() : destination(nullptr), destinationStride(0), firstRow(0), firstColumn(0) {}
	};

	struct RleRowCursor {
		const byte *pixels;
		uint32 size;
		uint32 position;
		byte count;
		bool literal;
		bool overrun;

		RleRowCursor() : pixels(nullptr), size(0), position(0), count(0), literal(false), overrun(false) {}

		byte peek();
		void loadCode();
		void skip(int32 length);
		void take();
	};

	// Sprite objects
	void pauseResumeSprite(uint16 sprite, bool pause);
	void scrollSprite(uint16 sprite, int16 deltaX, int16 deltaY);

	// Library
	bool clampScrollOffsetToBounds(Common::Point &delta, Common::Rect &outRect);
	void repaintScrollBoxRevealedColumn(int16 deltaX, Common::Rect rect);
	void repaintScrollBoxRevealedRow(int16 deltaY, Common::Rect rect);
	void performScrollOperation(byte *surface, int16 deltaX, int16 deltaY, const Common::Rect &rect);
	byte *initTileBackgroundLayout(TileBackground *tile);
	void renderTileBackgroundRect(const Common::Rect *rect);
	void renderStaggeredTileRows();
	void renderStaggeredTileRow(uint32 cellIndex, int16 column, int16 y, int16 sourceX, int16 sourceY, int16 visibleWidth, int16 height, bool shifted);
	void renderGridTiles();
	void computeWipeStepRect(bool insetBoth, uint16 steps, uint16 step, Common::Rect &out);
	void wipeBoxFromCenter(bool insetBoth, uint16 steps);
	void wipeBoxToCenter(bool insetBoth, uint16 steps);
	void drawWipeStepAndRepaint(Common::Rect rect);
	void drawWipeSideStrips(const Common::Rect &outer, const Common::Rect &inner);
	void drawWipeTopBottomStrips(const Common::Rect &outer, const Common::Rect &inner);
	// Queue interpreter
	bool processSpriteQueueCommand(uint16 sprite) override;
	bool processSpriteAnimationStep(uint16 sprite) override;
	bool tickMultiCelRecord(uint16 sprite);

	// Message builders
	bool dispatchSpriteMessage(uint16 mode, uint16 count, const uint16 *words, uint16 sprite);
	bool buildMultiCelMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite);
	bool markQueueLoopStart(uint16 mode, uint16 sprite);
	bool buildQueueLoopJumpMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite);
	bool buildCompositeMessage(uint16 mode, uint16 sprite);
	bool buildFlipMessage(uint16 mode, const uint16 *params, uint16 sprite);
	bool buildHideMessage(uint16 mode, uint16 sprite);
	bool buildHostEventMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite);
	bool buildWaitMessage(uint16 mode, const uint16 *params, uint16 sprite);
	bool buildDelayedMoveMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite);
	bool buildSetActiveMessage(uint16 mode, const uint16 *params, uint16 sprite);
	bool buildSetPositionMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite);
	bool buildTimingModeMessage(uint16 mode, const uint16 *params, uint16 sprite);
	bool buildSetRangeMessage(uint16 mode, const uint16 *params, uint16 sprite);
	bool buildFrameIntervalMessage(uint16 mode, const uint16 *params, uint16 sprite);
	bool buildMoveIntervalMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite);
	bool buildShowFrameMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite);

	// Hit tests
	uint16 hitTestMultipartDispatch(const SpriteObject_v1_2 *first, const SpriteObject_v1_2 *second);
	uint16 hitTestMultipartPair(const SpriteObject_v1_2 *first, const SpriteObject_v1_2 *second);
	uint16 hitTestMultipartVsSingle(const SpriteObject_v1_2 *single, const SpriteObject_v1_2 *multipart);

	// Blitters
	uint32 rleDataSize(const byte *bitmap) const;
	void decodeRleRow(const byte *bitmap, uint16 row, uint16 firstColumn, uint16 count, byte *values, byte *forced) const;
	bool blitSetupRleSource(byte *destination, const byte *source, uint16 &width, uint16 &height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags, RleBlitSetup &setup);
	void blitOpaqueRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) override;
	void blitTransparentRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) override;
	void blitMaskedRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) override;
	void blitRleRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags, RleBlitMode_v1_2_3 mode);
	void fetchBitmapRow(const byte *bitmap, int32 row, int32 firstColumn, uint16 count, byte *values) const;
	uint16 blitPixelOverlapTest(const byte *first, const byte *second, uint16 width, uint16 height, int16 firstX, int16 firstY, int16 secondX, int16 secondY, uint16 firstFlags, uint16 secondFlags);
	uint16 getBitmapPixel(const byte *bitmap, int16 x, int16 y, uint16 flags) override;

	Common::Rect _oldPartBoxes[TG_SPR_V1_2_NUM_SPRITE_PARTS];

	// State shared by the tile background renderers
	Common::Rect _tileClip;
	byte *_tileSurface = nullptr;
	int16 _tileDestOriginX = 0;
	int16 _tileDestOriginY = 0;
	uint16 _tileCountX = 0;
	uint16 _tileWidth = 0;
	int16 _tileHalfWidth = 0;

	// State shared by the box wipes
	uint16 _wipeExtentX = 0;
	uint16 _wipeExtentY = 0;
	byte *_wipeNewBitmap = nullptr;
	bool _wipeDrawFromNew = false;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_SPRITE_SPRITE_V1_2_3_H
