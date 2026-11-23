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

#include "common/array.h"
#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "topgun/display.h"
#include "topgun/libraries/sprite/sprite_v1_2_3.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

bool SpriteLibrary_v1_2::intersectRectLocal(const Common::Rect &first, const Common::Rect &second, Common::Rect &result) {
	int16 left = MAX(first.left, second.left);
	int16 right = MIN(first.right, second.right);
	if (left >= right)
		return false;

	int16 top = MAX(first.top, second.top);
	int16 bottom = MIN(first.bottom, second.bottom);
	if (top >= bottom)
		return false;

	result.left = left;
	result.top = top;
	result.right = right;
	result.bottom = bottom;
	return true;
}

void SpriteLibrary_v1_2::unionRectLocal(const Common::Rect &first, const Common::Rect &second, Common::Rect &result) {
	int16 left = MIN(first.left, second.left);
	int16 right = MAX(first.right, second.right);
	int16 top = MIN(first.top, second.top);
	int16 bottom = MAX(first.bottom, second.bottom);

	result.left = left;
	result.top = top;
	result.right = right;
	result.bottom = bottom;
}

byte *SpriteLibrary_v1_2::bitmapPixels(const byte *bitmap) const {
	return const_cast<byte *>(bitmap) + READ_LE_UINT16(bitmap);
}

byte *SpriteLibrary_v1_2::spriteDibCreate(uint16 width, uint16 height, uint16 flags) {
	uint16 pitch = (uint16)(width + 3) & 0xFFFC;
	uint32 size = (uint32)pitch * height + TG_SPR_V1_2_BITMAP_HEADER_SIZE;

	byte *bitmap = engine()->utilsLibrary()->tgMalloc(size, flags);
	if (!bitmap)
		return nullptr;

	memset(bitmap, 0, TG_SPR_V1_2_BITMAP_HEADER_SIZE);
	WRITE_LE_UINT16(bitmap, TG_SPR_V1_2_BITMAP_HEADER_SIZE);
	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET, width);
	WRITE_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET, pitch);
	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET, height);
	return bitmap;
}

void SpriteLibrary_v1_2::spriteDibBlit(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 mode) {
	if (!destination || !source)
		return;

	switch (mode) {
	case 0:
		blitOpaqueRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, 0);
		break;

	case 1:
		blitDibDiffRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, 0);
		break;

	case 2:
		blitTransparentRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, 0);
		break;

	default:
		break;
	}
}

void SpriteLibrary_v1_2::spriteDibDiff(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY) {
	if (!destination || !source) {
		warning("SpriteLibrary_v1_2::spriteDibDiff(): a bitmap is missing");
		return;
	}

	blitDibDiffRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, 0);
}

void SpriteLibrary_v1_2::spriteDibMerge(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY) {
	if (!destination || !source) {
		warning("SpriteLibrary_v1_2::spriteDibMerge(): a bitmap is missing");
		return;
	}

	blitTransparentRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, 0);
}

void SpriteLibrary_v1_2::spriteDibSet(byte *bitmap, int16 x, int16 y, uint16 width, uint16 height, uint16 color) {
	if (!bitmap) {
		warning("SpriteLibrary_v1_2::spriteDibSet(): the bitmap is missing");
		return;
	}

	fillBitmapByte(bitmap, x, y, width, height, color);
}

