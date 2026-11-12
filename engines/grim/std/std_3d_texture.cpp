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

#include "common/scummsys.h"

#include "engines/grim/std/std_3d_config.h"

#if defined(GRIM_STD3D)

#include "common/endian.h"
#include "common/textconsole.h"

#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_3d_backend.h"
#include "engines/grim/std/std_bitmap.h"

namespace Grim {

static byte expandChannel(uint value, int bits) {
	if (bits <= 0 || bits > 8)
		return 0;

	uint result = value;
	int filled = bits;

	while (filled < 8) {
		result = (result << bits) | value;
		filled += bits;
	}

	return (byte)(result >> (filled - 8));
}

static void texelToRgba(uint texel, const StdColorFormat *format, byte *out) {
	uint red = (texel >> format->redShift) & ((1u << format->redBits) - 1);
	uint green = (texel >> format->greenShift) & ((1u << format->greenBits) - 1);
	uint blue = (texel >> format->blueShift) & ((1u << format->blueBits) - 1);

	out[0] = expandChannel(red, format->redBits);
	out[1] = expandChannel(green, format->greenBits);
	out[2] = expandChannel(blue, format->blueBits);

	if (format->alphaBits > 0) {
		uint alpha = (texel >> format->alphaShift) & ((1u << format->alphaBits) - 1);
		out[3] = expandChannel(alpha, format->alphaBits);
	} else {
		out[3] = 255;
	}
}

static int isValidFormat(const StdColorFormat *format) {
	return format->bpp == 16 && format->redBits > 0 && format->redBits <= 8 && format->greenBits > 0 && format->greenBits <= 8 &&
		format->blueBits > 0 && format->blueBits <= 8 && format->alphaBits >= 0 && format->alphaBits <= 8 &&
		format->redShift >= 0 && format->redShift < 16 && format->greenShift >= 0 && format->greenShift < 16 &&
		format->blueShift >= 0 && format->blueShift < 16 && format->alphaShift >= 0 && format->alphaShift < 16;
}

int Std3D::setCurrentPalette(const byte *palette, byte alpha) {
	if (!_curDevice || !_texFormatOpaque) {
		warning("Std3D::setCurrentPalette(): device not started");
		return 0;
	}

	memcpy(_palette, palette, sizeof(_palette));

	if (_texFormatOpaque->format.colorMode == 1 && _texFormatOpaque->format.bpp == 16)
		_stdBitmap->buildPaletteOpaque(palette, _opaquePaletteTable, &_texFormatOpaque->format);

	if (_curDevice->alphaTexture) {
		if (_texFormatAlpha->format.colorMode == 2 && _texFormatAlpha->format.bpp == 16)
			_stdBitmap->buildPaletteAlpha(palette, _alphaPaletteTable, &_texFormatAlpha->format);

		if (!_curDevice->alphaBlend && _texFormatColorKey->format.colorMode == 2 && _texFormatColorKey->format.bpp == 16)
			_stdBitmap->buildPaletteColorKey(palette, _colorKeyPaletteTable, &_texFormatColorKey->format, alpha);
	}

	return 1;
}

void Std3D::getValidDimensions(uint width, uint height, uint *widthPointer, uint *heightPointer) {
	int squareTexturesOnly = 0;

	if (_curDevice) {
		squareTexturesOnly = _curDevice->squareTexturesOnly;
	} else {
		warning("Std3D::getValidDimensions(): device not started");
	}

	uint w = width;

	if (width) {
		if (width > _maxTexWidth)
			w = _maxTexWidth;
	} else {
		w = 1;
	}

	uint h = height;

	if (height) {
		if (height > _maxTexWidth)
			h = _maxTexWidth;
	} else {
		h = 1;
	}

	if (w >= _minTexWidth && h >= _minTexHeight && (!squareTexturesOnly || w == h)) {
		*widthPointer = w;
		*heightPointer = h;
		return;
	}

	if (!squareTexturesOnly || w == h) {
		if (_minTexWidth > w)
			w = _minTexWidth;

		if (_minTexHeight > h)
			h = _minTexHeight;

		*widthPointer = w;
		*heightPointer = h;
		return;
	}

	if (w <= h)
		w = h;

	*widthPointer = w;
	*heightPointer = w;
}

void Std3D::releaseTexture(RdTexCacheEntry *entry) {
	if (entry->texture && _backend)
		_backend->deleteTexture(entry->texture);

	entry->texture = 0;
}

void Std3D::purgeTextureCache() {
	while (_cacheHead) {
		RdTexCacheEntry *entry = _cacheHead;

		releaseTexture(entry);
		entry->inCache = 0;
		entry->lastFrame = 0;
		cacheListRemove(entry);
	}
}

int Std3D::addToTextureCache(StdVBuffer *src, RdTexCacheEntry *entry, int alpha, int colorKey) {
	if (!_curDevice || !_texFormatOpaque || !ensureBackend()) {
		warning("Std3D::addToTextureCache(): device not started");
		return 0;
	}

	if (!src || !entry || src->tex.width <= 0 || src->tex.height <= 0 || !src->pixels) {
		warning("Std3D::addToTextureCache(): invalid source");
		return 0;
	}

	StdVBuffer *buf = src;
	StdVBuffer *dst = nullptr;
	uint width = src->tex.width;

	if (width > 0x100)
		width = 256;

	uint srcWidth = width;
	uint height = src->tex.height;

	if (height > 0x100)
		height = 256;

	uint srcHeight = height;

	if (width < _minTexWidth || height < _minTexHeight || (_curDevice->squareTexturesOnly && width != height)) {
		StdVBufferTexFmt format = src->tex;

		if (!_curDevice->squareTexturesOnly || width == height) {
			if (_minTexWidth > width)
				width = _minTexWidth;

			if (_minTexHeight > height)
				height = _minTexHeight;
		} else {
			if (width <= height)
				width = height;

			height = width;
		}

		int tilesX = (int)(float)((double)width / (double)srcWidth + 0.5);
		int tilesY = (int)(float)((double)height / (double)srcHeight + 0.5);

		format.width *= tilesX;
		format.height *= tilesY;
		dst = _stdDisplay->vBufferNew(&format, 0, 0, nullptr);

		if (!dst) {
			warning("Std3D::addToTextureCache(): could not create the tiled texture");
			entry->texture = 0;
			entry->inCache = 0;
			entry->lastFrame = 0;
			return 0;
		}

		int y = 0;

		for (int row = 0; row < tilesY; row++) {
			int tileX = 0;

			for (int column = 0; column < tilesX; column++) {
				_stdDisplay->vBufferCopy(dst, buf, tileX, y, nullptr, 0);
				tileX += srcWidth;
			}

			y += srcHeight;
		}

		buf = dst;
		srcWidth = format.width;
		srcHeight = format.height;
		height = format.height;
		width = format.width;
	}

	uint size = width * height;
	const Std3DTexFormat *texFormat;

	if (alpha && _curDevice->alphaTexture) {
		entry->colorKey = 1;

		if (colorKey)
			texFormat = _texFormatColorKey;
		else
			texFormat = _texFormatAlpha;
	} else if (colorKey) {
		entry->colorKey = 1;
		texFormat = _texFormatColorKey;
	} else {
		entry->colorKey = 0;
		texFormat = _texFormatOpaque;
	}

	if (!texFormat || !isValidFormat(&texFormat->format) || (int)width > _backend->getMaxTextureSize() || (int)height > _backend->getMaxTextureSize()) {
		warning("Std3D::addToTextureCache(): invalid texture format or size");

		if (dst)
			_stdDisplay->vBufferFree(dst);

		entry->texture = 0;
		entry->inCache = 0;
		entry->lastFrame = 0;
		return 0;
	}

	byte *rgba = new byte[width * height * 4];
	memset(rgba, 0, width * height * 4);

	const StdColorFormat *texelFormat = &texFormat->format;
	int colorMode = buf->tex.format.colorMode;
	uint16 colorKeyValue = 0;
	int useColorKey = 0;

	if (colorMode == 0) {
		_stdDisplay->vBufferLock(buf);

		const uint16 *table;

		if (alpha && _curDevice->alphaTexture) {
			if (colorKey)
				table = _colorKeyPaletteTable;
			else
				table = _alphaPaletteTable;
		} else if (colorKey) {
			table = _colorKeyPaletteTable;
		} else {
			table = _opaquePaletteTable;
		}

		for (uint i = 0; i < srcHeight; i++) {
			const byte *rowPointer = &buf->pixels[buf->tex.pitchInBytes * i];

			for (uint x = 0; x < srcWidth; x++)
				texelToRgba(table[rowPointer[x]], texelFormat, &rgba[(i * width + x) * 4]);
		}

		_stdDisplay->vBufferUnlock(buf);
	} else if (colorMode > 0 && colorMode <= 2) {
		_stdDisplay->vBufferLock(buf);

		for (uint i = 0; i < srcHeight; i++) {
			const byte *rowPointer = &buf->pixels[buf->tex.pitchInBytes * i];

			for (uint x = 0; x < srcWidth; x++)
				texelToRgba(READ_LE_UINT16(rowPointer + 2 * x), texelFormat, &rgba[(i * width + x) * 4]);
		}

		_stdDisplay->vBufferUnlock(buf);
	} else {
		warning("Std3D::addToTextureCache(): unsupported source color mode");
	}

	if (alpha && !_curDevice->alphaTexture) {
		if (colorMode == 0) {
			colorKeyValue = _opaquePaletteTable[0];
			useColorKey = 1;
		} else if (colorMode == 1) {
			colorKeyValue = (uint16)buf->colorKey;
			useColorKey = 1;
		}

		if (useColorKey) {
			for (uint i = 0; i < srcHeight; i++) {
				const byte *rowPointer = &buf->pixels[buf->tex.pitchInBytes * i];

				for (uint x = 0; x < srcWidth; x++) {
					uint16 texel = colorMode == 0 ? _opaquePaletteTable[rowPointer[x]] : READ_LE_UINT16(rowPointer + 2 * x);

					if (texel == colorKeyValue)
						rgba[(i * width + x) * 4 + 3] = 0;
				}
			}
		}
	}

	if (size > _curDevice->texMemFree) {
		evictTextures(size);

		if (size > _curDevice->texMemFree) {
			delete[] rgba;

			if (dst)
				_stdDisplay->vBufferFree(dst);

			entry->texture = 0;
			entry->inCache = 0;
			entry->lastFrame = 0;
			return 0;
		}
	}

	uint texture = _backend->createTexture(rgba, width, height, _texMagLinear, _texMinLinear);

	if (!texture) {
		delete[] rgba;

		if (dst)
			_stdDisplay->vBufferFree(dst);

		entry->texture = 0;
		entry->inCache = 0;
		entry->lastFrame = 0;
		return 0;
	}

	delete[] rgba;

	if (dst)
		_stdDisplay->vBufferFree(dst);

	entry->texture = texture;
	entry->width = width;
	entry->height = srcHeight;
	entry->size = size;
	entry->inCache = 1;
	entry->lastFrame = _frameCounter;
	cacheListAppend(entry);
	return 1;
}

void Std3D::flushTextureCache() {
	RdTexCacheEntry *entry = _cacheHead;

	while (entry) {
		RdTexCacheEntry *next = entry->next;

		releaseTexture(entry);
		entry->inCache = 0;
		entry->lastFrame = 0;
		entry->next = nullptr;
		entry->prev = nullptr;
		entry = next;
	}

	_cacheHead = nullptr;
	_cacheTail = nullptr;
	_numCachedTextures = 0;

	if (_curDevice)
		_curDevice->texMemFree = _curDevice->texMemTotal;
	else
		warning("Std3D::flushTextureCache(): device not started");

	_frameCounter = 1;
}

void Std3D::cacheListAppend(RdTexCacheEntry *entry) {
	if (_cacheHead) {
		_cacheTail->next = entry;
		entry->prev = _cacheTail;
		entry->next = nullptr;
		_cacheTail = entry;
	} else {
		_cacheTail = entry;
		_cacheHead = entry;
		entry->prev = nullptr;
		entry->next = nullptr;
	}

	_numCachedTextures++;

	if (_curDevice)
		_curDevice->texMemFree -= entry->size;
	else
		warning("Std3D::cacheListAppend(): device not started");
}

void Std3D::cacheListRemove(RdTexCacheEntry *entry) {
	if (entry == _cacheHead) {
		_cacheHead = entry->next;

		if (_cacheHead) {
			_cacheHead->prev = nullptr;

			if (!_cacheHead->next)
				_cacheTail = _cacheHead;
		} else {
			_cacheTail = nullptr;
		}
	} else {
		RdTexCacheEntry *prev = entry->prev;

		if (entry == _cacheTail) {
			_cacheTail = entry->prev;
			prev->next = nullptr;
		} else {
			prev->next = entry->next;
			entry->next->prev = entry->prev;
		}
	}

	_numCachedTextures--;

	if (_curDevice)
		_curDevice->texMemFree += entry->size;
	else
		warning("Std3D::cacheListRemove(): device not started");
}

int Std3D::getTextureMemory(uint *total, uint *free) {
	uint used = 0;

	for (RdTexCacheEntry *entry = _cacheHead; entry; entry = entry->next)
		used += entry->size;

	uint budget = _backend ? _backend->getTextureBudget() : (uint)kStd3DTextureMemory;

	*total = budget;
	*free = budget - used;
	return 1;
}

void Std3D::cacheListTouch(RdTexCacheEntry *entry) {
	entry->lastFrame = _frameCounter;
	cacheListRemove(entry);
	cacheListAppend(entry);
}

int Std3D::evictTextures(uint size) {
	RdTexCacheEntry *scan = _cacheHead;
	uint bestDiff = (uint)-1;
	RdTexCacheEntry *best = nullptr;

	for (RdTexCacheEntry *entry = _cacheHead; entry; entry = entry->next) {
		if (entry->lastFrame != _frameCounter) {
			uint entrySize = entry->size;

			if (entrySize == size) {
				releaseTexture(entry);
				entry->inCache = 0;
				entry->lastFrame = 0;
				cacheListRemove(entry);
				return 1;
			}

			if (entrySize > size) {
				uint diff = entrySize - size;

				if (diff < bestDiff) {
					bestDiff = diff;
					best = entry;
				}
			}
		}
	}

	if (best) {
		releaseTexture(best);
		best->inCache = 0;
		best->lastFrame = 0;
		cacheListRemove(best);
		return 1;
	}

	while (scan) {
		RdTexCacheEntry *next = scan->next;

		if (scan->lastFrame != _frameCounter) {
			releaseTexture(scan);
			scan->inCache = 0;
			scan->lastFrame = 0;
			cacheListRemove(scan);
		}

		scan = next;
	}

	return 1;
}

} // End of namespace Grim

#endif
