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
#include "common/str.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_3d_config.h"

namespace Grim {

RdMaterialLoaderFunc RenderDroid::MaterialModule::registerLoader(RdMaterialLoaderFunc pfLoader) {
	RdMaterialLoaderFunc pfPrevious = _pfLoader;
	_pfLoader = pfLoader;
	return pfPrevious;
}

RdMaterialUnloaderFunc RenderDroid::MaterialModule::registerUnloader(RdMaterialUnloaderFunc pfUnloader) {
	RdMaterialUnloaderFunc pfPrevious = _pfUnloader;
	_pfUnloader = pfUnloader;
	return pfPrevious;
}

RdMaterial *RenderDroid::MaterialModule::load(char *path, int bVideoMemory, int bVideoMemoryOnly) {
	if (_pfLoader)
		return _pfLoader(path, bVideoMemory, bVideoMemoryOnly);

	RdMaterial *pMaterial = new RdMaterial;

	if (loadEntry(path, pMaterial, bVideoMemory, bVideoMemoryOnly))
		return pMaterial;

	destroy(pMaterial);
	return nullptr;
}

int RenderDroid::MaterialModule::loadEntry(char *path, RdMaterial *pMaterial, int bVideoMemory, int bVideoMemoryOnly) {
	memset(pMaterial, 0, sizeof(RdMaterial));

	Common::SeekableReadStream *pFile = _rd->_pHS->openFile(path);

	if (!pFile)
		return 0;

	uint32 magic = pFile->readUint32BE();
	int version = pFile->readSint32LE();

	if (magic != MKTAG('M', 'A', 'T', ' ') || version != 50) {
		delete pFile;
		return 0;
	}

	uint flags = pFile->readUint32LE();
	uint numCels = pFile->readUint32LE();
	uint numTextures = pFile->readUint32LE();

	if (numCels > 16) {
		warning("RenderDroid::MaterialModule::loadEntry(): too many cels in %s", path);
		delete pFile;
		return 0;
	}

	pMaterial->numTextures = numTextures;
	pMaterial->flags = flags;
	pMaterial->numCels = numCels;
	pMaterial->curCel = 0;

	StdColorFormat *pFormat = &pMaterial->format;
	pFormat->colorMode = pFile->readSint32LE();
	pFormat->bpp = pFile->readSint32LE();
	pFormat->redBits = pFile->readSint32LE();
	pFormat->greenBits = pFile->readSint32LE();
	pFormat->blueBits = pFile->readSint32LE();
	pFormat->redShift = pFile->readSint32LE();
	pFormat->greenShift = pFile->readSint32LE();
	pFormat->blueShift = pFile->readSint32LE();
	pFormat->redLoss = pFile->readSint32LE();
	pFormat->greenLoss = pFile->readSint32LE();
	pFormat->blueLoss = pFile->readSint32LE();
	pFormat->alphaBits = pFile->readSint32LE();
	pFormat->alphaShift = pFile->readSint32LE();
	pFormat->alphaLoss = pFile->readSint32LE();

	uint aTextureIndices[16];
	memset(aTextureIndices, 0, sizeof(aTextureIndices));

	for (uint i = 0; i < numCels; i++) {
		RdMatCel *pCel = new RdMatCel;
		memset(pCel, 0, sizeof(RdMatCel));
		pMaterial->apCels[i] = pCel;

		pCel->flags = pFile->readUint32LE();
		pCel->colorIndex = pFile->readSint32LE();
		pCel->reserved8 = pFile->readSint32LE();
		pCel->reserved12 = pFile->readSint32LE();
		pCel->reserved16 = pFile->readSint32LE();
		pCel->reserved20 = pFile->readSint32LE();

		if (pCel->flags & 8) {
			pCel->reserved24 = pFile->readSint32LE();
			pFile->skip(8);
			aTextureIndices[i] = pFile->readUint32LE();
		}
	}

	pMaterial->aTextures = nullptr;

	if (numTextures) {
		pMaterial->aTextures = new RdTexture[numTextures];
		memset(pMaterial->aTextures, 0, sizeof(RdTexture) * numTextures);
	}

	for (uint i = 0; i < numTextures; i++) {
		RdTexture *pTexture = &pMaterial->aTextures[i];

		int width = pFile->readSint32LE();
		int height = pFile->readSint32LE();
		pTexture->flags = pFile->readUint32LE();
		pTexture->reserved4 = pFile->readSint32LE();
		pTexture->colorKey = pFile->readSint32LE();
		uint numMips = pFile->readUint32LE();

		if (numMips > 4) {
			warning("RenderDroid::MaterialModule::loadEntry(): too many mip levels in %s", path);
			delete pFile;
			return 0;
		}

		int log2Width = 0;
		for (int w = width; w > 1; w >>= 1)
			log2Width++;

		pTexture->widthLog2 = log2Width;
		pTexture->widthMask = width - 1;
		pTexture->heightMask = height - 1;
		pTexture->numMips = numMips;

		StdVBufferTexFmt texFormat;
		memset(&texFormat, 0, sizeof(StdVBufferTexFmt));
		texFormat.width = width;
		texFormat.height = height;
		texFormat.format.colorMode = pMaterial->format.colorMode;
		texFormat.format.bpp = pMaterial->format.bpp;

		for (uint mip = 0; mip < numMips; mip++) {
			pTexture->aCache[0][mip].bInCache = 0;
			pTexture->aCache[0][mip].lastFrame = 0;
			pTexture->aCache[1][mip].bInCache = 0;
			pTexture->aCache[1][mip].lastFrame = 0;

			pTexture->apMips[mip] = _rd->_stdDisplay->vBufferNew(&texFormat, bVideoMemory, bVideoMemoryOnly, nullptr);

			if (pTexture->flags & 1)
				_rd->_stdDisplay->vBufferSetColorKey(pTexture->apMips[mip], pTexture->colorKey);

			_rd->_stdDisplay->vBufferLock(pTexture->apMips[mip]);
			pFile->read(pTexture->apMips[mip]->pPixels, pTexture->apMips[mip]->tex.sizeInBytes);
			_rd->_stdDisplay->vBufferUnlock(pTexture->apMips[mip]);

			texFormat.width = (uint)texFormat.width >> 1;
			texFormat.height = (uint)texFormat.height >> 1;
		}
	}

	for (uint i = 0; i < numCels; i++) {
		if (pMaterial->apCels[i]->flags & 8) {
			if (aTextureIndices[i] >= numTextures) {
				warning("RenderDroid::MaterialModule::loadEntry(): texture index out of range in %s", path);
				delete pFile;
				return 0;
			}

			pMaterial->apCels[i]->pTexture = &pMaterial->aTextures[aTextureIndices[i]];
		}
	}

	if (pMaterial->flags & 1)
		pFile->skip(768);

	Common::strlcpy(pMaterial->aName, _rd->_stdFile->fromPath(path), 32);

	delete pFile;
	return 1;
}

int RenderDroid::MaterialModule::destroy(RdMaterial *&pMaterial) {
	if (_pfUnloader) {
		int result = _pfUnloader(pMaterial);
		pMaterial = nullptr;
		return result;
	}

	freeEntry(pMaterial);
	delete pMaterial;
	pMaterial = nullptr;

	return 1;
}

void RenderDroid::MaterialModule::freeEntry(RdMaterial *pMaterial) {
	for (uint i = 0; i < pMaterial->numCels; i++) {
		delete pMaterial->apCels[i];
		pMaterial->apCels[i] = nullptr;
	}

	if (!pMaterial->aTextures)
		return;

	for (uint i = 0; i < pMaterial->numTextures; i++) {
		RdTexture *pTexture = &pMaterial->aTextures[i];

		for (uint mip = 0; mip < pTexture->numMips; mip++) {
			_rd->_stdDisplay->vBufferFree(pTexture->apMips[mip]);

#if defined(GRIM_STD3D)
			RdTexCacheEntry *pEntry = &pTexture->aCache[0][mip];

			if (pEntry->bInCache && _rd->_pStd3D) {
				_rd->_pStd3D->releaseTexture(pEntry);
				pEntry->bInCache = 0;
				pEntry->lastFrame = 0;
				_rd->_pStd3D->cacheListRemove(pEntry);
			}
#endif
		}
	}

	delete[] pMaterial->aTextures;
	pMaterial->aTextures = nullptr;
}

int RenderDroid::MaterialModule::addToTextureCache(RdMaterial *, RdTexture *pTexture, int mipLevel, int alt) {
#if defined(GRIM_STD3D)
	StdVBuffer *pMip = pTexture->apMips[mipLevel];
	RdTexCacheEntry *pEntry = &pTexture->aCache[alt ? 1 : 0][mipLevel];

	if (pEntry->bInCache) {
		_rd->_pStd3D->cacheListTouch(pEntry);
		return 1;
	}

	if (_rd->_pStd3D->addToTextureCache(pMip, pEntry, pTexture->flags & 1, alt))
		return 1;
#endif

	return 0;
}

} // End of namespace Grim
