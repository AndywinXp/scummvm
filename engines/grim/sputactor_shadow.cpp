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

#include "engines/grim/grim.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/sputcostume_components.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_3d_config.h"
#include "engines/grim/walkplane.h"

namespace Grim {

int SputActorModule::actorHasShadow(SputActor *pActor) {
	for (int i = 0; i < 6; i++) {
		if (pActor->pExtra->aShadows[i].bActive)
			return 1;
	}

	return 0;
}

void SputActorModule::activateActorShadow(SputActor *pActor, int shadowIdx, int bActive) {
	if (shadowIdx < 0 || shadowIdx >= 6) {
		warning("SputActorModule::activateActorShadow(): invalid shadow index %d", shadowIdx);
		return;
	}

	pActor->pExtra->aShadows[shadowIdx].bActive = bActive;
}

void SputActorModule::setShadowPlane(SputActor *pActor, const char *planeName) {
	int shadowIdx = pActor->pExtra->activeShadow;

	if (shadowIdx < 0 || shadowIdx >= 6) {
		warning("SputActorModule::setShadowPlane(): invalid active shadow %d", shadowIdx);
		return;
	}

	ActorShadow *pShadow = &pActor->pExtra->aShadows[shadowIdx];
	WalkPlane *pPlane = _pWalkSystem->planeNamed(planeName);

	if (!pPlane)
		return;

	pShadow->normal[0] = pPlane->normal[0];
	pShadow->normal[1] = pPlane->normal[1];
	pShadow->normal[2] = pPlane->normal[2];

	WpVertex *pVertex = pPlane->apVerts[0];
	double zTerm = (double)pVertex->z * pShadow->normal[2];
	double yTerm = (double)pVertex->y * pShadow->normal[1];
	double xTerm = (double)pVertex->x * pShadow->normal[0];

	pShadow->bActive = 1;
	pShadow->side = 0;
	pShadow->planeD = (float)-(xTerm + (yTerm + zTerm));
}

void SputActorModule::setShadowPoint(SputActor *pActor, float x, float y, float z) {
	int shadowIdx = pActor->pExtra->activeShadow;

	if (shadowIdx < 0 || shadowIdx >= 6) {
		warning("SputActorModule::setShadowPoint(): invalid active shadow %d", shadowIdx);
		return;
	}

	ActorShadow *pShadow = &pActor->pExtra->aShadows[shadowIdx];

	pShadow->lightPos[0] = x;
	pShadow->lightPos[1] = y;
	pShadow->lightPos[2] = z;
}

void SputActorModule::setShadowValid(SputActor *pActor, int shadowIdx, int side) {
	if (shadowIdx < 0 || shadowIdx >= 6) {
		warning("SputActorModule::setShadowValid(): invalid shadow index %d", shadowIdx);
		return;
	}

	pActor->pExtra->aShadows[shadowIdx].side = side;
}

void SputActorModule::addShadowClipPlane(SputActor *pActor, int shadowIdx, const char *planeName) {
	WalkPlane *pPlane = _pWalkSystem->planeNamed(planeName);

	if (!pPlane)
		return;

	if (shadowIdx < 0 || shadowIdx >= 6) {
		warning("SputActorModule::addShadowClipPlane(): invalid shadow index %d", shadowIdx);
		return;
	}

	ActorShadow *pShadow = &pActor->pExtra->aShadows[shadowIdx];

	for (int i = 0; i < 12; i++) {
		if (!pShadow->apClipPlane[i]) {
			pShadow->apClipPlane[i] = pPlane;
			return;
		}
	}
}

void SputActorModule::resetShadows(SputActor *pActor) {
	if (pActor && pActor->pExtra)
		memset(pActor->pExtra->aShadows, 0, sizeof(pActor->pExtra->aShadows));
}

void SputActorModule::loadShadowResources(SputActor *pActor) {
	if (!pActor->pExtra)
		return;

	_pGrim->setPerfCounter(11);

	for (int i = 0; i < 6; i++) {
		ActorShadow *pShadow = &pActor->pExtra->aShadows[i];

		if (!pShadow->bActive)
			continue;

		int side = pShadow->side;
		bool bInit = true;

		if (side) {
			double distance = (double)pActor->actorPos[0] * pShadow->normal[0] + (double)pActor->actorPos[2] * pShadow->normal[2];

			distance = distance + (double)pActor->actorPos[1] * pShadow->normal[1];
			distance = distance + pShadow->planeD;

			if (side == -1 && distance > 0.0)
				bInit = false;
			else if (side == 1 && distance < 0.0)
				bInit = false;
		}

		if (bInit)
			initShadow(pActor->pCostumeStack, pShadow);
	}

	_pGrim->queryPerfCounterDiff(11);
}

void SputActorModule::initShadow(CostumeNode *pNode, ActorShadow *pShadow) {
	if (!pNode)
		return;

	float aMin[2] = { 640.0f, 480.0f };
	float aMax[2] = { 0.0f, 0.0f };
	bool bDemo = (_pGrim->getGameFlags() & ADGF_DEMO) != 0;

	for (ModelInst *pModelInst = pNode->pHeadList; pModelInst; pModelInst = pModelInst->pNext) {
		RdThing *pThing = pModelInst->pThing;

		if (!pModelInst->bVisible || pModelInst->pSprite)
			continue;

		RdModel3 *pModel3 = pThing->data.pModel3;

		for (uint node = 0; node < pModel3->numHNodes; node++) {
			int meshNodeIndex = pModel3->aHNodes[node].meshNodeIndex;

			if (meshNodeIndex == -1)
				continue;

			RdModel3Mesh *pMesh = &pModel3->aGeo[0].aMeshes[meshNodeIndex];

			if (!pMesh->castShadow || !pThing->abMeshVisible[meshNodeIndex])
				continue;

			RdVector3 aQuads[16];
			int vertex = 0;

			while (vertex < 16) {
				// The shadows of the demo have three quads, the second one is missing
				if (bDemo && vertex == 4) {
					vertex = 8;
					continue;
				}

				_pRd->transformPoint34(aQuads[vertex], pMesh->aShadowVerts[vertex], pThing->aJointMatrices[node]);

				// The demo goes on when the projection fails
				if (!_pSputRender->buildShadowBitmap(aQuads[vertex], pShadow->lightPos, aQuads[vertex], pShadow->normal[0], pShadow->normal[1], pShadow->normal[2], pShadow->planeD) && !bDemo)
					break;

				vertex++;
			}

			if (vertex < 16)
				continue;

			if (!_pShadowMaterial) {
				if (bDemo) {
					_pShadowMaterial = &_demoShadowMaterial;
				} else {
					char materialName[] = "shadow.mat";

					_pShadowMaterial = _pRd->_material.load(materialName, 0, 0);
				}
			}

			RdColormap *pColormap = _pSputCostume->getShadowColormap();

			if (!pColormap && bDemo) {
				_pSputCostume->setShadowColor(0, 0, 0);
				pColormap = _pSputCostume->getShadowColormap();
			} else if (!pColormap) {
				char colormapName[] = "shadow.cmp";

				pColormap = _pRd->_colormap.load(colormapName);
				_pSputCostume->setShadowColormap(pColormap);
			}

			if (_pShadowMaterial) {
				_pRd->_colormap.setCurrent(pColormap);

#if defined(GRIM_STD3D)
				if (_pSputRender->is3DHardware()) {
					if (pColormap)
						_pStd3D->setCurrentPalette(pColormap->aColors, 0);
					else
						warning("SputActorModule::initShadow(): the shadow colormap could not be loaded");
				}
#endif

				drawShadowPolygon(aQuads[0], 4, aMin, aMax, pShadow);

				if (!bDemo)
					drawShadowPolygon(aQuads[4], 4, aMin, aMax, pShadow);

				drawShadowPolygon(aQuads[8], 4, aMin, aMax, pShadow);
				drawShadowPolygon(aQuads[12], 4, aMin, aMax, pShadow);
			}
		}
	}

	if (!(aMax[0] < aMin[0])) {
		StdRect rect;

		rect.x = (int)aMin[0];
		rect.y = (int)aMin[1];
		rect.width = (int)(aMax[0] - aMin[0]);
		rect.height = (int)(aMax[1] - aMin[1]);
		_pSputRender->markDirtyRect(&rect, 1);
	}
}

void SputActorModule::drawShadowPolygon(const float *pPoly, int numPoly, float *pMin, float *pMax, ActorShadow *pShadow) {
	RdVector3 aClipped[12];

	for (int i = 0; i < 12; i++) {
		WalkPlane *pPlane = pShadow->apClipPlane[i];

		if (!pPlane)
			break;

		int numClipped = pPlane->clipPolygon(pPoly, numPoly, aClipped[0]);

		if (numClipped > 2) {
			RdProcFace *pFace = _pRd->_primit3.drawPolygon(aClipped, numClipped, _pShadowMaterial);

			if (pFace) {
				double xMin = (double)(uint)pFace->xMin;

				if (xMin > pMin[0])
					xMin = pMin[0];

				pMin[0] = (float)xMin;

				double yMin = (double)(uint)pFace->yMin;

				if (pMin[1] < yMin)
					yMin = pMin[1];

				pMin[1] = (float)yMin;

				double xMax = (double)(uint)pFace->xMax;

				if (xMax < pMax[0])
					xMax = pMax[0];

				pMax[0] = (float)xMax;

				double yMax = (double)(uint)pFace->yMax;

				if (pMax[1] > yMax)
					yMax = pMax[1];

				pMax[1] = (float)yMax;
			}
		}
	}
}

} // End of namespace Grim
