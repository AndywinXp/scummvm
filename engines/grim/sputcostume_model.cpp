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

#include "common/str.h"
#include "common/stream.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/sputcostume.h"
#include "engines/grim/sputresource.h"

namespace Grim {

static const int mainThingKeyValues[2] = { 0, 4 };
static const char *const mainThingKeyNames[2] = { "Hide", "Show" };
static const CosComponentNames mainThingKeyDesc = { 2, mainThingKeyValues, mainThingKeyNames, nullptr, nullptr };

static const int thingMeshKeyValues[2] = { 0, 4 };
static const char *const thingMeshKeyNames[2] = { "Hide", "Show" };
static const CosComponentNames thingMeshKeyDesc = { 2, thingMeshKeyValues, thingMeshKeyNames, nullptr, nullptr };

static const int colormapKeyValues[1] = { 0 };
static const char *const colormapKeyNames[1] = { "Exist" };
static const CosComponentNames colormapKeyDesc = { 1, colormapKeyValues, colormapKeyNames, nullptr, nullptr };

void ThingHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	ModelInst *pInst = static_cast<ModelInst *>(pInstData);

	if (keyValue < 0 || keyValue >= 2) {
		warning("ThingHandler::change(): invalid key %d", keyValue);
		return;
	}

	pInst->bVisible = mainThingKeyValues[keyValue] != 0;
}

int ThingHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	char name[32];
	int resolveParentOffset = 1;

	memset(name, 0, sizeof(name));

	if (pParentId && parentFourcc != kSputCostumeTagMesh)
		return 0;

	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = pComp->name;

	Common::strlcpy(name, args, 32);

	char *pModelName = strtok(name, ",");
	char *pFlag = strtok(nullptr, ",");

	if (!pModelName) {
		warning("ThingHandler::request(): empty model name");
		return 0;
	}

	void *hModel = _pSputCostume->_pSputResource->alloc(kSputTagModl, pModelName);

	if (pFlag)
		resolveParentOffset = atoi(pFlag);

	if (!hModel)
		return 0;

	ThingCompRes *pRes = new ThingCompRes();

	_pSputCostume->_pRes->setReference(&pRes->ref, hModel);
	pRes->resolveParentOffset = resolveParentOffset;

	return _pSputCostume->_cos.registerComponent(hCostume, args, pRes, fourcc, pParentId, parentFourcc, &mainThingKeyDesc) != 0;
}

void ThingHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	ThingCompRes *pRes = static_cast<ThingCompRes *>(pCompRes);

	_pSputCostume->_pRes->removeReference(&pRes->ref);
	delete pRes;
}

void ThingHandler::stop(void *pInstData) {
	ModelInst *pInst = static_cast<ModelInst *>(pInstData);

	pInst->bVisible = pInst->pParentModel == nullptr;
}

int ThingHandler::adjust(void **ppCompRes, char *args, int argsSize) {
	ThingCompRes *pRes = static_cast<ThingCompRes *>(*ppCompRes);
	int value = pRes->resolveParentOffset != 0;
	int accepted = 0; // TODO: grimPlat_ControlPanel("Resolve parent offset (0 = off)")

	if (!accepted)
		return 0;

	char *pToken = strtok(args, ",");

	if (!pToken) {
		warning("ThingHandler::adjust(): empty component name");
		return 0;
	}

	pRes->resolveParentOffset = value;

	char text[32];

	snprintf(text, sizeof(text), "%s,%d", pToken, value);
	Common::strlcpy(pToken, text, 32);

	return 1;
}

