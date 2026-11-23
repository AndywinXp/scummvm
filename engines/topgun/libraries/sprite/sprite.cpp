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
#include "common/system.h"
#include "common/textconsole.h"

#include "topgun/display.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun.h"

namespace Topgun {

SpriteLibrary::SpriteLibrary(TopgunEngine *engine) : _engine(engine) {
	memset(_queueCommandSizeTable, 0, sizeof(_queueCommandSizeTable));
	memset(_spriteTable, 0, sizeof(_spriteTable));
	memset(_paletteCopy, 0, sizeof(_paletteCopy));
	memset(_colorTable, 0, sizeof(_colorTable));
}

SpriteLibrary::~SpriteLibrary() {
	for (SpriteMap::iterator i = _sprites.begin(); i != _sprites.end(); ++i) {
		_engine->utilsLibrary()->tgFree(i->_value->queueStart);
		delete i->_value;
	}

	_sprites.clear();
}

SpriteLibrary::SpriteObject *SpriteLibrary::findSprite(uint16 sprite, const char *caller) {
	if (sprite == 0)
		return nullptr;

	if (!_sprites.contains(sprite)) {
		warning("SpriteLibrary::%s(): invalid sprite handle %d", caller, sprite);
		return nullptr;
	}

	return _sprites[sprite];
}

uint16 SpriteLibrary::registerSprite(SpriteObject *object) {
	for (uint32 attempt = 0; attempt < 0xFFFF; attempt++) {
		uint16 handle = _nextSpriteHandle;
		_nextSpriteHandle = (_nextSpriteHandle == 0xFFFF) ? 1 : _nextSpriteHandle + 1;

		if (!_sprites.contains(handle)) {
			_sprites[handle] = object;
			return handle;
		}
	}

	return 0;
}

uint16 SpriteLibrary::spriteObjectAlloc() {
	if (_spriteCount >= TG_SPR_MAX_SPRITES) {
		warning("SpriteLibrary::spriteObjectAlloc(): the sprite table is full");
		return 0;
	}

	SpriteObject *object = new SpriteObject();
	uint16 handle = registerSprite(object);
	if (handle == 0) {
		delete object;
		return 0;
	}

	if (_topmostSprite != 0 && _spriteCount != 0) {
		_spriteTable[_spriteCount - 1] = handle;
		_spriteTable[_spriteCount] = _topmostSprite;
	} else {
		_spriteTable[_spriteCount] = handle;
	}

	_spriteCount++;
	return handle;
}

void SpriteLibrary::freeDistinctCelBitmaps(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "freeDistinctCelBitmaps");
	if (!object)
		return;

	for (uint16 i = 0; i < object->frames.size(); i++) {
		bool distinct = true;
		for (uint16 j = 0; j < i; j++) {
			if (object->frames[j] == object->frames[i]) {
				distinct = false;
				break;
			}
		}

		if (distinct)
			_engine->utilsLibrary()->tgFree(object->frames[i]);
	}
}

void SpriteLibrary::cleanupSpriteData(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "cleanupSpriteData");
	if (!object)
		return;

	if (_topmostSprite == sprite)
		_topmostSprite = 0;

	object->parts.reset();
	_engine->utilsLibrary()->tgFree(object->queueStart);
	_sprites.erase(sprite);
	delete object;
}

void SpriteLibrary::initSpriteFromFirstCel(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "initSpriteFromFirstCel");
	if (!object)
		return;

	if (object->frames.empty()) {
		warning("SpriteLibrary::initSpriteFromFirstCel(): the sprite has no frames");
		return;
	}

	byte *frame = object->frames[0];
	object->currentFrame = frame;

	int16 width = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 height = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_HEIGHT_OFFSET);
	int16 hotX = READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_X_OFFSET);
	int16 hotY = READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_Y_OFFSET);

	object->boundingBox.left = (int16)((int16)(1 - width) / 2 + hotX + object->x);
	object->boundingBox.right = (int16)(object->boundingBox.left + width);
	object->boundingBox.top = (int16)(object->y + hotY - (int16)(height - 1) / 2);
	object->boundingBox.bottom = (int16)(object->boundingBox.top + height);
}

bool SpriteLibrary::duplicateSpriteData(const Common::Array<byte *> &frames, Common::Array<byte *> &duplicate) {
	if (frames.empty())
		return false;

	duplicate = frames;
	return true;
}

int16 SpriteLibrary::findSpriteIndex(uint16 sprite) {
	for (uint16 i = 0; i < _spriteCount; i++) {
		if (_spriteTable[i] == sprite)
			return (int16)i;
	}

	return -1;
}