void SpriteLibrary_v1_2::spriteCopyRect(Common::Rect &rect, byte *destination, const byte *source) {
	if (!source || !destination)
		return;

	int16 sourceWidth = (int16)READ_LE_UINT16(source + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 sourceHeight = (int16)READ_LE_UINT16(source + TG_SPR_BITMAP_HEIGHT_OFFSET);

	if (sourceWidth < rect.right)
		rect.right = sourceWidth;

	if (sourceHeight < rect.bottom)
		rect.bottom = sourceHeight;

	int16 width = (int16)(rect.right - rect.left);
	int16 height = (int16)(rect.bottom - rect.top);
	if (width <= 0 || height <= 0)
		return;

	blitOpaqueRect(destination, source, width, height, rect.left, rect.top, rect.left, rect.top, 0);
}

bool SpriteLibrary_v1_2::blitSetupRaw(byte *destination, const byte *source, uint16 &width, uint16 &height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags, BlitSetup_v1_2 &setup) {
	uint16 sourceWidth = READ_LE_UINT16(source + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 sourcePitch = READ_LE_UINT16(source + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 sourceHeight = READ_LE_UINT16(source + TG_SPR_BITMAP_HEIGHT_OFFSET);
	uint16 destinationWidth = READ_LE_UINT16(destination + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 destinationPitch = READ_LE_UINT16(destination + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 destinationHeight = READ_LE_UINT16(destination + TG_SPR_BITMAP_HEIGHT_OFFSET);

	bool inside = (destinationX >= 0 && destinationY >= 0 && sourceX >= 0 && sourceY >= 0 &&
	               destinationX + width <= destinationWidth && destinationY + height <= destinationHeight &&
	               sourceX + width <= sourceWidth && sourceY + height <= sourceHeight);

	if (!inside) {
		warning("SpriteLibrary_v1_2::blitSetupRaw(): the rectangle is outside the bitmaps");

		if (destinationX < 0 || destinationY < 0 || sourceX < 0 || sourceY < 0)
			return false;

		int availableWidth = MIN(sourceWidth - sourceX, destinationWidth - destinationX);
		int availableHeight = MIN(sourceHeight - sourceY, destinationHeight - destinationY);
		if (availableWidth <= 0 || availableHeight <= 0)
			return false;

		width = MIN((int)width, availableWidth);
		height = MIN((int)height, availableHeight);
	}

	bool mirror = ((flags & 0xFF) != 0);

	int32 sourceStride;
	uint16 sourceRow;
	bool unflipped = (_engine->_version >= kVersionV1_2_3) ? ((flags & TG_SPR_V1_2_3_FLIP_FLAG_MASK) == 0) : (((flags >> 8) & 0xFF) == source[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET]);

	if (unflipped) {
		sourceRow = (uint16)(sourceHeight - sourceY - height);
		sourceStride = sourcePitch;
	} else {
		sourceRow = (uint16)(sourceY + height - 1);
		sourceStride = -(int32)sourcePitch;
	}

	int32 sourceOffset = (int32)((uint32)sourceRow * sourcePitch);
	if (mirror)
		sourceOffset += (int32)sourceWidth - sourceX;
	else
		sourceOffset += sourceX;

	setup.source = bitmapPixels(source) + sourceOffset;

	int32 destinationStride;
	uint16 destinationRow;
	if (destination[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET] == 0) {
		destinationRow = (uint16)(destinationHeight - destinationY - height);
		destinationStride = destinationPitch;
	} else {
		destinationRow = (uint16)(destinationY + height - 1);
		destinationStride = -(int32)destinationPitch;
	}

	setup.destination = bitmapPixels(destination) + (int32)((uint32)destinationRow * destinationPitch) + destinationX;

	if (mirror)
		setup.sourceSkip = sourceStride + width;
	else
		setup.sourceSkip = sourceStride - width;

	setup.destinationSkip = destinationStride - width;
	return true;
}

void SpriteLibrary_v1_2::blitOpaqueRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	BlitSetup_v1_2 setup;
	if (!blitSetupRaw(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, setup))
		return;

	for (uint16 row = 0; row < height; row++) {
		memmove(setup.destination, setup.source, width);
		setup.destination += width;
		setup.source += width;
		setup.destination += setup.destinationSkip;
		setup.source += setup.sourceSkip;
	}
}

void SpriteLibrary_v1_2::blitDibDiffRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	BlitSetup_v1_2 setup;
	if (!blitSetupRaw(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, setup))
		return;

	for (uint16 row = 0; row < height; row++) {
		for (uint16 column = 0; column < width; column++) {
			if (setup.destination[column] == TG_SPR_V1_2_OPAQUE_FILL_BYTE)
				setup.destination[column] = setup.source[column];
		}

		setup.destination += width;
		setup.source += width;
		setup.destination += setup.destinationSkip;
		setup.source += setup.sourceSkip;
	}
}

void SpriteLibrary_v1_2::blitTransparentRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	BlitSetup_v1_2 setup;
	if (!blitSetupRaw(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, setup))
		return;

	bool mirror = ((flags & 1) != 0);

	for (uint16 row = 0; row < height; row++) {
		for (uint16 column = 0; column < width; column++) {
			byte pixel = mirror ? setup.source[-1 - (int32)column] : setup.source[column];
			if (pixel != 0)
				setup.destination[column] = pixel;
		}

		setup.destination += width;
		setup.source += mirror ? -(int32)width : (int32)width;
		setup.destination += setup.destinationSkip;
		setup.source += setup.sourceSkip;
	}
}

void SpriteLibrary_v1_2::blitMaskedRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	BlitSetup_v1_2 setup;
	if (!blitSetupRaw(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, setup))
		return;

	bool mirror = ((flags & 1) != 0);

	for (uint16 row = 0; row < height; row++) {
		for (uint16 column = 0; column < width; column++) {
			byte pixel = mirror ? setup.source[-1 - (int32)column] : setup.source[column];
			if (pixel != 0 && setup.destination[column] != 0)
				setup.destination[column] = pixel;
		}

		setup.destination += width;
		setup.source += mirror ? -(int32)width : (int32)width;
		setup.destination += setup.destinationSkip;
		setup.source += setup.sourceSkip;
	}
}

uint16 SpriteLibrary_v1_2::blitPixelOverlapTest(const byte *first, const byte *second, uint16 width, uint16 height, int16 firstX, int16 firstY, int16 secondX, int16 secondY, uint16 flags) {
	if (width == 0 || height == 0)
		return 0;

	BlitSetup_v1_2 setup;
	if (!blitSetupRaw(const_cast<byte *>(first), second, width, height, firstX, firstY, secondX, secondY, flags, setup))
		return 0;

	bool mirror = ((flags & 1) != 0);

	for (uint16 row = 0; row < height; row++) {
		for (uint16 column = 0; column < width; column++) {
			byte pixel = mirror ? setup.source[-1 - (int32)column] : setup.source[column];
			if (pixel != 0 && setup.destination[column] != 0)
				return 1;
		}

		setup.destination += width;
		setup.source += mirror ? -(int32)width : (int32)width;
		setup.destination += setup.destinationSkip;
		setup.source += setup.sourceSkip;
	}

	return 0;
}

uint16 SpriteLibrary_v1_2::getBitmapPixel(const byte *bitmap, int16 x, int16 y, uint16 flip) {
	uint16 pitch = READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);

	uint16 row;
	if (bitmap[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET] == 0)
		row = (uint16)(height - y - 1);
	else
		row = (uint16)y;

	uint16 column = ((flip & 1) != 0) ? (uint16)(pitch - x) : (uint16)x;

	uint32 offset = (uint32)row * pitch + column;
	if (offset >= (uint32)pitch * height) {
		warning("SpriteLibrary_v1_2::getBitmapPixel(): the pixel is outside the bitmap");
		return 0;
	}

	return bitmapPixels(bitmap)[offset];
}

