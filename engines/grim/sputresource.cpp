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

#include "common/algorithm.h"
#include "common/fs.h"
#include "common/path.h"
#include "common/str-array.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/blasttext.h"
#include "engines/grim/chore/cos.h"
#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/text.h"
#include "engines/grim/wuregistry.h"

namespace Grim {

static const char *const g_resourceDirectories[] = {
	"Animation", "Backgrounds", "Costumes", "Colormaps", "Fonts", "Materials", "Models", "Movies", "Objects", "Scripts", "Sets",
	"Voices", "Footsteps", "SFX", "Loops",
	"DEBUG_Animation", "DEBUG_Backgrounds", "DEBUG_Costumes", "DEBUG_Colormaps", "DEBUG_Fonts", "DEBUG_Materials", "DEBUG_Models",
	"DEBUG_Movies", "DEBUG_Objects", "DEBUG_Scripts", "DEBUG_Sets", "DEBUG_Voices", "DEBUG_Footsteps", "DEBUG_SFX", "DEBUG_Loops"
};

static RdMaterial *materialLoader(char *path, int videoMemory, int videoMemoryOnly) {
	return g_grim->getSputResource()->mat(path, videoMemory, videoMemoryOnly);
}

static int materialUnloader(RdMaterial *material) {
	return g_grim->getSputResource()->unlock(material);
}

static int saveVerifier(const char *fileName) {
	return g_grim->getSputResource()->saveVerifyCallback(fileName);
}

SputResource::SputResource(Res *pRes, RenderDroid *pRd, StdDisplay *pStdDisplay, StdBitmap *pStdBitmap) : _pRes(pRes), _pRd(pRd),
		_pStdDisplay(pStdDisplay), _pStdBitmap(pStdBitmap), _pSputRender(nullptr), _pSputRoom(nullptr), _pCos(nullptr),
		_pText(nullptr), _pBlastText(nullptr), _pWuRegistry(nullptr), _pHS(nullptr), _bInitialized(0), _bStarted(0), _bLoaderLocked(0), _totalAllowedLimit(0xBB8000), _purgeAmount(0x271000),
		_oldMatLoader(nullptr), _oldMatUnloader(nullptr) {
}

int SputResource::handler(ResHandlerMsg *pMsg) {
	switch (pMsg->tag) {
	case kSputTagBknd:
		return loadResBknd(pMsg);
	case kSputTagCost:
		return loadResCost(pMsg);
	case kSputTagCmap:
		return loadResCmap(pMsg);
	case kSputTagFont:
		return loadResFont(pMsg);
	case kSputTagKeyf:
		return loadResKeyf(pMsg);
	case kSputTagMat:
		return loadResMat(pMsg);
	case kSputTagModl:
		return loadResModl(pMsg);
	case kSputTagSet:
		return loadResSet(pMsg);
	case kSputTagSnd:
		return loadResSnd(pMsg);
	case kSputTagSync:
		return loadResSync(pMsg);
	case kSputTagZbuf:
		return loadResZbuf(pMsg);
	case kSputTagMem:
		return loadResMem(pMsg);
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
	_pRes->fileCloseAll();

	return 1;
}

int SputResource::findResources() {
	char aDataDirectory[512];

	aDataDirectory[0] = 0;
	_pWuRegistry->getStr("GrimDataDir", aDataDirectory, sizeof(aDataDirectory), "");

	Common::String dataDirectory(aDataDirectory);
	Common::String currentDirectory(dataDirectory);

	if (!_extraDataPath.empty() && !_extraDataPath.equalsIgnoreCase(dataDirectory))
		addResourcePathInternal(_extraDataPath.c_str());

	if (!currentDirectory.equalsIgnoreCase(_extraDataPath))
		addResourcePathInternal(currentDirectory.c_str());

	return 1;
}

int SputResource::addResourcePath(const char *path) {
	_extraDataPath = path;

	return 1;
}

int SputResource::addResourcePathInternal(const char *path) {
	const char separator = Common::Path::kNativeSeparator;
	Common::String directory(path);

	if (!directory.empty() && (directory.lastChar() == '\\' || directory.lastChar() == '/'))
		directory.deleteLastChar();

	if (directory.empty())
		return 0;

	Common::FSNode node((Common::Path(directory, separator)));
	Common::FSList files;
	Common::FSList subDirectories;
	Common::StringArray labNames;

	node.getChildren(files, Common::FSNode::kListFilesOnly);
	node.getChildren(subDirectories, Common::FSNode::kListDirectoriesOnly);

	for (uint i = 0; i < files.size(); i++) {
		Common::String name(files[i].getName());

		name.toLowercase();

		if (!name.hasSuffix(".lab") || name == "datausr.lab")
			continue;

		labNames.push_back(name);
	}

	Common::sort(labNames.begin(), labNames.end());

	directory += separator;

	if (!_pRes->fileOpen(directory.c_str()))
		return 0;

	_pRes->fileMoveToFront(directory.c_str());

	for (uint i = 0; i < labNames.size(); i++) {
		Common::String labPath(directory);

		labPath += labNames[i];

		if (_pRes->fileOpen(labPath.c_str()))
			_pRes->fileMoveToFront(labPath.c_str());
	}

	for (uint i = 0; i < ARRAYSIZE(g_resourceDirectories); i++) {
		for (uint j = 0; j < subDirectories.size(); j++) {
			if (!subDirectories[j].getName().equalsIgnoreCase(g_resourceDirectories[i]))
				continue;

			Common::String subPath(directory);

			subPath += subDirectories[j].getName();
			subPath += separator;

			if (_pRes->fileOpen(subPath.c_str()))
				_pRes->fileMoveToFront(subPath.c_str());
		}
	}

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

	_oldMatLoader = _pRd->_material.registerLoader(materialLoader);
	_oldMatUnloader = _pRd->_material.registerUnloader(materialUnloader);
	_pCos->registerSaveVerify(saveVerifier);

	return _bStarted == 0;
}

int SputResource::saveVerifyCallback(const char *fileName) {
	return _pRes->initLists(kSputTagCost, fileName);
}

int SputResource::shutdown() {
	_pRes->sysShutdown();
	_pRd->_material.registerLoader(_oldMatLoader);
	_pRd->_material.registerUnloader(_oldMatUnloader);
	_bStarted = 0;

	return 0;
}

int SputResource::registerHandlers() {
	const int aTags[12] = { kSputTagBknd, kSputTagCost, kSputTagCmap, kSputTagFont, kSputTagKeyf, kSputTagMat, kSputTagModl, kSputTagSet,
		kSputTagSnd, kSputTagSync, kSputTagZbuf, kSputTagMem };
	int result = 0;

	for (uint i = 0; i < 12; i++) {
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

int SputResource::loadResSync(ResHandlerMsg *pMsg) {
	if (pMsg->tag != kSputTagSync)
		return 0;

	switch (pMsg->op) {
	case 1: {
		IMuseLipSync *pLipSync = g_grim->getImuse()->grimSoundLipSyncLoad(pMsg->pName);

		if (!pLipSync)
			return 0;

		return _pRes->allocCompletion(pMsg->tag, pMsg->pName, pLipSync);
	}
	case 2:
		g_grim->getImuse()->grimSoundLipSyncFree(static_cast<IMuseLipSync *>(pMsg->data));

		return 1;
	case 3: {
		IMuseLipSync *pLipSync = static_cast<IMuseLipSync *>(pMsg->data);

		pMsg->result = pLipSync->size + 40;

		return 1;
	}
	default:
		break;
	}

	return 0;
}

int SputResource::loadResBknd(ResHandlerMsg *pMsg) {
	if (pMsg->tag != kSputTagBknd)
		return 0;

	switch (pMsg->op) {
	case 1: {
		Common::String path(pMsg->pName);
		StdBitmapStruct *pBitmap = _pStdBitmap->loadLbm(path, 0, 0);

		if (!pBitmap)
			return 0;

		// The demo does not correct the colors of the backgrounds
		if (!_pRd->isDemo() && !pBitmap->encoding && pBitmap->format.colorMode == 1) {
			for (int i = 0; i < (int)pBitmap->numFrames; i++) {
				StdVBuffer *pFrame = pBitmap->apFrames[i];

				_pStdDisplay->vBufferLock(pFrame);

				for (int row = 0; row < pFrame->tex.height; row++)
					_pSputRender->colorCorrect((uint16 *)(pFrame->pPixels + pFrame->tex.pitchInBytes * row), pFrame->tex.width);

				_pStdDisplay->vBufferUnlock(pFrame);

				uint16 colorKey = 63519;

				_pSputRender->colorCorrect(&colorKey, 1);
				pFrame->colorKey = colorKey;
			}
		}

		return _pRes->allocCompletion(pMsg->tag, pMsg->pName, pBitmap);
	}
	case 2: {
		StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(pMsg->data);

		_pStdBitmap->unloadBitmap(pBitmap);

		return 1;
	}
	case 3: {
		StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(pMsg->data);

		pMsg->result = _pStdBitmap->memUsage(pBitmap);

		return 1;
	}
	default:
		break;
	}

	return 0;
}

void SputResource::loadBknd() {
	ResNode *pNext;
	void *handle;

	_pRes->allocatedIterFirst(&pNext, &handle, kSputTagBknd);

	while (handle) {
		StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(handle);

		if (!pBitmap->encoding) {
			Common::String path(pBitmap->aName);
			StdBitmapStruct *pNewBitmap = _pStdBitmap->loadLbm(path, 0, 0);

			if (pNewBitmap) {
				if (pNewBitmap->numFrames < pBitmap->numFrames) {
					warning("SputResource::loadBknd(): bitmap %s has fewer frames than the loaded one", pBitmap->aName);
				} else {
					for (int j = 0; j < (int)pBitmap->numFrames; j++) {
						StdVBuffer *pOldFrame = pBitmap->apFrames[j];

						pBitmap->apFrames[j] = pNewBitmap->apFrames[j];
						pNewBitmap->apFrames[j] = pOldFrame;
					}
				}

				_pStdBitmap->unloadBitmap(pNewBitmap);
			}

			for (int k = 0; k < (int)pBitmap->numFrames; k++) {
				StdVBuffer *pFrame = pBitmap->apFrames[k];

				_pStdDisplay->vBufferLock(pFrame);

				for (int row = 0; row < pFrame->tex.height; row++)
					_pSputRender->colorCorrect((uint16 *)(pFrame->pPixels + pFrame->tex.pitchInBytes * row), pFrame->tex.width);

				_pStdDisplay->vBufferUnlock(pFrame);
			}
		}

		uint16 colorKey = 63519;

		_pSputRender->colorCorrect(&colorKey, 1);
		pBitmap->colorKey = colorKey;

		for (int n = 0; n < (int)pBitmap->numFrames; n++) {
			uint16 frameColorKey = 63519;

			_pSputRender->colorCorrect(&frameColorKey, 1);
			pBitmap->apFrames[n]->colorKey = frameColorKey;
		}

		_pRes->allocatedIterNext(&pNext, &handle, kSputTagBknd);
	}
}

int SputResource::loadResCost(ResHandlerMsg *pMsg) {
	if (pMsg->tag != kSputTagCost)
		return 0;

	switch (pMsg->op) {
	case 1: {
		int hCostume = _pCos->load(pMsg->pName);

		if (!hCostume)
			return 0;

		return _pRes->allocCompletion(pMsg->tag, pMsg->pName, (void *)(intptr)hCostume);
	}
	case 2:
		_pCos->freeCostume((int)(intptr)pMsg->data);

		return 1;
	case 3:
		pMsg->result = 1;

		return 1;
	default:
		break;
	}

	return 0;
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
			_pSputRender->colorCorrect((uint16 *)pColormap->pRGB16, 0x4000);
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

int SputResource::loadResFont(ResHandlerMsg *pMsg) {
	if (pMsg->tag != kSputTagFont)
		return 0;

	switch (pMsg->op) {
	case 1: {
		BlastTextFont *pFont = nullptr;

		if (!_pBlastText->load(pMsg->pName, pFont)) {
			int result = _pRes->allocCompletion(pMsg->tag, pMsg->pName, pFont);

			// The demo can purge the fonts
			if (!_pRd->isDemo())
				_pRes->setPurgeOnFree(pFont, 0);

			return result;
		}

		break;
	}
	case 2: {
		BlastTextFont *pFont = static_cast<BlastTextFont *>(pMsg->data);

		_pText->freeFont(pFont);

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

int SputResource::loadResKeyf(ResHandlerMsg *pMsg) {
	if (pMsg->tag != kSputTagKeyf)
		return 0;

	switch (pMsg->op) {
	case 1: {
		RdKeyframe *pKeyframe = _pRd->_keyframe.load(const_cast<char *>(pMsg->pName));

		if (!pKeyframe)
			return 0;

		// The demo keeps the rate of the file
		if (!_pRd->isDemo())
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

int SputResource::loadResSet(ResHandlerMsg *pMsg) {
	if (pMsg->tag != kSputTagSet)
		return 0;

	switch (pMsg->op) {
	case 1: {
		Room *pRoom = _pSputRoom->loadRoom(pMsg->pName);

		if (!pRoom)
			return 0;

		return _pRes->allocCompletion(pMsg->tag, pMsg->pName, pRoom);
	}
	case 2: {
		Room *pRoom = static_cast<Room *>(pMsg->data);

		_pSputRoom->freeRoom(pRoom);

		return 1;
	}
	case 3: {
		Room *pRoom = static_cast<Room *>(pMsg->data);

		pMsg->result = pRoom->structSize;

		return 1;
	}
	default:
		break;
	}

	return 0;
}

int SputResource::loadResSnd(ResHandlerMsg *pMsg) {
	return 0;
}

int SputResource::loadResZbuf(ResHandlerMsg *pMsg) {
	if (pMsg->tag != kSputTagZbuf)
		return 0;

	switch (pMsg->op) {
	case 1: {
		StdVBuffer *pZBuffer = _pSputRoom->loadZBuffer(pMsg->pName);

		if (!pZBuffer)
			return 0;

		return _pRes->allocCompletion(pMsg->tag, pMsg->pName, pZBuffer);
	}
	case 2: {
		StdVBuffer *pZBuffer = static_cast<StdVBuffer *>(pMsg->data);

		_pSputRoom->freeZBuffer(pZBuffer);

		return 1;
	}
	case 3: {
		StdVBuffer *pZBuffer = static_cast<StdVBuffer *>(pMsg->data);

		pMsg->result = pZBuffer->tex.sizeInBytes;

		return 1;
	}
	default:
		break;
	}

	return 0;
}

int SputResource::loadResMem(ResHandlerMsg *pMsg) {
	ResMemBlock *pBlock = static_cast<ResMemBlock *>(pMsg->data);

	if (pMsg->op == 2) {
		if (!pBlock) {
			warning("SputResource::loadResMem(): no memory block to free");
			return 1;
		}

		delete[] pBlock->pData;
		pBlock->pData = nullptr;
		delete pBlock;
		pBlock = nullptr;
	} else if (pMsg->op == 3) {
		if (!pBlock) {
			warning("SputResource::loadResMem(): no memory block to measure");
			return 1;
		}

		pMsg->result = pBlock->size;

		return 1;
	}

	return 1;
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
			_pSputRender->colorCorrect((uint16 *)pColormap->pRGB16, 0x4000);
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

	if (!pMaterial->lockCount)
		_pRes->setPurgeOnFree(pMaterial, 0);

	pMaterial->lockCount++;

	return pMaterial;
}

int SputResource::unlock(RdMaterial *pMaterial) {
	if (!pMaterial) {
		warning("SputResource::unlock(): no material given");
		return 0;
	}

	if (pMaterial->lockCount)
		pMaterial->lockCount--;

	if (!pMaterial->lockCount)
		return _pRes->setPurgeOnFree(pMaterial, 1);

	return 1;
}

int SputResource::reportAllocations() {
	return _pRes->dump("Grim.res", 0);
}

} // End of namespace Grim
