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

#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/emi/layer.h"
#include "engines/grim/emi/lresource.h"

namespace Grim {

bool Layer::fix16Bit = true;
int Layer::_nextId = 1;

void Layer::set16BitFix(bool enable) {
	fix16Bit = enable;
}

void Layer::saveRestoreNextId(tSaveRestoreAccess *access) {
	access(&_nextId, sizeof(_nextId));
}

Layer::Layer(const char *layerName, int layerSortOrder, int layerFrame, int layerId) {
	if (layerId == -1) {
		id = _nextId;
		_nextId++;
	} else {
		id = layerId;
	}

	frame = 0;
	sortOrder = 0;

	if (strlen(layerName) >= kNameSize)
		warning("Layer::Layer(): the name is too long");

	Common::strlcpy(name, layerName, kNameSize);
	sortOrder = layerSortOrder;
	frame = layerFrame;

	LString resourceName = LString::format("BG%s", layerName);

	background.resource = gpResourceManager->findResource(resourceName.c_str());

	if (!background.resource) {
		background.resource = gpResourceManager->createResource(resourceName.c_str());

		if (!background.resource)
			background.allocateResource();

		if (background.resource) {
			background.resource->resSetName(resourceName.c_str());
			gpResourceManager->addResource(background.resource);
		}
	}

	background.setResource(background.resource);

	resourceName = LString::format("TS%s", layerName);

	tiledScene.resource = gpResourceManager->findResource(resourceName.c_str());

	if (!tiledScene.resource) {
		tiledScene.resource = gpResourceManager->createResource(resourceName.c_str());

		if (!tiledScene.resource)
			tiledScene.allocateResource();

		if (tiledScene.resource) {
			tiledScene.resource->resSetName(resourceName.c_str());
			gpResourceManager->addResource(tiledScene.resource);
		}
	}

	tiledScene.setResource(tiledScene.resource);
}

Layer::~Layer() {
}

} // End of namespace Grim
