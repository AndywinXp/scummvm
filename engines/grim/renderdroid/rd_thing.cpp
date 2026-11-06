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

#include "engines/grim/renderdroid/rd.h"

namespace Grim {

RdThing *RenderDroid::ThingModule::create(void *userArg) {
	RdThing *pThing = new RdThing;

	if (!newEntry(pThing, userArg))
		return nullptr;

	return pThing;
}

int RenderDroid::ThingModule::newEntry(RdThing *pThing, void *userArg) {
	pThing->curGeoSet = -1;
	pThing->curCel = -1;
	pThing->data.pModel3 = nullptr;
	pThing->textureMode = 2;
	pThing->curTextureMode = 2;
	pThing->type = 0;
	pThing->pPuppet = nullptr;
	pThing->bJointsLocked = 0;
	pThing->jointsFrame = 0;
	pThing->geometryMode = 4;
	pThing->lightingMode = 3;
	pThing->curGeometryMode = 4;
	pThing->curLightingMode = 3;
	pThing->frustrumCull = 1;
	pThing->aJointMatrices = nullptr;
	pThing->abJointAmputate = nullptr;
	pThing->userArg = userArg;
	pThing->abMeshVisible = nullptr;

	return 1;
}

void RenderDroid::ThingModule::destroy(RdThing *&pThing) {
	if (pThing) {
		freeEntry(pThing);
		delete pThing;
		pThing = nullptr;
	}
}

void RenderDroid::ThingModule::freeEntry(RdThing *pThing) {
	if (pThing->type == 1) {
		if (pThing->aJointMatrices) {
			delete[] pThing->aJointMatrices;
			pThing->aJointMatrices = nullptr;
		}
		if (pThing->aJointPyr) {
			delete[] pThing->aJointPyr;
			pThing->aJointPyr = nullptr;
		}
		if (pThing->abMeshVisible) {
			delete[] pThing->abMeshVisible;
			pThing->abMeshVisible = nullptr;
		}
		if (pThing->abJointAmputate) {
			delete[] pThing->abJointAmputate;
			pThing->abJointAmputate = nullptr;
		}
		if (pThing->pPuppet) {
			for (int i = 0; i < 6; i++) {
				if (pThing->pPuppet->aTracks[i].status)
					_rd->_puppet.removeTrack(pThing->pPuppet, i);
			}
		}
	}

	if (pThing->pPuppet) {
		delete pThing->pPuppet;
		pThing->pPuppet = nullptr;
	}
}

int RenderDroid::ThingModule::setModel3(RdThing *pThing, RdModel3 *pModel3) {
	pThing->type = 1;
	pThing->data.pModel3 = pModel3;
	pThing->curGeoSet = -1;
	pThing->aJointMatrices = new RdMatrix34[pModel3->numHNodes];
	pThing->aJointPyr = new RdVector3[pModel3->numHNodes];
	memset(pThing->aJointPyr, 0, 12 * pModel3->numHNodes);

	pThing->abJointAmputate = new int[pModel3->numHNodes];
	memset(pThing->abJointAmputate, 0, 4 * pModel3->numHNodes);

	pThing->abMeshVisible = new int[pModel3->numHNodes];
	for (uint i = 0; i < pModel3->numHNodes; i++)
		pThing->abMeshVisible[i] = 1;

	for (uint i = 0; i < pModel3->numHNodes; i++) {
		RdModel3HNode *pHNode = &pModel3->aHNodes[i];
		_rd->build34(pHNode->mat, pHNode->pyr, pHNode->xyz);
	}

	return 1;
}

int RenderDroid::ThingModule::setCamera(RdThing *pThing, RdCamera *pCamera) {
	pThing->type = 2;
	pThing->data.pCamera = pCamera;
	return 1;
}

int RenderDroid::ThingModule::setSprite3(RdThing *pThing, RdSprite *pSprite) {
	pThing->type = 4;
	pThing->data.pSprite = pSprite;
	pThing->curCel = -1;
	return 1;
}

void RenderDroid::ThingModule::accumulateMatrices(RdThing *pThing, RdModel3HNode *pHNode, RdMatrix34 pParent) {
	RdMatrix34 out;

	_rd->buildTranslate34(out, pHNode->pivot);
	_rd->postMultiply34(out, pThing->aJointMatrices[pHNode->num]);

	if (pHNode->pParentNode) {
		RdVector3 negatedPivot;
		negatedPivot[0] = -pHNode->pParentNode->pivot[0];
		negatedPivot[1] = -pHNode->pParentNode->pivot[1];
		negatedPivot[2] = -pHNode->pParentNode->pivot[2];
		_rd->postTranslate34(out, negatedPivot);
	}

	_rd->multiply34(pThing->aJointMatrices[pHNode->num], pParent, out);

	if (pHNode->numChildren) {
		RdModel3HNode *pChild = pHNode->pFirstChildNode;

		for (uint i = 0; i < pHNode->numChildren; i++) {
			if (!pThing->abJointAmputate[pChild->num])
				accumulateMatrices(pThing, pChild, pThing->aJointMatrices[pHNode->num]);

			pChild = pChild->pSiblingNode;
		}
	}
}

int RenderDroid::ThingModule::draw(RdThing *pThing, RdMatrix34 mat) {
	if (!_rd->_curGeometryMode)
		return 0;

	switch (pThing->type) {
	case 1:
		return _rd->_model3.draw(pThing, mat);
	case 4:
		return _rd->_sprite.draw(pThing, mat);
	case 5:
		return _rd->_model3.drawParticle(pThing, mat);
	case 6:
		return _rd->_model3.drawPolyline(pThing, mat);
	case 0:
	case 2:
	case 3:
		return 0;
	default:
		return pThing->type;
	}
}

} // End of namespace Grim