uint16 SpriteLibrary::spriteObjectSetCelRange(uint16 sprite, uint16 startFrame, uint16 first, uint16 last) {
	SpriteObject *object = findSprite(sprite, "spriteObjectSetCelRange");
	if (!object)
		return 0;

	uint16 count = object->frames.size();
	if (count <= first || count <= last)
		return 0;

	object->rangeStart = first;
	object->rangeEnd = last;
	object->framePending = true;
	object->forward = (last >= first);

	if (object->forward) {
		if (startFrame >= first && startFrame <= last)
			object->nextFrameIndex = startFrame;
		else
			object->nextFrameIndex = first;

		return (uint16)(last - first + 1);
	}

	if (startFrame <= first && startFrame >= last)
		object->nextFrameIndex = startFrame;
	else
		object->nextFrameIndex = first;

	return (uint16)(first - last + 1);
}

bool SpriteLibrary::spriteAppendBitmap(uint16 sprite, byte *frame) {
	SpriteObject *object = findSprite(sprite, "spriteAppendBitmap");
	if (!object)
		return false;

	uint16 count = object->frames.size();
	object->frames.push_back(frame);

	if (count == 0) {
		initSpriteFromFirstCel(sprite);
	} else if (object->forward) {
		if (object->rangeEnd == (uint16)(count - 1))
			object->rangeEnd++;
	} else {
		if (object->rangeStart == (uint16)(count - 1))
			object->rangeStart++;
	}

	return true;
}

bool SpriteLibrary::spriteAppendBitmaps(uint16 sprite, byte **frames, uint16 count) {
	SpriteObject *object = findSprite(sprite, "spriteAppendBitmaps");
	if (!object)
		return false;

	uint16 oldCount = object->frames.size();
	if (count == 0 && oldCount == 0) {
		warning("SpriteLibrary::spriteAppendBitmaps(): there are no frames to append to a sprite without frames");
		return true;
	}

	for (uint16 i = 0; i < count; i++)
		object->frames.push_back(frames[i]);

	if (oldCount == 0) {
		initSpriteFromFirstCel(sprite);
	} else if (object->forward) {
		if (object->rangeEnd == (uint16)(oldCount - 1))
			object->rangeEnd += count;
	} else {
		if (object->rangeStart == (uint16)(oldCount - 1))
			object->rangeStart += count;
	}

	return true;
}

void SpriteLibrary::spriteBreakLoops(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "spriteBreakLoops");
	if (!object)
		return;

	object->loopsDisabled = true;
}

void SpriteLibrary::spriteClearUpdate(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "spriteClearUpdate");
	if (!object)
		return;

	object->framePending = false;
}

uint16 SpriteLibrary::spriteCopy(uint16 sprite) {
	SpriteObject *source = findSprite(sprite, "spriteCopy");
	if (!source)
		return 0;

	uint16 copy = spriteObjectAlloc();
	if (copy == 0)
		return 0;

	SpriteObject *object = findSprite(copy, "spriteCopy");
	if (!object)
		return 0;

	Common::Array<byte *> duplicate;
	if (!duplicateSpriteData(source->frames, duplicate)) {
		spriteDestroy(copy, false, false);
		return 0;
	}

	*object = *source;
	object->frames = duplicate;
	object->queueStart = nullptr;
	object->queueCurrent = nullptr;
	object->queueEnd = nullptr;

	if (source->queueStart) {
		if (!spriteSetQueue(copy, source->queueStart, (uint16)(source->queueEnd - source->queueStart))) {
			spriteDestroy(copy, false, false);
			return 0;
		}
	}

	object->visible = false;
	return copy;
}

uint16 SpriteLibrary::spriteCreate(byte *frame) {
	uint16 sprite = spriteObjectAlloc();
	if (sprite == 0)
		return 0;

	if (!frame)
		return sprite;

	SpriteObject *object = findSprite(sprite, "spriteCreate");
	if (!object)
		return 0;

	object->frames.push_back(frame);
	object->forward = true;

	int16 width = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 height = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_HEIGHT_OFFSET);

	object->boundingBox.left = (int16)(object->x + READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_X_OFFSET) - width / 2);
	object->boundingBox.right = (int16)(object->boundingBox.left + width);
	object->boundingBox.top = (int16)(object->y + READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_Y_OFFSET) - height / 2);
	object->boundingBox.bottom = (int16)(object->boundingBox.top + height);
	return sprite;
}

