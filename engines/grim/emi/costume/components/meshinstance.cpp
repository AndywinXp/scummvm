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
#include "engines/grim/emi/nodes/cweightmap.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/costume/components/meshcomponent.h"
#include "engines/grim/emi/costume/components/meshinstance.h"
#include "engines/grim/emi/nodes/rmeshinstance.h"

namespace Grim {

MeshInstance::MeshInstance(EmiEngine *engine, const LString &componentName, Actor *actor) : ComponentInstance(engine, componentName) {
	owner = actor;
	meshInstance = nullptr;

	if (handle.lock()) {
		MeshComponent *component = dynamic_cast<MeshComponent *>(handle.typed);

		if (handle.typed && !component)
			warning("MeshInstance::MeshInstance(): %s is not a mesh component", componentName.c_str());

		if (component && !owner) {
			warning("MeshInstance::MeshInstance(): no owner");
			component = nullptr;
		}

		if (component) {
			meshInstance = static_cast<RMeshInstance *>(component->RMesh::createInstance());
			meshInstance->visible = false;

			if (component->weightMap) {
				uint numNodes = component->weightMap->nodeNames.size();

				meshInstance->boneNodes = (CNode **)_engine->getMemoryServices()->mallocImpl(numNodes * sizeof(CNode *));

				// The disasm leaves the bones it cannot find uninitialized
				if (meshInstance->boneNodes)
					memset(meshInstance->boneNodes, 0, numNodes * sizeof(CNode *));

				component->weightMap->bindNodes(owner, meshInstance->boneNodes);
			}

			owner->instances.push_back(meshInstance);
		}
	}

	handle.unlock();
}

MeshInstance::~MeshInstance() {
	if (owner && meshInstance) {
		int index = owner->findPrimitiveInstance(meshInstance);

		owner->removePrimitiveInstance(index);
		delete meshInstance;
		meshInstance = nullptr;
	}
}

void MeshInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	// The disasm treats NaN as zero
	if (meshInstance)
		meshInstance->visible = (value < 0.0f || value > 0.0f);
}

} // End of namespace Grim
