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
#include "common/util.h"

#include "engines/grim/emi/anode.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lformatparser.h"
#include "engines/grim/emi/skeletoncomponent.h"
#include "engines/grim/emi/skeletoninstance.h"

namespace Grim {

SkeletonComponent::SkeletonComponent() {
}

SkeletonComponent::~SkeletonComponent() {
}

uint SkeletonComponent::resSizeOfItem() {
	return sizeof(SkeletonComponent) + sizeof(SkeletonNode) * nodes.size();
}

ComponentInstance *SkeletonComponent::createInstance(Actor *owner) {
	return new SkeletonInstance(LResource::name, owner);
}

bool SkeletonComponent::resLoadItem(const char *itemName) {
	if (strlen(itemName) < Component::kNamePrefixSize) {
		warning("SkeletonComponent::resLoadItem(): invalid resource name %s", itemName);

		return false;
	}

	const char *fileName = itemName + Component::kNamePrefixSize;
	LString textPath(fileName);
	LString binaryPath = textPath + "b";
	bool binaryExists = gpFileServices->exists(binaryPath.c_str());
	bool textExists = gpFileServices->exists(textPath.c_str());
	bool loadText = !binaryExists;

	if (binaryExists && textExists) {
		int binaryTime = gpFileServices->getModTime(binaryPath.c_str());
		int textTime = gpFileServices->getModTime(textPath.c_str());

		loadText = binaryTime < textTime;
	}

	if (loadText) {
		LFile *file = gpFileServices->open(fileName, "rt");

		if (!file || !LFormatParser::skipCopyrightBanner(file) || !restore(file)) {
			if (file)
				gpFileServices->close(file, true);

			if (gpConsoleServices)
				gpConsoleServices->logError("Parse problem in file '%s'.\n", fileName);
			else
				warning("SkeletonComponent::resLoadItem(): parse problem in file '%s'", fileName);

			return false;
		}

		gpFileServices->close(file, true);
		saveBinary(binaryPath.c_str());
	} else if (!loadBinary(binaryPath.c_str())) {
		if (gpConsoleServices)
			gpConsoleServices->logError("Parse problem in file '%s'.\n", binaryPath.c_str());
		else
			warning("SkeletonComponent::resLoadItem(): parse problem in file '%s'", binaryPath.c_str());

		return false;
	}

	return true;
}

CNode *SkeletonComponent::buildNodeTree() {
	CNode *rootNode = nullptr;

	for (uint i = 0; i < nodes.size(); i++) {
		SkeletonNode &skeletonNode = nodes[i];
		ANode *animNode = new ANode();

		animNode->setLocalTransform(skeletonNode.pos, skeletonNode.quat);
		memcpy(animNode->name, skeletonNode.name, CNode::kNameSize);

		if (scumm_strnicmp(skeletonNode.parent, "no_parent", SkeletonNode::kNameSize) != 0) {
			if (rootNode)
				animNode->setParent(rootNode->findNode(skeletonNode.parent));
		} else if (!rootNode) {
			rootNode = animNode;
		}
	}

	return rootNode;
}

} // End of namespace Grim