uint16 SpriteLibrary::bitmapHeaderSize() const {
	return (_engine->_version >= kVersionV1_1) ? TG_SPR_BITMAP_HEADER_SIZE_V1_1 : TG_SPR_BITMAP_HEADER_SIZE;
}

byte *SpriteLibrary::spriteDibCreate(uint16 width, uint16 height, uint16 flags) {
	uint16 headerSize = bitmapHeaderSize();
	uint16 pitch = (uint16)(width + 3) & 0xFFFC;
	uint32 size = (uint32)pitch * height + headerSize;

	byte *bitmap = _engine->utilsLibrary()->tgMalloc(size, flags);
	if (!bitmap)
		return nullptr;

	memset(bitmap, 0, headerSize);
	WRITE_LE_UINT16(bitmap, headerSize);
	WRITE_LE_UINT16(bitmap + 8, (size >> 16) != 0 ? 1 : 0);
	WRITE_LE_UINT16(bitmap + 2, width);
	WRITE_LE_UINT16(bitmap + 4, pitch);
	WRITE_LE_UINT16(bitmap + 6, height);
	return bitmap;
}

void SpriteLibrary::spriteEnable(uint16 sprite, uint16 enable) {
	SpriteObject *object = findSprite(sprite, "spriteEnable");
	if (!object)
		return;

	object->scriptActive = (byte)enable;
}

void SpriteLibrary::spriteExitLibrary() {
	spriteDestroyAll(false);
	spriteDibFree(_composeBuffer);
	_composeBuffer = nullptr;
	releasePaletteResources();

	if (_initialized)
		_engine->display()->setViewportOrigin(0, 0);
}

uint16 SpriteLibrary::spriteDestroy(uint16 sprite, bool hideRefresh, bool freeFrames) {
	int16 index = findSpriteIndex(sprite);
	if (index < 0)
		return 0;

	SpriteObject *object = findSprite(sprite, "spriteDestroy");
	if (!object)
		return 0;

	if (object->visible && hideRefresh)
		spriteHide(sprite, true);

	if (freeFrames)
		freeDistinctCelBitmaps(sprite);

	if (_inTick && _tickCursor >= 0 && _tickCursor < TG_SPR_MAX_SPRITES && _spriteTable[_tickCursor] == sprite)
		_tickCursor--;

	cleanupSpriteData(sprite);

	for (uint16 i = index; i < _spriteCount - 1; i++)
		_spriteTable[i] = _spriteTable[i + 1];

	_spriteCount--;
	return 0;
}

void SpriteLibrary::spriteDestroyAll(bool hideRefresh) {
	if (hideRefresh && _spriteCount != 0)
		warning("SpriteLibrary::spriteDestroyAll(): the repaint runs with destroyed sprites in the table");

	for (uint16 i = 0; i < _spriteCount; i++) {
		uint16 handle = _spriteTable[i];
		if (handle == 0)
			continue;

		SpriteObject *object = findSprite(handle, "spriteDestroyAll");
		if (!object)
			continue;

		if (hideRefresh && object->visible)
			spriteHide(handle, true);

		cleanupSpriteData(handle);
	}

	_spriteCount = 0;
	_tickCursor = 0;
}

uint16 SpriteLibrary::spriteGetId(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "spriteGetId");
	if (!object)
		return 0;

	return object->id;
}

void SpriteLibrary::spriteGetInfo(uint16 sprite, SpriteInfo &info) {
	SpriteObject *object = findSprite(sprite, "spriteGetInfo");
	if (!object)
		return;

	info.frames[0] = object->currentFrame;
	info.frameCount = object->currentFrame ? 1 : 0;

	if (object->partCount != 0) {
		if (object->partCount > TG_SPR_NUM_SPRITE_PARTS || !object->parts)
			warning("SpriteLibrary::spriteGetInfo(): invalid multipart record count %d", object->partCount);

		for (uint16 i = 0; i < object->partCount && i < TG_SPR_NUM_SPRITE_PARTS && object->parts; i++)
			info.frames[i] = object->parts->parts[i].frame;

		info.frameCount = object->partCount;
	}

	info.scriptActive = object->scriptActive;
	info.visible = object->visible ? 1 : 0;
	info.x = object->x;
	info.y = object->y;
	info.boundingBox = object->boundingBox;
	info.id = object->id;
	info.currentCommand = object->queueCurrent;
}

void SpriteLibrary::spriteHide(uint16 sprite, bool refresh) {
	SpriteObject *object = findSprite(sprite, "spriteHide");
	if (!object)
		return;

	if (object->visible && refresh)
		repaintPreviousRect(sprite, false);

	object->previousValid = false;
	object->visible = false;
}