void SpriteLibrary_v1_2::fillBitmapByte(byte *bitmap, int16 x, int16 y, uint16 width, uint16 height, uint16 color) {
	uint16 pitch = READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 bitmapHeight = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);

	uint16 row;
	if (bitmap[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET] == 0)
		row = (uint16)(bitmapHeight - y - height);
	else
		row = (uint16)y;

	if (x < 0 || (uint32)row + height > bitmapHeight || (uint32)x + width > pitch) {
		warning("SpriteLibrary_v1_2::fillBitmapByte(): the rectangle is outside the bitmap");
		return;
	}

	byte *destination = bitmapPixels(bitmap) + (uint32)row * pitch + x;
	for (uint16 i = 0; i < height; i++) {
		memset(destination, color & 0xFF, width);
		destination += pitch;
	}
}

bool SpriteLibrary_v1_2::ensureComposeBufferSize(uint16 width, uint16 height) {
	uint16 pitch = (uint16)(width + 3) & 0xFFFC;
	uint32 size = (uint32)pitch * height + TG_SPR_V1_2_BITMAP_HEADER_SIZE;

	_composeBuffer[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET] = 0;
	WRITE_LE_UINT16(_composeBuffer + TG_SPR_BITMAP_WIDTH_OFFSET, width);
	WRITE_LE_UINT16(_composeBuffer + TG_SPR_V1_2_BITMAP_PITCH_OFFSET, pitch);
	WRITE_LE_UINT16(_composeBuffer + TG_SPR_BITMAP_HEIGHT_OFFSET, height);

	if (size > _composeBufferCapacity) {
		byte *grown = engine()->utilsLibrary()->tgRealloc(_composeBuffer, size, 0);
		if (!grown)
			return false;

		_composeBuffer = grown;
		_composeBufferCapacity = size;
	}

	return true;
}

bool SpriteLibrary_v1_2::renderRect(const Common::Rect &rect) {
	Common::Rect visible;
	if (!intersectRectLocal(rect, _clipRect, visible))
		return true;

	if (!_composeBuffer) {
		warning("SpriteLibrary_v1_2::renderRect(): the compose buffer does not exist");
		return false;
	}

	uint16 width = (uint16)(visible.right - visible.left);
	uint16 height = (uint16)(visible.bottom - visible.top);
	if (!ensureComposeBufferSize(width, height))
		return false;

	if (_backgroundBitmap)
		blitOpaqueRect(_composeBuffer, _backgroundBitmap, width, height, 0, 0, (int16)(visible.left + _scrollX), (int16)(visible.top + _scrollY), 0);
	else if (_fillColor != TG_SPR_V1_2_FILL_NONE)
		fillBitmapByte(_composeBuffer, 0, 0, width, height, _fillColor);

	compositeSpritesInRect(visible);
	blitSpriteToScreen(visible.left, visible.top, _composeBuffer);
	return true;
}

bool SpriteLibrary_v1_2::repaintUnionOfCurrentAndPreviousRect(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "repaintUnionOfCurrentAndPreviousRect");
	if (!object)
		return false;

	Common::Rect combined;
	unionRectLocal(object->boundingBox, object->previousBox, combined);
	return renderRect(combined);
}

void SpriteLibrary_v1_2::compositeSpritesInRect(const Common::Rect &rect) {
	for (uint16 i = 0; i < _spriteCount; i++) {
		uint16 handle = _spriteTable[i];
		SpriteObject_v1_2 *object = findSprite(handle, "compositeSpritesInRect");
		if (!object || !object->visible)
			continue;

		Common::Rect common;
		if (!intersectRectLocal(rect, object->boundingBox, common))
			continue;

		if (object->partCount != 0) {
			compositeSpritePartsIntoBuffer(handle, rect);
			continue;
		}

		if (!object->currentFrame) {
			warning("SpriteLibrary_v1_2::compositeSpritesInRect(): the sprite has no frame");
			continue;
		}

		uint16 width = (uint16)(common.right - common.left);
		uint16 height = (uint16)(common.bottom - common.top);
		int16 destinationX = (int16)(common.left - rect.left);
		int16 destinationY = (int16)(common.top - rect.top);
		int16 sourceX = (int16)(common.left - object->boundingBox.left);
		int16 sourceY = (int16)(common.top - object->boundingBox.top);

		if (_bkgTransparent && handle != _topmostSprite)
			blitMaskedRect(_composeBuffer, object->currentFrame, width, height, destinationX, destinationY, sourceX, sourceY, flipFlags(object));
		else
			blitTransparentRect(_composeBuffer, object->currentFrame, width, height, destinationX, destinationY, sourceX, sourceY, flipFlags(object));
	}
}

void SpriteLibrary_v1_2::compositeSpritePartsIntoBuffer(uint16 sprite, const Common::Rect &rect) {
	SpriteObject_v1_2 *object = findSprite(sprite, "compositeSpritePartsIntoBuffer");
	if (!object)
		return;

	if (!object->parts) {
		warning("SpriteLibrary_v1_2::compositeSpritePartsIntoBuffer(): the multipart sprite has no parts");
		return;
	}

	for (uint16 i = 0; i < object->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		const SpritePart &part = object->parts->parts[i];

		Common::Rect common;
		if (!intersectRectLocal(rect, part.box, common))
			continue;

		if (!part.frame) {
			warning("SpriteLibrary_v1_2::compositeSpritePartsIntoBuffer(): the sprite part has no frame");
			continue;
		}

		uint16 width = (uint16)(common.right - common.left);
		uint16 height = (uint16)(common.bottom - common.top);
		int16 destinationX = (int16)(common.left - rect.left);
		int16 destinationY = (int16)(common.top - rect.top);
		int16 sourceX = (int16)(common.left - part.box.left);
		int16 sourceY = (int16)(common.top - part.box.top);

		if (_bkgTransparent && sprite != _topmostSprite)
			blitMaskedRect(_composeBuffer, part.frame, width, height, destinationX, destinationY, sourceX, sourceY, flipFlags(object));
		else
			blitTransparentRect(_composeBuffer, part.frame, width, height, destinationX, destinationY, sourceX, sourceY, flipFlags(object));
	}
}

