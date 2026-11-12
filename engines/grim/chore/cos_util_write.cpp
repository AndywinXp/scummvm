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

int CosUtil::write(const char *path, CosCostume *costume, const char *header) {
	if (!path || !costume) {
		warning("CosUtil::write(): null path or costume");
		return 0;
	}

	for (int c = 0; c < costume->numChores; c++) {
		CosChore *chore = &costume->chores[c];

		for (int i = 0; i < chore->numTracks; i++) {
			for (int j = 0; j < chore->numTracks; j++) {
				if (i != j && chore->tracks[i].compIndex == chore->tracks[j].compIndex)
					return 0;
			}
		}
	}

	int numTags = collectTags(costume);

	if (_cos->_saveHook) {
		int costumeIndex = -1;

		for (int i = 0; i < _cos->_maxCostumes; i++) {
			if (&_cos->_costumes[i] == costume) {
				costumeIndex = i;
				break;
			}
		}

		if (costumeIndex < 0) {
			warning("CosUtil::write(): the costume is not in the costume table");
			return 0;
		}

		if (!_cos->_saveHook(path, _cos->indexToHandle(costumeIndex), header))
			return 0;
	}

	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	const char *name = strrchr(path, '\\');

	if (name)
		name++;
	else
		name = path;

	if (strlen(name) > 0x20) {
		file.close();
		return 0;
	}

	Common::strlcpy(costume->name, name, 32);

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
		CosComponentClass *classInfo = &_cos->_classes[_cos->_tagMap[i]];
		char fourcc[5];
		const char *type;

		WRITE_LE_UINT32(fourcc, classInfo->fourcc);
		fourcc[4] = 0;

		if (classInfo->type == kCosTagStateBased)
			type = "fs";
		else if (classInfo->type == kCosTagInt)
			type = "int";
		else
			type = "float";

		file.writeString(Common::String::format("\t%d\t'%s'\t%s\t\t%s\n", i, fourcc, type, classInfo->name));
	}

	file.writeString("\n\nsection components\n");
	file.writeString(Common::String::format("\tnumcomponents %d\n\n", costume->numComponents));
	file.writeString("#\tID\tTagID\tHash\tParentID\tName\n");

	for (int i = 0; i < costume->numComponents; i++) {
		CosComponent *comp = &costume->components[i];
		int tagIndex = 0;

		while (tagIndex < numTags && _cos->_tagMap[tagIndex] != comp->classIdx)
			tagIndex++;

		if (tagIndex == numTags)
			warning("CosUtil::write(): j != numTags");

		file.writeString(Common::String::format("\t%d\t%d\t\t%d\t\t%d\t\t\t%s\n", i, tagIndex, comp->depthIdx, comp->parent, comp->name));
	}

	file.writeString("\n\nsection chores\n");
	file.writeString(Common::String::format("\tnumchores %d\n\n", costume->numChores));
	file.writeString("#\tID\tLength\tTracks\tName\n");

	for (int c = 0; c < costume->numChores; c++) {
		CosChore *chore = &costume->chores[c];
		int numUsedTracks = 0;

		for (int t = 0; t < chore->numTracks; t++) {
			if (chore->tracks[t].numKeys)
				numUsedTracks++;
		}

		file.writeString(Common::String::format("\t%d\t%d\t\t%d\t\t\t%s\n", c, chore->length, numUsedTracks, chore->name));
	}

	file.writeString("\n\nsection keys\n");

	for (int c = 0; c < costume->numChores; c++) {
		CosChore *chore = &costume->chores[c];

		file.writeString(Common::String::format("chore %d\t# %s\n", c, chore->name));

		for (int t = 0; t < chore->numTracks; t++) {
			CosChoreTrack *track = &chore->tracks[t];

			if (!track->numKeys)
				continue;

			if (track->numKeys < 0 || track->compIndex < 0 || track->compIndex >= costume->numComponents) {
				warning("CosUtil::write(): track %d of chore %d has an invalid component or key count", t, c);
				continue;
			}

			CosComponent *comp = &costume->components[track->compIndex];

			if (comp->classIdx < 0 || comp->classIdx >= _cos->_numClasses) {
				warning("CosUtil::write(): component %d has an invalid tag index %d", track->compIndex, comp->classIdx);
				continue;
			}

			CosComponentClass *classInfo = &_cos->_classes[comp->classIdx];

			file.writeString("#\tCompID\tNumKeys\n");
			file.writeString("#\t\tTime\t\tValue\n");
			file.writeString(Common::String::format("\t%d\t\t\t%d\t\t# %s\n", track->compIndex, track->numKeys, comp->name));

			for (int k = 0; k < track->numKeys; k++) {
				CosChoreKey *key = &track->keys[k];

				if (classInfo->type == kCosTagStateBased || classInfo->type == kCosTagInt) {
					file.writeString(Common::String::format("\t\t%d\t\t\t%d\n", key->time, key->value));
				} else if (classInfo->type == kCosTagFloat) {
					float value;
					memcpy(&value, &key->value, sizeof(value));
					file.writeString(Common::String::format("\t\t%d\t\t\t%f\n", key->time, (double)value));
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

int CosUtil::collectTags(CosCostume *costume) {
	int numTags = 0;

	for (int i = 0; i < costume->numComponents; i++) {
		int classIdx = costume->components[i].classIdx;

		if (classIdx < 0 || classIdx >= _cos->_numClasses) {
			warning("CosUtil::collectTags(): component %d has an invalid tag index %d", i, classIdx);
			continue;
		}

		int j = 0;

		while (j < numTags && _cos->_tagMap[j] != classIdx)
			j++;

		if (j == numTags) {
			_cos->_tagMap[numTags] = classIdx;
			numTags++;
		}
	}

	return numTags;
}

} // End of namespace Grim
