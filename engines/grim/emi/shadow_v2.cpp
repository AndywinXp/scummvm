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

#include "engines/grim/emi/clightnode.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/shadow_v2.h"
#include "engines/grim/emi/walkplane_v2.h"
#include "engines/grim/emi/walkplanecollection.h"

namespace Grim {

Shadow_v2::Shadow_v2() {
	point[0] = 0.0f;
	point[1] = 0.0f;
	point[2] = 0.0f;
}

Shadow_v2::~Shadow_v2() {
	sectors.clear();
	sectorNames.clear();
	lightName.clear();
	name.clear();
}

void *Shadow_v2::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void Shadow_v2::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

int Shadow_v2::getSize() {
	return (int)(sizeof(Shadow_v2) + name.size() + lightName.size() + sectors.size() * sizeof(WalkPlane_v2 *) + sectorNames.size() * sizeof(LString));
}

void Shadow_v2::resolveSectors(WalkPlaneCollection *collection) {
	if (!collection)
		return;

	for (uint i = 0; i < sectorNames.size(); i++)
		sectors.push_back(collection->planeNamed(sectorNames[i]));
}

float *Shadow_v2::getPosition(float *out) {
	if (!lightName.empty()) {
		Room_v2 *room = Room_v2::getCurrentRoom();
		CLightNode *light = nullptr;

		if (room)
			light = room->findLight(lightName.c_str());
		else
			warning("Shadow_v2::getPosition(): there is no current room");

		if (light) {
			if ((light->nodeFlags & CNode::kWorldValid) == 0)
				light->updateWorldTransform();

			out[0] = light->world.pos.x;
			out[1] = light->world.pos.y;
			out[2] = light->world.pos.z;

			return out;
		}
	}

	out[0] = point[0];
	out[1] = point[1];
	out[2] = point[2];

	return out;
}

void Shadow_v2::setName(const LString &newName) {
	name = newName;
}

void Shadow_v2::setLightName(const LString &newLightName) {
	lightName = newLightName;
}

void Shadow_v2::setColor(const Color &newColor) {
	color = newColor;
}

void Shadow_v2::addSector(const LString &sectorName) {
	sectorNames.push_back(sectorName);
}

} // End of namespace Grim