void SpriteLibrary_v1_2::compositeSpriteIntoBackground(uint16 sprite, uint16 flag) {
	if (!_backgroundBitmap)
		return;

	SpriteObject_v1_2 *object = findSprite(sprite, "compositeSpriteIntoBackground");
	if (!object || !object->visible)
		return;

	Common::Rect common;
	if (intersectRectLocal(_clipRect, object->boundingBox, common)) {
		if (object->partCount != 0) {
			compositeMultipartIntoBackground(sprite, common);
		} else if (!object->currentFrame) {
			warning("SpriteLibrary_v1_2::compositeSpriteIntoBackground(): the sprite has no frame");
		} else {
			blitTransparentRect(_backgroundBitmap, object->currentFrame, (uint16)(common.right - common.left), (uint16)(common.bottom - common.top),
			                    (int16)(common.left + _scrollX), (int16)(common.top + _scrollY),
			                    (int16)(common.left - object->boundingBox.left), (int16)(common.top - object->boundingBox.top), flipFlags(object));
		}
	}

	if (_engine->_version >= kVersionV1_2_1) {
		if (flag == 0) {
			spriteHide(sprite, false);
			spriteEmptyQueue(sprite);
		}
	} else {
		spriteHide(sprite, true);
	}
}

void SpriteLibrary_v1_2::spriteCompToBackground(uint16 sprite, uint16 flag) {
	if (sprite != 0) {
		compositeSpriteIntoBackground(sprite, flag);
		return;
	}

	for (uint16 i = 0; i < _spriteCount; i++)
		compositeSpriteIntoBackground(_spriteTable[i], flag);
}

void SpriteLibrary_v1_2::compositeMultipartIntoBackground(uint16 sprite, const Common::Rect &rect) {
	SpriteObject_v1_2 *object = findSprite(sprite, "compositeMultipartIntoBackground");
	if (!object)
		return;

	if (!object->parts) {
		warning("SpriteLibrary_v1_2::compositeMultipartIntoBackground(): the multipart sprite has no parts");
		return;
	}

	for (uint16 i = 0; i < object->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		const SpritePart &part = object->parts->parts[i];

		Common::Rect common;
		if (!intersectRectLocal(rect, part.box, common))
			continue;

		if (!part.frame) {
			warning("SpriteLibrary_v1_2::compositeMultipartIntoBackground(): the sprite part has no frame");
			continue;
		}

		const Common::Rect &origin = (_engine->_version >= kVersionV1_2_1) ? part.box : object->boundingBox;

		blitTransparentRect(_backgroundBitmap, part.frame, (uint16)(common.right - common.left), (uint16)(common.bottom - common.top),
		                    (int16)(common.left + _scrollX), (int16)(common.top + _scrollY),
		                    (int16)(common.left - origin.left), (int16)(common.top - origin.top), flipFlags(object));
	}
}

void SpriteLibrary_v1_2::blitSpriteToScreen(int16 x, int16 y, const byte *bitmap) {
	uint16 width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 pitch = READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
	const byte *pixels = bitmapPixels(bitmap);

	if (_zoomActive) {
		int16 destinationX = (int16)((int16)((int32)(int16)_zoomFactor * x) - _zoomOffsetX);
		int16 destinationY = (int16)((int16)((int32)(int16)_zoomFactor * y) - _zoomOffsetY);

		engine()->display()->putBitmapRowsStretched(destinationX, destinationY, (uint16)(width * _zoomFactor), (uint16)(height * _zoomFactor), width, height, pixels, pitch, &_colorTable[0][0]);
	} else {
		engine()->display()->putBitmapRows(x, y, width, height, pixels, pitch, &_colorTable[0][0]);
	}
}

uint16 SpriteLibrary_v1_2::spritePicked(int16 x, int16 y) {
	if (_clipRect.left > x || _clipRect.right <= x || _clipRect.top > y || _clipRect.bottom <= y)
		return 0;

	for (int index = (int)_spriteCount - 1; index >= 0; index--) {
		uint16 handle = _spriteTable[index];
		if (spriteIsPicked(handle, x, y) != 0)
			return handle;
	}

	return 0;
}

