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
#include "common/textconsole.h"
#include "common/util.h"

#include "topgun/display.h"
#include "topgun/libraries/sprite/sprite_v1_2.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2.h"

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
	if (((flags >> 8) & 0xFF) == source[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET]) {
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

void SpriteLibrary_v1_2::compositeSpriteIntoBackground(uint16 sprite) {
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

	spriteHide(sprite, true);
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

		blitTransparentRect(_backgroundBitmap, part.frame, (uint16)(common.right - common.left), (uint16)(common.bottom - common.top),
		                    (int16)(common.left + _scrollX), (int16)(common.top + _scrollY),
		                    (int16)(common.left - object->boundingBox.left), (int16)(common.top - object->boundingBox.top), flipFlags(object));
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
		SpriteObject_v1_2 *object = findSprite(handle, "spritePicked");
		if (!object || !object->visible)
			continue;

		const Common::Rect &box = object->boundingBox;
		if (box.left > x || box.right <= x || box.top > y || box.bottom <= y)
			continue;

		if (object->pickWholeBox)
			return handle;

		if (object->partCount != 0) {
			if (!object->parts) {
				warning("SpriteLibrary_v1_2::spritePicked(): the multipart sprite has no parts");
				continue;
			}

			for (uint16 part = 0; part < object->partCount && part < TG_SPR_V1_2_NUM_SPRITE_PARTS; part++) {
				const SpritePart &spritePart = object->parts->parts[part];
				if (spritePart.box.left > x || spritePart.box.right <= x || spritePart.box.top > y || spritePart.box.bottom <= y)
					continue;

				if (!spritePart.frame) {
					warning("SpriteLibrary_v1_2::spritePicked(): the sprite part has no frame");
					continue;
				}

				if (getBitmapPixel(spritePart.frame, (int16)(x - spritePart.box.left), (int16)(y - spritePart.box.top), flipFlags(object)) != 0)
					return handle;
			}
		} else {
			if (!object->currentFrame) {
				warning("SpriteLibrary_v1_2::spritePicked(): the sprite has no frame");
				continue;
			}

			if (getBitmapPixel(object->currentFrame, (int16)(x - box.left), (int16)(y - box.top), flipFlags(object)) != 0)
				return handle;
		}
	}

	return 0;
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

} // End of namespace Topgun
