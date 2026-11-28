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

#include "topgun/games/virgil/refengine.h"

namespace Topgun {

ReferenceEngine::~ReferenceEngine() {
	for (uint32 i = 0; i < _objects.size(); i++)
		delete _objects[i];
}

uint16 ReferenceEngine::reMemory(ReObject *&object, const ReObject *parent, uint16 type, uint16 operation, uint16 count) {
	if (operation == kReOperationClose) {
		for (uint32 i = 0; i < _objects.size(); i++) {
			if (_objects[i] == object) {
				delete _objects[i];
				_objects.remove_at(i);
				return kReStatusOk;
			}
		}

		warning("ReferenceEngine::reMemory(): the object of type %d does not exist", type);
		return kReStatusFailed;
	}

	if (operation != kReOperationOpen)
		return kReStatusFailed;

	ReObject *created = nullptr;

	switch (type) {
	case kReTypeApplication:
		created = new ReApplication();
		break;

	case kReTypeDatabase:
		created = new ReDatabase();
		break;

	case kReTypeIndexList:
		created = new ReIndexList();
		break;

	case kReTypeEntryList:
		created = new ReEntryList();
		break;

	case kReTypeTokenSet:
		created = new ReTokenSet();
		break;

	default:
		return kReStatusFailed;
	}

	_objects.push_back(created);
	object = created;

	return kReStatusOk;
}

uint16 ReferenceEngine::reApp(ReApplication *application, uint16 option, uint16 operation) {
	return kReStatusOk;
}

uint16 ReferenceEngine::reDb(ReDatabase *database, uint16 option, uint16 operation) {
	return kReStatusFailed;
}

uint16 ReferenceEngine::reEntLst(ReEntryList *list, uint16 option, uint16 operation) {
	return kReStatusFailed;
}

uint16 ReferenceEngine::reIdxLst(ReIndexList *list, uint16 option, uint16 operation) {
	return kReStatusFailed;
}

uint16 ReferenceEngine::reToken(ReTokenSet *tokenSet, uint16 option, uint16 operation) {
	return kReStatusFailed;
}

} // End of namespace Topgun