uint16 SpriteLibrary_v1_2::spriteIsPicked(uint16 sprite, int16 x, int16 y) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteIsPicked");
	if (!object || (!object->visible && !object->pickInvisible))
		return 0;

	const Common::Rect &box = object->boundingBox;
	if (box.left > x || box.right <= x || box.top > y || box.bottom <= y)
		return 0;

	if (object->pickWholeBox)
		return 1;

	if (object->partCount != 0) {
		if (!object->parts) {
			warning("SpriteLibrary_v1_2::spriteIsPicked(): the multipart sprite has no parts");
			return 0;
		}

		for (uint16 part = 0; part < object->partCount && part < TG_SPR_V1_2_NUM_SPRITE_PARTS; part++) {
			const SpritePart &spritePart = object->parts->parts[part];
			if (spritePart.box.left > x || spritePart.box.right <= x || spritePart.box.top > y || spritePart.box.bottom <= y)
				continue;

			if (!spritePart.frame) {
				warning("SpriteLibrary_v1_2::spriteIsPicked(): the sprite part has no frame");
				continue;
			}

			if (getBitmapPixel(spritePart.frame, (int16)(x - spritePart.box.left), (int16)(y - spritePart.box.top), flipFlags(object)) != 0)
				return 1;
		}

		return 0;
	}

	if (!object->currentFrame) {
		warning("SpriteLibrary_v1_2::spriteIsPicked(): the sprite has no frame");
		return 0;
	}

	return (getBitmapPixel(object->currentFrame, (int16)(x - box.left), (int16)(y - box.top), flipFlags(object)) != 0) ? 1 : 0;
}

void SpriteLibrary_v1_2::renderSpriteToSurface(uint16 sprite) {
	if (_engine->_version >= kVersionV1_2_3) {
		SpriteObject_v1_2 *object = findSprite(sprite, "renderSpriteToSurface");
		if (!object)
			return;

		object->renderPending = false;
	}

	SpriteLibrary::renderSpriteToSurface(sprite);
}

uint16 SpriteLibrary_v1_2::spriteHitDetect(uint16 spriteA, uint16 spriteB) {
	SpriteObject_v1_2 *first = findSprite(spriteA, "spriteHitDetect");
	SpriteObject_v1_2 *second = findSprite(spriteB, "spriteHitDetect");
	if (!first || !second || !second->visible || !first->visible)
		return 0;

	Common::Rect overlap;
	if (!intersectRectLocal(first->boundingBox, second->boundingBox, overlap))
		return 0;

	if (first->partCount != 0 || second->partCount != 0)
		return 1;

	if (!first->currentFrame || !second->currentFrame) {
		warning("SpriteLibrary_v1_2::spriteHitDetect(): the sprite has no current frame");
		return 0;
	}

	int16 width = (int16)(overlap.right - overlap.left);
	int16 height = (int16)(overlap.bottom - overlap.top);
	int16 firstX = (int16)(overlap.left - first->boundingBox.left);
	int16 firstY = (int16)(overlap.top - first->boundingBox.top);
	int16 secondX = (int16)(overlap.left - second->boundingBox.left);
	int16 secondY = (int16)(overlap.top - second->boundingBox.top);

	if (first->mirrorX)
		firstX = (int16)(READ_LE_UINT16(first->currentFrame + TG_SPR_BITMAP_WIDTH_OFFSET) - firstX - width);

	if (first->flipY)
		firstY = (int16)(READ_LE_UINT16(first->currentFrame + TG_SPR_BITMAP_HEIGHT_OFFSET) - firstY - height);

	if (second->mirrorX)
		secondX = (int16)(READ_LE_UINT16(second->currentFrame + TG_SPR_BITMAP_WIDTH_OFFSET) - secondX - width);

	if (second->flipY)
		secondY = (int16)(READ_LE_UINT16(second->currentFrame + TG_SPR_BITMAP_HEIGHT_OFFSET) - secondY - height);

	return blitPixelOverlapTest(first->currentFrame, second->currentFrame, (uint16)width, (uint16)height, firstX, firstY, secondX, secondY, 0);
}

uint16 SpriteLibrary_v1_2_3::hitTestMultipartDispatch(const SpriteObject_v1_2 *first, const SpriteObject_v1_2 *second) {
	if (first->partCount != 0 && second->partCount != 0)
		return hitTestMultipartPair(first, second);

	if (first->partCount != 0)
		return hitTestMultipartVsSingle(second, first);

	return hitTestMultipartVsSingle(first, second);
}

uint16 SpriteLibrary_v1_2_3::hitTestMultipartPair(const SpriteObject_v1_2 *first, const SpriteObject_v1_2 *second) {
	if (!first->parts || !second->parts) {
		warning("SpriteLibrary_v1_2_3::hitTestMultipartPair(): the multipart sprite has no parts");
		return 0;
	}

	for (uint16 i = 0; i < first->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		const SpritePart &firstPart = first->parts->parts[i];

		for (uint16 j = 0; j < second->partCount && j < TG_SPR_V1_2_NUM_SPRITE_PARTS; j++) {
			const SpritePart &secondPart = second->parts->parts[j];

			Common::Rect overlap;
			if (!intersectRectLocal(firstPart.box, secondPart.box, overlap))
				continue;

			if (!firstPart.frame || !secondPart.frame) {
				warning("SpriteLibrary_v1_2_3::hitTestMultipartPair(): the sprite part has no frame");
				continue;
			}

			uint16 hit = blitPixelOverlapTest(firstPart.frame, secondPart.frame, (uint16)(overlap.right - overlap.left), (uint16)(overlap.bottom - overlap.top),
			                                  (int16)(overlap.left - firstPart.box.left), (int16)(overlap.top - firstPart.box.top),
			                                  (int16)(overlap.left - secondPart.box.left), (int16)(overlap.top - secondPart.box.top),
			                                  flipFlags(first), flipFlags(second));
			if (hit != 0)
				return 1;
		}
	}

	return 0;
}

