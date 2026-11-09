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

#include "engines/grim/chore/cos.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/sputcostume_components.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_3d_config.h"
#include "engines/grim/std/std_display.h"

namespace Grim {

float (*SputActorModule::getOrientMatrix(SputActor *pActor))[3] {
	if (!pActor->matrixValid) {
		_pRd->build34(pActor->orientMatrix, pActor->actorRot, pActor->actorPos);
		pActor->matrixValid = 1;
	}

	return pActor->orientMatrix;
}

void SputActorModule::drawToClean(SputActor *pActor) {
	Room *pRoom = _pSputRoom->getCurrentRoom();
	RoomSetup *pSetup = &pRoom->pSetups[pRoom->curSetup];
	RdCanvas *pCanvas = pSetup->camera.prdCamera->pCanvas;

	if (!pCanvas)
		return;

	StdVBuffer *pVBuffer = pCanvas->pVBuffer;
	StdVBuffer *pZBuffer = pCanvas->pZBuffer;

	pCanvas->pVBuffer = _pSputRender->cleanArtBuffer();
	pCanvas->pZBuffer = _pSputRender->cleanZBuffer();

	RdColormap *pColormap;

	if (pActor->pColorMap->valid) {
		pColormap = static_cast<RdColormap *>(pActor->pColorMap->handle);
	} else {
		ResRef *pColormapRef = &pRoom->apColormaps[pRoom->field_88];

		if (!pColormapRef->valid)
			error("SputActorModule::drawToClean(): Bad Colormap Reference");

		pColormap = static_cast<RdColormap *>(pColormapRef->handle);
	}

	_pRd->_colormap.setCurrent(pColormap);

	SputRenderPaletteFunc pPaletteFunc = _pSputRender->getPaletteFunc();

	if (pPaletteFunc)
		pPaletteFunc(pColormap->aColors, 0);

	if (pActor->scale != 1.0f) {
		RdVector3 scale;

		scale[0] = pActor->scale;
		scale[1] = pActor->scale;
		scale[2] = pActor->scale;
		_pRd->preScale34(getOrientMatrix(pActor), scale);
	}

	drawCostume(pActor->pCostumeStack, getOrientMatrix(pActor));
	pActor->matrixValid = 0;
	_pRd->_cache.flush();
	pCanvas->pVBuffer = pVBuffer;
	pCanvas->pZBuffer = pZBuffer;
}

void SputActorModule::drawCostume(CostumeNode *pNode, RdMatrix34 pMatrix) {
	drawCostumeImpl(pNode, pMatrix);
}

void SputActorModule::drawCostumeImpl(CostumeNode *pNode, RdMatrix34 pMatrix) {
	int b3DAcceleration = _pSputRender->is3DHardware();
	bool bDemo = _pRd->isDemo();
	RdThing *pHeadThing = pNode->pHeadThing;

	for (CostumeNode *pCurrent = pNode; pCurrent; pCurrent = pCurrent->pNext) {
		if (pCurrent->pHeadThing != pHeadThing)
			break;

		syncCostumeComponents(pCurrent->hCostumeInstance);
	}

	SputActor *pActor = &_aActors[_pCos->instGetUserData(pNode->hCostumeInstance)];

	if (pActor->pExtra && pActor->pExtra->clipPlaneActive) {
		_pRd->_model3.activateClipPlane(1);
		_pRd->_model3.setClipPlane(pActor->pExtra->clipPlaneNorm, pActor->pExtra->clipPlanePt);
	} else {
		_pRd->_model3.activateClipPlane(0);
	}

	applyPendingScale(pActor, pNode->pHeadList);

	for (ModelInst *pModelInst = pNode->pHeadList; pModelInst; pModelInst = pModelInst->pNext) {
		if (!pModelInst->bVisible)
			continue;

		int *pViewport = _pRd->_cache.getViewportRect();

		pViewport[0] = 640;
		pViewport[1] = 480;
		pViewport[2] = 0;
		pViewport[3] = 0;

		if (pModelInst->pParentModel) {
			ModelInst *pColormapModel = pModelInst;

			// The demo only uses the colormap of the model itself
			if (!bDemo) {
				do {
					if (pColormapModel->pColormap)
						break;

					pColormapModel = pColormapModel->pParentModel;
				} while (pColormapModel);
			}

			if (pColormapModel && pColormapModel->pColormap) {
				_pRd->_colormap.setCurrent(pColormapModel->pColormap);

#if defined(GRIM_STD3D)
				if (b3DAcceleration)
					_pStd3D->setCurrentPalette(pColormapModel->pColormap->aColors, 0);
#endif
			}

			if (!bDemo) {
				buildJointMatricesRec(pModelInst->pParentModel, pMatrix);
			} else if (pModelInst->pParentModel->pThing->jointsFrame != _pRd->getFrameTrue()) {
				// The demo builds only the parent, and only if it is not up to date
				_pRd->_puppet.buildJointMatrices(pModelInst->pParentModel->pThing, pMatrix);
			}

			RdThing *pParentThing = pModelInst->pParentModel->pThing;
			RdModel3HNode *pParentNode = &pParentThing->data.pModel3->aHNodes[pModelInst->parentMeshIdx];
			RdModel3HNode *pChildNode = pModelInst->pThing->data.pModel3->aHNodes;
			float aInsertOffset[3] = { 0.0f, 0.0f, 0.0f };

			if (!b3DAcceleration)
				_pStdDisplay->vBufferLock(_pSputRender->drawBuffer());

			if (pModelInst->resolveParentOffset && pModelInst->pThing->type != 4) {
				float *pInsertOffset = pModelInst->pThing->data.pModel3->insertOffset;

				aInsertOffset[0] = pInsertOffset[0];
				aInsertOffset[1] = pInsertOffset[1];
				aInsertOffset[2] = pInsertOffset[2];
				pInsertOffset[0] = 0.0f;
				pInsertOffset[1] = 0.0f;
				pInsertOffset[2] = 0.0f;
				pChildNode->pivot[0] = pChildNode->pivot[0] - pParentNode->pivot[0];
				pChildNode->pivot[1] = pChildNode->pivot[1] - pParentNode->pivot[1];
				pChildNode->pivot[2] = pChildNode->pivot[2] - pParentNode->pivot[2];
			}

			if (!bDemo)
				pActor->onScreen = 0;

			if (pActor->pExtra && pActor->pExtra->subClipNode == pModelInst->parentMeshIdx) {
				_pRd->_model3.activateClipPlane(1);
				_pRd->_model3.setClipPlane(pActor->orientMatrix[1], pActor->actorPos);
			}

			if (_pRd->_thing.draw(pModelInst->pThing, pParentThing->aJointMatrices[pModelInst->parentMeshIdx]))
				pActor->onScreen = 1;

			if (pActor->pExtra && pActor->pExtra->subClipNode == pModelInst->parentMeshIdx)
				_pRd->_model3.activateClipPlane(0);

			if (pModelInst->resolveParentOffset && pModelInst->pThing->type != 4) {
				float *pInsertOffset = pModelInst->pThing->data.pModel3->insertOffset;

				pChildNode->pivot[0] = pParentNode->pivot[0] + pChildNode->pivot[0];
				pChildNode->pivot[1] = pParentNode->pivot[1] + pChildNode->pivot[1];
				pChildNode->pivot[2] = pParentNode->pivot[2] + pChildNode->pivot[2];
				pInsertOffset[0] = aInsertOffset[0];
				pInsertOffset[1] = aInsertOffset[1];
				pInsertOffset[2] = aInsertOffset[2];
			}

			if (!b3DAcceleration)
				_pStdDisplay->vBufferUnlock(_pSputRender->drawBuffer());
		} else {
			if (pModelInst->pColormap) {
				_pRd->_colormap.setCurrent(pModelInst->pColormap);

#if defined(GRIM_STD3D)
				if (b3DAcceleration)
					_pStd3D->setCurrentPalette(pModelInst->pColormap->aColors, 0);
#endif
			}

			if (!b3DAcceleration)
				_pStdDisplay->vBufferLock(_pSputRender->drawBuffer());

			int bDrawn = _pRd->_thing.draw(pModelInst->pThing, pMatrix) != 0;

			// The demo only sets the flag, the sorting clears it
			if (bDrawn || !bDemo)
				pActor->onScreen = bDrawn;

			if (!b3DAcceleration)
				_pStdDisplay->vBufferUnlock(_pSputRender->drawBuffer());
		}

		if (pActor->pColorMap->valid) {
			RdColormap *pActorColormap = static_cast<RdColormap *>(pActor->pColorMap->handle);

			_pRd->_colormap.setCurrent(pActorColormap);

#if defined(GRIM_STD3D)
			if (b3DAcceleration)
				_pStd3D->setCurrentPalette(pActorColormap->aColors, 0);
#endif
		}

		StdRect rect;

		rect.x = pViewport[0];
		rect.y = pViewport[1];
		rect.width = pViewport[2] - pViewport[0];
		rect.height = pViewport[3] - pViewport[1];
		_pSputRender->markDirtyRect(&rect, 1);

		if (pModelInst->pThing == pNode->pHeadThing)
			pActor->drawnRect = rect;
	}

	loadShadowResources(pActor);

	for (CostumeNode *pCurrent = pNode; pCurrent; pCurrent = pCurrent->pNext) {
		if (pCurrent->pHeadThing != pHeadThing)
			break;

		syncCostumeComponents(pCurrent->hCostumeInstance);
	}

	applyPendingScale(pActor, pNode->pHeadList);
}

void SputActorModule::applyPendingScale(SputActor *pActor, ModelInst *pList) {
	for (ModelInst *pModelInst = pList; pModelInst; pModelInst = pModelInst->pNext) {
		if (pActor->scale == 1.0f)
			continue;

		RdThing *pThing = pModelInst->pThing;
		RdModel3 *pModel3 = pThing->data.pModel3;

		pModel3->radius = pModel3->radius * pActor->scale;

		if (pThing->type == 1) {
			for (uint i = 0; i < pModel3->aGeo[pModel3->curGeoSet].numMeshes; i++) {
				RdModel3Mesh *pMesh = &pModel3->aGeo[pModel3->curGeoSet].aMeshes[i];

				pMesh->radius = pActor->scale * pMesh->radius;
			}
		}

		double difference = 1.0 - 1.0 / (double)pActor->scale;

		pActor->scale = (float)(1.0 / (double)pActor->scale);

		double absDifference = difference;

		if (difference < 0.0)
			absDifference = -difference;

		if (absDifference <= 0.0000099999997)
			difference = 0.0;

		if (difference == 0.0)
			pActor->scale = 1.0f;
	}
}

void SputActorModule::syncCostumeComponents(int hInst) {
	int numComponents = _pCos->instCountComponents(hInst);

	for (int i = 0; i < numComponents; i++) {
		int fourcc = 0;
		void *pInstData = _pCos->instGetInstanceData(hInst, i, &fourcc);

		if (fourcc == kSputCostumeTagMaterial) {
			MaterialInst *pInst = static_cast<MaterialInst *>(pInstData);
			RdModel3 *pModel3 = pInst->pModelInst->pThing->data.pModel3;
			RdMaterial *pMaterial = pModel3->apMaterials[(intptr)pInst->pCompRes];
			int curCel = pMaterial->curCel;

			pMaterial->curCel = pInst->key;
			pInst->key = curCel;
		} else if (fourcc == kSputCostumeTagColormap) {
			ColormapInst *pInst = static_cast<ColormapInst *>(pInstData);

			if (!pInst->ref.valid)
				error("SputActorModule::syncCostumeComponents(): Bad Colormap Reference");

			pInst->pModelInst->pColormap = static_cast<RdColormap *>(pInst->ref.handle);
		}
	}
}

void SputActorModule::buildJointMatricesRec(ModelInst *pInst, RdMatrix34 pMatrix) {
	ModelInst *pParentModel = pInst->pParentModel;
	RdThing *pThing = pInst->pThing;

	if (pParentModel) {
		buildJointMatricesRec(pParentModel, pMatrix);
		_pRd->_puppet.buildJointMatrices(pThing, pParentModel->pThing->aJointMatrices[pInst->parentMeshIdx]);
	} else {
		_pRd->_puppet.buildJointMatrices(pThing, pMatrix);
	}
}

void SputActorModule::enumCostumeComponents(int hInst) {
	_pSputRoom->getCurrentRoom();

	int numComponents = _pCos->instCountComponents(hInst);

	for (int i = 0; i < numComponents; i++) {
		int fourcc = 0;

		_pCos->instGetInstanceData(hInst, i, &fourcc);
	}
}

} // End of namespace Grim
