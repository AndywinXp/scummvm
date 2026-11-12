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

#include "engines/grim/emi/scene/clightnode.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/scene/room_v2.h"
#include "engines/grim/emi/actor/shadow_v2.h"
#include "engines/grim/emi/scene/walkplane_v2.h"
#include "engines/grim/emi/scene/walkplanecollection.h"

namespace Grim {

Shadow_v2::Shadow_v2(EmiEngine *engine) : _engine(engine) {
	_point[0] = 0.0f;
	_point[1] = 0.0f;
	_point[2] = 0.0f;
}

Shadow_v2::~Shadow_v2() {
	_sectors.clear();
	_sectorNames.clear();
	_lightName.clear();
	_name.clear();
}

void *Shadow_v2::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void Shadow_v2::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

int Shadow_v2::getSize() {
	return (int)(sizeof(Shadow_v2) + _name.size() + _lightName.size() + _sectors.size() * sizeof(WalkPlane_v2 *) + _sectorNames.size() * sizeof(LString));
}

void Shadow_v2::resolveSectors(WalkPlaneCollection *collection) {
	if (!collection)
		return;

	for (uint i = 0; i < _sectorNames.size(); i++)
		_sectors.push_back(collection->planeNamed(_sectorNames[i]));
}

float *Shadow_v2::getPosition(float *out) {
	if (!_lightName.empty()) {
		Room_v2 *room = Room_v2::getCurrentRoom(_engine);
		CLightNode *light = nullptr;

		if (room)
			light = room->findLight(_lightName.c_str());
		else
			warning("Shadow_v2::getPosition(): there is no current room");

		if (light) {
			if ((light->_nodeFlags & CNode::kWorldValid) == 0)
				light->updateWorldTransform();

			out[0] = light->_world._pos._x;
			out[1] = light->_world._pos._y;
			out[2] = light->_world._pos._z;

			return out;
		}
	}

	out[0] = _point[0];
	out[1] = _point[1];
	out[2] = _point[2];

	return out;
}

void Shadow_v2::setName(const LString &newName) {
	_name = newName;
}

void Shadow_v2::setLightName(const LString &newLightName) {
	_lightName = newLightName;
}

void Shadow_v2::setColor(const Color &newColor) {
	_color = newColor;
}

void Shadow_v2::addSector(const LString &sectorName) {
	_sectorNames.push_back(sectorName);
}

} // End of namespace Grim