uint16 SpriteLibrary_v1_2_3::hitTestMultipartVsSingle(const SpriteObject_v1_2 *single, const SpriteObject_v1_2 *multipart) {
	if (!multipart->parts) {
		warning("SpriteLibrary_v1_2_3::hitTestMultipartVsSingle(): the multipart sprite has no parts");
		return 0;
	}

	for (uint16 i = 0; i < multipart->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		const SpritePart &part = multipart->parts->parts[i];

		Common::Rect overlap;
		if (!intersectRectLocal(single->boundingBox, part.box, overlap))
			continue;

		if (!single->currentFrame || !part.frame) {
			warning("SpriteLibrary_v1_2_3::hitTestMultipartVsSingle(): the sprite has no frame");
			continue;
		}

		uint16 hit = blitPixelOverlapTest(single->currentFrame, part.frame, (uint16)(overlap.right - overlap.left), (uint16)(overlap.bottom - overlap.top),
		                                  (int16)(overlap.left - single->boundingBox.left), (int16)(overlap.top - single->boundingBox.top),
		                                  (int16)(overlap.left - part.box.left), (int16)(overlap.top - part.box.top),
		                                  flipFlags(single), flipFlags(multipart));
		if (hit != 0)
			return 1;
	}

	return 0;
}

uint16 SpriteLibrary_v1_2_3::spriteHitDetect(uint16 spriteA, uint16 spriteB, uint16 allowInvisibleA, uint16 allowInvisibleB, uint16 sameLevelOnly) {
	SpriteObject_v1_2 *first = findSprite(spriteA, "spriteHitDetect");
	SpriteObject_v1_2 *second = findSprite(spriteB, "spriteHitDetect");
	if (!first || !second)
		return 0;

	if (sameLevelOnly != 0 && first->level != second->level)
		return 0;

	if (!first->visible && allowInvisibleA == 0)
		return 0;

	if (!second->visible && allowInvisibleB == 0)
		return 0;

	Common::Rect overlap;
	if (!intersectRectLocal(first->boundingBox, second->boundingBox, overlap))
		return 0;

	if (first->partCount == 0 && second->partCount == 0) {
		if (!first->currentFrame || !second->currentFrame)
			return 0;

		return blitPixelOverlapTest(first->currentFrame, second->currentFrame, (uint16)(overlap.right - overlap.left), (uint16)(overlap.bottom - overlap.top),
		                            (int16)(overlap.left - first->boundingBox.left), (int16)(overlap.top - first->boundingBox.top),
		                            (int16)(overlap.left - second->boundingBox.left), (int16)(overlap.top - second->boundingBox.top),
		                            flipFlags(first), flipFlags(second));
	}

	return hitTestMultipartDispatch(first, second);
}

uint16 SpriteLibrary_v1_2_3::spriteHitDetectRect(uint16 spriteA, uint16 spriteB, const Common::Rect &rect) {
	SpriteObject_v1_2 *first = findSprite(spriteA, "spriteHitDetectRect");
	SpriteObject_v1_2 *second = findSprite(spriteB, "spriteHitDetectRect");
	if (!first || !second)
		return 0;

	if (!first->visible || !second->visible)
		return 0;

	Common::Rect overlap;
	if (!intersectRectLocal(rect, second->boundingBox, overlap))
		return 0;

	if (first->partCount == 0 && second->partCount == 0) {
		if (!first->currentFrame || !second->currentFrame)
			return 0;

		return blitPixelOverlapTest(first->currentFrame, second->currentFrame, (uint16)(overlap.right - overlap.left), (uint16)(overlap.bottom - overlap.top),
		                            (int16)(overlap.left - first->boundingBox.left), (int16)(overlap.top - first->boundingBox.top),
		                            (int16)(overlap.left - second->boundingBox.left), (int16)(overlap.top - second->boundingBox.top),
		                            flipFlags(first), flipFlags(second));
	}

	return hitTestMultipartDispatch(first, second);
}

void SpriteLibrary_v1_2_3::spriteDibGetBox(const byte *bitmap, Common::Rect &box) {
	int16 width = (int16)READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 height = (int16)READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
	int16 heightMinusOne = (int16)(height - 1);

	box.left = (int16)((int16)(1 - width) / 2 + READ_LE_INT16(bitmap + TG_SPR_BITMAP_HOT_X_OFFSET));
	box.right = (int16)(width + box.left);
	box.top = (int16)(READ_LE_INT16(bitmap + TG_SPR_BITMAP_HOT_Y_OFFSET) - heightMinusOne / 2);
	box.bottom = (int16)(height + box.top);
}

void SpriteLibrary_v1_2_3::spriteScreenShake(int16 deltaX, int16 deltaY) {
	Display *display = engine()->display();

	int16 clientWidth = display->width();
	int16 clientHeight = display->height();
	int16 widthDifference = (int16)(clientWidth - _clipRect.right + _clipRect.left - 1);
	int16 heightDifference = (int16)(clientHeight - _clipRect.bottom + _clipRect.top - 1);

	Common::Rect area = _clipRect;
	if (widthDifference < 0 || heightDifference < 0) {
		int16 inflateX = widthDifference / 2;
		int16 inflateY = heightDifference / 2;
		area.left = (int16)(area.left - inflateX);
		area.right = (int16)(area.right + inflateX);
		area.top = (int16)(area.top - inflateY);
		area.bottom = (int16)(area.bottom + inflateY);
	}

	display->scrollDevice(area, deltaX, deltaY);
	display->scrollDevice(area, (int16)(-2 * deltaX), (int16)(-2 * deltaY));
	display->scrollDevice(area, deltaX, deltaY);

	int16 stripWidth = (deltaX < 0) ? (int16)-deltaX : deltaX;
	int16 stripHeight = (deltaY < 0) ? (int16)-deltaY : deltaY;

	if (stripWidth != 0) {
		Common::Rect strip = area;
		strip.right = (int16)(stripWidth + strip.left);
		spritePaintRect(strip, false);

		strip.right = area.right;
		strip.left = (int16)(area.right - stripWidth);
		spritePaintRect(strip, false);
	}

	if (stripHeight != 0) {
		Common::Rect strip = area;
		strip.bottom = (int16)(stripHeight + strip.top);
		spritePaintRect(strip, false);

		strip.bottom = area.bottom;
		strip.top = (int16)(area.bottom - stripHeight);
		spritePaintRect(strip, false);
	}
}

