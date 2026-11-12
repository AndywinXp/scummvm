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
#include "common/str.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/chore/cos.h"

namespace Grim {

int Cos::readLine(Common::SeekableReadStream *file, char *buffer, int size) {
	int gotLine = 0;

	while (size != 1) {
		int count = 0;

		while (count < size - 2 && file->pos() < file->size()) {
			char c = file->readByte();

			if (c == '\r' && file->pos() < file->size()) {
				if (file->readByte() == '\n')
					c = '\n';
				else
					file->seek(-1, SEEK_CUR);
			}

			buffer[count++] = c;

			if (c == '\n')
				break;
		}

		if (!count)
			return 0;

		buffer[count] = 0;
		_lineNumber++;

		char first = *buffer;

		if (first != ';' && first != '#' && first != '\n') {
			char *comment = strchr(buffer, '#');

			if (comment)
				*comment = 0;

			if (*buffer)
				gotLine = 1;
		}

		if (gotLine)
			return 1;
	}

	return 1;
}

int Cos::parseTags(Common::SeekableReadStream *file) {
	int numTags;

	if (!readLine(file, _lineBuffer, 1024))
		return 0;

	if (sscanf(_lineBuffer, " numtags %d", &numTags) != 1)
		return 0;

	if (numTags < 0) {
		warning("Cos::parseTags(): negative tag count %d", numTags);
		return 0;
	}

	for (int i = 0; i < numTags; i++) {
		int tagIndex;
		char fourccChars[4];
		char typeName[12];
		char tagName[32];

		if (!readLine(file, _lineBuffer, 1024) || sscanf(_lineBuffer, " %d '%4c' %11s %31[^\n\r]", &tagIndex, fourccChars, typeName, tagName) != 4)
			return 0;

		int type;

		if (!scumm_stricmp(typeName, "fs"))
			type = kCosTagStateBased;
		else if (!scumm_stricmp(typeName, "int"))
			type = kCosTagInt;
		else if (!scumm_stricmp(typeName, "float"))
			type = kCosTagFloat;
		else
			return 0;

		int classIdx = findClass(READ_LE_UINT32(fourccChars), tagName, type);

		if (tagIndex < 0 || tagIndex >= 100) {
			warning("Cos::parseTags(): tag index %d is out of range", tagIndex);
			return 0;
		}

		_tagMap[tagIndex] = classIdx;
	}

	return 1;
}

int Cos::parseComponents(Common::SeekableReadStream *file, int costumeHandle) {
	CosCostume *costume = ptrFromHandle(costumeHandle);

	if (!costume) {
		warning("Cos::parseComponents(): invalid costume handle %d", costumeHandle);
		return 0;
	}

	if (!readLine(file, _lineBuffer, 1024))
		return 0;

	int numComps;

	if (sscanf(_lineBuffer, " numcomponents %d", &numComps) != 1)
		return 0;

	if (numComps < 0) {
		warning("Cos::parseComponents(): negative component count %d", numComps);
		return 0;
	}

	if (numComps == 0)
		return 0;

	CosComponent *tempArray = new CosComponent[numComps]();

	if (_compMapSize < numComps) {
		int *compMap = new int[numComps]();

		for (int i = 0; i < _compMapSize; i++)
			compMap[i] = _compMap[i];

		delete[] _compMap;
		_compMap = compMap;
		_compMapSize = numComps;
	}

	for (int i = 0; i < numComps; i++) {
		int compNum;
		int tagNum;

		if (!readLine(file, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " %d %d %d %d %31[^\n\r]", &compNum, &tagNum, &tempArray[i].depthIdx, &tempArray[i].parent, tempArray[i].name) != 5) {
			delete[] tempArray;
			return 0;
		}

		if (tagNum < 0 || tagNum >= 100 || _tagMap[tagNum] < 0 || _tagMap[tagNum] >= _numClasses) {
			warning("Cos::parseComponents(): component %d uses an unknown tag %d", i, tagNum);
			delete[] tempArray;
			return 0;
		}

		tempArray[i].classIdx = _tagMap[tagNum];
	}

	int *treeDepths = new int[numComps];
	int maxDepth = 0;

	for (int i = 0; i < numComps; i++) {
		int parent = tempArray[i].parent;
		int depth = 0;

		while (parent != -1) {
			if (parent < 0 || parent >= numComps || depth > numComps) {
				warning("Cos::parseComponents(): component %d has an invalid parent chain", i);
				delete[] tempArray;
				delete[] treeDepths;
				return 0;
			}

			parent = tempArray[parent].parent;
			depth++;
		}

		treeDepths[i] = depth;

		if (maxDepth <= depth)
			maxDepth = depth;
	}

	bool failed = false;

	for (int depth = 0; depth <= maxDepth && !failed; depth++) {
		for (int i = 0; i < numComps && !failed; i++) {
			if (treeDepths[i] != depth)
				continue;

			CosComponent *temp = &tempArray[i];
			int fourcc = _classes[temp->classIdx].fourcc;

			if (findComponentByParent(costume, fourcc, temp->depthIdx, temp->name, temp->parent) >= 0)
				continue;

			if (temp->parent < 0) {
				if (!addComponent(costumeHandle, temp, fourcc, nullptr, 0)) {
					failed = true;
					continue;
				}
			} else {
				CosComponent *parentTemp = &tempArray[temp->parent];
				int parentIdx = findComponent(costume, _classes[parentTemp->classIdx].fourcc, parentTemp->depthIdx, parentTemp->name);

				if (parentIdx < 0) {
					failed = true;
					continue;
				}

				CosComponent *parentPointer = &costume->components[parentIdx];

				if (!addComponent(costumeHandle, temp, fourcc, parentPointer->id, _classes[parentPointer->classIdx].fourcc)) {
					failed = true;
					continue;
				}
			}

			_compMap[i] = findComponent(costume, fourcc, temp->depthIdx, temp->name);

			if (_compMap[i] < 0)
				failed = true;
		}
	}

	delete[] tempArray;
	delete[] treeDepths;

	return failed ? 0 : 1;
}

int Cos::parseChores(Common::SeekableReadStream *file, CosCostume *costume) {
	if (!readLine(file, _lineBuffer, 1024))
		return 0;

	if (sscanf(_lineBuffer, " numchores %d", &costume->numChores) != 1)
		return 0;

	if (costume->numChores < 0) {
		warning("Cos::parseChores(): negative chore count %d", costume->numChores);
		costume->numChores = 0;
		return 0;
	}

	if (costume->numChores)
		costume->chores = new CosChore[costume->numChores]();
	else
		costume->chores = nullptr;

	for (int i = 0; i < costume->numChores; i++)
		initChore(&costume->chores[i]);

	for (int i = 0; i < costume->numChores; i++) {
		CosChore *chore = &costume->chores[i];
		int choreNum;

		if (!readLine(file, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " %d %d %d %31[^\n\r]", &choreNum, &chore->length, &chore->numTracks, chore->name) != 4)
			return 0;

		if (chore->numTracks < 0) {
			warning("Cos::parseChores(): negative track count %d in chore %d", chore->numTracks, i);
			chore->numTracks = 0;
			return 0;
		}

		if (chore->numTracks)
			chore->tracks = new CosChoreTrack[chore->numTracks]();
		else
			chore->tracks = nullptr;
	}

	return 1;
}

int Cos::parseKeys(Common::SeekableReadStream *file, CosCostume *costume) {
	for (int c = 0; c < costume->numChores; c++) {
		int choreIndex;

		if (!readLine(file, _lineBuffer, 1024) || sscanf(_lineBuffer, " chore %d", &choreIndex) != 1)
			return 0;

		if (choreIndex < 0 || choreIndex >= costume->numChores) {
			warning("Cos::parseKeys(): chore index %d is out of range", choreIndex);
			return 0;
		}

		CosChore *chore = &costume->chores[choreIndex];

		for (int t = 0; t < chore->numTracks; t++) {
			CosChoreTrack *track = &chore->tracks[t];

			if (!readLine(file, _lineBuffer, 1024) || sscanf(_lineBuffer, " %d %d", &track->compIndex, &track->numKeys) != 2)
				return 0;

			if (track->compIndex < 0 || track->compIndex >= _compMapSize) {
				warning("Cos::parseKeys(): component index %d is out of range", track->compIndex);
				track->numKeys = 0;
				return 0;
			}

			track->compIndex = _compMap[track->compIndex];

			if (track->numKeys < 0) {
				warning("Cos::parseKeys(): negative key count %d", track->numKeys);
				track->numKeys = 0;
				return 0;
			}

			if (track->numKeys)
				track->keys = new CosChoreKey[track->numKeys]();
			else
				track->keys = nullptr;

			if (track->compIndex < 0 || track->compIndex >= costume->numComponents) {
				warning("Cos::parseKeys(): track %d of chore %d uses an unknown component", t, choreIndex);
				return 0;
			}

			CosComponentClass *classInfo = &_classes[costume->components[track->compIndex].classIdx];

			for (int k = 0; k < track->numKeys; k++) {
				CosChoreKey *key = &track->keys[k];

				if (!readLine(file, _lineBuffer, 1024))
					return 0;

				if (classInfo->type > kCosTagInt) {
					float value;

					if (sscanf(_lineBuffer, " %d %f", &key->time, &value) != 2)
						return 0;

					memcpy(&key->value, &value, sizeof(key->value));
				} else if (sscanf(_lineBuffer, " %d %d", &key->time, &key->value) != 2) {
					return 0;
				}
			}

			if (track->numKeys)
				track->length = track->keys[track->numKeys - 1].time;
			else
				track->length = 0;
		}
	}

	return 1;
}

} // End of namespace Grim
