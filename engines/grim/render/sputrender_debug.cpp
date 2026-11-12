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

#include "engines/grim/grim.h"
#include "engines/grim/puck.h"
#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/actor/sputactor.h"
#include "engines/grim/render/sputrender.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/walkplane.h"

namespace Grim {

void SputRender::drawActorDebug(SputActor *actor) {
	Extra *extra = actor->extra;

	if (!extra)
		return;

	if (extra->puck)
		extra->puck->draw(_colors[16]);

	if (actor->mode & 1) {
		if (extra->numPathPoints > 30) {
			warning("SputRender::drawActorDebug(): too many path points");
		} else {
			for (int i = 0; i < extra->numPathPoints - 1; i++)
				_rd->_primit3.drawLine(extra->path[i], extra->path[i + 1], nullptr, nullptr, nullptr, nullptr, _colors[10], 0xFFFFFFFF);
		}
	}

	// The demo has no collision sphere to draw
	if (extra->collided && !(_grim->getGameFlags() & ADGF_DEMO)) {
		if (!actor->costumeStack || !actor->costumeStack->headThing || !actor->costumeStack->headThing->data.model3) {
			warning("SputRender::drawActorDebug(): the actor has no model");
			return;
		}

		RdModel3 *model3 = actor->costumeStack->headThing->data.model3;
		float (*orient)[3] = _sputActor->getOrientMatrix(actor);
		RdVector3 out;

		_rd->transformPoint34(out, model3->geomCenter, orient);

		float radius = (float)((double)model3->radius * (double)extra->collisionScale);

		_rd->_primit3.drawPoint(out, radius, 10.0f, 0xFFFF, 0xFFFFFFFF);
	}
}

void SputRender::drawRoomDebug() {
	Room *debugRoom = _sputRoom->getCurrentRoom();

	if (_debugFlags & 2)
		drawCameraDebug(debugRoom);

	if (_debugFlags & 4)
		drawActorBoxes(debugRoom, 0xFFFFFFFF);
}

void SputRender::drawCameraDebug(Room *room) {
	if (!room)
		return;

	uint numLights = room->numLights;

	if (numLights > 64) {
		warning("SputRender::drawCameraDebug(): too many lights");
		numLights = 64;
	}

	for (uint i = 0; i < numLights; i++) {
		RoomLight *roomLight = &room->lights[i];

		if (roomLight->light.type >= 3) {
			RdVector3 target;
			RdMatrix34 mat;

			target[0] = (float)((double)roomLight->light.direction[0] + (double)roomLight->position[0]);
			target[1] = (float)((double)roomLight->light.direction[1] + (double)roomLight->position[1]);
			target[2] = (float)((double)roomLight->light.direction[2] + (double)roomLight->position[2]);
			_rd->lookAt(mat, roomLight->position, target, 0.0f);

			float size = (float)((double)roomLight->light.intensity * (double)0.1f);
			float width = (float)((1.0 - (double)roomLight->light.cosAngleY) * (double)size);
			RdVector3 points[4];

			points[0][0] = width;
			points[0][1] = size;
			points[0][2] = width;
			points[1][0] = width;
			points[1][1] = size;
			points[1][2] = -width;
			points[2][0] = -width;
			points[2][1] = size;
			points[2][2] = -width;
			points[3][0] = -width;
			points[3][1] = size;
			points[3][2] = width;

			for (int j = 0; j < 4; j++) {
				_rd->transformPoint34Acc(points[j], mat);
				_rd->_primit3.drawLine(roomLight->position, points[j], nullptr, nullptr, nullptr, nullptr, _colors[7], 0xFFFFFFFF);
			}

			_rd->_primit3.drawLine(points[0], points[1], nullptr, nullptr, nullptr, nullptr, _colors[7], 0xFFFFFFFF);
			_rd->_primit3.drawLine(points[1], points[2], nullptr, nullptr, nullptr, nullptr, _colors[7], 0xFFFFFFFF);
			_rd->_primit3.drawLine(points[2], points[3], nullptr, nullptr, nullptr, nullptr, _colors[7], 0xFFFFFFFF);
			_rd->_primit3.drawLine(points[3], points[0], nullptr, nullptr, nullptr, nullptr, _colors[7], 0xFFFFFFFF);
		} else {
			RdVector3 ground;

			ground[0] = roomLight->position[0];
			ground[1] = roomLight->position[1];
			ground[2] = 0.0f;
			_rd->_primit3.drawLine(roomLight->position, ground, nullptr, nullptr, nullptr, nullptr, _colors[5], 0xFFFFFFFF);
			_rd->_primit3.drawPoint(roomLight->position, roomLight->light.falloffMin, 19.0f, _colors[5], 0xFFFFFFFF);
		}
	}
}

void SputRender::drawActorBoxes(Room *room, uint setup) {
	if (!room)
		return;

	if (setup != 0xFFFFFFFF) {
		if (room->numSetups == 0) {
			// The original could divide by zero here, let's avoid that...
			warning("SputRender::drawActorBoxes(): the room has no setups");
			return;
		}

		drawActorBox(&room->setups[setup % room->numSetups]);
		return;
	}

	for (uint i = 0; i < room->numSetups; i++) {
		if (i != (uint)room->curSetup)
			drawActorBox(&room->setups[i]);
	}
}

void SputRender::drawActorBox(RoomSetup *setup) {
	float x = setup->camera.interest[0];
	float y = setup->camera.interest[1];
	float z = setup->camera.interest[2];
	RdVector3 p0;
	RdVector3 p1;

	p0[0] = (float)((double)x + 1.0);
	p0[1] = y;
	p0[2] = z;
	p1[0] = (float)((double)x - 1.0);
	p1[1] = y;
	p1[2] = z;
	_rd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _colors[6], 0xFFFFFFFF);