bool SpriteLibrary_v1_2_3::isRleBitmap(const byte *bitmap) const {
	return (bitmap[TG_SPR_V1_2_3_BITMAP_FLAGS_OFFSET] & TG_SPR_V1_2_3_BITMAP_RLE_FLAG) != 0;
}

void SpriteLibrary_v1_2_3::decodeRleRow(const byte *bitmap, uint16 row, uint16 firstColumn, uint16 count, byte *values, byte *forced) const {
	const byte *pixels = bitmapPixels(bitmap);
	const byte *position = pixels + READ_LE_UINT16(pixels + 2 * row);

	uint32 column = 0;
	uint16 filled = 0;

	while (filled < count) {
		byte code = *position++;
		if (code == 0)
			break;

		uint16 length;
		bool literal = false;
		bool tested = false;
		byte runValue = 0;

		if (code <= TG_SPR_V1_2_3_RLE_LAST_RUN_CODE) {
			length = code;
			runValue = *position++;
		} else {
			length = (uint16)(TG_SPR_V1_2_3_RLE_TESTED_LITERAL_CODE - code);
			literal = true;
			if (length == 0) {
				length = *position++;
				tested = true;
			}
		}

		for (uint16 i = 0; i < length; i++) {
			byte value = literal ? *position++ : runValue;
			if (column >= firstColumn && filled < count) {
				values[filled] = value;
				forced[filled] = (literal && !tested) ? 1 : 0;
				filled++;
			}

			column++;
		}
	}

	if (filled < count) {
		warning("SpriteLibrary_v1_2_3::decodeRleRow(): the row ends before the requested pixels");
		for (; filled < count; filled++) {
			values[filled] = 0;
			forced[filled] = 0;
		}
	}
}

bool SpriteLibrary_v1_2_3::blitSetupRleSource(byte *destination, const byte *source, uint16 &width, uint16 &height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags, RleBlitSetup &setup) {
	uint16 sourceWidth = READ_LE_UINT16(source + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 sourceHeight = READ_LE_UINT16(source + TG_SPR_BITMAP_HEIGHT_OFFSET);
	uint16 destinationWidth = READ_LE_UINT16(destination + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 destinationPitch = READ_LE_UINT16(destination + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 destinationHeight = READ_LE_UINT16(destination + TG_SPR_BITMAP_HEIGHT_OFFSET);

	bool inside = (destinationX >= 0 && destinationY >= 0 && sourceX >= 0 && sourceY >= 0 &&
	               destinationX + width <= destinationWidth && destinationY + height <= destinationHeight &&
	               sourceX + width <= sourceWidth && sourceY + height <= sourceHeight);

	if (!inside) {
		warning("SpriteLibrary_v1_2_3::blitSetupRleSource(): the rectangle is outside the bitmaps");

		if (destinationX < 0 || destinationY < 0 || sourceX < 0 || sourceY < 0)
			return false;

		int availableWidth = MIN(sourceWidth - sourceX, destinationWidth - destinationX);
		int availableHeight = MIN(sourceHeight - sourceY, destinationHeight - destinationY);
		if (availableWidth <= 0 || availableHeight <= 0)
			return false;

		width = MIN((int)width, availableWidth);
		height = MIN((int)height, availableHeight);
	}

	bool mirror = ((flags & TG_SPR_V1_2_3_MIRROR_FLAG_MASK) != 0);
	bool flip = ((flags & TG_SPR_V1_2_3_FLIP_FLAG_MASK) != 0);

	if (mirror)
		setup.firstColumn = (uint16)(sourceWidth - width - sourceX);
	else
		setup.firstColumn = (uint16)sourceX;

	if (flip)
		setup.firstRow = (int32)sourceHeight - sourceY - 1;
	else
		setup.firstRow = (int32)sourceY + height - 1;

	int32 destinationRow;
	if (destination[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET] != 1) {
		if (flip) {
			destinationRow = (int32)destinationHeight - destinationY - 1;
			setup.destinationStride = -(int32)destinationPitch;
		} else {
			destinationRow = (int32)destinationHeight - destinationY - height;
			setup.destinationStride = destinationPitch;
		}
	} else {
		if (flip) {
			destinationRow = destinationY;
			setup.destinationStride = destinationPitch;
		} else {
			destinationRow = (int32)destinationY + height - 1;
			setup.destinationStride = -(int32)destinationPitch;
		}
	}

	setup.destination = bitmapPixels(destination) + destinationRow * (int32)destinationPitch + destinationX;
	return true;
}

void SpriteLibrary_v1_2_3::blitRleRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags, RleBlitMode_v1_2_3 mode) {
	RleBlitSetup setup;
	if (!blitSetupRleSource(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, setup))
		return;

	Common::Array<byte> values;
	Common::Array<byte> forced;
	values.resize(width);
	forced.resize(width);

	bool mirror = ((flags & TG_SPR_V1_2_3_MIRROR_FLAG_MASK) != 0);

	for (uint16 row = 0; row < height; row++) {
		decodeRleRow(source, (uint16)(setup.firstRow - row), setup.firstColumn, width, &values[0], &forced[0]);

		byte *line = setup.destination + (int32)row * setup.destinationStride;
		for (uint16 column = 0; column < width; column++) {
			uint16 index = mirror ? (uint16)(width - 1 - column) : column;
			byte value = values[index];
			bool opaque = (value != 0 || forced[index] != 0);

			switch (mode) {
			case kSprV1_2_3BlitCopy:
				line[column] = value;
				break;

			case kSprV1_2_3BlitTransparent:
				if (opaque)
					line[column] = value;

				break;

			case kSprV1_2_3BlitMasked:
				if (opaque && line[column] != 0)
					line[column] = value;

				break;
			}
		}
	}
}

void SpriteLibrary_v1_2_3::blitOpaqueRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	if (isRleBitmap(source))
		blitRleRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, kSprV1_2_3BlitCopy);
	else
		SpriteLibrary_v1_2::blitOpaqueRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags);
}

