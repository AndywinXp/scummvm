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

#include "engines/grim/chore/cos_util.h"
#include "engines/grim/std/std_sort.h"

namespace Grim {

CosUtil::CosUtil(Cos *cos) : _cos(cos) {
}

CosComponent *CosUtil::compPtrFromHandle(CosCostume *costume, int compIndex) {
	if (!costume) {
		warning("CosUtil::compPtrFromHandle(): null costume");
		return nullptr;
	}

	if (compIndex < 0 || compIndex >= costume->numComponents)
		return nullptr;

	return &costume->components[compIndex];
}

int CosUtil::adjustComponent(CosCostume *costume, int whichComponent) {
	if (!costume || whichComponent < 0 || whichComponent >= costume->numComponents) {
		warning("CosUtil::adjustComponent(): invalid costume or component %d", whichComponent);
		return 0;
	}

	CosComponent *comp = &costume->components[whichComponent];

	if (comp->classIdx < 0 || comp->classIdx >= _cos->_numClasses) {
		warning("CosUtil::adjustComponent(): invalid tag index %d", comp->classIdx);
		return 0;
	}

	CosComponentClass *classInfo = &_cos->_classes[comp->classIdx];

	if (classInfo->type < kCosTagStateBased || classInfo->type > kCosTagFloat) {
		warning("CosUtil::adjustComponent(): invalid tag type %d", classInfo->type);
		return 0;
	}

	if (!classInfo->handler || !classInfo->handler->hasAdjuster()) {
		warning("CosUtil::adjustComponent(): tag %s has no adjuster", classInfo->name);
		return 0;
	}

	return classInfo->handler->adjust(&comp->id, comp->name, 32);
}

int CosUtil::componentHasAdjuster(CosCostume *costume, int whichComponent) {
	if (!costume || whichComponent < 0 || whichComponent >= costume->numComponents) {
		warning("CosUtil::componentHasAdjuster(): invalid costume or component %d", whichComponent);
		return 0;
	}

	CosComponent *comp = &costume->components[whichComponent];

	if (comp->classIdx < 0 || comp->classIdx >= _cos->_numClasses) {
		warning("CosUtil::componentHasAdjuster(): invalid tag index %d", comp->classIdx);
		return 0;
	}

	CosComponentClass *classInfo = &_cos->_classes[comp->classIdx];

	if (classInfo->type < kCosTagStateBased || classInfo->type > kCosTagFloat)
		return 0;

	if (classInfo->handler && classInfo->handler->hasAdjuster())
		return 1;

	return 0;
}

int CosUtil::addChore(CosCostume *costume) {
	if (!costume) {
		warning("CosUtil::addChore(): null costume");
		return -1;
	}

	CosChore *chores = new CosChore[costume->numChores + 1]();

	for (int i = 0; i < costume->numChores; i++)
		chores[i] = costume->chores[i];

	delete[] costume->chores;
	costume->chores = chores;
	costume->numChores++;
	_cos->initChore(&chores[costume->numChores - 1]);

	return costume->numChores - 1;
}

void CosUtil::deleteChore(CosCostume *costume, int choreNum) {
	if (!costume || choreNum < 0 || choreNum >= costume->numChores) {
		warning("CosUtil::deleteChore(): invalid costume or chore number %d", choreNum);
		return;
	}

	CosChore *chore = &costume->chores[choreNum];

	if (chore->numTracks) {
		for (int t = 0; t < chore->numTracks; t++) {
			if (chore->tracks[t].numKeys) {
				delete[] chore->tracks[t].keys;
				chore->tracks[t].keys = nullptr;
			}
		}

		delete[] chore->tracks;
		chore->tracks = nullptr;
	}

	int newNumChores = costume->numChores - 1;
	CosChore *chores = nullptr;

	if (newNumChores > 0) {
		chores = new CosChore[newNumChores]();

		for (int i = 0; i < choreNum; i++)
			chores[i] = costume->chores[i];

		for (int i = choreNum + 1; i < costume->numChores; i++)
			chores[i - 1] = costume->chores[i];
	}

	delete[] costume->chores;
	costume->chores = chores;
	costume->numChores = newNumChores;
}

void CosUtil::nameChore(CosCostume *costume, int choreNum, const char *name) {
	if (!costume || !name || choreNum < 0 || choreNum >= costume->numChores) {
		warning("CosUtil::nameChore(): invalid costume, name or chore number %d", choreNum);
		return;
	}

	CosChore *chore = &costume->chores[choreNum];
	size_t length = strlen(name);

	if (length < 32) {
		Common::strlcpy(chore->name, name, 32);

		for (size_t i = 0; i < length; i++) {
			if (chore->name[i] == ' ')
				chore->name[i] = '_';
		}
	}
}

int CosUtil::deleteKey(CosCostume *costume, int whichChore, int whichComponent, int time) {
	if (!costume || whichChore < 0 || whichChore >= costume->numChores
			|| whichComponent < 0 || whichComponent >= costume->numComponents) {
		warning("CosUtil::deleteKey(): invalid costume, chore %d or component %d", whichChore, whichComponent);
		return 0;
	}

	CosChore *chore = &costume->chores[whichChore];
	int trackIndex = 0;

	while (trackIndex < chore->numTracks && chore->tracks[trackIndex].compIndex != whichComponent)
		trackIndex++;

	if (trackIndex == chore->numTracks)
		return 0;

	CosChoreTrack *track = &chore->tracks[trackIndex];
	int keyIndex = 0;

	while (keyIndex < track->numKeys && track->keys[keyIndex].time != time)
		keyIndex++;

	if (keyIndex >= track->numKeys)
		return 0;

	int newNumKeys = track->numKeys - 1;
	CosChoreKey *keys = nullptr;

	if (newNumKeys) {
		keys = new CosChoreKey[newNumKeys]();

		for (int i = 0; i < keyIndex; i++)
			keys[i] = track->keys[i];

		for (int i = keyIndex + 1; i < track->numKeys; i++)
			keys[i - 1] = track->keys[i];
	}

	delete[] track->keys;
	track->keys = keys;
	track->numKeys = newNumKeys;

	return 1;
}

CosChoreKey *CosUtil::queryValueAtTime(CosCostume *costume, int whichChore, int whichComponent, int time) {
	if (!costume || whichChore < 0 || whichChore >= costume->numChores
			|| whichComponent < 0 || whichComponent >= costume->numComponents) {
		warning("CosUtil::queryValueAtTime(): invalid costume, chore %d or component %d", whichChore, whichComponent);
		return nullptr;
	}

	CosChore *chore = &costume->chores[whichChore];
	int trackIndex = 0;

	while (trackIndex < chore->numTracks && chore->tracks[trackIndex].compIndex != whichComponent)
		trackIndex++;

	if (trackIndex == chore->numTracks)
		return nullptr;

	CosChoreTrack *track = &chore->tracks[trackIndex];

	for (int k = 0; k < track->numKeys; k++) {
		if ((uint32)track->keys[k].time == (uint32)time)
			return &track->keys[k];

		if ((uint32)track->keys[k].time > (uint32)time) {
			if (k == 0)
				return nullptr;

			return &track->keys[k - 1];
		}
	}

	if (track->numKeys == 0)
		return nullptr;

	return &track->keys[track->numKeys - 1];
}

int CosUtil::addFSKey(CosCostume *costume, int choreNum, int compNum, int time, int value) {
	if (!costume || compNum < 0 || compNum >= costume->numComponents) {
		warning("CosUtil::addFSKey(): invalid costume or component %d", compNum);
		return 0;
	}

	int classIdx = costume->components[compNum].classIdx;

	if (classIdx < 0 || classIdx >= _cos->_numClasses) {
		warning("CosUtil::addFSKey(): invalid tag index %d", classIdx);
		return 0;
	}

	if (_cos->_classes[classIdx].type != kCosTagStateBased)
		return 0;

	CosChoreKey *key = addKey(costume, choreNum, compNum, time);

	if (!key)
		return 0;

	key->value = value;

	return 1;
}

int CosUtil::addIntKey(CosCostume *costume, int choreNum, int compNum, int time, int value) {
	if (!costume || compNum < 0 || compNum >= costume->numComponents) {
		warning("CosUtil::addIntKey(): invalid costume or component %d", compNum);
		return 0;
	}

	int classIdx = costume->components[compNum].classIdx;

	if (classIdx < 0 || classIdx >= _cos->_numClasses) {
		warning("CosUtil::addIntKey(): invalid tag index %d", classIdx);
		return 0;
	}

	if (_cos->_classes[classIdx].type != kCosTagInt)
		return 0;

	CosChoreKey *key = addKey(costume, choreNum, compNum, time);

	if (!key)
		return 0;

	key->value = value;

	return 1;
}

int CosUtil::addFloatKey(CosCostume *costume, int choreNum, int compNum, int time, float value) {
	if (!costume || compNum < 0 || compNum >= costume->numComponents) {
		warning("CosUtil::addFloatKey(): invalid costume or component %d", compNum);
		return 0;
	}

	int classIdx = costume->components[compNum].classIdx;

	if (classIdx < 0 || classIdx >= _cos->_numClasses) {
		warning("CosUtil::addFloatKey(): invalid tag index %d", classIdx);
		return 0;
	}

	if (_cos->_classes[classIdx].type != kCosTagFloat)
		return 0;

	CosChoreKey *key = addKey(costume, choreNum, compNum, time);

	if (!key)
		return 0;

	memcpy(&key->value, &value, sizeof(key->value));

	return 1;
}

int CosUtil::countChores(CosCostume *costume) {
	if (!costume) {
		warning("CosUtil::countChores(): null costume");
		return 0;
	}

	return costume->numChores;
}

const char *CosUtil::choreName(CosCostume *costume, int choreNum) {
	if (!costume || choreNum < 0 || choreNum >= costume->numChores) {
		warning("CosUtil::choreName(): invalid costume or chore number %d", choreNum);
		return nullptr;
	}

	return costume->chores[choreNum].name;
}

int CosUtil::countTags() {
	return _cos->_numClasses;
}

const char *CosUtil::tagInfo(int whichTag, int *fourcc) {
	if (whichTag < 0 || whichTag >= _cos->_numClasses) {
		warning("CosUtil::tagInfo(): invalid tag number %d", whichTag);
		return nullptr;
	}

	if (fourcc)
		*fourcc = _cos->_classes[whichTag].fourcc;

	return _cos->_classes[whichTag].name;
}

CosChoreKey *CosUtil::addKey(CosCostume *costume, int choreNum, int compNum, int time) {
	if (!costume || choreNum < 0 || choreNum >= costume->numChores) {
		warning("CosUtil::addKey(): invalid costume or chore number %d", choreNum);
		return nullptr;
	}

	CosChore *chore = &costume->chores[choreNum];
	int trackIndex = 0;

	while (trackIndex < chore->numTracks && chore->tracks[trackIndex].compIndex != compNum)
		trackIndex++;

	if (trackIndex == chore->numTracks) {
		CosChoreTrack *tracks = new CosChoreTrack[chore->numTracks + 1]();

		for (int t = 0; t < chore->numTracks; t++)
			tracks[t] = chore->tracks[t];

		delete[] chore->tracks;
		chore->tracks = tracks;
		chore->numTracks++;

		tracks[trackIndex].numKeys = 0;
		tracks[trackIndex].keys = nullptr;
		tracks[trackIndex].length = 0;
		tracks[trackIndex].compIndex = compNum;
	}

	CosChoreTrack *track = &chore->tracks[trackIndex];
	CosChoreKey searchKey;
	searchKey.time = time;
	searchKey.value = 0;

	CosChoreKey *key = nullptr;

	if (track->numKeys)
		key = (CosChoreKey *)bsearch(&searchKey, track->keys, track->numKeys, sizeof(CosChoreKey), Cos::compareFunction);

	if (!key) {
		CosChoreKey *keys = new CosChoreKey[track->numKeys + 1]();

		for (int k = 0; k < track->numKeys; k++)
			keys[k] = track->keys[k];

		delete[] track->keys;
		track->keys = keys;
		track->numKeys++;
		key = &keys[track->numKeys - 1];
	}

	key->time = time;

	if ((uint32)time > (uint32)track->length)
		track->length = time;

	uint32 minLength = time ? (uint32)time : 1;

	if (minLength > (uint32)chore->length)
		chore->length = (int)minLength;

	sort(track->keys, track->numKeys, sizeof(CosChoreKey), Cos::compareFunction);

	for (int i = 0; i < chore->numTracks; i++) {
		for (int j = 0; j < chore->numTracks; j++) {
			if (i != j && chore->tracks[i].compIndex == chore->tracks[j].compIndex)
				return nullptr;
		}
	}

	return (CosChoreKey *)bsearch(&searchKey, track->keys, track->numKeys, sizeof(CosChoreKey), Cos::compareFunction);
}

} // End of namespace Grim