	p0[0] = x;
	p0[1] = (float)((double)y + 1.0);
	p0[2] = z;
	p1[0] = x;
	p1[1] = (float)((double)y - 1.0);
	p1[2] = z;
	_rd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _colors[6], 0xFFFFFFFF);

	p0[0] = x;
	p0[1] = y;
	p0[2] = (float)((double)z + 1.0);
	p1[0] = x;
	p1[1] = y;
	p1[2] = (float)((double)z - 1.0);
	_rd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _colors[6], 0xFFFFFFFF);

	SputActor *camActor = _sputActor->actorDeref(_camActorIndex);

	if (camActor->costumeStack)
		_sputActor->drawCostume(camActor->costumeStack, setup->camera.posMatrix);
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
		_rd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _colors[4], 0xFF00FF00);
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
		_rd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _colors[4], 0xFF00FF00);
		value = (float)((double)value + (double)0.1f);
	}

	p0[0] = 0.0f;
	p0[1] = 0.0f;
	p0[2] = 0.0f;
	p1[0] = 2.0f;
	p1[1] = 0.0f;
	p1[2] = 0.0f;
	_rd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _colors[3], 0xFFFFFFFF);

	p1[0] = 0.0f;
	p1[1] = 2.0f;
	p1[2] = 0.0f;
	_rd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _colors[3], 0xFFFFFFFF);

	p1[0] = 0.0f;
	p1[1] = 0.0f;
	p1[2] = 2.0f;
	_rd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, _colors[3], 0xFFFFFFFF);
}

void SputRender::drawWalkPlanes() {
	for (WalkPlane *plane = _walkSystem->getHead(); plane; plane = plane->_next) {
		uint16 color = _colors[18];

		if (plane->_type & 0x1000)
			color = _colors[3];

		if (plane->_type & 0x2000)
			color = _colors[8];

		if (plane->_type & 0x4000)
			color = _colors[7];

		if (plane->_type & 0x8000)
			color = _colors[5];

		for (Puck *puck = _walkSystem->getPuckHead(); puck; puck = puck->_next) {
			if (puck->_plane == plane) {
				color = _colors[20];
				break;
			}
		}

		plane->draw(color);
	}
}

} // End of namespace Grim
