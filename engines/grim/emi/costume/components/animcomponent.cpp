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

#include "engines/grim/emi/costume/components/animcomponent.h"
#include "engines/grim/emi/costume/components/animinstance.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/lformatparser.h"

namespace Grim {

AnimComponent::AnimComponent(EmiEngine *engine) : Animation(engine) {
	bindEngine(engine);
}

AnimComponent::~AnimComponent() {
}

uint AnimComponent::resSizeOfItem() {
	uint size = sizeof(AnimComponent) + sizeof(AnimEntry) * _entries.size();

	for (uint i = 0; i < _entries.size(); i++) {
		if (_entries[i].track)
			size += _entries[i].track->getSize();
	}

	return size;
}

ComponentInstance *AnimComponent::createInstance(Actor *owner) {
	return new AnimInstance(LResource::_engine, LResource::_name, owner);
}

bool AnimComponent::resLoadItem(const char *itemName) {
	if (strlen(itemName) < Component::kNamePrefixSize) {
		warning("AnimComponent::resLoadItem(): invalid resource name %s", itemName);

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
				warning("AnimComponent::resLoadItem(): parse problem in file '%s'", fileName);

			return false;
		}

		engine->getFileServices()->close(file, true);

		if (!engine->isDemo())
			saveBinary(binaryPath.c_str());
	} else if (!loadBinary(binaryPath.c_str())) {
		if (engine->getConsoleServices())
			engine->getConsoleServices()->logError("Parse problem in file '%s'.\n", binaryPath.c_str());
		else
			warning("AnimComponent::resLoadItem(): parse problem in file '%s'", binaryPath.c_str());

		return false;
	}

	return true;
}

} // End of namespace Grim
