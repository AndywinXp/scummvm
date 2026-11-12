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

#include "engines/grim/chore/cos.h"

namespace Grim {

CosCostume *Cos::ptrFromHandle(int costume) {
	int index = handleToIndex(costume);

	if (index < 0 || index >= _maxCostumes)
		return nullptr;

	return &_costumes[index];
}

int Cos::findClass(int fourcc, const char *name, int type) {
	for (int i = 0; i < _numClasses; i++) {
		if (_classes[i].fourcc == fourcc && _classes[i].type == type && !scumm_stricmp(_classes[i].name, name))
			return i;
	}

	return -1;
}

int Cos::findComponent(CosCostume *costume, int fourcc, int depthIdx, const char *name) {
	for (int i = 0; i < costume->numComponents; i++) {
		CosComponent *comp = &costume->components[i];

		if (comp->depthIdx == depthIdx && _classes[comp->classIdx].fourcc == fourcc && !scumm_stricmp(comp->name, name))
			return i;
	}

	return -1;
}

int Cos::findComponentByParent(CosCostume *costume, int fourcc, int depthIdx, const char *name, int parent) {
	for (int i = 0; i < costume->numComponents; i++) {
		CosComponent *comp = &costume->components[i];

		if (comp->depthIdx == depthIdx && _classes[comp->classIdx].fourcc == fourcc && comp->parent == parent
				&& !scumm_stricmp(comp->name, name))
			return i;
	}

	return -1;
}

int Cos::initComponent(CosCostume *costume, CosComponent *comp, const char *name, int classIdx, void *id, int parent) {
	int result = 0;

	if (strlen(name) < 0x20) {
		Common::strlcpy(comp->name, name, 32);
		comp->id = id;
		comp->classIdx = classIdx;
		comp->parent = parent;
		comp->depthIdx = 0;

		for (result = findComponent(costume, _classes[classIdx].fourcc, 0, name); result >= 0;
				result = findComponent(costume, _classes[comp->classIdx].fourcc, comp->depthIdx, name))
			comp->depthIdx++;

		comp->numNames = 0;
		comp->nameIds = nullptr;
		comp->names = nullptr;
		comp->stateBitmaps = nullptr;
	}

	return result;
}

void Cos::setComponentNames(CosComponent *comp, const CosComponentNames *names) {
	if (_classes[comp->classIdx].type != kCosTagStateBased)
		return;

	if (!names || names->numNames == 0)
		return;

	if (names->numNames < 0) {
		warning("Cos::setComponentNames(): negative name count %d", names->numNames);
		return;
	}

	int numNames = names->numNames;
	comp->numNames = numNames;

	if (names->nameIds) {
		comp->nameIds = new int[numNames];
		memcpy(comp->nameIds, names->nameIds, numNames * sizeof(int));
	}

	if (names->names) {
		comp->names = new char *[numNames];

		for (int i = 0; i < numNames; i++) {
			size_t size = strlen(names->names[i]) + 1;
			comp->names[i] = new char[size];
			Common::strlcpy(comp->names[i], names->names[i], size);
		}
	}

	if (names->stateBitmaps) {
		comp->stateBitmaps = new CosStateBitmap *[numNames];

		for (int i = 0; i < numNames; i++) {
			CosStateBitmap *bitmap = new CosStateBitmap;
			const CosStateBitmapDesc *desc = &names->stateBitmaps[i];
			comp->stateBitmaps[i] = bitmap;

			bitmap->pixels = desc->pixels;
			bitmap->width = desc->width;
			bitmap->height = desc->height;
			bitmap->reserved12 = desc->reserved12;

			if (_bitmapMunger) {
				if (names->bitmapExtra)
					bitmap->mungerResult = _bitmapMunger(desc, names->bitmapExtra[i]);
				else
					bitmap->mungerResult = _bitmapMunger(desc, 0);
			} else {
				bitmap->mungerResult = 0;
			}
		}
	}
}

int Cos::registerComponent(int costumeHandle, const char *name, void *id, int fourcc, void *parentId, int parentFourcc, const CosComponentNames *names) {
	int index = handleToIndex(costumeHandle);

	if (!name)
		return 0;

	if (index < 0 || index >= _maxCostumes) {
		warning("Cos::registerComponent(): invalid costume handle %d", costumeHandle);
		return 0;
	}

	if (strlen(name) >= 0x20) {
		warning("Cos::registerComponent(): component name %s is too long", name);
		return 0;
	}

	CosCostume *costume = &_costumes[index];
	int parentCompIndex = 0;

	if (parentId) {
		int parentClassIdx = 0;

		while (parentClassIdx < _numClasses && _classes[parentClassIdx].fourcc != parentFourcc)
			parentClassIdx++;

		if (parentClassIdx == _numClasses)
			return 0;

		while (parentCompIndex < costume->numComponents) {
			CosComponent *parent = &costume->components[parentCompIndex];

			if (_classes[parent->classIdx].fourcc == parentFourcc && parent->id == parentId)
				break;

			parentCompIndex++;
		}

		if (parentCompIndex == costume->numComponents)
			return 0;
	}

	int classIdx = 0;

	while (classIdx < _numClasses && _classes[classIdx].fourcc != fourcc)
		classIdx++;

	if (classIdx == _numClasses)
		return 0;

	CosComponent *components = new CosComponent[costume->numComponents + 1]();

	for (int i = 0; i < costume->numComponents; i++)
		components[i] = costume->components[i];

	delete[] costume->components;
	costume->components = components;

	CosComponent *comp = &components[costume->numComponents];
	initComponent(costume, comp, name, classIdx, id, parentId ? parentCompIndex : -1);
	costume->numComponents++;
	setComponentNames(comp, names);

	if (_compNotifier)
		_compNotifier(costume, comp, costume->numComponents - 1, 1);

	return 1;
}

int Cos::removeComponent(int costumeHandle, int fourcc, void *id) {
	CosCostume *costume = ptrFromHandle(costumeHandle);

	if (!costume)
		return 0;

	for (int i = 0; i < costume->numComponents; i++) {
		CosComponent *comp = &costume->components[i];

		if (comp->id == id && _classes[comp->classIdx].fourcc == fourcc)
			return deleteComponent(costumeHandle, i);
	}

	return 0;
}

int Cos::deleteComponent(int costumeHandle, int compIndex) {
	CosCostume *costume = ptrFromHandle(costumeHandle);

	if (!costume)
		return 0;

	int numComponents = costume->numComponents;

	if (compIndex < 0 || compIndex >= numComponents)
		return 0;

	int *deleteArray = new int[numComponents]();
	int *remap = new int[numComponents];

	for (int i = 0; i < numComponents; i++)
		remap[i] = i;

	deleteArray[compIndex] = 1;
	remap[compIndex] = -1;

	for (int i = 0; i < numComponents; i++) {
		int parent = costume->components[i].parent;
		int steps = 0;
		bool ancestorDeleted = false;

		while (parent != -1) {
			if (parent < 0 || parent >= numComponents || steps++ > numComponents) {
				warning("Cos::deleteComponent(): invalid parent chain at component %d", i);
				break;
			}

			if (deleteArray[parent]) {
				ancestorDeleted = true;
				break;
			}

			parent = costume->components[parent].parent;
		}

		if (ancestorDeleted) {
			deleteArray[i] = 1;
			remap[i] = -1;
		}
	}

	for (int i = 0; i < numComponents; i++) {
		if (!deleteArray[i])
			continue;

		CosComponent *comp = &costume->components[i];
		int nextDepth = comp->depthIdx + 2;
		int found = findComponent(costume, _classes[comp->classIdx].fourcc, comp->depthIdx + 1, comp->name);

		while (found >= 0) {
			costume->components[found].depthIdx--;
			found = findComponent(costume, _classes[comp->classIdx].fourcc, nextDepth, comp->name);
			nextDepth++;
		}
	}

	int numDeleted = 0;

	for (int i = 0; i < numComponents; i++) {
		if (deleteArray[i])
			numDeleted++;

		if (!deleteArray[i] && numDeleted) {
			remap[i] = i - numDeleted;
			costume->components[i - numDeleted] = costume->components[i];
		}
	}

	int newNumComponents = numComponents - numDeleted;
	CosComponent *components = nullptr;

	if (newNumComponents) {
		components = new CosComponent[newNumComponents]();

		for (int i = 0; i < newNumComponents; i++)
			components[i] = costume->components[i];
	}

	delete[] costume->components;
	costume->components = components;
	costume->numComponents = newNumComponents;

	for (int i = 0; i < newNumComponents; i++) {
		CosComponent *comp = &costume->components[i];

		if (comp->parent != -1) {
			if (comp->parent < 0 || comp->parent >= numComponents) {
				warning("Cos::deleteComponent(): component %d has an invalid parent", i);
				comp->parent = -1;
			} else {
				comp->parent = remap[comp->parent];
			}
		}
	}

	for (int c = 0; c < costume->numChores; c++) {
		CosChore *chore = &costume->chores[c];

		for (int t = 0; t < chore->numTracks; t++) {
			CosChoreTrack *track = &chore->tracks[t];

			if (track->compIndex < 0 || track->compIndex >= numComponents) {
				warning("Cos::deleteComponent(): track %d of chore %d has an invalid component", t, c);
				track->compIndex = -1;
			} else {
				track->compIndex = remap[track->compIndex];
			}
		}
	}

	for (int c = 0; c < costume->numChores; c++) {
		CosChore *chore = &costume->chores[c];
		int numTracks = chore->numTracks;
		int numRemoved = 0;

		for (int t = 0; t < numTracks; t++) {
			CosChoreTrack *track = &chore->tracks[t];

			if (track->compIndex == -1) {
				numRemoved++;

				if (track->numKeys) {
					delete[] track->keys;
					track->keys = nullptr;
				}
			} else if (numRemoved) {
				chore->tracks[t - numRemoved] = *track;
			}
		}

		int newNumTracks = numTracks - numRemoved;
		CosChoreTrack *tracks = nullptr;

		if (newNumTracks > 0) {
			tracks = new CosChoreTrack[newNumTracks]();

			for (int t = 0; t < newNumTracks; t++)
				tracks[t] = chore->tracks[t];
		}

		delete[] chore->tracks;
		chore->tracks = tracks;
		chore->numTracks = newNumTracks;
	}

	if (_compNotifier) {
		int numNotified = 0;

		for (int i = 0; i < numComponents; i++) {
			if (deleteArray[i])
				_compNotifier(costume, nullptr, i - numNotified++, 0);
		}
	}

	delete[] deleteArray;
	delete[] remap;

	return 1;
}

int Cos::addComponent(int costume, CosComponent *comp, int fourcc, void *parentId, int parentFourcc) {
	int classIdx = 0;

	while (classIdx < _numClasses && _classes[classIdx].fourcc != fourcc)
		classIdx++;

	if (classIdx == _numClasses)
		return 0;

	CosComponentClass *classInfo = &_classes[classIdx];

	if (classInfo->type < kCosTagStateBased || classInfo->type > kCosTagFloat) {
		warning("Cos::addComponent(): invalid tag type %d", classInfo->type);
		return comp != nullptr;
	}

	return classInfo->handler->request(costume, fourcc, parentId, parentFourcc, comp) != 0;
}

int Cos::countComponents(CosCostume *costume) {
	if (!costume) {
		warning("Cos::countComponents(): null costume");
		return 0;
	}

	return costume->numComponents;
}

CosComponent *Cos::componentInfo(CosCostume *costume, int compNum, int *fourcc, void **id, int *parent, int *type) {
	if (!costume || compNum < 0 || compNum >= costume->numComponents) {
		warning("Cos::componentInfo(): invalid costume or component number %d", compNum);
		return nullptr;
	}

	CosComponent *comp = &costume->components[compNum];
	CosComponentClass *classInfo = &_classes[comp->classIdx];

	if (fourcc)
		*fourcc = classInfo->fourcc;

	if (id)
		*id = comp->id;

	if (type)
		*type = classInfo->type;

	if (parent)
		*parent = comp->parent;

	return comp;
}

void Cos::initChore(CosChore *chore) {
	chore->length = 15;
	chore->numTracks = 0;
	chore->tracks = nullptr;
	Common::strlcpy(chore->name, "<unnamed>", 32);
}

void Cos::initChoreInstances(CosChore *chores, CosChoreInstance *inst, int count) {
	for (int i = 0; i < count; i++) {
		inst[i].state = 0;
		inst[i].choreIndex = i;
		inst[i].time = 0;
	}
}

void Cos::setChoreLength(CosCostume *costume, int choreNum, int length) {
	if (!costume || choreNum < 0 || choreNum >= costume->numChores) {
		warning("Cos::setChoreLength(): invalid costume or chore number %d", choreNum);
		return;
	}

	CosChore *chore = &costume->chores[choreNum];

	if ((uint32)length <= (uint32)chore->length) {
		for (int t = 0; t < chore->numTracks; t++) {
			CosChoreTrack *track = &chore->tracks[t];
			int k = 0;

			while (k < track->numKeys && (uint32)track->keys[k].time <= (uint32)chore->length)
				k++;

			if (k < track->numKeys) {
				CosChoreKey *keys = nullptr;

				if (k > 0) {
					keys = new CosChoreKey[k];

					for (int i = 0; i < k; i++)
						keys[i] = track->keys[i];
				}

				delete[] track->keys;
				track->keys = keys;
				track->numKeys = k;
			}
		}
	}

	chore->length = length;
}

} // End of namespace Grim