bool SpriteLibrary::spriteInitLibrary(uint16 mode) {
	_inTick = false;
	_bkgTransparent = false;
	_debugProc = 0;
	_messageProcSet = false;
	_serviceProcSet = false;
	_composeBuffer = nullptr;
	_composeBufferCapacity = 0;
	_backgroundBitmap = nullptr;
	_initialized = true;
	_topmostSprite = 0;
	_tickCursor = 0;
	_spriteCount = 0;
	_clipRect = Common::Rect();
	_scrollX = 0;
	_scrollY = 0;
	_hasLogicalPalette = false;
	_dibColorUsage = 0;
	memset(_paletteCopy, 0, sizeof(_paletteCopy));
	memset(_colorTable, 0, sizeof(_colorTable));

	_composeBuffer = spriteDibCreate(TG_SPR_COMPOSE_BUFFER_WIDTH, TG_SPR_COMPOSE_BUFFER_HEIGHT, 0);
	if (!_composeBuffer)
		return false;

	_composeBufferCapacity = (uint32)READ_LE_UINT16(_composeBuffer + 4) * READ_LE_UINT16(_composeBuffer + 6) + bitmapHeaderSize();

	Display *display = _engine->display();
	int16 screenWidth = display->width();
	int16 screenHeight = display->height();

	display->setViewportOrigin(screenWidth / 2, screenHeight / 2);
	_clipRect.right = screenWidth;
	_clipRect.bottom = screenHeight;
	_scrollX = (screenWidth - 1) / 2;
	_scrollY = (screenHeight - 1) / 2;
	_clipRect.translate(-_scrollX, -_scrollY);

	initQueueRecordSizeTable();
	return true;
}

void SpriteLibrary::initQueueRecordSizeTable() {
	_queueCommandSizeTable[1] = 12;
	_queueCommandSizeTable[2] = 14;
	_queueCommandSizeTable[3] = 18;
	_queueCommandSizeTable[4] = 2;
	_queueCommandSizeTable[5] = 18;
	_queueCommandSizeTable[6] = 6;
	_queueCommandSizeTable[7] = 7;
	_queueCommandSizeTable[8] = 6;
	_queueCommandSizeTable[9] = 3;
	_queueCommandSizeTable[10] = 6;
	_queueCommandSizeTable[11] = 8;
	_queueCommandSizeTable[12] = 6;
	_queueCommandSizeTable[13] = 4;
	_queueCommandSizeTable[14] = 2;
	_queueCommandSizeTable[15] = 12;
	_queueCommandSizeTable[16] = 4;
	_queueCommandSizeTable[17] = 2;
	_queueCommandSizeTable[18] = 4;
	_queueCommandSizeTable[19] = 6;
	_queueCommandSizeTable[20] = 4;
	_queueCommandSizeTable[21] = 11;
	_queueCommandSizeTable[22] = 4;
	_queueCommandSizeTable[23] = 6;
	_queueCommandSizeTable[24] = 3;
	_queueCommandSizeTable[25] = 7;
	_queueCommandSizeTable[26] = 9;
	_queueCommandSizeTable[27] = 4;
	_queueCommandSizeTable[28] = 4;
}

void SpriteLibrary::spriteSetId(uint16 sprite, uint16 id) {
	SpriteObject *object = findSprite(sprite, "spriteSetId");
	if (!object)
		return;

	object->id = id;
}

void SpriteLibrary::spriteSetMotion(uint16 sprite, uint32 milliseconds) {
	SpriteObject *object = findSprite(sprite, "spriteSetMotion");
	if (!object)
		return;

	object->frameInterval = milliseconds;

	uint32 now = _engine->getEngineMillis();
	object->moveDue = now + object->moveInterval;
	object->frameDue = now + milliseconds;
}

void SpriteLibrary::spriteSetPosition(uint16 sprite, int16 x, int16 y) {
	SpriteObject *object = findSprite(sprite, "spriteSetPosition");
	if (!object)
		return;

	int16 deltaX = (int16)(x - object->x);
	int16 deltaY = (int16)(y - object->y);
	object->x = x;
	object->y = y;

	object->boundingBox.left = (int16)(object->boundingBox.left + deltaX);
	object->boundingBox.top = (int16)(object->boundingBox.top + deltaY);
	object->boundingBox.right = (int16)(object->boundingBox.right + deltaX);
	object->boundingBox.bottom = (int16)(object->boundingBox.bottom + deltaY);

	object->useUnionRefresh = (deltaX > -32 && deltaX < 32 && deltaY > -32 && deltaY < 32);

	if (_engine->_version >= kVersionV1_1 && object->parts) {
		for (uint16 i = 0; i < object->partCount; i++)
			object->parts->parts[i].box.translate(deltaX, deltaY);
	}

	renderSpriteToSurface(sprite);
}

