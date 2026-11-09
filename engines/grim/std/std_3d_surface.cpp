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

static int resolveRect(const Std3DSurface *pSurface, const StdRect *pRect, StdRect *pResult) {
	if (pRect) {
		*pResult = *pRect;
	} else {
		pResult->x = 0;
		pResult->y = 0;
		pResult->width = pSurface->width;
		pResult->height = pSurface->height;
	}

	if (pResult->x < 0 || pResult->y < 0 || pResult->width < 0 || pResult->height < 0 ||
		pResult->x + pResult->width > pSurface->width || pResult->y + pResult->height > pSurface->height) {
		warning("Std3D: rectangle outside the surface");
		return 0;
	}

	return 1;
}

// A surface that outlived its backend has no backend data
static int hasBackendData(const Std3DSurface *pSurface, const char *pFunction) {
	if (!pSurface->pBackendData) {
		warning("Std3D::%s(): the surface has no backend data", pFunction);
		return 0;
	}

	return 1;
}

void Std3D::markGpuDirty(Std3DSurface *pSurface, int x, int y, int width, int height) {
	int left = MAX(x, 0);
	int top = MAX(y, 0);
	int right = MIN(x + width, pSurface->width);
	int bottom = MIN(y + height, pSurface->height);

	if (right <= left || bottom <= top)
		return;

	if (!pSurface->gpuDirty) {
		pSurface->gpuDirty = 1;
		pSurface->dirtyLeft = left;
		pSurface->dirtyTop = top;
		pSurface->dirtyRight = right;
		pSurface->dirtyBottom = bottom;
		return;
	}

	pSurface->dirtyLeft = MIN(pSurface->dirtyLeft, left);
	pSurface->dirtyTop = MIN(pSurface->dirtyTop, top);
	pSurface->dirtyRight = MAX(pSurface->dirtyRight, right);
	pSurface->dirtyBottom = MAX(pSurface->dirtyBottom, bottom);
}

Std3DSurface *Std3D::createSurface(int width, int height, int bHasDepth) {
	if (!ensureBackend())
		return nullptr;

	int maxSize = _pBackend->getMaxTextureSize();

	if (width <= 0 || height <= 0 || width > maxSize || height > maxSize) {
		warning("Std3D::createSurface(): invalid surface size");
		return nullptr;
	}

	Std3DSurface *pSurface = new Std3DSurface;

	memset(pSurface, 0, sizeof(Std3DSurface));
	pSurface->width = width;
	pSurface->height = height;

	if (!_pBackend->createSurface(pSurface, bHasDepth, _zBufferBitDepth)) {
		delete pSurface;
		return nullptr;
	}

	return pSurface;
}

void Std3D::destroySurface(Std3DSurface *&pSurface) {
	if (!pSurface)
		return;

	if (pSurface == _pRenderTarget)
		_pRenderTarget = nullptr;

	if (_pBackend)
		_pBackend->destroySurface(pSurface);

	delete[] pSurface->pReference;
	delete pSurface;
	pSurface = nullptr;
}

int Std3D::uploadPixels(Std3DSurface *pSurface, const StdRect *pRect, const byte *pPixels, int pitchInBytes) {
	if (!pSurface || !pPixels || !ensureBackend() || !hasBackendData(pSurface, "uploadPixels"))
		return 0;

	StdRect rect;

	if (!resolveRect(pSurface, pRect, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	return _pBackend->uploadPixels(pSurface, &rect, pPixels, pitchInBytes);
}

int Std3D::readPixels(Std3DSurface *pSurface, const StdRect *pRect, byte *pOut, int pitchInBytes) {
	if (!pSurface || !pOut || !ensureBackend() || !hasBackendData(pSurface, "readPixels"))
		return 0;

	StdRect rect;

	if (!resolveRect(pSurface, pRect, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	return _pBackend->readPixels(pSurface, &rect, pOut, pitchInBytes);
}

int Std3D::fillRect(Std3DSurface *pSurface, const StdRect *pRect, uint16 color) {
	if (!pSurface || !ensureBackend() || !hasBackendData(pSurface, "fillRect"))
		return 0;

	StdRect rect;

	if (!resolveRect(pSurface, pRect, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	markGpuDirty(pSurface, rect.x, rect.y, rect.width, rect.height);
	return _pBackend->fillRect(pSurface, &rect, color);
}

int Std3D::blit(Std3DSurface *pDst, Std3DSurface *pSrc, const StdRect *pSrcRect, int dstX, int dstY, int bColorKey, uint16 colorKey) {
	if (!pDst || !pSrc || !ensureBackend() || !hasBackendData(pDst, "blit") || !hasBackendData(pSrc, "blit"))
		return 0;

	int srcX = 0;
	int srcY = 0;
	int width = pSrc->width;
	int height = pSrc->height;

	if (pSrcRect) {
		srcX = pSrcRect->x;
		srcY = pSrcRect->y;
		width = pSrcRect->width;
		height = pSrcRect->height;
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

	if (srcX + width > pSrc->width)
		width = pSrc->width - srcX;

	if (srcY + height > pSrc->height)
		height = pSrc->height - srcY;

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

	if (dstX + width > pDst->width)
		width = pDst->width - dstX;

	if (dstY + height > pDst->height)
		height = pDst->height - dstY;

	if (width <= 0 || height <= 0)
		return 1;

	if (pDst == pSrc && !_pBackend->canBlitOntoSelf()) {
		Std3DSurface *pScratch = createSurface(width, height, 0);

		if (!pScratch)
			return 0;

		StdRect srcRect;

		srcRect.x = srcX;
		srcRect.y = srcY;
		srcRect.width = width;
		srcRect.height = height;

		int result = blit(pScratch, pSrc, &srcRect, 0, 0, 0, 0) && blit(pDst, pScratch, nullptr, dstX, dstY, bColorKey, colorKey);

		destroySurface(pScratch);
		return result;
	}

	markGpuDirty(pDst, dstX, dstY, width, height);
	return _pBackend->blit(pDst, pSrc, srcX, srcY, width, height, dstX, dstY, bColorKey, colorKey);
}

int Std3D::uploadDepth(Std3DSurface *pSurface, const StdRect *pRect, const byte *pZData, int pitchInBytes) {
	if (!pSurface || !pZData || !ensureBackend() || !hasBackendData(pSurface, "uploadDepth"))
		return 0;

	if (!_pBackend->supportsDepthUpload(pSurface)) {
		warning("Std3D::uploadDepth(): depth upload is not available");
		return 0;
	}

	StdRect rect;

	if (!resolveRect(pSurface, pRect, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	return _pBackend->uploadDepth(pSurface, &rect, pZData, pitchInBytes);
}

int Std3D::presentSurface(Std3DSurface *pSurface) {
	if (!pSurface || !ensureBackend() || !hasBackendData(pSurface, "presentSurface"))
		return 0;

	return _pBackend->presentSurface(pSurface);
}

} // End of namespace Grim

#endif
