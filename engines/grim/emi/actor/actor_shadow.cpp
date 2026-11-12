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

#include <math.h>

#include "common/textconsole.h"

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/cdcvertexarray.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/rmesh.h"
#include "engines/grim/emi/rmeshinstance.h"
#include "engines/grim/emi/rprimitiveinstance.h"
#include "engines/grim/emi/walkplane_v2.h"
#include "engines/grim/emi/wpvertex.h"

namespace Grim {

static const int kShadowTriangleFlags = 0x301;
static const int kShadowVertexCapacity = 2000;
static const float kMinClipLength = 1e-20f;

uint Actor::findShadowPrimitive(RPrimitiveInstance *instance) {
	for (uint i = 0; i < shadowMeshes.size(); i++) {
		if (shadowMeshes[i] == instance)
			return i;
	}

	return (uint)-1;
}

void Actor::removeShadowPrimitive(int index) {
	if (index >= 0 && (uint)index < shadowMeshes.size())
		shadowMeshes.remove_at(index);
}

void Actor::activateShadow(bool active, Shadow_v2 *shadow) {
	if (active) {
		if (shadow) {
			if (findShadowIndex(shadow) < 0)
				shadows.push_back(shadow);
		} else {
			shadows.clear();

			bool locked = room.lock();
			Room_v2 *currentRoom = locked ? room.typed : nullptr;

			if (currentRoom) {
				Common::List<Shadow_v2 *>::iterator it;

				for (it = currentRoom->shadows.begin(); it != currentRoom->shadows.end(); ++it) {
					if (*it)
						activateShadow(active, *it);
				}
			} else {
				warning("Actor::activateShadow(): the room could not be locked");
			}

			room.unlock();
		}
	} else if (shadow) {
		int index = findShadowIndex(shadow);

		if (index >= 0)
			shadows.remove_at(index);
	} else {
		shadows.clear();
	}
}

int Actor::findShadowIndex(Shadow_v2 *shadow) {
	for (uint i = 0; i < shadows.size(); i++) {
		if (shadows[i] == shadow)
			return i;
	}

	return -1;
}

int Actor::drawShadow(Camera *camera, Shadow_v2 *shadow) {
	_engine->_actorDrawShadowTimer.start();

	CDrawContext *drawContext = _engine->getDrawContext();

	if (!onScreen) {
		_engine->_actorDrawShadowTimer.stop();

		return 0;
	}

	if (!drawContext || !_engine->_shadowVertices) {
		warning("Actor::drawShadow(): no draw context or shadow vertices");
		_engine->_actorDrawShadowTimer.stop();

		return 0;
	}

	drawContext->setCameraMatrix(reinterpret_cast<const float *>(&camera->view));

	float shadowPoint[3];
	float *position = shadow->getPosition(shadowPoint);
	float shadowX = position[0];
	float shadowY = position[1];
	float shadowZ = position[2];

	Color shadowColor(shadow->color);

	Common::Array<RPrimitiveInstance *> *meshInstances = &instances;

	if (shadowMeshes.size() > 0)
		meshInstances = &shadowMeshes;

	if (shadow->sectors.empty() || !shadow->sectors[0]) {
		warning("Actor::drawShadow(): shadow without sectors");
		drawContext->resetCameraMatrix();
		_engine->_actorDrawShadowTimer.stop();

		return 0;
	}

	WalkPlane_v2 *firstSector = shadow->sectors[0];
	float planeNormalX = firstSector->normal[0];
	float planeNormalY = firstSector->normal[1];
	float planeD = firstSector->d;
	float planeNormalZ = firstSector->normal[2];

	for (uint i = 0; i < meshInstances->size(); i++) {
		RPrimitiveInstance *instance = (*meshInstances)[i];

		if (!instance || !instance->prim) {
			warning("Actor::drawShadow(): primitive instance without primitive");

			continue;
		}

		RMesh *mesh = dynamic_cast<RMesh *>(instance->prim);

		if (!mesh)
			continue;

		instance->update(this, camera);

		RMeshInstance *meshInstance = static_cast<RMeshInstance *>((*meshInstances)[i]);

		if (!meshInstance->vertexArray) {
			warning("Actor::drawShadow(): mesh instance without vertex array");

			continue;
		}

		if (meshInstance->vertexArray->numVerts > kShadowVertexCapacity) {
			warning("Actor::drawShadow(): too many vertices for the shadow vertex array");

			continue;
		}

		_engine->_shadowVertices->numVerts = meshInstance->vertexArray->numVerts;

		CDCVertex *destination = static_cast<CDCVertex *>(_engine->_shadowVertices->lock());
		CDCVertex *source = static_cast<CDCVertex *>(meshInstance->vertexArray->lock());

		if (!destination || !source) {
			warning("Actor::drawShadow(): could not lock the vertex arrays");

			continue;
		}

		for (int v = 0; v < _engine->_shadowVertices->numVerts; v++) {
			CVector localPoint;
			CVector worldPoint;

			localPoint.x = source[v].x;
			localPoint.y = source[v].y;
			localPoint.z = source[v].z;

			localToWorld(&localPoint, &worldPoint);

			destination[v].x = worldPoint.x;
			destination[v].y = worldPoint.y;
			destination[v].z = worldPoint.z;

			double deltaX = (double)destination[v].x - shadowX;
			float deltaY = (float)((double)destination[v].y - shadowY);
			float deltaZ = (float)((double)destination[v].z - shadowZ);

			double numerator = (((double)planeNormalY * destination[v].y + (double)planeNormalZ * destination[v].z) + (double)planeNormalX * destination[v].x) + planeD;
			double denominator = ((double)planeNormalZ * deltaZ + (double)planeNormalY * deltaY) + deltaX * planeNormalX;
			float projectionScale = (float)(-(numerator / denominator));

			float offsetY = (float)((double)deltaY * projectionScale);
			float offsetZ = (float)((double)deltaZ * projectionScale);

			destination[v].x = (float)(deltaX * projectionScale + destination[v].x);
			destination[v].y = (float)(offsetY + destination[v].y);
			destination[v].z = (float)(offsetZ + destination[v].z);

			uint red = (uint)shadowColor.r <= 0xFFu ? (uint)shadowColor.r : 0xFFu;
			uint green = (uint)shadowColor.g <= 0xFFu ? (uint)shadowColor.g : 0xFFu;
			uint blue = (uint)shadowColor.b <= 0xFFu ? (uint)shadowColor.b : 0xFFu;
			uint alpha = (uint)shadowColor.a <= 0xFFu ? (uint)shadowColor.a : 0xFFu;

			destination[v].color = (alpha << 24) | (red << 16) | (green << 8) | blue;
		}

		meshInstance->vertexArray->unlock();
		_engine->_shadowVertices->unlock();

		for (uint s = 0; s < shadow->sectors.size(); s++) {
			WalkPlane_v2 *sector = shadow->sectors[s];

			if (!sector) {
				warning("Actor::drawShadow(): unresolved shadow sector");

				continue;
			}

			if (sector->numVerts > drawContext->getMaxClipPlanes())
				continue;

			int numVerts = sector->numVerts;

			for (int e = 0; e < numVerts; e++) {
				wpVertex *start = sector->verts[e];
				wpVertex *end = sector->verts[(e + 1) % numVerts];

				double toShadowX = (double)shadowX - start->x;
				double toShadowY = (double)shadowY - start->y;
				double toShadowZ = (double)shadowZ - start->z;

				float edgeX = (float)((double)end->x - start->x);
				float edgeY = (float)((double)end->y - start->y);
				double edgeZ = (double)end->z - start->z;

				float clipX = (float)(edgeY * toShadowZ - edgeZ * toShadowY);
				float clipY = (float)(edgeZ * toShadowX - toShadowZ * edgeX);
				float clipZ = (float)(toShadowY * edgeX - edgeY * toShadowX);

				double length = sqrt(((double)clipX * clipX + (double)clipZ * clipZ) + (double)clipY * clipY);

				if (length >= kMinClipLength) {
					double inverse = 1.0 / length;

					clipX = (float)(clipX * inverse);
					clipY = (float)(inverse * clipY);
					clipZ = (float)(inverse * clipZ);
				}

				float clipD = (float)(-(((double)shadowY * clipY + (double)shadowZ * clipZ) + (double)shadowX * clipX));

				drawContext->addClipPlane(clipX, clipY, clipZ, clipD);
			}

			drawContext->submitVertices(_engine->_shadowVertices);

			for (uint t = 0; t < mesh->trisets.size(); t++) {
				TriSet *triset = mesh->trisets[t];

				if (triset)
					drawContext->drawTriangles(kShadowTriangleFlags, nullptr, (int)triset->indices.size(), triset->indices.data());
			}

			drawContext->releaseVertices();
			drawContext->clearClipPlanes();
		}
	}

	drawContext->resetCameraMatrix();

	_engine->_actorDrawShadowTimer.stop();

	return 0;
}

} // End of namespace Grim
