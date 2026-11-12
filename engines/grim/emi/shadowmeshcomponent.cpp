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

#include "engines/grim/emi/cweightmap.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lformatparser.h"
#include "engines/grim/emi/shadowmeshcomponent.h"
#include "engines/grim/emi/shadowmeshinstance.h"

namespace Grim {

ShadowMeshComponent::ShadowMeshComponent(EmiEngine *engine) : RMesh(engine) {
	bindEngine(engine);
}

ShadowMeshComponent::~ShadowMeshComponent() {
}

uint ShadowMeshComponent::resSizeOfItem() {
	int size = (int)sizeof(ShadowMeshComponent);

	if (pos)
		size += (int)(3 * sizeof(float)) * numVerts;

	if (normals)
		size += (int)(3 * sizeof(float)) * numVerts;

	if (vertIndex)
		size += (int)sizeof(int) * numVerts;

	if (colors)
		size += (int)sizeof(PackedColor) * numVerts;

	if (uv)
		size += (int)(2 * sizeof(float)) * numVerts;

	size += (int)(sizeof(LResourceHandle<CMaterial>) * matHandles.size());
	size += (int)(sizeof(int) * texFlags.size());

	if (weightMap)
		size += weightMap->getSize();

	for (uint i = 0; i < trisets.size(); i++) {
		if (trisets[i])
			size += (int)(sizeof(TriSet) + sizeof(uint16) * trisets[i]->indices.size());
	}

	return (uint)size;
}

ComponentInstance *ShadowMeshComponent::createInstance(Actor *owner) {
	return new ShadowMeshInstance(LResource::_engine, LResource::name, owner);
}

bool ShadowMeshComponent::resLoadItem(const char *itemName) {
	if (strlen(itemName) < Component::kNamePrefixSize) {
		warning("ShadowMeshComponent::resLoadItem(): invalid resource name %s", itemName);

		return false;
	}

	EmiEngine *engine = LResource::_engine;
	const char *fileName = itemName + Component::kNamePrefixSize;
	LString textPath(fileName);
	LString binaryPath = textPath + "b";
	bool binaryExists = engine->getFileServices()->exists(binaryPath.c_str());
	bool textExists = engine->getFileServices()->exists(textPath.c_str());
	bool loadText = !binaryExists;

	if (binaryExists && textExists) {
		int binaryTime = engine->getFileServices()->getModTime(binaryPath.c_str());
		int textTime = engine->getFileServices()->getModTime(textPath.c_str());

		loadText = binaryTime < textTime;
	}

	if (loadText) {
		LFile *file = engine->getFileServices()->open(fileName, "rt");

		if (!file || !LFormatParser::skipCopyrightBanner(file) || !restore(file)) {
			if (file)
				engine->getFileServices()->close(file, true);

			if (engine->getConsoleServices())
				engine->getConsoleServices()->logError("Parse problem in file '%s'.\n", fileName);
			else
				warning("ShadowMeshComponent::resLoadItem(): parse problem in file '%s'", fileName);

			return false;
		}

		engine->getFileServices()->close(file, true);
		CPrimitive::name = fileName;
		saveBinary(binaryPath.c_str());
	} else if (!loadBinary(binaryPath.c_str())) {
		if (engine->getConsoleServices())
			engine->getConsoleServices()->logError("Parse problem in file '%s'.\n", binaryPath.c_str());
		else
			warning("ShadowMeshComponent::resLoadItem(): parse problem in file '%s'", binaryPath.c_str());

		return false;
	}

	return true;
}

} // End of namespace Grim
