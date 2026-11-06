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

#include "common/endian.h"
#include "common/file.h"
#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/chore/cos_util.h"

namespace Grim {

int CosUtil::write(const char *path, CosCostume *pCostume, const char *header) {
	if (!path || !pCostume) {
		warning("CosUtil::write(): null path or costume");
		return 0;
	}

	for (int c = 0; c < pCostume->numChores; c++) {
		CosChore *pChore = &pCostume->aChores[c];

		for (int i = 0; i < pChore->numTracks; i++) {
			for (int j = 0; j < pChore->numTracks; j++) {
				if (i != j && pChore->aTracks[i].compIndex == pChore->aTracks[j].compIndex)
					return 0;
			}
		}
	}

	int numTags = collectTags(pCostume);

	if (_pCos->_pSaveHook) {
		int costumeIndex = -1;

		for (int i = 0; i < _pCos->_maxCostumes; i++) {
			if (&_pCos->_aCostumes[i] == pCostume) {
				costumeIndex = i;
				break;
			}
		}

		if (costumeIndex < 0) {
			warning("CosUtil::write(): the costume is not in the costume table");
			return 0;
		}

		if (!_pCos->_pSaveHook(path, _pCos->indexToHandle(costumeIndex), header))
			return 0;
	}

	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	const char *pName = strrchr(path, '\\');

	if (pName)
		pName++;
	else
		pName = path;

	if (strlen(pName) > 0x20) {
		file.close();
		return 0;
	}

	Common::strlcpy(pCostume->name, pName, 32);

	if (!header) {
		warning("CosUtil::write(): null header");
		header = "(null)";
	}

	file.writeString(Common::String::format("# %s\n", header));
	file.writeString(Common::String::format("costume v%d.%d\n\n\n", 0, 1));
	file.writeString("section tags\n");
	file.writeString(Common::String::format("\tnumtags %d\n\n", numTags));
	file.writeString("#\tID\tTag\t\t\tClass\tName\n");

	for (int i = 0; i < numTags; i++) {
		CosComponentClass *pClass = &_pCos->_aClasses[_pCos->_aTagMap[i]];
		char fourcc[5];
		const char *pType;

		WRITE_LE_UINT32(fourcc, pClass->fourcc);
		fourcc[4] = 0;

		if (pClass->type == kCosTagStateBased)
			pType = "fs";
		else if (pClass->type == kCosTagInt)
			pType = "int";
		else
			pType = "float";

		file.writeString(Common::String::format("\t%d\t'%s'\t%s\t\t%s\n", i, fourcc, pType, pClass->name));
	}

	file.writeString("\n\nsection components\n");
	file.writeString(Common::String::format("\tnumcomponents %d\n\n", pCostume->numComponents));
	file.writeString("#\tID\tTagID\tHash\tParentID\tName\n");

	for (int i = 0; i < pCostume->numComponents; i++) {
		CosComponent *pComp = &pCostume->aComponents[i];
		int tagIndex = 0;

		while (tagIndex < numTags && _pCos->_aTagMap[tagIndex] != pComp->classIdx)
			tagIndex++;

		if (tagIndex == numTags)
			warning("CosUtil::write(): j != numTags");

		file.writeString(Common::String::format("\t%d\t%d\t\t%d\t\t%d\t\t\t%s\n", i, tagIndex, pComp->depthIdx, pComp->parent, pComp->name));
	}

	file.writeString("\n\nsection chores\n");
	file.writeString(Common::String::format("\tnumchores %d\n\n", pCostume->numChores));
	file.writeString("#\tID\tLength\tTracks\tName\n");

	for (int c = 0; c < pCostume->numChores; c++) {
		CosChore *pChore = &pCostume->aChores[c];
		int numUsedTracks = 0;

		for (int t = 0; t < pChore->numTracks; t++) {
			if (pChore->aTracks[t].numKeys)
				numUsedTracks++;
		}

		file.writeString(Common::String::format("\t%d\t%d\t\t%d\t\t\t%s\n", c, pChore->length, numUsedTracks, pChore->name));
	}

	file.writeString("\n\nsection keys\n");

	for (int c = 0; c < pCostume->numChores; c++) {
		CosChore *pChore = &pCostume->aChores[c];

		file.writeString(Common::String::format("chore %d\t# %s\n", c, pChore->name));

		for (int t = 0; t < pChore->numTracks; t++) {
			CosChoreTrack *pTrack = &pChore->aTracks[t];

			if (!pTrack->numKeys)
				continue;

			if (pTrack->numKeys < 0 || pTrack->compIndex < 0 || pTrack->compIndex >= pCostume->numComponents) {
				warning("CosUtil::write(): track %d of chore %d has an invalid component or key count", t, c);
				continue;
			}

			CosComponent *pComp = &pCostume->aComponents[pTrack->compIndex];

			if (pComp->classIdx < 0 || pComp->classIdx >= _pCos->_numClasses) {
				warning("CosUtil::write(): component %d has an invalid tag index %d", pTrack->compIndex, pComp->classIdx);
				continue;
			}

			CosComponentClass *pClass = &_pCos->_aClasses[pComp->classIdx];

			file.writeString("#\tCompID\tNumKeys\n");
			file.writeString("#\t\tTime\t\tValue\n");
			file.writeString(Common::String::format("\t%d\t\t\t%d\t\t# %s\n", pTrack->compIndex, pTrack->numKeys, pComp->name));

			for (int k = 0; k < pTrack->numKeys; k++) {
				CosChoreKey *pKey = &pTrack->aKeys[k];

				if (pClass->type == kCosTagStateBased || pClass->type == kCosTagInt) {
					file.writeString(Common::String::format("\t\t%d\t\t\t%d\n", pKey->time, pKey->value));
				} else if (pClass->type == kCosTagFloat) {
					float value;
					memcpy(&value, &pKey->value, sizeof(value));
					file.writeString(Common::String::format("\t\t%d\t\t\t%f\n", pKey->time, (double)value));
				} else {
					warning("CosUtil::write(): FALSE");
				}
			}

			file.writeString("\n");
		}
	}

	file.close();

	return 1;
}

int CosUtil::collectTags(CosCostume *pCostume) {
	int numTags = 0;

	for (int i = 0; i < pCostume->numComponents; i++) {
		int classIdx = pCostume->aComponents[i].classIdx;

		if (classIdx < 0 || classIdx >= _pCos->_numClasses) {
			warning("CosUtil::collectTags(): component %d has an invalid tag index %d", i, classIdx);
			continue;
		}

		int j = 0;

		while (j < numTags && _pCos->_aTagMap[j] != classIdx)
			j++;

		if (j == numTags) {
			_pCos->_aTagMap[numTags] = classIdx;
			numTags++;
		}
	}

	return numTags;
}

} // End of namespace Grim