void SpriteLibrary::spriteSetCollision(uint16 sprite, uint16 targetId, uint16 notifyValue) {
	SpriteObject *object = findSprite(sprite, "spriteSetCollision");
	if (!object)
		return;

	object->collisionTarget = targetId;
	object->collisionValue = notifyValue;
}

bool SpriteLibrary::spriteResetComposeBuffer() {
	if (_composeBuffer)
		spriteDibFree(_composeBuffer);

	_composeBuffer = spriteDibCreate(TG_SPR_COMPOSE_BUFFER_WIDTH, TG_SPR_COMPOSE_BUFFER_HEIGHT, 0);
	if (!_composeBuffer)
		return false;

	_composeBufferCapacity = (uint32)READ_LE_UINT16(_composeBuffer + 4) * READ_LE_UINT16(_composeBuffer + 6) + bitmapHeaderSize();
	return true;
}

void SpriteLibrary::spriteSetSpeed(uint16 sprite, uint32 milliseconds) {
	SpriteObject *object = findSprite(sprite, "spriteSetSpeed");
	if (!object)
		return;

	object->moveInterval = milliseconds;

	uint32 now = _engine->getEngineMillis();
	object->moveDue = now + milliseconds;
	object->frameDue = now + object->frameInterval;
}

void SpriteLibrary::spriteSetTopMost(uint16 sprite) {
	int16 index = findSpriteIndex(sprite);
	if (sprite == 0)
		_topmostSprite = 0;

	if (index < 0)
		return;

	SpriteObject *object = findSprite(sprite, "spriteSetTopMost");
	if (!object)
		return;

	for (uint16 i = index; i < _spriteCount - 1; i++)
		_spriteTable[i] = _spriteTable[i + 1];

	_spriteTable[_spriteCount - 1] = sprite;
	_topmostSprite = sprite;

	bool framePending = object->framePending;
	object->framePending = false;
	renderSpriteToSurface(sprite);
	object->framePending = framePending;
}

void SpriteLibrary::spriteShow(uint16 sprite, uint16 frame, bool refresh) {
	SpriteObject *object = findSprite(sprite, "spriteShow");
	if (!object)
		return;

	if (object->visible && object->currentFrameIndex == frame)
		return;

	object->visible = true;
	if (object->frames.size() > frame) {
		object->nextFrameIndex = frame;
		object->framePending = true;
	}

	if (refresh)
		renderSpriteToSurface(sprite);
}

bool SpriteLibrary::spriteSwapOrder(uint16 spriteA, uint16 spriteB, bool onlyIfBAbove, bool refresh) {
	int16 indexA = findSpriteIndex(spriteA);
	int16 indexB = findSpriteIndex(spriteB);
	if (indexA < 0 || indexB < 0)
		return false;

	if (onlyIfBAbove && indexB <= indexA)
		return false;

	uint16 temp = _spriteTable[indexA];
	_spriteTable[indexA] = _spriteTable[indexB];
	_spriteTable[indexB] = temp;

	if (refresh) {
		renderSpriteToSurface(_spriteTable[indexB > indexA ? indexA : indexB]);
		renderSpriteToSurface(_spriteTable[indexB < indexA ? indexA : indexB]);
	}

	return true;
}

void SpriteLibrary::spriteTransfer(uint16 source, uint16 destination) {
	SpriteObject *from = findSprite(source, "spriteTransfer");
	SpriteObject *to = findSprite(destination, "spriteTransfer");
	if (!from || !to)
		return;

	to->currentFrame = from->currentFrame;
	to->partCount = from->partCount;
	from->partCount = 0;
	to->parts = from->parts;
	from->parts.reset();
	to->visible = from->visible;
	to->boundingBox = from->boundingBox;
	to->x = from->x;
	to->y = from->y;
	to->previousValid = from->previousValid;
	to->previousBox = from->previousBox;
	from->previousValid = false;
	from->visible = false;
	from->framePending = false;
	to->framePending = false;
}

void SpriteLibrary::spriteCallback(uint16 sprite, bool notifyOnFrame) {
	SpriteObject *object = findSprite(sprite, "spriteCallback");
	if (!object)
		return;

	object->notifyOnFrame = notifyOnFrame;
}

} // End of namespace Topgun
