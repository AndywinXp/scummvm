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

#include "common/file.h"
#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/registry.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/std/std_lbm.h"

namespace Grim {

static SputCostume *g_pSputCostume = nullptr;

static int instanceHookThunk(int hInst, int bCreate) {
	return g_pSputCostume->instanceHook(hInst);
}

static int saveHookThunk(const char *path, int hCostume, const char *header) {
	return g_pSputCostume->saveHook(path, hCostume, header);
}

void SputCostume::puppetHookCallback(void *userArg, int trackNum, int markerType) {
	g_pSputCostume->puppetHook(static_cast<ModelInst *>(userArg), trackNum, markerType);
}

SputCostume::SputCostume(Res *pRes, RenderDroid *pRd, SputResource *pSputResource, SputRender *pSputRender, SputRoom *pSputRoom, StdLbm *pStdLbm) :
		_pRes(pRes), _pRd(pRd), _pSputResource(pSputResource), _pSputRender(pSputRender), _pSputRoom(pSputRoom), _pStdLbm(pStdLbm),
		_aActors(nullptr), _pHS(nullptr), _bInitialized(0), _bStarted(0), _pShadowColormap(nullptr), _aLECBitmaps(nullptr),
		_pLastBitmap(nullptr), _oldInstanceHook(nullptr), _oldSaveHook(nullptr),
		_thingHandler(this), _colormapHandler(this), _meshHandler(this), _materialHandler(this), _keyframeHandler(this),
		_bitmapHandler(this), _soundHandler(this), _smushHandler(this), _spriteHandler(this), _imuseHandler(this), _luaVarHandler(this) {
	memset(_keyframePath, 0, sizeof(_keyframePath));
	memset(_colormapPath, 0, sizeof(_colormapPath));
	memset(_materialPath, 0, sizeof(_materialPath));
	memset(_modelPath, 0, sizeof(_modelPath));
	memset(_soundPath, 0, sizeof(_soundPath));
	memset(_smushPath, 0, sizeof(_smushPath));
	memset(_bitmapPath, 0, sizeof(_bitmapPath));
}

void SputCostume::setShadowColor(int red, int green, int blue) {
	byte greenByte = (byte)green;
	uint16 color = (uint16)(((uint)blue >> 3) | (32 * ((((uint)red >> 3) << 6) | ((uint)green >> 2))));

	_pSputRender->colorCorrect(&color, 1);

	if (!_pShadowColormap) {
		char fileName[] = "shadow.cmp";

		_pShadowColormap = _pRd->_colormap.load(fileName);
	}

	if (!_pShadowColormap || !_pShadowColormap->pRGB16) {
		warning("SputCostume::setShadowColor(): the shadow colormap has no RGB16 table");
		return;
	}

	uint16 *pRGB16 = (uint16 *)_pShadowColormap->pRGB16;

	for (int i = 0; i < 0x4000; i++)
		pRGB16[i] = color;

	for (int i = 0; i < 768; i += 3) {
		_pShadowColormap->aColors[i] = (byte)red;
		_pShadowColormap->aColors[i + 1] = greenByte;
		_pShadowColormap->aColors[i + 2] = (byte)blue;
	}
}

void SputCostume::printCostumeInfo(CostumeNode *pNode) {
	if (!pNode)
		return;

	CostumeNode *pCurrent = pNode;

	do {
		int numChores = _cos.instCountChores(pCurrent->hCostumeInstance);

		for (int i = 0; i < numChores; i++)
			_cos.instChoreStatus(pCurrent->hCostumeInstance, i);

		pCurrent = pCurrent->pNext;
	} while (pCurrent);

	for (ModelInst *pModelInst = pNode->pHeadList; pModelInst; pModelInst = pModelInst->pNext) {
		if (!pModelInst->bVisible)
			continue;

		if (!pModelInst->pThing || pModelInst->pThing->type != 1) {
			warning("SputCostume::printCostumeInfo(): the visible thing is not a model");
			continue;
		}

		RdModel3 *pModel3 = pModelInst->pThing->data.pModel3;

		for (uint i = 0; i < pModel3->numHNodes; i++)
			strstr(pModel3->aHNodes[i].aName, "skirt");
	}
}

void SputCostume::freeCostumeNodes(CostumeNode *pNode, int bFreeInstance) {
	CostumeNode *pCurrent = pNode;

	while (pCurrent) {
		CostumeNode *pNext = pCurrent->pNext;

		if (bFreeInstance)
			_cos.freeCosInstance(pCurrent->hCostumeInstance);

		if (pCurrent->rCostume.valid)
			_pRes->removeReference(&pCurrent->rCostume);

		delete pCurrent;
		pCurrent = pNext;
	}
}

CostumeNode *SputCostume::popCostume(CostumeNode *pNode, int bFreeInstance) {
	CostumeNode *pNext = pNode;

	if (pNode) {
		pNext = pNode->pNext;
		pNode->pNext = nullptr;
		_cos.instStopChores(pNode->hCostumeInstance);
		freeCostumeNodes(pNode, bFreeInstance);
	}

	return pNext;
}

CostumeNode *SputCostume::setCostume(CostumeNode *pNode, int hCostume, int hInstance) {
	CostumeNode *pNext = popCostume(pNode, 1);

	return pushCostume(pNext, hCostume, hInstance);
}

CostumeNode *SputCostume::pushCostume(CostumeNode *pPrev, int hCostume, int hInstance) {
	CostumeNode *pNode = allocCostumeNode();

	pNode->pNext = pPrev;

	if (hInstance) {
		pNode->hCostumeInstance = hInstance;
	} else {
		int hParentInstance = 0;

		if (pPrev)
			hParentInstance = pPrev->hCostumeInstance;

		pNode->hCostumeInstance = _cos.createCosInstance(hCostume, hParentInstance);
		_pRes->setReference(&pNode->rCostume, (void *)(intptr)hCostume);
	}

	pNode->pHeadThing = getMainThing(pNode->hCostumeInstance);

	if (pNode->pHeadThing && pPrev && pPrev->pHeadThing == pNode->pHeadThing) {
		pNode->aHeadJoint[0] = pPrev->aHeadJoint[0];
		pNode->aHeadJoint[1] = pPrev->aHeadJoint[1];
		pNode->aHeadJoint[2] = pPrev->aHeadJoint[2];
		pNode->headTurnRate = pPrev->headTurnRate;
		pNode->headYawLimit = pPrev->headYawLimit;
		pNode->headPitchLimit = pPrev->headPitchLimit;
		pNode->headLookRef = pPrev->headLookRef;
		pNode->pHeadList = pPrev->pHeadList;
	}

	int numComponents = _cos.instCountComponents(pNode->hCostumeInstance);

	for (int i = 0; i < numComponents; i++) {
		int fourcc = 0;
		ModelInst *pModelInst = static_cast<ModelInst *>(_cos.instGetInstanceData(pNode->hCostumeInstance, i, &fourcc));

		if ((fourcc == kSputCostumeTagMainThing || fourcc == kSputCostumeTagThing || fourcc == kSputCostumeTagSprite) && pModelInst && pModelInst->refCount == 1) {
			pModelInst->pNext = pNode->pHeadList;
			pNode->pHeadList = pModelInst;
		}
	}

	return pNode;
}

CostumeNode *SputCostume::allocCostumeNode() {
	CostumeNode *pNode = new CostumeNode();

	initCostumeNode(pNode);

	return pNode;
}

void SputCostume::initCostumeNode(CostumeNode *pNode) {
	pNode->hCostumeInstance = 0;
	pNode->aHeadJoint[0] = -1;
	pNode->aHeadJoint[1] = -1;
	pNode->aHeadJoint[2] = -1;
	pNode->headYawLimit = 30.0f;
	pNode->headPitchLimit = 90.0f;
	pNode->headLookRef = -0.35f;
	pNode->headTurnRate = 360.0f;
	pNode->walkChore = -1;
	pNode->turnLeftChore = -1;
	pNode->turnRightChore = -1;
	pNode->restChore = -1;
	pNode->mumbleChore = -1;
	memset(pNode->aChore, 0xFF, sizeof(pNode->aChore));
	memset(&pNode->rCostume, 0, sizeof(pNode->rCostume));
	pNode->pHeadList = nullptr;
	pNode->pNext = nullptr;
}

RdThing *SputCostume::getMainThing(int hInst) {
	int numComponents = _cos.instCountComponents(hInst);

	for (int i = 0; i < numComponents; i++) {
		int fourcc = 0;
		ModelInst *pModelInst = static_cast<ModelInst *>(_cos.instGetInstanceData(hInst, i, &fourcc));

		if (fourcc == kSputCostumeTagMainThing) {
			if (!pModelInst) {
				warning("SputCostume::getMainThing(): the main thing has no instance data");
				return nullptr;
			}

			return pModelInst->pThing;
		}
	}

	return nullptr;
}

void SputCostume::update(CostumeNode *pNode, float delta) {
	double scaled = (double)delta * 1000.0;
	double rounded;

	if (scaled > 0.0)
		rounded = 0.5 + scaled;
	else
		rounded = scaled - 0.5;

	int deltaMs = 0;

	if (rounded > -2147483649.0 && rounded < 2147483648.0)
		deltaMs = (int)rounded;
	else
		warning("SputCostume::update(): the time delta is out of range");

	float seconds = (float)((double)(uint)deltaMs * (double)0.001f);

	if (!pNode)
		return;

	RdThing *pHeadThing = pNode->pHeadThing;
	CostumeNode *pCurrent = pNode;

	do {
		if (pCurrent->pHeadThing != pHeadThing)
			break;

		_cos.instUpdateChores(pCurrent->hCostumeInstance, deltaMs);
		pCurrent = pCurrent->pNext;
	} while (pCurrent);

	for (ModelInst *pModelInst = pNode->pHeadList; pModelInst; pModelInst = pModelInst->pNext) {
		RdPuppet *pPuppet = pModelInst->pThing->pPuppet;

		if (pPuppet)
			_pRd->_puppet.updateTracks(pPuppet, seconds);
	}
}

int SputCostume::initialize(SysStruct *pHS) {
	_pHS = pHS;

	int result = _cos.initialize(pHS);

	_bInitialized = result == 0;

	return result != 0;
}

void SputCostume::terminate() {
	_pHS = nullptr;

	if (_bInitialized) {
		_bInitialized = 0;
		_cos.terminate();

		if (_pShadowColormap)
			_pRd->_colormap.destroy(_pShadowColormap);
	}
}

int SputCostume::startup() {
	if (_bStarted)
		return 1;

	g_pSputCostume = this;

	_cos.startup();

	_cos.newStateBasedTag(kSputCostumeTagMainThing, "MAIN RDroid Thing (.3DO)", 0, &_thingHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagThing, "RDroid Thing (.3DO)", 0, &_thingHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagColormap, "RDroid Colormap", 0, &_colormapHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagMesh, "RDroid Thing Mesh", 0, &_meshHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagMaterial, "RDroid Animated Material", 0, &_materialHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagKeyframe, "RDroid Keyframe (.KEY)", 0, &_keyframeHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagBitmap, "General Bitmap", 0, &_bitmapHandler, 1);
	_cos.newStateBasedTag(kSputCostumeTagAnimation, "Animation (old,use Bitmap)", 0, &_bitmapHandler, 1);
	_cos.newStateBasedTag(kSputCostumeTagStreamedSound, "iMuse Streamed Sound FX", 0, &_soundHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagResidentSound, "iMuse Resident Sound FX", 0, &_soundHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagSmush, "Smush Movie", 0, &_smushHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagSprite, "Sprite", 0, &_spriteHandler, 2);
	_cos.newIntTag(kSputCostumeTagSequence, "iMuse Sequence", 0, &_imuseHandler, 2);
	_cos.newIntTag(kSputCostumeTagState, "iMuse State", 0, &_imuseHandler, 2);
	_cos.newIntTag(kSputCostumeTagLuaVar, "Lua Variable", 0, &_luaVarHandler, 1);

	_oldInstanceHook = _cos.registerInstanceHook(instanceHookThunk);
	_oldSaveHook = _cos.registerSaveHook(saveHookThunk);

	// TODO: sputLua_RegisterFunctions(luaCostumeFuncs, 2): GetKeyLength, GetSmushLength

	_bStarted = 1;

	getDataPath("KeyframePath", _keyframePath, "Animation");
	getDataPath("ColormapPath", _colormapPath, "Colormaps");
	getDataPath("MaterialPath", _materialPath, "Materials");
	getDataPath("ModelPath", _modelPath, "Models");
	getDataPath("SoundPath", _soundPath, "SFX");
	getDataPath("SmushPath", _smushPath, "Movies");
	getDataPath("BitmapPath", _bitmapPath, "Objects");

	return 0;
}

int SputCostume::shutdown() {
	writeRegistryValue("KeyframePath", _keyframePath);
	writeRegistryValue("BitmapPath", _bitmapPath);
	writeRegistryValue("ColormapPath", _colormapPath);
	writeRegistryValue("MaterialPath", _materialPath);
	writeRegistryValue("ModelPath", _modelPath);
	writeRegistryValue("SoundPath", _soundPath);
	writeRegistryValue("SmushPath", _smushPath);

	_oldInstanceHook = _cos.registerInstanceHook(_oldInstanceHook);
	_oldSaveHook = _cos.registerSaveHook(_oldSaveHook);

	if (_aLECBitmaps) {
		delete[] _aLECBitmaps;
		_aLECBitmaps = nullptr;
	}

	int result = _cos.shutdown();

	_bStarted = 0;

	return result;
}

int SputCostume::saveHook(const char *path, int hCostume, const char *header) {
	const char *pName = strrchr(path, '\\');

	if (pName)
		pName++;
	else
		pName = path;

	const char *costumeName = _cos.getCostumeName(hCostume);

	if (!costumeName || scumm_stricmp(pName, costumeName)) {
		// TODO: grimPlat_SaveFileDialog("Name chore list file...", "Script files (*.lua, *.def)")
		return 0;
	}

	const char *pExtension = _pStdLbm->getExtension(path);

	if (!pExtension) {
		warning("SputCostume::saveHook(): %s has no extension", path);
		return 0;
	}

	Common::String fileName = Common::String(path, pExtension) + "lua";
	Common::DumpFile file;

	if (!file.open(fileName.c_str())) {
		// TODO: grimPlat_QueryDialog("File Error!", "Unable to open chore list file...")
		warning("SputCostume::saveHook(): unable to open chore list file %s", fileName.c_str());
		return 0;
	}

	if (!header) {
		warning("SputCostume::saveHook(): null header");
		header = "(null)";
	}

	file.writeString(Common::String::format("-- Chore numbering for %s.\n", _pStdLbm->stripPath(path)));
	file.writeString(Common::String::format("-- %s\n\n", header));

	char baseName[32];

	_pStdLbm->copyBaseName(baseName, 32, path);

	int hInst = _cos.createCosInstance(hCostume, 0);
	int numChores = _cos.instCountChores(hInst);

	for (int i = 0; i < numChores; i++) {
		const char *choreName = _cos.instChoreName(hInst, i);

		if (!choreName)
			choreName = "";

		file.writeString(Common::String::format("%s_%s\t\t=\t%d\n", baseName, choreName, i));
	}

	file.close();
	_cos.freeCosInstance(hInst);

	return 1;
}

int SputCostume::instanceHook(int hInst) {
	_cos.instSetUserData(hInst, 0);

	return 1;
}

void SputCostume::puppetHook(ModelInst *pModel, int trackNum, int markerType) {
	if (!pModel)
		return;

	// TODO: sputLua_DoCostumeMarkerCallback(&aActors[Cos_InstGetUserData(pModel->trackUserArg)], markerType)

	if (!markerType) {
		if (trackNum < 0 || trackNum >= 6) {
			warning("SputCostume::puppetHook(): invalid track %d", trackNum);
			return;
		}

		_pRes->removeReference(&pModel->aTrackRefs[trackNum]);
	}
}

void SputCostume::fadeKeysIn(int hInst, int choreIndex, int timeMs) {
	int hCostume = 0;

	_cos.instGetCostume(hInst, &hCostume);

	CosCostume *pCostume = _cos.ptrFromHandle(hCostume);

	if (!pCostume) {
		warning("SputCostume::fadeKeysIn(): invalid costume instance %d", hInst);
		return;
	}

	if (choreIndex < 0 || choreIndex >= pCostume->numChores) {
		warning("SputCostume::fadeKeysIn(): invalid chore %d", choreIndex);
		return;
	}

	CosChore *pChore = &pCostume->aChores[choreIndex];

	for (int i = 0; i < pChore->numTracks; i++) {
		int fourcc = 0;
		KeyframeInst *pKeyframeInst = static_cast<KeyframeInst *>(_cos.instGetInstanceData(hInst, pChore->aTracks[i].compIndex, &fourcc));

		if (fourcc != kSputCostumeTagKeyframe || !pKeyframeInst)
			continue;

		RdPuppet *pPuppet = pKeyframeInst->pModelInst->pThing->pPuppet;
		int trackNum = 0;

		if (!pKeyframeInst->ref.valid)
			warning("SputCostume::fadeKeysIn(): Bad keyframe Reference");

		while (trackNum < 6 && pPuppet->aTracks[trackNum].pKeyframe != pKeyframeInst->ref.handle)
			trackNum++;

		if (trackNum == 6)
			continue;

		if (pPuppet->aTracks[trackNum].fade == 1.0f)
			pPuppet->aTracks[trackNum].fade = 0.0f;

		float time = (float)((double)timeMs * (double)0.001f);

		_pRd->_puppet.fadeInTrack(pPuppet, trackNum, time);
	}
}

void SputCostume::fadeKeysOut(int hInst, int choreIndex, int timeMs) {
	int hCostume = 0;

	_cos.instGetCostume(hInst, &hCostume);

	CosCostume *pCostume = _cos.ptrFromHandle(hCostume);

	if (!pCostume) {
		warning("SputCostume::fadeKeysOut(): invalid costume instance %d", hInst);
		return;
	}

	if (choreIndex < 0 || choreIndex >= pCostume->numChores) {
		warning("SputCostume::fadeKeysOut(): invalid chore %d", choreIndex);
		return;
	}

	CosChore *pChore = &pCostume->aChores[choreIndex];

	for (int i = 0; i < pChore->numTracks; i++) {
		int fourcc = 0;
		KeyframeInst *pKeyframeInst = static_cast<KeyframeInst *>(_cos.instGetInstanceData(hInst, pChore->aTracks[i].compIndex, &fourcc));

		if (fourcc != kSputCostumeTagKeyframe || !pKeyframeInst)
			continue;

		RdPuppet *pPuppet = pKeyframeInst->pModelInst->pThing->pPuppet;
		int trackNum = 0;

		if (!pKeyframeInst->ref.valid)
			warning("SputCostume::fadeKeysOut(): Bad keyframe Reference");

		while (trackNum < 6 && pPuppet->aTracks[trackNum].pKeyframe != pKeyframeInst->ref.handle)
			trackNum++;

		if (trackNum == 6)
			continue;

		float time = (float)((double)timeMs * (double)0.001f);

		_pRd->_puppet.fadeOutTrack(pPuppet, trackNum, time);
	}
}

void SputCostume::getDataPath(const char *valueName, char *pData, const char *defaultDirectory) {
	// TODO: read the "Requesters\<valueName>" registry value into pData

	if (*pData)
		return;

	Common::String dataDirectory = g_registry->getString("GrimDataDir");

	if (dataDirectory.size() >= 128)
		return;

	Common::strlcpy(pData, dataDirectory.c_str(), 128);
	Common::strlcat(pData, "\\", 128);
	Common::strlcat(pData, defaultDirectory, 128);
}

void SputCostume::writeRegistryValue(const char *valueName, const char *pData) {
	if (*pData) {
		// TODO: save the "Requesters\<valueName>" registry value
	}
}

void SputCostume::nullSub1() {
}

int SputCostume::stub() {
	return 1;
}

} // End of namespace Grim
