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

#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_3d_backend.h"

namespace Grim {

static int resolveRect(const Std3DSurface *surface, const StdRect *rect, StdRect *result) {
	if (rect) {
		*result = *rect;
	} else {
		result->x = 0;
		result->y = 0;
		result->width = surface->width;
		result->height = surface->height;
	}

	if (result->x < 0 || result->y < 0 || result->width < 0 || result->height < 0 ||
		result->x + result->width > surface->width || result->y + result->height > surface->height) {
		warning("Std3D: rectangle outside the surface");
		return 0;
	}

	return 1;
}

// A surface that outlived its backend has no backend data
static int hasBackendData(const Std3DSurface *surface, const char *function) {
	if (!surface->backendData) {
		warning("Std3D::%s(): the surface has no backend data", function);
		return 0;
	}

	return 1;
}

void Std3D::markGpuDirty(Std3DSurface *surface, int x, int y, int width, int height) {
	int left = MAX(x, 0);
	int top = MAX(y, 0);
	int right = MIN(x + width, surface->width);
	int bottom = MIN(y + height, surface->height);

	if (right <= left || bottom <= top)
		return;

	if (!surface->gpuDirty) {
		surface->gpuDirty = 1;
		surface->dirtyLeft = left;
		surface->dirtyTop = top;
		surface->dirtyRight = right;
		surface->dirtyBottom = bottom;
		return;
	}

	surface->dirtyLeft = MIN(surface->dirtyLeft, left);
	surface->dirtyTop = MIN(surface->dirtyTop, top);
	surface->dirtyRight = MAX(surface->dirtyRight, right);
	surface->dirtyBottom = MAX(surface->dirtyBottom, bottom);
}

Std3DSurface *Std3D::createSurface(int width, int height, int hasDepth) {
	if (!ensureBackend())
		return nullptr;

	int maxSize = _backend->getMaxTextureSize();

	if (width <= 0 || height <= 0 || width > maxSize || height > maxSize) {
		warning("Std3D::createSurface(): invalid surface size");
		return nullptr;
	}

	Std3DSurface *surface = new Std3DSurface;

	memset(surface, 0, sizeof(Std3DSurface));
	surface->width = width;
	surface->height = height;

	if (!_backend->createSurface(surface, hasDepth, _zBufferBitDepth)) {
		delete surface;
		return nullptr;
	}

	return surface;
}

void Std3D::destroySurface(Std3DSurface *&surface) {
	if (!surface)
		return;

	if (surface == _renderTarget)
		_renderTarget = nullptr;

	if (_backend)
		_backend->destroySurface(surface);

	delete[] surface->reference;
	delete surface;
	surface = nullptr;
}

int Std3D::uploadPixels(Std3DSurface *surface, const StdRect *rectPointer, const byte *pixels, int pitchInBytes) {
	if (!surface || !pixels || !ensureBackend() || !hasBackendData(surface, "uploadPixels"))
		return 0;

	StdRect rect;

	if (!resolveRect(surface, rectPointer, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	return _backend->uploadPixels(surface, &rect, pixels, pitchInBytes);
}

int Std3D::readPixels(Std3DSurface *surface, const StdRect *rectPointer, byte *out, int pitchInBytes) {
	if (!surface || !out || !ensureBackend() || !hasBackendData(surface, "readPixels"))
		return 0;

	StdRect rect;

	if (!resolveRect(surface, rectPointer, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	return _backend->readPixels(surface, &rect, out, pitchInBytes);
}

int Std3D::fillRect(Std3DSurface *surface, const StdRect *rectPointer, uint16 color) {
	if (!surface || !ensureBackend() || !hasBackendData(surface, "fillRect"))
		return 0;

	StdRect rect;

	if (!resolveRect(surface, rectPointer, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	markGpuDirty(surface, rect.x, rect.y, rect.width, rect.height);
	return _backend->fillRect(surface, &rect, color);
}

int Std3D::blit(Std3DSurface *dst, Std3DSurface *src, const StdRect *srcRectPointer, int dstX, int dstY, int colorKeyFlag, uint16 colorKey) {
	if (!dst || !src || !ensureBackend() || !hasBackendData(dst, "blit") || !hasBackendData(src, "blit"))
		return 0;

	int srcX = 0;
	int srcY = 0;
	int width = src->width;
	int height = src->height;

	if (srcRectPointer) {
		srcX = srcRectPointer->x;
		srcY = srcRectPointer->y;
		width = srcRectPointer->width;
		height = srcRectPointer->height;
	}

	if (srcX < 0) {
		dstX -= srcX;
		width += srcX;
		srcX = 0;
	}

	if (srcY < 0) {
		dstY -= srcY;
		height += srcY;
		srcY = 0;
	}

	if (srcX + width > src->width)
		width = src->width - srcX;

	if (srcY + height > src->height)
		height = src->height - srcY;

	if (dstX < 0) {
		srcX -= dstX;
		width += dstX;
		dstX = 0;
	}

	if (dstY < 0) {
		srcY -= dstY;
		height += dstY;
		dstY = 0;
	}

	if (dstX + width > dst->width)
		width = dst->width - dstX;

	if (dstY + height > dst->height)
		height = dst->height - dstY;

	if (width <= 0 || height <= 0)
		return 1;

	if (dst == src && !_backend->canBlitOntoSelf()) {
		Std3DSurface *scratch = createSurface(width, height, 0);

		if (!scratch)
			return 0;

		StdRect srcRect;

		srcRect.x = srcX;
		srcRect.y = srcY;
		srcRect.width = width;
		srcRect.height = height;

		int result = blit(scratch, src, &srcRect, 0, 0, 0, 0) && blit(dst, scratch, nullptr, dstX, dstY, colorKeyFlag, colorKey);

		destroySurface(scratch);
		return result;
	}

	markGpuDirty(dst, dstX, dstY, width, height);
	return _backend->blit(dst, src, srcX, srcY, width, height, dstX, dstY, colorKeyFlag, colorKey);
}

int Std3D::uploadDepth(Std3DSurface *surface, const StdRect *rectPointer, const byte *zData, int pitchInBytes) {
	if (!surface || !zData || !ensureBackend() || !hasBackendData(surface, "uploadDepth"))
		return 0;

	if (!_backend->supportsDepthUpload(surface)) {
		warning("Std3D::uploadDepth(): depth upload is not available");
		return 0;
	}

	StdRect rect;

	if (!resolveRect(surface, rectPointer, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	return _backend->uploadDepth(surface, &rect, zData, pitchInBytes);
}

int Std3D::presentSurface(Std3DSurface *surface) {
	if (!surface || !ensureBackend() || !hasBackendData(surface, "presentSurface"))
		return 0;

	return _backend->presentSurface(surface);
}

} // End of namespace Grim

#endif