void *ThingHandler::instance(int hInst, int compIndex, int bCreate) {
	Cos *pCos = &_pSputCostume->_cos;

	if (!bCreate) {
		ModelInst *pModelInst = static_cast<ModelInst *>(pCos->instGetInstanceData(hInst, compIndex, nullptr));

		if (!pModelInst) {
			warning("ThingHandler::instance(): no instance data for component %d", compIndex);
			return nullptr;
		}

		pModelInst->refCount--;

		if (!pModelInst->refCount) {
			_pSputCostume->_pRes->removeReference(&pModelInst->modelRef);
			_pSputCostume->_pRd->_thing.destroy(pModelInst->pThing);
			delete pModelInst;
		}

		return nullptr;
	}

	int fourcc = 0;
	void *pId = nullptr;
	int parentIndex = -1;
	CosComponent *pComp = pCos->instComponentInfo(hInst, compIndex, &fourcc, &pId, &parentIndex);

	if (!pComp) {
		warning("ThingHandler::instance(): invalid component %d", compIndex);
		return nullptr;
	}

	int hParentInst = pCos->instGetUserData(hInst);

	if (hParentInst) {
		int numComponents = pCos->instCountComponents(hParentInst);

		for (int i = 0; i < numComponents; i++) {
			int parentFourcc = 0;
			CosComponent *pParentComp = pCos->instComponentInfo(hParentInst, i, &parentFourcc, nullptr, nullptr);

			if (parentFourcc == fourcc && pParentComp && !scumm_stricmp(pParentComp->name, pComp->name)) {
				ModelInst *pShared = static_cast<ModelInst *>(pCos->instGetInstanceData(hParentInst, i, nullptr));

				pShared->refCount++;

				return pShared;
			}
		}
	}

	ThingCompRes *pRes = static_cast<ThingCompRes *>(pId);

	if (!pRes->ref.valid) {
		warning("ThingHandler::instance(): Bad model Reference");
		return nullptr;
	}

	RdModel3 *pModel3 = static_cast<RdModel3 *>(pRes->ref.handle);
	ModelInst *pModelInst = new ModelInst();

	_pSputCostume->_pRes->setReference(&pModelInst->modelRef, pModel3);

	RdThing *pThing = _pSputCostume->_pRd->_thing.create(0);

	_pSputCostume->_pRd->_thing.setModel3(pThing, pModel3);
	pThing->pPuppet = _pSputCostume->_pRd->_puppet.create(pThing);
	pModelInst->pThing = pThing;
	pModelInst->resolveParentOffset = pRes->resolveParentOffset;

	if (parentIndex < 0) {
		pModelInst->pParentModel = nullptr;
		pModelInst->parentMeshIdx = -1;
		pModelInst->bVisible = 1;
	} else {
		MeshInstData *pMeshInst = static_cast<MeshInstData *>(pCos->instGetInstanceData(hInst, parentIndex, nullptr));

		pModelInst->pParentModel = pMeshInst->pModelInst;
		pModelInst->parentMeshIdx = pMeshInst->meshIndex;
		pModelInst->bVisible = 0;
	}

	pModelInst->refCount++;

	return pModelInst;
}

void MeshHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	MeshInstData *pInst = static_cast<MeshInstData *>(pInstData);

	if (keyValue < 0 || keyValue >= 2) {
		warning("MeshHandler::change(): invalid key %d", keyValue);
		return;
	}

	RdThing *pThing = pInst->pModelInst->pThing;
	int bVisible = thingMeshKeyValues[keyValue] != 0;

	pInst->bVisible = bVisible;
	pThing->abMeshVisible[pInst->meshIndex] = bVisible;
}

int MeshHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	if (!pParentId || (parentFourcc != kSputCostumeTagThing && parentFourcc != kSputCostumeTagMainThing))
		return 0;

	ThingCompRes *pParentRes = static_cast<ThingCompRes *>(pParentId);

	if (!pParentRes->ref.valid) {
		warning("MeshHandler::request(): Bad model Reference");
		return 0;
	}

	RdModel3 *pModel3 = static_cast<RdModel3 *>(pParentRes->ref.handle);

	// TODO: grimPlat_TreeSelectDialog branch (args == NULL)
	const char *args = pComp->name;
	const char *pDigit = args;

	while (*pDigit && !Common::isDigit(*pDigit))
		pDigit++;

	if (!Common::isDigit(*pDigit))
		return 0;

	int meshIndex = atoi(pDigit);

	if (meshIndex < 0 || meshIndex >= (int)pModel3->numHNodes) {
		warning("MeshHandler::request(): mesh %d is out of range", meshIndex);
		return 0;
	}

	char name[45];

	Common::strlcpy(name, args, sizeof(name));

	uint id = ((uint)(uintptr)pParentId & 0xffff) | ((uint)meshIndex << 16);

	return _pSputCostume->_cos.registerComponent(hCostume, name, (void *)(uintptr)id, fourcc, pParentId, parentFourcc, &thingMeshKeyDesc) != 0;
}

void MeshHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	_pSputCostume->stub();
}

void MeshHandler::stop(void *pInstData) {
	MeshInstData *pInst = static_cast<MeshInstData *>(pInstData);

	pInst->bVisible = 1;
}

int MeshHandler::adjust(void **ppCompRes, char *args, int argsSize) {
	return strchr(args, ')') == nullptr;
}

