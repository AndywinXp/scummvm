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

#include "engines/grim/emi/actor/actor.h"
#include "engines/grim/emi/scene/camera.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/actor/puck_v2.h"
#include "engines/grim/emi/nodes/rmesh.h"
#include "engines/grim/emi/nodes/rprimitiveinstance.h"
#include "engines/grim/emi/scene/walkplane_v2.h"
#include "engines/grim/emi/scene/walkplanecollection.h"

namespace Grim {

static const int kSortOrderSetups = 32;
static const int kSortWalkPlaneMask = 0x1000;

static int truncateScreenValue(double value) {
	if (!(value > -2147483649.0 && value < 2147483648.0)) {
		warning("Actor::calcOnScreen(): the screen rectangle is out of range");

		return -2147483647 - 1;
	}

	return (int)value;
}

int Actor::getSortOrder(int setupIndex) {
	Actor *sortActor = this;

	if (_parentActorId) {
		do {
			if (sortActor->_sortOrder != -2)
				break;

			Actor *parentActor = actorDeref(_engine, sortActor->_parentActorId);

			if (!parentActor)
				break;

			sortActor = parentActor;
		} while (sortActor->_parentActorId);
	}

	Extra_v2 *sortExtra = sortActor->_extra;

	if (!sortExtra || !sortExtra->_inBoxes || !sortActor->_room._resource)
		return sortActor->_sortOrder;

	WalkPlane_v2 *plane = nullptr;

	if (sortExtra->_puck) {
		plane = sortExtra->_puck->_plane;
	} else {
		bool locked = sortActor->_room.lock();
		Room_v2 *currentRoom = locked ? sortActor->_room._typed : nullptr;
		WalkPlaneCollection *collection = nullptr;

		if (currentRoom)
			collection = currentRoom->_walkPlanes;
		else
			warning("Actor::getSortOrder(): the room could not be locked");

		if (locked)
			sortActor->_room.unlock();

		if ((sortActor->_nodeFlags & kWorldValid) == 0)
			sortActor->updateWorldTransform();

		if (collection)
			plane = collection->planeAt(&sortActor->_world._pos._x, kSortWalkPlaneMask);
		else
			warning("Actor::getSortOrder(): the room has no walk planes");
	}

	if (!plane)
		return sortActor->_sortOrder;

	if (setupIndex < 0) {
		warning("Actor::getSortOrder(): negative setup index");

		return sortActor->_sortOrder;
	}

	return plane->_sortOrder[setupIndex % kSortOrderSetups];
}

bool Actor::calcOnScreen(Camera *camera) {
	_engine->_actorCalcOnScreenTimer.start();

	CVector point;

	camera->applyProjection();
	camera->updateViewMatrix();

	CTransform viewTransform = camera->_view;
	IRect viewport = camera->getViewportRect();
	int viewLeft = viewport.x;
	int viewTop = viewport.y;
	int viewRight = viewport.x + viewport.w;
	int viewBottom = viewport.y + viewport.h;

	float minX = (float)((double)viewRight + 1.0);
	float minY = (float)((double)viewBottom + 1.0);
	float maxX = (float)((double)viewLeft - 1.0);
	float maxY = (float)((double)viewTop - 1.0);

	const float *m = viewTransform._rot._m;

	for (uint i = 0; i < _instances.size(); i++) {
		RPrimitiveInstance *instance = _instances[i];

		if (!instance)
			continue;

		if (!instance->_prim) {
			warning("Actor::calcOnScreen(): primitive instance without primitive");

			continue;
		}

		const BBox &box = instance->_prim->_box;

		for (int corner = 0; corner < 8; corner++) {
			point._x = (corner & 4) ? box._max._x : box._min._x;
			point._y = (corner & 2) ? box._max._y : box._min._y;
			point._z = (corner & 1) ? box._max._z : box._min._z;

			localToWorld(&point);

			float rowY = (float)(((double)m[1] * point._x + (double)m[4] * point._y) + (double)m[7] * point._z);
			float rowZ = (float)(((double)m[2] * point._x + (double)m[5] * point._y) + (double)m[8] * point._z);
			double viewX = (((double)m[3] * point._y + (double)m[6] * point._z) + (double)m[0] * point._x) + viewTransform._pos._x;
			float viewY = (float)((double)viewTransform._pos._y + rowY);
			double viewZ = (double)viewTransform._pos._z + rowZ;

			if (viewZ >= 0.0) {
				float inverseZ = (float)(1.0 / viewZ);
				float screenX = (float)(((viewX * camera->_focalX) * inverseZ) + camera->_halfWidth);
				float screenY = (float)(camera->_halfHeight - (((double)viewY * camera->_focalY) * inverseZ));

				// The disasm updates the minimum also when the comparison is unordered
				if (!(screenX >= minX))
					minX = screenX;

				if (screenX > maxX)
					maxX = screenX;

				if (!(screenY >= minY))
					minY = screenY;

				if (screenY > maxY)
					maxY = screenY;
			}
		}
	}

	_drawnRect[0] = truncateScreenValue(minX);
	_drawnRect[1] = truncateScreenValue(minY);
	_drawnRect[2] = truncateScreenValue((double)maxX - minX);
	_drawnRect[3] = truncateScreenValue((double)maxY - minY);

	bool visible = true;

	if ((int)((uint)_drawnRect[2] + (uint)_drawnRect[0]) < viewLeft
		|| _drawnRect[0] > viewRight
		|| (int)((uint)_drawnRect[3] + (uint)_drawnRect[1]) < viewTop
		|| _drawnRect[1] > viewBottom)
		visible = false;

	_engine->_actorCalcOnScreenTimer.stop();

	return visible;
}

int Actor::drawActor(Camera *camera) {
	_engine->_actorDrawTimer.start();

	if (_onScreen)
		draw(camera);

	_engine->_actorDrawTimer.stop();

	return 0;
}

} // End of namespace Grim
