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

#include "common/textconsole.h"

#include "engines/grim/registry.h"
#include "engines/grim/sputresource.h"

namespace Grim {

static SputResource *g_pSputResource = nullptr;

static RdMaterial *materialLoader(char *path, int bVideoMemory, int bVideoMemoryOnly) {
	return g_pSputResource->mat(path, bVideoMemory, bVideoMemoryOnly);
}

static int materialUnloader(RdMaterial *pMaterial) {
	return g_pSputResource->unlock(pMaterial);
}

SputResource::SputResource(Res *pRes, RenderDroid *pRd) : _pRes(pRes), _pRd(pRd),
		_pHS(nullptr), _bInitialized(0), _bStarted(0), _bLoaderLocked(0), _totalAllowedLimit(0xBB8000), _purgeAmount(0x271000),
		_oldMatLoader(nullptr), _oldMatUnloader(nullptr) {
}

int SputResource::handler(ResHandlerMsg *pMsg) {
	switch (pMsg->tag) {
	case kSputTagCmap:
		return loadResCmap(pMsg);
	case kSputTagKeyf:
		return loadResKeyf(pMsg);
	case kSputTagMat:
		return loadResMat(pMsg);
	case kSputTagModl:
		return loadResModl(pMsg);
	default:
		break;
	}

	return 0;
}

int SputResource::stopPurge(int state) {
	_bLoaderLocked = state;

	return state;
}

void *SputResource::alloc(int resType, const char *name) {
	void *handle = _pRes->getResource(resType, name);

	if (handle && _pRes->totalAllocated() > _totalAllowedLimit && !_bLoaderLocked) {
		ResRef ref;
		uint bytesFreed;

		ref.valid = 0;
		_pRes->setReference(&ref, handle);
		_pRes->freeResources(_purgeAmount, &bytesFreed);
		_pRes->removeReference(&ref);
	}

	return handle;
}

int SputResource::closeResources() {
	// TODO: close the mounted LAB files

	return 1;
}

int SputResource::findResources() {
	Common::String dataDirectory = g_registry->getString("GrimDataDir");

	// TODO: the current directory is not added
	addResourcePathInternal(dataDirectory.c_str());

	if (!_extraDataPath.empty() && !_extraDataPath.equalsIgnoreCase(dataDirectory))
		addResourcePathInternal(_extraDataPath.c_str());

	return 1;
}

int SputResource::addResourcePath(const char *path) {
	_extraDataPath = path;

	return 1;
}

int SputResource::addResourcePathInternal(const char *path) {
	// TODO: mount the LAB files and the resource subdirectories found in path
	return 1;
}

int SputResource::initialize(SysStruct *pHS) {
	_pHS = pHS;

	int result = _pRes->sysInitialize(pHS);

	if (!result)
		_bInitialized = 1;

	return result;
}

int SputResource::logToFile(const char *path) {
	return _pRes->log(path) == 1;
}

int SputResource::terminate() {
	_pRes->sysTerminate();
	_pHS = nullptr;
	_bInitialized = 0;

	return 0;
}

int SputResource::startup() {
	if (_bStarted)
		return 1;

	if (!_pRes->sysStartup())
		_bStarted = registerHandlers();

	g_pSputResource = this;
	_oldMatLoader = _pRd->_material.registerLoader(materialLoader);
	_oldMatUnloader = _pRd->_material.registerUnloader(materialUnloader);

	return _bStarted == 0;
}

void SputResource::saveVerifyCallback(const char *fileName) {
	_pRes->initLists(kSputTagCost, fileName);
}

int SputResource::shutdown() {
	_pRes->sysShutdown();
	_pRd->_material.registerLoader(_oldMatLoader);
	_pRd->_material.registerUnloader(_oldMatUnloader);
	_bStarted = 0;

	return 0;
}

int SputResource::registerHandlers() {
	const int aTags[4] = { kSputTagCmap, kSputTagKeyf, kSputTagMat, kSputTagModl };
	int result = 0;

	for (uint i = 0; i < 4; i++) {
		result = _pRes->sysRegisterHandler(aTags[i], this);

		if (result != 1)
			return 0;
	}

	return result;
}

int SputResource::setMemoryUsage(int totalAllowedLimit, int purgeAmount) {
	if (totalAllowedLimit)
		_totalAllowedLimit = totalAllowedLimit;

	if (purgeAmount)
		_purgeAmount = purgeAmount;

	return purgeAmount;
}

int SputResource::loadResCmap(ResHandlerMsg *pMsg) {
	StdColorFormat colorFormat;

	colorFormat.colorMode = 1;
	colorFormat.bpp = 16;
	colorFormat.redBits = 5;
	colorFormat.greenBits = 6;
	colorFormat.blueBits = 5;
	colorFormat.redShift = 11;
	colorFormat.greenShift = 5;
	colorFormat.blueShift = 0;
	colorFormat.redLoss = 3;
	colorFormat.greenLoss = 2;
	colorFormat.blueLoss = 3;
	colorFormat.alphaBits = 0;
	colorFormat.alphaShift = 0;
	colorFormat.alphaLoss = 0;

	if (pMsg->tag != kSputTagCmap)
		return 0;

	switch (pMsg->op) {
	case 1: {
		RdColormap *pColormap = _pRd->_colormap.load(const_cast<char *>(pMsg->pName));

		if (!pColormap)
			return 0;

		if (pColormap->pRGB16) {
			_pRd->_colormap.buildRGB16(pColormap->pRGB16, pColormap->aColors, 0, 0, 0, &colorFormat);
			// TODO: color correct the RGB16 table
		} else {
			warning("SputResource::loadResCmap(): colormap %s has no RGB16 table", pMsg->pName);
		}

		return _pRes->allocCompletion(pMsg->tag, pMsg->pName, pColormap);
	}
	case 2: {
		RdColormap *pColormap = static_cast<RdColormap *>(pMsg->data);

		_pRd->_colormap.destroy(pColormap);

		return 1;
	}
	case 3: {
		RdColormap *pColormap = static_cast<RdColormap *>(pMsg->data);

		pMsg->result = _pRd->_colormap.size(pColormap);

		return 1;
	}
	default:
		break;
	}

	return 0;
}

int SputResource::loadResKeyf(ResHandlerMsg *pMsg) {
	if (pMsg->tag != kSputTagKeyf)
		return 0;

	switch (pMsg->op) {
	case 1: {
		RdKeyframe *pKeyframe = _pRd->_keyframe.load(const_cast<char *>(pMsg->pName));

		if (!pKeyframe)
			return 0;

		pKeyframe->fps = 15.0f;

		return _pRes->allocCompletion(pMsg->tag, pMsg->pName, pKeyframe);
	}
	case 2: {
		RdKeyframe *pKeyframe = static_cast<RdKeyframe *>(pMsg->data);

		_pRd->_keyframe.destroy(pKeyframe);

		return 1;
	}
	case 3: {
		RdKeyframe *pKeyframe = static_cast<RdKeyframe *>(pMsg->data);

		pMsg->result = 132;

		for (int i = 0; i < (int)pKeyframe->numNodes; i++) {
			pMsg->result += 44;
			pMsg->result += 56 * pKeyframe->paNodes[i].numEntries;
		}

		return 1;
	}
	default:
		break;
	}

	return 0;
}

int SputResource::loadResMat(ResHandlerMsg *pMsg) {
	if (pMsg->tag != kSputTagMat)
		return 0;

	switch (pMsg->op) {
	case 1: {
		char defaultName[] = "dflt.mat";
		RdMaterial *pMaterial = new RdMaterial;

		if (!_pRd->_material.loadEntry(const_cast<char *>(pMsg->pName), pMaterial, 0, 0) && !_pRd->_material.loadEntry(defaultName, pMaterial, 0, 0)) {
			_pRd->_material.freeEntry(pMaterial);
			delete pMaterial;
			pMaterial = nullptr;

			return 0;
		}

		int result = _pRes->allocCompletion(pMsg->tag, pMsg->pName, pMaterial);
		_pRes->setPurgeOnFree(pMaterial, 0);

		return result;
	}
	case 2: {
		RdMaterial *pMaterial = static_cast<RdMaterial *>(pMsg->data);

		_pRd->_material.freeEntry(pMaterial);
		delete pMaterial;
		pMaterial = nullptr;

		return 1;
	}
	case 3:
		pMsg->result = 0;

		return 1;
	default:
		break;
	}

	return 0;
}

int SputResource::loadResModl(ResHandlerMsg *pMsg) {
	if (pMsg->tag != kSputTagModl)
		return 0;

	switch (pMsg->op) {
	case 1: {
		RdModel3 *pModel3 = _pRd->_model3.load(const_cast<char *>(pMsg->pName));

		if (!pModel3)
			return 0;

		return _pRes->allocCompletion(pMsg->tag, pMsg->pName, pModel3);
	}
	case 2: {
		RdModel3 *pModel3 = static_cast<RdModel3 *>(pMsg->data);

		_pRd->_model3.destroy(pModel3);

		return 1;
	}
	case 3:
		pMsg->result = 1;

		return 1;
	default:
		break;
	}

	return 0;
}

int SputResource::cmap() {
	StdColorFormat colorFormat;
	ResNode *pNext;
	void *handle;

	colorFormat.colorMode = 1;
	colorFormat.bpp = 16;
	colorFormat.redBits = 5;
	colorFormat.greenBits = 6;
	colorFormat.blueBits = 5;
	colorFormat.redShift = 11;
	colorFormat.greenShift = 5;
	colorFormat.blueShift = 0;
	colorFormat.redLoss = 3;
	colorFormat.greenLoss = 2;
	colorFormat.blueLoss = 3;
	colorFormat.alphaBits = 0;
	colorFormat.alphaShift = 0;
	colorFormat.alphaLoss = 0;

	_pRes->allocatedIterFirst(&pNext, &handle, kSputTagCmap);

	while (handle) {
		RdColormap *pColormap = static_cast<RdColormap *>(handle);

		if (pColormap->pRGB16) {
			_pRd->_colormap.buildRGB16(pColormap->pRGB16, pColormap->aColors, 0, 0, 0, &colorFormat);
			// TODO: color correct the RGB16 table
		} else {
			warning("SputResource::cmap(): colormap %s has no RGB16 table", pColormap->aName);
		}

		_pRes->allocatedIterNext(&pNext, &handle, kSputTagCmap);
	}

	return 0;
}

RdMaterial *SputResource::mat(char *path, int bVideoMemory, int bVideoMemoryOnly) {
	RdMaterial *pMaterial = static_cast<RdMaterial *>(alloc(kSputTagMat, path));

	if (!pMaterial) {
		warning("SputResource::mat(): could not load material %s", path);
		return nullptr;
	}

	if (!pMaterial->unk_36)
		_pRes->setPurgeOnFree(pMaterial, 0);

	pMaterial->unk_36++;

	return pMaterial;
}

int SputResource::unlock(RdMaterial *pMaterial) {
	if (!pMaterial) {
		warning("SputResource::unlock(): no material given");
		return 0;
	}

	if (pMaterial->unk_36)
		pMaterial->unk_36--;

	if (!pMaterial->unk_36)
		return _pRes->setPurgeOnFree(pMaterial, 1);

	return 1;
}

int SputResource::reportAllocations() {
	return _pRes->dump("Grim.res", 0);
}

} // End of namespace Grim