void *MeshHandler::instance(int hInst, int compIndex, int bCreate) {
	Cos *pCos = &_pSputCostume->_cos;

	if (!bCreate) {
		MeshInstData *pMeshInst = static_cast<MeshInstData *>(pCos->instGetInstanceData(hInst, compIndex, nullptr));

		delete pMeshInst;

		return nullptr;
	}

	int fourcc = 0;
	void *pId = nullptr;
	int parentIndex = -1;

	pCos->instComponentInfo(hInst, compIndex, &fourcc, &pId, &parentIndex);

	MeshInstData *pMeshInst = new MeshInstData();

	pMeshInst->meshIndex = (int)(intptr)pId >> 16;
	pMeshInst->pModelInst = static_cast<ModelInst *>(pCos->instGetInstanceData(hInst, parentIndex, nullptr));
	pMeshInst->bVisible = 1;

	return pMeshInst;
}

void MaterialHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	MaterialInst *pInst = static_cast<MaterialInst *>(pInstData);

	pInst->key = keyValue;
}

int MaterialHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	if (parentFourcc == kSputCostumeTagSprite) {
		warning("MaterialHandler::request(): a material cannot be attached to a sprite");
		return 0;
	}

	if (parentFourcc != kSputCostumeTagThing && parentFourcc != kSputCostumeTagMainThing)
		return 0;

	if (!pParentId) {
		warning("MaterialHandler::request(): no parent component");
		return 0;
	}

	ThingCompRes *pParentRes = static_cast<ThingCompRes *>(pParentId);

	if (!pParentRes->ref.valid) {
		warning("MaterialHandler::request(): Bad model Reference");
		return 0;
	}

	RdModel3 *pModel3 = static_cast<RdModel3 *>(pParentRes->ref.handle);

	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = pComp->name;
	uint materialIndex = 0;

	if (pModel3->numMaterials) {
		do {
			RdMaterial *pMaterial = pModel3->apMaterials[materialIndex];

			if (pMaterial && !scumm_stricmp(args, pMaterial->aName))
				break;

			materialIndex++;
		} while (materialIndex < pModel3->numMaterials);
	}

	if (materialIndex == pModel3->numMaterials)
		return 0;

	RdMaterial *pMaterial = pModel3->apMaterials[materialIndex];

	if (_pSputCostume->_aLECBitmaps)
		delete[] _pSputCostume->_aLECBitmaps;

	_pSputCostume->_aLECBitmaps = new CosStateBitmapDesc[pMaterial->numCels]();

	for (uint i = 0; i < pMaterial->numCels; i++) {
		RdMatCel *pCel = pMaterial->apCels[i];

		if (!pCel || !pCel->pTexture || !pCel->pTexture->apMips[0]) {
			warning("MaterialHandler::request(): cel %u of %s has no texture", i, pMaterial->aName);
			continue;
		}

		StdVBuffer *pFrame = pCel->pTexture->apMips[0];

		_pSputCostume->_aLECBitmaps[i].pPixels = pFrame->pPixels;
		_pSputCostume->_aLECBitmaps[i].field_4 = pFrame->tex.width;
		_pSputCostume->_aLECBitmaps[i].field_8 = pFrame->tex.height;
		_pSputCostume->_aLECBitmaps[i].field_C = 1;
	}

	CosComponentNames keyDesc = { (int)pMaterial->numCels, nullptr, nullptr, _pSputCostume->_aLECBitmaps, nullptr };

	return _pSputCostume->_cos.registerComponent(hCostume, args, (void *)(uintptr)materialIndex, fourcc, pParentId, parentFourcc, &keyDesc) != 0;
}

void MaterialHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	_pSputCostume->stub();
}

void *MaterialHandler::instance(int hInst, int compIndex, int bCreate) {
	Cos *pCos = &_pSputCostume->_cos;

	if (!bCreate) {
		MaterialInst *pInst = static_cast<MaterialInst *>(pCos->instGetInstanceData(hInst, compIndex, nullptr));

		delete pInst;

		return nullptr;
	}

	MaterialInst *pInst = new MaterialInst();
	int fourcc = 0;
	int parentIndex = -1;

	pCos->instComponentInfo(hInst, compIndex, &fourcc, &pInst->pCompRes, &parentIndex);
	pInst->pModelInst = static_cast<ModelInst *>(pCos->instGetInstanceData(hInst, parentIndex, nullptr));
	pInst->key = 0;

	return pInst;
}