void SpriteLibrary_v1_2_3::blitTransparentRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	if (isRleBitmap(source))
		blitRleRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, kSprV1_2_3BlitTransparent);
	else
		SpriteLibrary_v1_2::blitTransparentRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags);
}

void SpriteLibrary_v1_2_3::blitMaskedRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	if (isRleBitmap(source))
		blitRleRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, kSprV1_2_3BlitMasked);
	else
		SpriteLibrary_v1_2::blitMaskedRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags);
}

void SpriteLibrary_v1_2_3::fetchBitmapRow(const byte *bitmap, int32 row, int32 firstColumn, uint16 count, byte *values) const {
	int32 width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	int32 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);

	memset(values, 0, count);

	if (row < 0 || row >= height)
		return;

	int32 from = MAX(firstColumn, (int32)0);
	int32 to = MIN(firstColumn + count, width);
	if (from >= to)
		return;

	uint16 length = (uint16)(to - from);

	if (isRleBitmap(bitmap)) {
		Common::Array<byte> forced;
		forced.resize(length);
		decodeRleRow(bitmap, (uint16)row, (uint16)from, length, values + (from - firstColumn), &forced[0]);
	} else {
		uint16 pitch = READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
		const byte *line = bitmapPixels(bitmap) + (int32)(height - 1 - row) * pitch;
		memcpy(values + (from - firstColumn), line + from, length);
	}
}

uint16 SpriteLibrary_v1_2_3::blitPixelOverlapTest(const byte *first, const byte *second, uint16 width, uint16 height, int16 firstX, int16 firstY, int16 secondX, int16 secondY, uint16 firstFlags, uint16 secondFlags) {
	if (width == 0 || height == 0)
		return 0;

	int16 firstWidth = (int16)READ_LE_UINT16(first + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 firstHeight = (int16)READ_LE_UINT16(first + TG_SPR_BITMAP_HEIGHT_OFFSET);
	int16 secondWidth = (int16)READ_LE_UINT16(second + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 secondHeight = (int16)READ_LE_UINT16(second + TG_SPR_BITMAP_HEIGHT_OFFSET);

	if ((firstFlags & TG_SPR_V1_2_3_MIRROR_FLAG_MASK) != 0)
		firstX = (int16)(firstWidth - firstX - width);

	if ((firstFlags & TG_SPR_V1_2_3_FLIP_FLAG_MASK) != 0)
		firstY = (int16)(firstHeight - firstY - height);

	if ((secondFlags & TG_SPR_V1_2_3_MIRROR_FLAG_MASK) != 0)
		secondX = (int16)(secondWidth - secondX - width);

	if ((secondFlags & TG_SPR_V1_2_3_FLIP_FLAG_MASK) != 0)
		secondY = (int16)(secondHeight - secondY - height);

	if (firstX < 0 || firstY < 0 || firstX + width > firstWidth || firstY + height > firstHeight ||
	    secondX < 0 || secondY < 0 || secondX + width > secondWidth || secondY + height > secondHeight)
		warning("SpriteLibrary_v1_2_3::blitPixelOverlapTest(): the rectangle is outside the bitmaps");

	Common::Array<byte> firstRow;
	Common::Array<byte> secondRow;
	firstRow.resize(width);
	secondRow.resize(width);

	for (uint16 row = 0; row < height; row++) {
		fetchBitmapRow(first, firstY + row, firstX, width, &firstRow[0]);
		fetchBitmapRow(second, secondY + row, secondX, width, &secondRow[0]);

		for (uint16 column = 0; column < width; column++) {
			if (firstRow[column] != 0 && secondRow[column] != 0)
				return 1;
		}
	}

	return 0;
}

uint16 SpriteLibrary_v1_2_3::getBitmapPixel(const byte *bitmap, int16 x, int16 y, uint16 flags) {
	int32 width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	int32 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);

	bool mirror = ((flags & TG_SPR_V1_2_3_MIRROR_FLAG_MASK) != 0);
	bool flip = ((flags & TG_SPR_V1_2_3_FLIP_FLAG_MASK) != 0);

	int32 column = mirror ? width - x - 1 : x;
	int32 row = flip ? height - y - 1 : y;

	if (column < 0 || column >= width || row < 0 || row >= height) {
		warning("SpriteLibrary_v1_2_3::getBitmapPixel(): the pixel is outside the bitmap");
		return 0;
	}

	if (isRleBitmap(bitmap)) {
		byte value = 0;
		byte forced = 0;
		decodeRleRow(bitmap, (uint16)row, (uint16)column, 1, &value, &forced);
		return value;
	}

	uint16 pitch = READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	return bitmapPixels(bitmap)[(height - 1 - row) * pitch + column];
}

} // End of namespace Topgun
