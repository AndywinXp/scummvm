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
#include "common/util.h"

#include "engines/grim/puck.h"
#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/walkplane.h"

namespace Grim {

void SputRender::drawActorDebug(SputActor *pActor) {
	Extra *pExtra = pActor->pExtra;

	if (!pExtra)
		return;

	if (pExtra->puck)
		pExtra->puck->draw(_aColors[16]);

	if (pActor->mode & 1) {
		if (pExtra->numPathPoints > 30) {
			warning("SputRender::drawActorDebug(): too many path points");
		} else {
			for (int i = 0; i < pExtra->numPathPoints - 1; i++)
				_pRd->_primit3.drawLine(pExtra->aPath[i], pExtra->aPath[i + 1], nullptr, nullptr, nullptr, nullptr, _aColors[10], 0xFFFFFFFF);
		}
	}

	if (pExtra->field_3F8) {
		if (!pActor->pCostumeStack || !pActor->pCostumeStack->pHeadThing || !pActor->pCostumeStack->pHeadThing->data.pModel3) {
			warning("SputRender::drawActorDebug(): the actor has no model");
			return;
		}

		RdModel3 *pModel3 = pActor->pCostumeStack->pHeadThing->data.pModel3;
		float (*pOrient)[3] = _pSputActor->getOrientMatrix(pActor);
		RdVector3 out;

		_pRd->transformPoint34(out, pModel3->geomCenter, pOrient);

		float radius = (float)((double)pModel3->radius * (double)pExtra->collisionScale);

		_pRd->_primit3.drawPoint(out, radius, 10.0f, 0xFFFF, 0xFFFFFFFF);
	}
}

void SputRender::drawCameraDebug(Room *pRoom) {
	if (!pRoom)
		return;

	uint numLights = pRoom->numLights;

	if (numLights > 64) {
		warning("SputRender::drawCameraDebug(): too many lights");
		numLights = 64;
	}

	for (uint i = 0; i < numLights; i++) {
		RoomLight *pRoomLight = &pRoom->aLights[i];

		if (pRoomLight->light.type >= 3) {
			RdVector3 target;
			RdMatrix34 mat;

			target[0] = (float)((double)pRoomLight->light.direction[0] + (double)pRoomLight->position[0]);
			target[1] = (float)((double)pRoomLight->light.direction[1] + (double)pRoomLight->position[1]);
			target[2] = (float)((double)pRoomLight->light.direction[2] + (double)pRoomLight->position[2]);
			_pRd->lookAt(mat, pRoomLight->position, target, 0.0f);

			float size = (float)((double)pRoomLight->light.intensity * (double)0.1f);
			float width = (float)((1.0 - (double)pRoomLight->light.cosAngleY) * (double)size);
			RdVector3 aPoints[4];

			aPoints[0][0] = width;
			aPoints[0][1] = size;
			aPoints[0][2] = width;
			aPoints[1][0] = width;
			aPoints[1][1] = size;
			aPoints[1][2] = -width;
			aPoints[2][0] = -width;
			aPoints[2][1] = size;
			aPoints[2][2] = -width;
			aPoints[3][0] = -width;
			aPoints[3][1] = size;
			aPoints[3][2] = width;

			for (int j = 0; j < 4; j++) {
				_pRd->transformPoint34Acc(aPoints[j], mat);
				_pRd->_primit3.drawLine(pRoomLight->position, aPoints[j], nullptr, nullptr, nullptr, nullptr, _aColors[7], 0xFFFFFFFF);
			}

			_pRd->_primit3.drawLine(aPoints[0], aPoints[1], nullptr, nullptr, nullptr, nullptr, _aColors[7], 0xFFFFFFFF);
			_pRd->_primit3.drawLine(aPoints[1], aPoints[2], nullptr, nullptr, nullptr, nullptr, _aColors[7], 0xFFFFFFFF);
			_pRd->_primit3.drawLine(aPoints[2], aPoints[3], nullptr, nullptr, nullptr, nullptr, _aColors[7], 0xFFFFFFFF);
			_pRd->_primit3.drawLine(aPoints[3], aPoints[0], nullptr, nullptr, nullptr, nullptr, _aColors[7], 0xFFFFFFFF);
		} else {
			RdVector3 ground;

			ground[0] = pRoomLight->position[0];
			ground[1] = pRoomLight->position[1];
			ground[2] = 0.0f;
			_pRd->_primit3.drawLine(pRoomLight->position, ground, nullptr, nullptr, nullptr, nullptr, _aColors[5], 0xFFFFFFFF);
			_pRd->_primit3.drawPoint(pRoomLight->position, pRoomLight->light.falloffMin, 19.0f, _aColors[5], 0xFFFFFFFF);
		}
	}
}

void SputRender::drawActorBoxes(Room *pRoom, uint setup) {
	if (!pRoom)
		return;

	if (setup != 0xFFFFFFFF) {
		if (pRoom->numSetups == 0) {
			// The original could divide by zero here, let's avoid that...
			warning("SputRender::drawActorBoxes(): the room has no setups");
			return;
		}

		drawActorBox(&pRoom->pSetups[setup % pRoom->numSetups]);
		return;
	}

	for (uint i = 0; i < pRoom->numSetups; i++) {
		if (i != (uint)pRoom->curSetup)
			drawActorBox(&pRoom->pSetups[i]);
	}
}

void SputRender::drawActorBox(RoomSetup *pSetup) {
	float x = pSetup->camera.interest[0];
	float y = pSetup->camera.interest[1];
	float z = pSetup->camera.interest[2];
	RdVector3 p0;
	RdVector3 p1;

	p0[0] = (float)((double)x + 1.0);
	p0[1] = y;
	p0[2] = z;
	p1[0] = (float)((double)x - 1.0);
	p1[1] = y;
	p1[2] = z;
	_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _aColors[6], 0xFFFFFFFF);

