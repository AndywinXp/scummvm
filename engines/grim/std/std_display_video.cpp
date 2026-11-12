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

#include "common/endian.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_3d_config.h"
#include "engines/grim/std/std_display.h"
#include "engines/grim/std/std_sort.h"

namespace Grim {

enum {
	kPixelFlagIndexed8 = 0x20,
	kPixelFlagRgb = 0x40,
	kVideoMemoryTotal = 0x4000000
};

enum {
	kClipReject = 0,
	kClipEmpty = 1,
	kClipCopy = 2
};

struct SystemMode {
	int width;
	int height;
	float aspect;
	int bpp;
	int colorMode;
	int redBits;
	int greenBits;
	int blueBits;
	int redShift;
	int greenShift;
	int blueShift;
	int redLoss;
	int greenLoss;
	int blueLoss;
};

struct CopyRegion {
	int srcX;
	int srcY;
	int dstX;
	int dstY;
	int width;
	int rows;
};

static const byte dibSectionGuid[16] = {
	0x00, 0x9F, 0xA6, 0x92, 0xFA, 0x13, 0xD1, 0x11, 0x97, 0xC0, 0x00, 0xA0, 0x24, 0x29, 0x30, 0x05
};

static const byte tinyGlDisplayGuid[16] = {
	0x53, 0x63, 0x75, 0x6D, 0x6D, 0x56, 0x4D, 0x20, 0x54, 0x69, 0x6E, 0x79, 0x47, 0x4C, 0x00, 0x02
};

static const StdDisplayModeDesc syntheticModes[1] = {
	{ 640, 480, 1280, kPixelFlagRgb, 16, 0xF800, 0x07E0, 0x001F }
};

static const SystemMode systemModes[10] = {
	{ 320, 200, 0.75f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 320, 240, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 400, 400, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 512, 384, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 640, 480, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1024, 768, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1280, 1024, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 320, 240, 1.0f, 16, 1, 5, 5, 5, 10, 5, 0, 3, 3, 3 },
	{ 640, 480, 1.0f, 16, 1, 5, 5, 5, 10, 5, 0, 3, 3, 3 },
	{ 640, 480, 1.0f, 16, 1, 5, 6, 5, 11, 5, 0, 3, 2, 3 }
};

static int log2Int(uint value) {
	int result = 0;

	while (value > 1) {
		value >>= 1;
		result++;
	}

	return result;
}

static int readMask(uint mask, int *shiftPointer, int *loss, int *bitsPointer) {
	if (!mask) {
		warning("StdDisplay::enumModesCallback(): empty color mask");
		return 0;
	}

	uint shifted = mask;
	int shift = 0;

	while ((shifted & 1) == 0) {
		shifted >>= 1;
		shift++;
	}

	*shiftPointer = shift;
	*loss = log2Int(0xFF / (mask >> shift));

	int bits = 0;

	while ((shifted & 1) != 0) {
		shifted >>= 1;
		bits++;
	}

	*bitsPointer = bits;
	return 1;
}

static int fillMode(StdDisplayMode *mode, const StdDisplayModeDesc *desc, int modeX) {
	memset(mode, 0, sizeof(StdDisplayMode));

	mode->modeX = modeX;
	mode->tex.width = desc->width;
	mode->tex.height = desc->height;
	mode->tex.pitchInBytes = desc->pitch;

	if (desc->width == 320 && desc->height == 200)
		mode->aspect = 0.75f;
	else
		mode->aspect = 1.0f;

	StdColorFormat *format = &mode->tex.format;

	if (desc->pixelFlags & kPixelFlagIndexed8) {
		format->colorMode = 0;
		format->bpp = 8;
		format->redBits = 0;
		format->greenBits = 0;
		format->blueBits = 0;
	} else if (desc->pixelFlags & kPixelFlagRgb) {
		format->colorMode = 1;
		format->bpp = desc->rgbBitCount;

		if (!readMask(desc->rBitMask, &format->redShift, &format->redLoss, &format->redBits))
			return 0;

		if (!readMask(desc->gBitMask, &format->greenShift, &format->greenLoss, &format->greenBits))
			return 0;

		if (!readMask(desc->bBitMask, &format->blueShift, &format->blueLoss, &format->blueBits))
			return 0;
	} else {
		warning("StdDisplay::enumModesCallback(): unsupported pixel format");
		return 0;
	}

	switch (format->bpp) {
	case 8:
		mode->tex.pitchInPixels = mode->tex.pitchInBytes;
		break;
	case 16:
		mode->tex.pitchInPixels = (uint)mode->tex.pitchInBytes >> 1;
		break;
	case 24:
		mode->tex.pitchInPixels = mode->tex.pitchInBytes / 3u;
		break;
	case 32:
		mode->tex.pitchInPixels = (uint)mode->tex.pitchInBytes >> 2;
		break;
	default:
		break;
	}

	mode->tex.sizeInBytes = mode->tex.width * mode->tex.height * ((uint)format->bpp >> 3);
	return 1;
}

static int compareDisplayModes(const void *a, const void *b) {
	const StdDisplayMode *modeA = (const StdDisplayMode *)a;
	const StdDisplayMode *modeB = (const StdDisplayMode *)b;

	if (modeA->modeX && !modeB->modeX)
		return modeB->tex.width != modeA->tex.width ? -1 : 1;

	if (!modeA->modeX && modeB->modeX)
		return modeA->tex.width != modeB->tex.width ? 1 : -1;

	if (modeA->tex.format.bpp != modeB->tex.format.bpp)
		return modeA->tex.format.bpp - modeB->tex.format.bpp;

	if (modeA->tex.width != modeB->tex.width)
		return modeA->tex.width - modeB->tex.width;

	return modeA->tex.height - modeB->tex.height;
}

#if defined(GRIM_STD3D)

static void setupVideoBuffer(StdVBuffer *buffer, const StdDisplayMode *mode, byte *palette) {
	uint bytesPerPixel = (uint)mode->tex.format.bpp >> 3;

	buffer->type = 1;
	buffer->lockCount = 0;
	buffer->videoMemory = 0;
	buffer->tex = mode->tex;
	buffer->tex.pitchInBytes = mode->tex.width * bytesPerPixel;
	buffer->tex.pitchInPixels = mode->tex.width;
	buffer->tex.sizeInBytes = mode->tex.width * mode->tex.height * bytesPerPixel;
	buffer->paletteRGB = palette;
	buffer->pixels = nullptr;
	buffer->colorKey = 0;
	buffer->allocSize = 0;
	buffer->surface = nullptr;
	buffer->surfaceColorKey = -1;
}

static int clipBlitRect(StdRect *srcRect, int srcWidth, int srcHeight, int dstWidth, int dstHeight, int *dstX, int *dstY) {
	int srcX = srcRect->x;
	int srcY = srcRect->y;
	int width = srcRect->width;
	int height = srcRect->height;

	if (srcX < 0) {
		*dstX -= srcX;
		width += srcX;
		srcX = 0;
	}

	if (srcY < 0) {
		*dstY -= srcY;
		height += srcY;
		srcY = 0;
	}

	if (srcX + width > srcWidth)
		width = srcWidth - srcX;

	if (srcY + height > srcHeight)
		height = srcHeight - srcY;

	if (*dstX < 0) {
		srcX -= *dstX;
		width += *dstX;
		*dstX = 0;
	}

	if (*dstY < 0) {
		srcY -= *dstY;
		height += *dstY;
		*dstY = 0;
	}

	if (*dstX + width > dstWidth)
		width = dstWidth - *dstX;

	if (*dstY + height > dstHeight)
		height = dstHeight - *dstY;

	if (width <= 0 || height <= 0)
		return 0;

	srcRect->x = srcX;
	srcRect->y = srcY;
	srcRect->width = width;
	srcRect->height = height;
	return 1;
}

static int clipCopyRegion(const StdVBuffer *dst, const StdVBuffer *src, const StdRect *srcRect, int x, int y, CopyRegion *region) {
	if (x >= dst->tex.width)
		return kClipReject;

	int endX = x + srcRect->width;

	if (!endX)
		return kClipReject;

	uint firstRow = y;

	if (y >= dst->tex.height || !(srcRect->height + y))
		return kClipReject;

	uint lastRow = srcRect->height + y - 1;
	uint lastColumn = endX - 1;
	int srcStartX = srcRect->x;
	int srcY = srcRect->y;

	if (y < 0) {
		srcY -= y;
		firstRow = 0;
	}

	if (lastRow >= (uint)dst->tex.height)
		lastRow = dst->tex.height - 1;

	if (x < 0) {
		srcStartX = srcRect->x - x;
		x = 0;
	}

	if (lastColumn >= (uint)dst->tex.width)
		lastColumn = dst->tex.width - 1;

	int copyWidth = lastColumn - x + 1;

	if (!copyWidth)
		return kClipEmpty;

	int rows = 0;

	if (firstRow <= lastRow)
		rows = lastRow - firstRow + 1;

	if (rows > 0 && (srcStartX < 0 || srcY < 0 || srcStartX + copyWidth > src->tex.width || srcY + rows > src->tex.height)) {
		warning("StdDisplay::copyBlt(): source rectangle outside the source buffer");
		return kClipReject;
	}

	region->srcX = srcStartX;
	region->srcY = srcY;
	region->dstX = x;
	region->dstY = firstRow;
	region->width = copyWidth;
	region->rows = rows;
	return kClipCopy;
}

static void copyRows(byte *dstRow, int dstStride, const byte *srcRow, int srcStride, int width, int rows, int bpp, int colorKeyFlag, int colorKey) {
	int bytesPerPixel = (uint)bpp >> 3;

	for (int row = 0; row < rows; row++) {
		if (!colorKeyFlag) {
			memcpy(dstRow, srcRow, width * bytesPerPixel);
		} else {
			for (int column = 0; column < width; column++) {
				if (bpp == 8) {
					if (srcRow[column] != (byte)colorKey)
						dstRow[column] = srcRow[column];
				} else if (bpp == 16) {
					uint16 value = READ_LE_UINT16(srcRow + 2 * column);

					if (value != (uint16)colorKey)
						WRITE_LE_UINT16(dstRow + 2 * column, value);
				} else {
					uint32 value = READ_LE_UINT32(srcRow + 4 * column);

					if (value != (uint32)colorKey)
						WRITE_LE_UINT32(dstRow + 4 * column, value);
				}
			}
		}

		srcRow += srcStride;
		dstRow += dstStride;
	}
}

static void copyPixelRows(StdVBuffer *dst, const StdVBuffer *src, const CopyRegion *region, int colorKey) {
	int bpp = src->tex.format.bpp;

	if (bpp != 8 && bpp != 16 && bpp != 32)
		return;

	int bytesPerPixel = (uint)bpp >> 3;
	int srcStride = bytesPerPixel * src->tex.pitchInPixels;
	int dstStride = bytesPerPixel * dst->tex.pitchInPixels;
	const byte *srcRow = src->pixels + bytesPerPixel * (src->tex.pitchInPixels * region->srcY + region->srcX);
	byte *dstRow = dst->pixels + bytesPerPixel * (dst->tex.pitchInPixels * region->dstY + region->dstX);

	copyRows(dstRow, dstStride, srcRow, srcStride, region->width, region->rows, bpp, colorKey, src->colorKey);
}

static int intersectsGpuDirty(const Std3DSurface *surface, const StdRect *rect) {
	if (!surface->gpuDirty)
		return 0;

	return rect->x < surface->dirtyRight && rect->x + rect->width > surface->dirtyLeft &&
		rect->y < surface->dirtyBottom && rect->y + rect->height > surface->dirtyTop;
}

static int shadowCoversRect(const Std3DSurface *surface, const StdRect *rect) {
	return surface->shadowValid && !intersectsGpuDirty(surface, rect);
}

static void copyRectToReference(Std3DSurface *surface, const byte *shadow, const StdRect *rect) {
	if (!surface->reference)
		return;

	int pitch = surface->width * 2;

	for (int y = rect->y; y < rect->y + rect->height; y++) {
		int offset = y * pitch + rect->x * 2;

		memcpy(surface->reference + offset, shadow + offset, rect->width * 2);
	}
}

static int findChangedRect(const Std3DSurface *surface, const byte *shadow, StdRect *rect) {
	int pitch = surface->width * 2;
	int top = -1;
	int bottom = -1;
	int left = surface->width;
	int right = -1;

	for (int y = 0; y < surface->height; y++) {
		const byte *row = shadow + y * pitch;
		const byte *ref = surface->reference + y * pitch;

		if (!memcmp(row, ref, pitch))
			continue;

		if (top < 0)
			top = y;

		bottom = y;

		int firstByte = 0;
		int lastByte = pitch - 1;

		while (row[firstByte] == ref[firstByte])
			firstByte++;

		while (row[lastByte] == ref[lastByte])
			lastByte--;

		left = MIN(left, firstByte >> 1);
		right = MAX(right, lastByte >> 1);
	}

	if (top < 0)
		return 0;

	rect->x = left;
	rect->y = top;
	rect->width = right - left + 1;
	rect->height = bottom - top + 1;
	return 1;
}

#endif

int StdDisplay::startup() {
	_startup = 1;
	return 1;
}

void StdDisplay::shutdown() {
	_startup = 0;
}

int StdDisplay::enumerateDevices(uint backendMask) {
	StdDisplayDevice *device = &_devices[0];

	memcpy(device->guid, dibSectionGuid, sizeof(device->guid));
	_enumerated = 1;
	_numDevices = 0;
	_backendMask = backendMask;

	Common::strlcpy(device->name, "Window Display Device", sizeof(device->name));
	Common::strlcpy(device->description, "DIBsection windowed display", sizeof(device->description));
	device->type = 0;
	device->hasGuid = 0;
	device->b3D = 0;
	device->primary = 1;
	device->hasHardware = 0;
	device->vidMemTotal = 2000000;
	device->vidMemFree = 2000000;
	device->backend = kStdDisplayBackendNone;
	device->usable = 1;
	_numDevices = 1;

	if (isBackendEnabled(kStdDisplayBackendGl))
		enumDevicesCallback(nullptr, "ScummVM OpenGL display", "ScummVM OpenGL", kStdDisplayBackendGl);

	if (isBackendEnabled(kStdDisplayBackendTinyGl))
		enumDevicesCallback(tinyGlDisplayGuid, "ScummVM TinyGL display", "ScummVM TinyGL", kStdDisplayBackendTinyGl);

	return 1;
}

int StdDisplay::isBackendEnabled(int backend) {
	if (backend <= kStdDisplayBackendNone || backend > kStdDisplayBackendTinyGl)
		return 0;

	return (_backendMask >> backend) & 1;
}

void StdDisplay::setDeviceUsable(uint deviceIndex, int usable) {
	if (deviceIndex < _numDevices)
		_devices[deviceIndex].usable = usable;
}

int StdDisplay::enumDevicesCallback(const byte *guid, const char *description, const char *name, int backend) {
	if (_numDevices >= kStdDisplayMaxDevices)
		return 0;

	StdDisplayDevice *device = &_devices[_numDevices];

	device->backend = backend;
	device->usable = 1;
	device->type = 1;
	device->hasHardware = 1;

	if (guid) {
		memcpy(device->guid, guid, sizeof(device->guid));
		device->primary = 0;
	} else {
		memset(device->guid, 0, sizeof(device->guid));
		device->primary = 1;
	}

	Common::strlcpy(device->description, description, sizeof(device->description));
	Common::strlcpy(device->name, name, sizeof(device->name));
	device->hasGuid = guid ? 1 : 0;
	device->b3D = 1;
	device->vidMemTotal = kVideoMemoryTotal;
	device->vidMemFree = kVideoMemoryTotal;
	_numDevices++;
	return 1;
}

void StdDisplay::restoreDisplayMode() {
	_enumerated = 0;
}

int StdDisplay::enumerateModes(uint deviceIndex) {
	if (_displayOpen)
		return 0;

	if (deviceIndex >= _numDevices)
		return 0;

	_curDeviceIndex = deviceIndex;
	_curDevice = &_devices[deviceIndex];

	if (_curDevice->type == 1) {
		if (!createDirectDraw())
			return 0;
	} else if (_curDevice->type == 0) {
		for (int i = 0; i < ARRAYSIZE(systemModes); i++) {
			const SystemMode *info = &systemModes[i];
			StdDisplayMode *mode = &_modes[i];
			int bytesPerPixel = info->bpp >> 3;

			memset(mode, 0, sizeof(StdDisplayMode));
			mode->modeX = 0;
			mode->aspect = info->aspect;
			mode->tex.width = info->width;
			mode->tex.height = info->height;
			mode->tex.pitchInBytes = info->width * bytesPerPixel;
			mode->tex.pitchInPixels = info->width;
			mode->tex.sizeInBytes = info->width * info->height * bytesPerPixel;
			mode->tex.format.colorMode = info->colorMode;
			mode->tex.format.bpp = info->bpp;
			mode->tex.format.redBits = info->redBits;
			mode->tex.format.greenBits = info->greenBits;
			mode->tex.format.blueBits = info->blueBits;
			mode->tex.format.redShift = info->redShift;
			mode->tex.format.greenShift = info->greenShift;
			mode->tex.format.blueShift = info->blueShift;
			mode->tex.format.redLoss = info->redLoss;
			mode->tex.format.greenLoss = info->greenLoss;
			mode->tex.format.blueLoss = info->blueLoss;
		}

		_numModes = ARRAYSIZE(systemModes);
	}

	Grim::sort(_modes, _numModes, sizeof(StdDisplayMode), compareDisplayModes);
	_displayOpen = 1;
	return 1;
}

int StdDisplay::createDirectDraw() {
	if (!isBackendEnabled(_curDevice->backend)) {
		warning("StdDisplay::createDirectDraw(): the display backend of the device is not available");
		return 0;
	}

	_numModes = 0;

	for (int i = 0; i < ARRAYSIZE(syntheticModes); i++) {
		if (!enumModesCallback(&syntheticModes[i]))
			break;
	}

	for (int i = 0; i < ARRAYSIZE(syntheticModes); i++) {
		if (!enumModesCallback2(&syntheticModes[i]))
			break;
	}

	return 1;
}

int StdDisplay::enumModesCallback(const StdDisplayModeDesc *desc) {
	if (_numModes >= kStdDisplayMaxModes)
		return 0;

	if (desc->width == 320) {
		if (fillMode(&_modes[_numModes], desc, 1))
			_numModes++;
	}

	return 1;
}

int StdDisplay::enumModesCallback2(const StdDisplayModeDesc *desc) {
	if (_numModes >= kStdDisplayMaxModes)
		return 0;

	if (fillMode(&_modes[_numModes], desc, 0))
		_numModes++;

	return 1;
}

uint StdDisplay::findClosestDevice(const StdDisplayDeviceFlags *wanted) {
	if (!_numDevices)
		return 0;

	int bestScore = 0;
	uint best = 0;

	for (uint i = 0; i < _numDevices; i++) {
		const StdDisplayDevice *device = &_devices[i];
		int score = 0;

		if (!device->usable)
			continue;

		if (wanted->backend != kStdDisplayBackendAny && device->backend != wanted->backend)
			continue;

		if (device->hasHardware == wanted->hasHardware) {
			score = 1;

			if (device->type == wanted->type) {
				score = 2;

				if (device->primary == wanted->primary) {
					score = 3;

					if (device->b3D == wanted->b3D) {
						score = 4;

						if (device->hasGuid == wanted->hasGuid)
							return i;
					}
				}
			}
		}

		if (score > bestScore) {
			best = i;
			bestScore = score;
		}
	}

	return best;
}

uint StdDisplay::findClosestMode(const StdDisplayMode *wanted, const StdDisplayMode *modes, uint numModes) {
	if (!_numModes)
		return 0;

	int bestScore = 0;
	uint best = 0;
	int colorMode = wanted->tex.format.colorMode;

	for (uint i = 0; i < numModes; i++) {
		const StdDisplayMode *mode = &modes[i];
		int score = 0;

		if (mode->tex.format.colorMode == colorMode) {
			score = 1;

			if (mode->tex.format.bpp == wanted->tex.format.bpp) {
				score = 2;

				if (mode->tex.width == wanted->tex.width) {
					score = 3;

					if (mode->tex.height == wanted->tex.height) {
						score = 4;

						if (colorMode != 1 || (mode->tex.format.redBits == wanted->tex.format.redBits &&
								mode->tex.format.greenBits == wanted->tex.format.greenBits &&
								mode->tex.format.blueBits == wanted->tex.format.blueBits))
							return i;
					}
				}
			}
		}

		if (score > bestScore) {
			best = i;
			bestScore = score;
		}
	}

	return best;
}

int StdDisplay::openDisplay(uint modeIndex, byte *palette, int fullscreen) {
	if (modeIndex >= _numModes)
		return 0;

	if (!_curDevice) {
		warning("StdDisplay::openDisplay(): no device selected");
		return 0;
	}

	if (_surfacesCreated)
		releaseSurfaces();

	_fullscreen = fullscreen;
	_curModeIndex = modeIndex;
	_numDrawBuffers = 1;
	_displayMode = &_modes[modeIndex];

	if (_curDevice->type == 1) {
		if (!initDisplay(&_modes[modeIndex], palette))
			return 0;
	} else {
		// TODO: DIB display device
		warning("StdDisplay::openDisplay(): only the video memory device is supported");
		return 0;
	}

	vBufferFill(_backBuffer, 0, nullptr);
	update();

	if (fullscreen)
		vBufferFill(_backBuffer, 0, nullptr);

	_surfacesCreated = 1;
	return 1;
}

void StdDisplay::closeDisplay() {
	if (!_displayOpen)
		return;

	if (_surfacesCreated)
		releaseSurfaces();

	_displayOpen = 0;
}

#if defined(GRIM_STD3D)

static int toBackendKind(int backend, Std3DBackendKind *kind) {
	switch (backend) {
	case kStdDisplayBackendGl:
		*kind = kStd3DBackendGl;
		break;
	case kStdDisplayBackendTinyGl:
		*kind = kStd3DBackendTinyGl;
		break;
	default:
		return 0;
	}

	return Std3D::isBackendCompiled(*kind);
}

int StdDisplay::createBackend(int backend) {
	Std3DBackendKind kind;

	if (!_std3D || !toBackendKind(backend, &kind))
		return 0;

	return _std3D->createBackend(kind);
}

int StdDisplay::openBackend(int backend, int width, int height) {
	Std3DBackendKind kind;

	if (!_std3D || !toBackendKind(backend, &kind))
		return 0;

	return _std3D->openBackend(kind, width, height);
}

int StdDisplay::initDisplay(StdDisplayMode *mode, byte *palette) {
	if (!_std3D || !_mainGfxBuffer || !_backBuffer) {
		warning("StdDisplay::initDisplay(): the Std3D module and the display buffers are not set");
		return 0;
	}

	if (mode->tex.format.bpp != 16 || mode->tex.format.colorMode == 0) {
		warning("StdDisplay::initDisplay(): only 16 bpp RGB display modes are supported");
		return 0;
	}

	if (!openBackend(_curDevice->backend, mode->tex.width, mode->tex.height)) {
		warning("StdDisplay::initDisplay(): the 3D backend of the device is not available");
		return 0;
	}

	setupVideoBuffer(_mainGfxBuffer, mode, palette);
	_mainGfxBuffer->surface = _std3D->createSurface(mode->tex.width, mode->tex.height, 0);

	if (!_mainGfxBuffer->surface) {
		_std3D->closeBackend();
		return 0;
	}

	_mainGfxBuffer->videoMemory = 1;

	if (_fullscreen) {
		_numDrawBuffers = 2;
		setupVideoBuffer(_backBuffer, mode, palette);
		_backBuffer->surface = _std3D->createSurface(mode->tex.width, mode->tex.height, 0);

		if (!_backBuffer->surface) {
			_std3D->destroySurface(_mainGfxBuffer->surface);
			_std3D->closeBackend();
			return 0;
		}

		_backBuffer->videoMemory = 1;
	} else {
		_numDrawBuffers = 1;
		*_backBuffer = *_mainGfxBuffer;
	}

	_std3D->setDisplayMode(&mode->tex);
	_std3D->setRenderTarget(_backBuffer->surface);
	return 1;
}

#endif

void StdDisplay::releaseSurfaces() {
	if (_surfacesCreated) {
		if (_curDevice && _curDevice->type == 1)
			freeBuffers();
		// TODO: DIB display device
	}

	_surfacesCreated = 0;
}

#if defined(GRIM_STD3D)

void StdDisplay::freeBuffers() {
	if (!_mainGfxBuffer || !_backBuffer)
		return;

	if (_fullscreen) {
		vBufferFreeEntry(_backBuffer);
	} else {
		_backBuffer->surface = nullptr;
		_backBuffer->pixels = nullptr;
		_backBuffer->allocSize = 0;
	}

	vBufferFreeEntry(_mainGfxBuffer);

	if (_std3D)
		vBufferFreeEntry(_std3D->getZBuffer());
}

#endif

int StdDisplay::prepareWindowForDraw() {
	if (!_displayOpen || !_surfacesCreated || !_curDevice)
		return 0;

	return _curDevice->type - 1;
}

#if defined(GRIM_STD3D)

void StdDisplay::prepareOutputForDraw() {
	if (_displayOpen && _surfacesCreated && _curDevice && _curDevice->type == 1 && _std3D)
		_std3D->purgeTextureCache();
}

int StdDisplay::update() {
	if (!_curDevice || _curDevice->type != 1) {
		// TODO: DIB display device
		return 0;
	}

	if (!_std3D || !_mainGfxBuffer || !_backBuffer || !_mainGfxBuffer->surface) {
		warning("StdDisplay::update(): the display is not open");
		return 0;
	}

	if (_fullscreen) {
		if (_backBuffer->lockCount || !_backBuffer->surface)
			return 0;

		if (!_std3D->blit(_mainGfxBuffer->surface, _backBuffer->surface, nullptr, 0, 0, 0, 0))
			return 0;
	}

	if (!_std3D->presentSurface(_mainGfxBuffer->surface))
		return 0;

	g_system->updateScreen();
	return 1;
}

int StdDisplay::isZBuffer(StdVBuffer *vBuffer) {
	return _std3D && vBuffer == _std3D->getZBuffer();
}

void StdDisplay::presentIfPrimary(StdVBuffer *vBuffer) {
	if (!_fullscreen || !_std3D || !_mainGfxBuffer || !vBuffer->surface)
		return;

	if (vBuffer->surface != _mainGfxBuffer->surface)
		return;

	_std3D->presentSurface(vBuffer->surface);
	g_system->updateScreen();
}

byte *StdDisplay::surfaceLock(StdVBuffer *vBuffer) {
	if (!_std3D) {
		warning("StdDisplay::surfaceLock(): the Std3D module is not set");
		return nullptr;
	}

	if (vBuffer->lockCount)
		return vBuffer->pixels;

	if (isZBuffer(vBuffer)) {
		int zPitch = vBuffer->tex.width * ((uint)vBuffer->tex.format.bpp >> 3);
		int zSize = zPitch * vBuffer->tex.height;

		if (!vBuffer->pixels) {
			vBuffer->pixels = new byte[zSize];
			vBuffer->allocSize = zSize;
		}

		vBuffer->tex.pitchInBytes = zPitch;
		vBuffer->tex.pitchInPixels = vBuffer->tex.width;

		warning("StdDisplay::surfaceLock(): the Z buffer can not be read back, it is locked as empty");
		memset(vBuffer->pixels, 0, zSize);
		return vBuffer->pixels;
	}

	if (!vBuffer->surface) {
		warning("StdDisplay::surfaceLock(): the buffer has no surface");
		return nullptr;
	}

	if (!syncShadow(vBuffer))
		return nullptr;

	return vBuffer->pixels;
}

int StdDisplay::syncShadow(StdVBuffer *vBuffer) {
	Std3DSurface *surface = vBuffer->surface;

	if (!_std3D || !surface)
		return 0;

	if (vBuffer->tex.format.bpp != 16) {
		warning("StdDisplay::syncShadow(): only 16 bpp video memory buffers are supported");
		return 0;
	}

	int pitch = vBuffer->tex.width * 2;
	int size = pitch * vBuffer->tex.height;

	if (!vBuffer->pixels) {
		vBuffer->pixels = new byte[size];
		vBuffer->allocSize = size;
		surface->shadowValid = 0;
	}

	if (!surface->reference)
		surface->reference = new byte[size];

	vBuffer->tex.pitchInBytes = pitch;
	vBuffer->tex.pitchInPixels = vBuffer->tex.width;

	if (!surface->shadowValid) {
		if (!_std3D->readPixels(surface, nullptr, vBuffer->pixels, pitch))
			return 0;

		memcpy(surface->reference, vBuffer->pixels, size);
		surface->shadowValid = 1;
		surface->gpuDirty = 0;
		return 1;
	}

	if (surface->gpuDirty) {
		StdRect rect;

		rect.x = surface->dirtyLeft;
		rect.y = surface->dirtyTop;
		rect.width = surface->dirtyRight - surface->dirtyLeft;
		rect.height = surface->dirtyBottom - surface->dirtyTop;

		if (!_std3D->readPixels(surface, &rect, vBuffer->pixels + rect.y * pitch + rect.x * 2, pitch))
			return 0;

		copyRectToReference(surface, vBuffer->pixels, &rect);
		surface->gpuDirty = 0;
	}

	return 1;
}

int StdDisplay::surfaceUnlock(StdVBuffer *vBuffer, int upload) {
	if (vBuffer->lockCount > 1 || !upload || isZBuffer(vBuffer))
		return 1;

	Std3DSurface *surface = vBuffer->surface;

	if (!_std3D || !surface || !vBuffer->pixels || !surface->reference || !surface->shadowValid) {
		warning("StdDisplay::surfaceUnlock(): the buffer has no surface");
		return 0;
	}

	StdRect changed;

	if (!findChangedRect(surface, vBuffer->pixels, &changed))
		return 1;

	int pitch = surface->width * 2;

	if (!_std3D->uploadPixels(surface, &changed, vBuffer->pixels + changed.y * pitch + changed.x * 2, pitch))
		return 0;

	copyRectToReference(surface, vBuffer->pixels, &changed);
	presentIfPrimary(vBuffer);
	return 1;
}

void StdDisplay::unlockForRead(StdVBuffer *vBuffer) {
	if (vBuffer->type != 1) {
		vBufferUnlock(vBuffer);
		return;
	}

	if (vBuffer->lockCount) {
		surfaceUnlock(vBuffer, 0);
		vBuffer->lockCount--;
	}
}

int StdDisplay::surfaceBlt(StdVBuffer *vBuffer, int color, const StdRect *rect) {
	if (rect && (rect->width == 0 || rect->height == 0))
		return 0;

	if (vBuffer->lockCount)
		return 0;

	if (!_std3D || !vBuffer->surface) {
		warning("StdDisplay::surfaceBlt(): the buffer has no surface, the Z buffer can not be filled");
		return 0;
	}

	if (!_std3D->fillRect(vBuffer->surface, rect, (uint16)color))
		return 0;

	presentIfPrimary(vBuffer);
	return 1;
}

int StdDisplay::copyToZBuffer(StdVBuffer *src, int x, int y, const StdRect *srcRectPointer) {
	Std3DSurface *target = _std3D->getRenderTarget();

	if (!target || !src->surface) {
		warning("StdDisplay::copyToZBuffer(): no render target or no source surface");
		return 0;
	}

	StdRect srcRect = *srcRectPointer;
	int dstX = x;
	int dstY = y;

	if (!clipBlitRect(&srcRect, src->tex.width, src->tex.height, target->width, target->height, &dstX, &dstY))
		return 1;

	byte *data = new byte[srcRect.width * srcRect.height * 2];
	int result = 1;

	if (shadowCoversRect(src->surface, &srcRect) && src->pixels) {
		int srcPitch = src->tex.width * 2;

		for (int row = 0; row < srcRect.height; row++)
			memcpy(data + row * srcRect.width * 2, src->pixels + (srcRect.y + row) * srcPitch + srcRect.x * 2, srcRect.width * 2);
	} else {
		result = _std3D->readPixels(src->surface, &srcRect, data, srcRect.width * 2);
	}

	if (result) {
		StdRect dstRect;

		dstRect.x = dstX;
		dstRect.y = dstY;
		dstRect.width = srcRect.width;
		dstRect.height = srcRect.height;
		result = _std3D->uploadDepth(target, &dstRect, data, srcRect.width * 2);
	}

	delete[] data;
	return result;
}

int StdDisplay::copyBltB(StdVBuffer *dst, StdVBuffer *src, int x, int y, StdRect *rect, char colorKey) {
	if (dst->lockCount || src->lockCount)
		return 0;

	if (dst->type != 1 || src->type != 1 || !_std3D || !src->surface) {
		warning("StdDisplay::copyBltB(): both buffers must be video memory buffers");
		return 0;
	}

	StdRect srcRect;

	if (rect) {
		if (!rect->width || !rect->height)
			return 0;

		srcRect = *rect;
	} else {
		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.width = src->tex.width;
		srcRect.height = src->tex.height;
	}

	if (isZBuffer(dst))
		return copyToZBuffer(src, x, y, &srcRect);

	if (!dst->surface) {
		warning("StdDisplay::copyBltB(): the destination has no surface");
		return 0;
	}

	int key = (colorKey & 1) != 0;

	if (key && src->surfaceColorKey < 0) {
		warning("StdDisplay::copyBltB(): the source has no color key");
		return 0;
	}

	int result = _std3D->blit(dst->surface, src->surface, &srcRect, x, y, key, (uint16)src->surfaceColorKey);

	if (result)
		presentIfPrimary(dst);

	return result;
}

int StdDisplay::copyBltC(StdVBuffer *dst, StdVBuffer *src, int x, int y, StdRect *rect, char colorKey) {
	if (dst->lockCount)
		return 0;

	if (dst->type != 1 || src->type != 0 || !src->pixels || !_std3D) {
		warning("StdDisplay::copyBltC(): the destination must be a video memory buffer and the source a system memory buffer");
		return 0;
	}

	StdRect srcRect;

	if (rect) {
		if (!rect->width || !rect->height)
			return 0;

		srcRect = *rect;
	} else {
		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.width = src->tex.width;
		srcRect.height = src->tex.height;
	}

	CopyRegion region;
	int clipResult = clipCopyRegion(dst, src, &srcRect, x, y, &region);

	if (clipResult == kClipReject)
		return 0;

	if (clipResult == kClipEmpty)
		return 1;

	if (isZBuffer(dst)) {
		if (src->tex.format.bpp != 16) {
			warning("StdDisplay::copyBltC(): the Z buffer needs a 16 bpp source");
			return 0;
		}

		if (region.rows > 0) {
			StdRect dstRect;

			dstRect.x = region.dstX;
			dstRect.y = region.dstY;
			dstRect.width = region.width;
			dstRect.height = region.rows;

			const byte *srcRows = src->pixels + 2 * (src->tex.pitchInPixels * region.srcY + region.srcX);

			_std3D->uploadDepth(_std3D->getRenderTarget(), &dstRect, srcRows, 2 * src->tex.pitchInPixels);
		}

		return 1;
	}

	Std3DSurface *surface = dst->surface;

	if (!surface || src->tex.format.bpp != 16 || dst->tex.format.bpp != 16) {
		warning("StdDisplay::copyBltC(): only 16 bpp video memory buffers with a surface are supported");
		return 0;
	}

	if (region.rows <= 0)
		return 1;

	StdRect dstRect;

	dstRect.x = region.dstX;
	dstRect.y = region.dstY;
	dstRect.width = region.width;
	dstRect.height = region.rows;

	int key = (colorKey & 1) != 0;
	int srcPitch = 2 * src->tex.pitchInPixels;
	const byte *srcRows = src->pixels + 2 * (src->tex.pitchInPixels * region.srcY + region.srcX);
	int result = 0;

	if (shadowCoversRect(surface, &dstRect) && dst->pixels) {
		int dstPitch = surface->width * 2;
		byte *dstRows = dst->pixels + region.dstY * dstPitch + region.dstX * 2;

		copyRows(dstRows, dstPitch, srcRows, srcPitch, region.width, region.rows, 16, key, src->colorKey);
		result = _std3D->uploadPixels(surface, &dstRect, dstRows, dstPitch);
		copyRectToReference(surface, dst->pixels, &dstRect);
	} else if (!key) {
		result = _std3D->uploadPixels(surface, &dstRect, srcRows, srcPitch);
	} else {
		int tempPitch = region.width * 2;
		byte *temp = new byte[tempPitch * region.rows];

		result = _std3D->readPixels(surface, &dstRect, temp, tempPitch);

		if (result) {
			copyRows(temp, tempPitch, srcRows, srcPitch, region.width, region.rows, 16, 1, src->colorKey);
			result = _std3D->uploadPixels(surface, &dstRect, temp, tempPitch);
		}

		delete[] temp;
	}

	if (!result)
		return 0;

	presentIfPrimary(dst);
	return 1;
}

int StdDisplay::copyBltD(StdVBuffer *dst, StdVBuffer *src, int x, int y, StdRect *rect, char colorKey) {
	if (src->lockCount)
		return 0;

	if (dst->type != 0 || src->type != 1 || !dst->pixels || !_std3D) {
		warning("StdDisplay::copyBltD(): the source must be a video memory buffer and the destination a system memory buffer");
		return 0;
	}

	StdRect srcRect;

	if (rect) {
		if (!rect->width || !rect->height)
			return 0;

		srcRect = *rect;
	} else {
		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.width = src->tex.width;
		srcRect.height = src->tex.height;
	}

	CopyRegion region;
	int clipResult = clipCopyRegion(dst, src, &srcRect, x, y, &region);

	if (clipResult == kClipReject)
		return 0;

	if (clipResult == kClipEmpty)
		return 1;

	if (isZBuffer(src)) {
		if (!vBufferLock(src)) {
			warning("StdDisplay::copyBltD(): could not lock the source");
			return 0;
		}

		copyPixelRows(dst, src, &region, (colorKey & 1) != 0);
		unlockForRead(src);
		return 1;
	}

	if (!src->surface) {
		warning("StdDisplay::copyBltD(): the source has no surface");
		return 0;
	}

	if (region.rows <= 0)
		return 1;

	StdRect srcArea;

	srcArea.x = region.srcX;
	srcArea.y = region.srcY;
	srcArea.width = region.width;
	srcArea.height = region.rows;

	if (!shadowCoversRect(src->surface, &srcArea) && !syncShadow(src)) {
		warning("StdDisplay::copyBltD(): could not read the source");
		return 0;
	}

	copyPixelRows(dst, src, &region, (colorKey & 1) != 0);
	return 1;
}

#else

int StdDisplay::createBackend(int backend) {
	return 0;
}

int StdDisplay::openBackend(int backend, int width, int height) {
	return 0;
}

int StdDisplay::initDisplay(StdDisplayMode *mode, byte *palette) {
	return 0;
}

void StdDisplay::freeBuffers() {
}

int StdDisplay::update() {
	return 0;
}

#endif

} // End of namespace Grim