void MaterialHandler::saveLoad(int hInst, int compIndex, int bSave, void *pInstData, Common::WriteStream *pWrite, Common::ReadStream *pRead) {
	MaterialInst *pInst = static_cast<MaterialInst *>(pInstData);

	if (bSave)
		pWrite->writeSint32LE(pInst->key);
	else
		pInst->key = pRead->readSint32LE();
}

void ColormapHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	_pSputCostume->nullSub1();
}

int ColormapHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	if (parentFourcc != kSputCostumeTagThing && parentFourcc != kSputCostumeTagMainThing && parentFourcc != kSputCostumeTagSprite)
		return 0;

	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = pComp->name;
	void *hColormap = _pSputCostume->_pSputResource->alloc(kSputTagCmap, args);

	if (!hColormap)
		return 0;

	ResRef *pRes = new ResRef();

	_pSputCostume->_pRes->setReference(pRes, hColormap);

	return _pSputCostume->_cos.registerComponent(hCostume, args, pRes, fourcc, pParentId, parentFourcc, &colormapKeyDesc) != 0;
}

void ColormapHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	ResRef *pRes = static_cast<ResRef *>(pCompRes);

	_pSputCostume->_pRes->removeReference(pRes);
	delete pRes;
}

void *ColormapHandler::instance(int hInst, int compIndex, int bCreate) {
	Cos *pCos = &_pSputCostume->_cos;

	if (!bCreate) {
		ColormapInst *pInst = static_cast<ColormapInst *>(pCos->instGetInstanceData(hInst, compIndex, nullptr));

		_pSputCostume->_pRes->removeReference(&pInst->ref);
		delete pInst;

		return nullptr;
	}

	ColormapInst *pInst = new ColormapInst();
	int fourcc = 0;
	void *pId = nullptr;
	int parentIndex = -1;

	pCos->instComponentInfo(hInst, compIndex, &fourcc, &pId, &parentIndex);

	ResRef *pRes = static_cast<ResRef *>(pId);

	if (!pRes->valid)
		warning("ColormapHandler::instance(): Bad Colormap Reference");

	_pSputCostume->_pRes->setReference(&pInst->ref, pRes->handle);
	pInst->pModelInst = static_cast<ModelInst *>(pCos->instGetInstanceData(hInst, parentIndex, nullptr));

	return pInst;
}

void SpriteHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	ModelInst *pInst = static_cast<ModelInst *>(pInstData);

	if (keyValue) {
		pInst->bVisible = 1;
		pInst->pSprite->pMaterial->curCel = keyValue - 1;
	} else {
		pInst->bVisible = 0;
	}
}

int SpriteHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	char name[32];
	float width = 0.0f;
	float height = 0.0f;
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	float offsetZ = 0.0f;

	memset(name, 0, sizeof(name));

	if (!pParentId || parentFourcc != kSputCostumeTagMesh)
		return 0;

	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = pComp->name;

	Common::strlcpy(name, args, 32);
	strtok(name, ",");

	char *pToken = strtok(nullptr, ",");

	if (pToken) {
		width = (float)((double)atoi(pToken) * (double)0.01f);

		const char *aTokens[4];

		for (int i = 0; i < 4; i++) {
			aTokens[i] = strtok(nullptr, ",");

			if (!aTokens[i])
				warning("SpriteHandler::request(): missing sprite parameter in %s", args);
		}

		if (aTokens[0])
			height = (float)((double)atoi(aTokens[0]) * (double)0.01f);

		if (aTokens[1])
			offsetX = (float)((double)atoi(aTokens[1]) * (double)0.01f);

		if (aTokens[2])
			offsetY = (float)((double)atoi(aTokens[2]) * (double)0.01f);

		if (aTokens[3])
			offsetZ = (float)((double)atoi(aTokens[3]) * (double)0.01f);
	}

	SpriteCompRes *pRes = new SpriteCompRes();
	RdVector3 offset;

	offset[0] = offsetX;
	offset[1] = offsetY;
	offset[2] = offsetZ;

	pRes->pSprite = _pSputCostume->_pRd->_sprite.create(0, name, name, width, height, 5, 0, 1, 0.0f, &offset);
	Common::strlcpy(pRes->name, name, 32);

	if (!pRes->pSprite || !pRes->pSprite->pMaterial) {
		warning("SpriteHandler::request(): sprite %s has no material", name);
		return 0;
	}

	CosComponentNames keyDesc = { (int)pRes->pSprite->pMaterial->numCels + 1, nullptr, nullptr, nullptr, nullptr };

	return _pSputCostume->_cos.registerComponent(hCostume, args, pRes, fourcc, pParentId, kSputCostumeTagMesh, &keyDesc) != 0;
}

void SpriteHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	SpriteCompRes *pRes = static_cast<SpriteCompRes *>(pCompRes);

	_pSputCostume->_pRd->_material.destroy(pRes->pSprite->pMaterial);
	_pSputCostume->_pRd->_sprite.destroy(pRes->pSprite);
	delete pRes;
}

int SpriteHandler::adjust(void **ppCompRes, char *args, int argsSize) {
	SpriteCompRes *pRes = static_cast<SpriteCompRes *>(*ppCompRes);
	RdSprite *pSprite = pRes->pSprite;
	int aValues[5];

	aValues[0] = (int)(int64)((double)pSprite->width * 100.0 - (double)-0.2f);
	aValues[1] = (int)(int64)((double)pSprite->height * 100.0 - (double)-0.2f);
	aValues[2] = (int)(int64)((double)pSprite->offset[0] * 100.0 - (double)-0.2f);
	aValues[3] = (int)(int64)((double)pSprite->offset[1] * 100.0 - (double)-0.2f);
	aValues[4] = (int)(int64)((double)pSprite->offset[2] * 100.0 - (double)-0.2f);

	int accepted = 0; // TODO: grimPlat_ControlPanel("Width", "Height", "X", "Y", "Z")

	if (!accepted)
		return 0;

	pSprite->width = (float)((double)aValues[0] * (double)0.01f);
	pSprite->height = (float)((double)aValues[1] * (double)0.01f);
	pSprite->halfWidth = (float)((double)pSprite->width * (double)0.5f);
	pSprite->halfHeight = (float)((double)pSprite->height * (double)0.5f);
	pSprite->offset[0] = (float)((double)aValues[2] * (double)0.01f);
	pSprite->offset[1] = (float)((double)aValues[3] * (double)0.01f);
	pSprite->offset[2] = (float)((double)aValues[4] * (double)0.01f);

	char *pToken = strtok(args, ",");

	if (!pToken) {
		warning("SpriteHandler::adjust(): empty component name");
		return 0;
	}

	char text[32];

	snprintf(text, sizeof(text), "%s,%d,%d,%d,%d,%d", pToken, aValues[0], aValues[1], aValues[2], aValues[3], aValues[4]);
	Common::strlcpy(pToken, text, 32);

	return 1;
}

void *SpriteHandler::instance(int hInst, int compIndex, int bCreate) {
	Cos *pCos = &_pSputCostume->_cos;

	if (!bCreate) {
		ModelInst *pModelInst = static_cast<ModelInst *>(pCos->instGetInstanceData(hInst, compIndex, nullptr));

		pModelInst->refCount--;

		if (!pModelInst->refCount) {
			_pSputCostume->_pRd->_thing.destroy(pModelInst->pThing);
			delete pModelInst;
		}

		return nullptr;
	}

	int fourcc = 0;
	void *pId = nullptr;
	int parentIndex = -1;

	pCos->instComponentInfo(hInst, compIndex, &fourcc, &pId, &parentIndex);

	SpriteCompRes *pRes = static_cast<SpriteCompRes *>(pId);
	ModelInst *pModelInst = new ModelInst();

	pModelInst->pSprite = pRes->pSprite;

	RdThing *pThing = _pSputCostume->_pRd->_thing.create(0);

	pModelInst->pThing = pThing;
	pThing->curCel = 0;
	_pSputCostume->_pRd->_thing.setSprite3(pThing, pRes->pSprite);

	if (parentIndex < 0) {
		pModelInst->pParentModel = nullptr;
		pModelInst->refCount++;
		pModelInst->parentMeshIdx = -1;
		pModelInst->bVisible = 1;
	} else {
		MeshInstData *pMeshInst = static_cast<MeshInstData *>(pCos->instGetInstanceData(hInst, parentIndex, nullptr));

		pModelInst->pParentModel = pMeshInst->pModelInst;
		pModelInst->parentMeshIdx = pMeshInst->meshIndex;
		pModelInst->refCount++;
		pModelInst->bVisible = 0;
	}

	return pModelInst;
}

} // End of namespace Grim
