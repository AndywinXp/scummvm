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

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/cweightmap.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/rmeshinstance.h"
#include "engines/grim/emi/shadowmeshcomponent.h"
#include "engines/grim/emi/shadowmeshinstance.h"

namespace Grim {

ShadowMeshInstance::ShadowMeshInstance(const LString &componentName, Actor *actor) : ComponentInstance(componentName) {
	owner = actor;
	meshInstance = nullptr;

	if (handle.lock()) {
		ShadowMeshComponent *component = dynamic_cast<ShadowMeshComponent *>(handle.typed);

		if (handle.typed && !component)
			warning("ShadowMeshInstance::ShadowMeshInstance(): %s is not a shadow mesh component", componentName.c_str());

		if (component && !owner) {
			warning("ShadowMeshInstance::ShadowMeshInstance(): no owner");
			component = nullptr;
		}

		if (component) {
			meshInstance = static_cast<RMeshInstance *>(component->RMesh::createInstance());
			meshInstance->visible = false;

			if (component->weightMap) {
				uint numNodes = component->weightMap->nodeNames.size();

				meshInstance->boneNodes = (CNode **)gpMemoryServices->malloc(numNodes * sizeof(CNode *));

				// The disasm leaves the bones it cannot find uninitialized
				if (meshInstance->boneNodes)
					memset(meshInstance->boneNodes, 0, numNodes * sizeof(CNode *));

				component->weightMap->findNodes(owner, meshInstance->boneNodes);
			}

			// The disasm frees the normals of the shared mesh
			if (component->normals) {
				gpMemoryServices->free(component->normals);
				component->normals = nullptr;
			}

			component->vertFlags &= ~1;
			owner->shadowMeshes.push_back(meshInstance);
		}
	}

	handle.unlock();
}

ShadowMeshInstance::~ShadowMeshInstance() {
	if (owner && meshInstance) {
		uint index = owner->findShadowPrimitive(meshInstance);

		owner->removeShadowPrimitive((int)index);
		delete meshInstance;
		meshInstance = nullptr;
	}
}

void ShadowMeshInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	// The disasm treats NaN as zero
	if (meshInstance)
		meshInstance->visible = (value < 0.0f || value > 0.0f);
}

} // End of namespace Grim
