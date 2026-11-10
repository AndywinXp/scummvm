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

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/anode.h"

namespace Grim {

void Actor::attachNode(CNode *node) {
	if (node) {
		registerANodes(node);
		node->setParent(this);
	}
}

void Actor::detachNode(CNode *node) {
	if (node) {
		unregisterANodes(node);
		node->detachFromParent();
	}
}

void Actor::registerANodes(CNode *node) {
	for (CNode *current = node; current; current = current->nextSibling) {
		registerANodes(current->firstChild);

		ANode *animNode = dynamic_cast<ANode *>(current);

		if (animNode)
			animNodes.push_back(animNode);
	}
}

void Actor::unregisterANodes(CNode *node) {
	for (CNode *current = node; current; current = current->nextSibling) {
		unregisterANodes(current->firstChild);

		ANode *animNode = dynamic_cast<ANode *>(current);

		if (animNode) {
			Common::List<ANode *>::iterator it = animNodes.begin();

			while (it != animNodes.end()) {
				if (*it == animNode)
					it = animNodes.erase(it);
				else
					++it;
			}
		}
	}
}

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

} // End of namespace Grim
