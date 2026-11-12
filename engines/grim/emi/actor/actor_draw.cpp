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

	if (parentActorId) {
		do {
			if (sortActor->sortOrder != -2)
				break;

			Actor *parentActor = actorDeref(_engine, sortActor->parentActorId);

			if (!parentActor)
				break;

			sortActor = parentActor;
		} while (sortActor->parentActorId);
	}

	Extra_v2 *sortExtra = sortActor->extra;

	if (!sortExtra || !sortExtra->inBoxes || !sortActor->room.resource)
		return sortActor->sortOrder;

	WalkPlane_v2 *plane = nullptr;

	if (sortExtra->puck) {
		plane = sortExtra->puck->plane;
	} else {
		bool locked = sortActor->room.lock();
		Room_v2 *currentRoom = locked ? sortActor->room.typed : nullptr;
		WalkPlaneCollection *collection = nullptr;

		if (currentRoom)
			collection = currentRoom->walkPlanes;
		else
			warning("Actor::getSortOrder(): the room could not be locked");

		if (locked)
			sortActor->room.unlock();

		if ((sortActor->nodeFlags & kWorldValid) == 0)
			sortActor->updateWorldTransform();

		if (collection)
			plane = collection->planeAt(&sortActor->world.pos.x, kSortWalkPlaneMask);
		else
			warning("Actor::getSortOrder(): the room has no walk planes");
	}

	if (!plane)
		return sortActor->sortOrder;

	if (setupIndex < 0) {
		warning("Actor::getSortOrder(): negative setup index");

		return sortActor->sortOrder;
	}

	return plane->sortOrder[setupIndex % kSortOrderSetups];
}

bool Actor::calcOnScreen(Camera *camera) {
	_engine->_actorCalcOnScreenTimer.start();

	CVector point;

	camera->applyProjection();
	camera->updateViewMatrix();

	CTransform viewTransform = camera->view;
	IRect viewport = camera->getViewportRect();
	int viewLeft = viewport.x;
	int viewTop = viewport.y;
	int viewRight = viewport.x + viewport.w;
	int viewBottom = viewport.y + viewport.h;

	float minX = (float)((double)viewRight + 1.0);
	float minY = (float)((double)viewBottom + 1.0);
	float maxX = (float)((double)viewLeft - 1.0);
	float maxY = (float)((double)viewTop - 1.0);

	const float *m = viewTransform.rot.m;

	for (uint i = 0; i < instances.size(); i++) {
		RPrimitiveInstance *instance = instances[i];

		if (!instance)
			continue;

		if (!instance->prim) {
			warning("Actor::calcOnScreen(): primitive instance without primitive");

			continue;
		}

		const BBox &box = instance->prim->box;

		for (int corner = 0; corner < 8; corner++) {
			point.x = (corner & 4) ? box.max.x : box.min.x;
			point.y = (corner & 2) ? box.max.y : box.min.y;
			point.z = (corner & 1) ? box.max.z : box.min.z;

			localToWorld(&point);

			float rowY = (float)(((double)m[1] * point.x + (double)m[4] * point.y) + (double)m[7] * point.z);
			float rowZ = (float)(((double)m[2] * point.x + (double)m[5] * point.y) + (double)m[8] * point.z);
			double viewX = (((double)m[3] * point.y + (double)m[6] * point.z) + (double)m[0] * point.x) + viewTransform.pos.x;
			float viewY = (float)((double)viewTransform.pos.y + rowY);
			double viewZ = (double)viewTransform.pos.z + rowZ;

			if (viewZ >= 0.0) {
				float inverseZ = (float)(1.0 / viewZ);
				float screenX = (float)(((viewX * camera->focalX) * inverseZ) + camera->halfWidth);
				float screenY = (float)(camera->halfHeight - (((double)viewY * camera->focalY) * inverseZ));

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

	drawnRect[0] = truncateScreenValue(minX);
	drawnRect[1] = truncateScreenValue(minY);
	drawnRect[2] = truncateScreenValue((double)maxX - minX);
	drawnRect[3] = truncateScreenValue((double)maxY - minY);

	bool visible = true;

	if ((int)((uint)drawnRect[2] + (uint)drawnRect[0]) < viewLeft
		|| drawnRect[0] > viewRight
		|| (int)((uint)drawnRect[3] + (uint)drawnRect[1]) < viewTop
		|| drawnRect[1] > viewBottom)
		visible = false;

	_engine->_actorCalcOnScreenTimer.stop();

	return visible;
}

int Actor::drawActor(Camera *camera) {
	_engine->_actorDrawTimer.start();

	if (onScreen)
		draw(camera);

	_engine->_actorDrawTimer.stop();

	return 0;
}

} // End of namespace Grim