	p0[0] = x;
	p0[1] = (float)((double)y + 1.0);
	p0[2] = z;
	p1[0] = x;
	p1[1] = (float)((double)y - 1.0);
	p1[2] = z;
	_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _aColors[6], 0xFFFFFFFF);

	p0[0] = x;
	p0[1] = y;
	p0[2] = (float)((double)z + 1.0);
	p1[0] = x;
	p1[1] = y;
	p1[2] = (float)((double)z - 1.0);
	_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _aColors[6], 0xFFFFFFFF);

	SputActor *pCamActor = _pSputActor->actorDeref(_gCamActorIndex);

	if (pCamActor->pCostumeStack)
		_pSputActor->drawCostume(pCamActor->pCostumeStack, pSetup->camera.posMatrix);
}

void SputRender::drawBoundingBox() {
	RdVector3 p0;
	RdVector3 p1;
	float value = -1.0f;

	for (int i = 0; i < 20; i++) {
		p0[0] = -1.0f;
		p0[1] = value;
		p0[2] = 0.0f;
		p1[0] = 1.0f;
		p1[1] = value;
		p1[2] = 0.0f;
		_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _aColors[4], 0xFF00FF00);
		value = (float)((double)value + (double)0.1f);
	}

	value = -1.0f;

	for (int i = 0; i < 20; i++) {
		p0[0] = value;
		p0[1] = -1.0f;
		p0[2] = 0.0f;
		p1[0] = value;
		p1[1] = 1.0f;
		p1[2] = 0.0f;
		_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _aColors[4], 0xFF00FF00);
		value = (float)((double)value + (double)0.1f);
	}

	p0[0] = 0.0f;
	p0[1] = 0.0f;
	p0[2] = 0.0f;
	p1[0] = 2.0f;
	p1[1] = 0.0f;
	p1[2] = 0.0f;
	_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _aColors[3], 0xFFFFFFFF);

	p1[0] = 0.0f;
	p1[1] = 2.0f;
	p1[2] = 0.0f;
	_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _aColors[3], 0xFFFFFFFF);

	p1[0] = 0.0f;
	p1[1] = 0.0f;
	p1[2] = 2.0f;
	_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _aColors[3], 0xFFFFFFFF);
}

void SputRender::drawWalkPlanes() {
	for (WalkPlane *pPlane = _pWalkSystem->getHead(); pPlane; pPlane = pPlane->pNext) {
		uint16 color = _aColors[18];

		if (pPlane->type & 0x1000)
			color = _aColors[3];

		if (pPlane->type & 0x2000)
			color = _aColors[8];

		if (pPlane->type & 0x4000)
			color = _aColors[7];

		if (pPlane->type & 0x8000)
			color = _aColors[5];

		for (Puck *pPuck = _pWalkSystem->getPuckHead(); pPuck; pPuck = pPuck->pNext) {
			if (pPuck->pPlane == pPlane) {
				color = _aColors[20];
				break;
			}
		}

		pPlane->draw(color);
	}
}

} // End of namespace Grim
