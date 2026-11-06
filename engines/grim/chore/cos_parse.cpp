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

int Cos::readLine(Common::SeekableReadStream *pFile, char *pBuffer, int size) {
	int gotLine = 0;

	while (size != 1) {
		int count = 0;

		while (count < size - 2 && pFile->pos() < pFile->size()) {
			char c = pFile->readByte();

			if (c == '\r' && pFile->pos() < pFile->size()) {
				if (pFile->readByte() == '\n')
					c = '\n';
				else
					pFile->seek(-1, SEEK_CUR);
			}

			pBuffer[count++] = c;

			if (c == '\n')
				break;
		}

		if (!count)
			return 0;

		pBuffer[count] = 0;
		_lineNumber++;

		char first = *pBuffer;

		if (first != ';' && first != '#' && first != '\n') {
			char *pComment = strchr(pBuffer, '#');

			if (pComment)
				*pComment = 0;

			if (*pBuffer)
				gotLine = 1;
		}

		if (gotLine)
			return 1;
	}

	return 1;
}

int Cos::parseTags(Common::SeekableReadStream *pFile) {
	int numTags;

	if (!readLine(pFile, _lineBuffer, 1024))
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

		if (!readLine(pFile, _lineBuffer, 1024) || sscanf(_lineBuffer, " %d '%4c' %11s %31[^\n\r]", &tagIndex, fourccChars, typeName, tagName) != 4)
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

		_aTagMap[tagIndex] = classIdx;
	}

	return 1;
}

int Cos::parseComponents(Common::SeekableReadStream *pFile, int hCostume) {
	CosCostume *pCostume = ptrFromHandle(hCostume);

	if (!pCostume) {
		warning("Cos::parseComponents(): invalid costume handle %d", hCostume);
		return 0;
	}

	if (!readLine(pFile, _lineBuffer, 1024))
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

	CosComponent *aTemp = new CosComponent[numComps]();

	if (_compMapSize < numComps) {
		int *aCompMap = new int[numComps]();

		for (int i = 0; i < _compMapSize; i++)
			aCompMap[i] = _aCompMap[i];

		delete[] _aCompMap;
		_aCompMap = aCompMap;
		_compMapSize = numComps;
	}

	for (int i = 0; i < numComps; i++) {
		int compNum;
		int tagNum;

		if (!readLine(pFile, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " %d %d %d %d %31[^\n\r]", &compNum, &tagNum, &aTemp[i].depthIdx, &aTemp[i].parent, aTemp[i].name) != 5) {
			delete[] aTemp;
			return 0;
		}

		if (tagNum < 0 || tagNum >= 100 || _aTagMap[tagNum] < 0 || _aTagMap[tagNum] >= _numClasses) {
			warning("Cos::parseComponents(): component %d uses an unknown tag %d", i, tagNum);
			delete[] aTemp;
			return 0;
		}

		aTemp[i].classIdx = _aTagMap[tagNum];
	}

	int *aTreeDepths = new int[numComps];
	int maxDepth = 0;

	for (int i = 0; i < numComps; i++) {
		int parent = aTemp[i].parent;
		int depth = 0;

		while (parent != -1) {
			if (parent < 0 || parent >= numComps || depth > numComps) {
				warning("Cos::parseComponents(): component %d has an invalid parent chain", i);
				delete[] aTemp;
				delete[] aTreeDepths;
				return 0;
			}

			parent = aTemp[parent].parent;
			depth++;
		}

		aTreeDepths[i] = depth;

		if (maxDepth <= depth)
			maxDepth = depth;
	}

	bool bFailed = false;

	for (int depth = 0; depth <= maxDepth && !bFailed; depth++) {
		for (int i = 0; i < numComps && !bFailed; i++) {
			if (aTreeDepths[i] != depth)
				continue;

			CosComponent *pTemp = &aTemp[i];
			int fourcc = _aClasses[pTemp->classIdx].fourcc;

			if (findComponentByParent(pCostume, fourcc, pTemp->depthIdx, pTemp->name, pTemp->parent) >= 0)
				continue;

			if (pTemp->parent < 0) {
				if (!addComponent(hCostume, pTemp, fourcc, nullptr, 0)) {
					bFailed = true;
					continue;
				}
			} else {
				CosComponent *pParentTemp = &aTemp[pTemp->parent];
				int parentIdx = findComponent(pCostume, _aClasses[pParentTemp->classIdx].fourcc, pParentTemp->depthIdx, pParentTemp->name);

				if (parentIdx < 0) {
					bFailed = true;
					continue;
				}

				CosComponent *pParent = &pCostume->aComponents[parentIdx];

				if (!addComponent(hCostume, pTemp, fourcc, pParent->id, _aClasses[pParent->classIdx].fourcc)) {
					bFailed = true;
					continue;
				}
			}

			_aCompMap[i] = findComponent(pCostume, fourcc, pTemp->depthIdx, pTemp->name);

			if (_aCompMap[i] < 0)
				bFailed = true;
		}
	}

	delete[] aTemp;
	delete[] aTreeDepths;

	return bFailed ? 0 : 1;
}

int Cos::parseChores(Common::SeekableReadStream *pFile, CosCostume *pCostume) {
	if (!readLine(pFile, _lineBuffer, 1024))
		return 0;

	if (sscanf(_lineBuffer, " numchores %d", &pCostume->numChores) != 1)
		return 0;

	if (pCostume->numChores < 0) {
		warning("Cos::parseChores(): negative chore count %d", pCostume->numChores);
		pCostume->numChores = 0;
		return 0;
	}

	if (pCostume->numChores)
		pCostume->aChores = new CosChore[pCostume->numChores]();
	else
		pCostume->aChores = nullptr;

	for (int i = 0; i < pCostume->numChores; i++)
		initChore(&pCostume->aChores[i]);

	for (int i = 0; i < pCostume->numChores; i++) {
		CosChore *pChore = &pCostume->aChores[i];
		int choreNum;

		if (!readLine(pFile, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " %d %d %d %31[^\n\r]", &choreNum, &pChore->length, &pChore->numTracks, pChore->name) != 4)
			return 0;

		if (pChore->numTracks < 0) {
			warning("Cos::parseChores(): negative track count %d in chore %d", pChore->numTracks, i);
			pChore->numTracks = 0;
			return 0;
		}

		if (pChore->numTracks)
			pChore->aTracks = new CosChoreTrack[pChore->numTracks]();
		else
			pChore->aTracks = nullptr;
	}

	return 1;
}

int Cos::parseKeys(Common::SeekableReadStream *pFile, CosCostume *pCostume) {
	for (int c = 0; c < pCostume->numChores; c++) {
		int choreIndex;

		if (!readLine(pFile, _lineBuffer, 1024) || sscanf(_lineBuffer, " chore %d", &choreIndex) != 1)
			return 0;

		if (choreIndex < 0 || choreIndex >= pCostume->numChores) {
			warning("Cos::parseKeys(): chore index %d is out of range", choreIndex);
			return 0;
		}

		CosChore *pChore = &pCostume->aChores[choreIndex];

		for (int t = 0; t < pChore->numTracks; t++) {
			CosChoreTrack *pTrack = &pChore->aTracks[t];

			if (!readLine(pFile, _lineBuffer, 1024) || sscanf(_lineBuffer, " %d %d", &pTrack->compIndex, &pTrack->numKeys) != 2)
				return 0;

			if (pTrack->compIndex < 0 || pTrack->compIndex >= _compMapSize) {
				warning("Cos::parseKeys(): component index %d is out of range", pTrack->compIndex);
				pTrack->numKeys = 0;
				return 0;
			}

			pTrack->compIndex = _aCompMap[pTrack->compIndex];

			if (pTrack->numKeys < 0) {
				warning("Cos::parseKeys(): negative key count %d", pTrack->numKeys);
				pTrack->numKeys = 0;
				return 0;
			}

			if (pTrack->numKeys)
				pTrack->aKeys = new CosChoreKey[pTrack->numKeys]();
			else
				pTrack->aKeys = nullptr;

			if (pTrack->compIndex < 0 || pTrack->compIndex >= pCostume->numComponents) {
				warning("Cos::parseKeys(): track %d of chore %d uses an unknown component", t, choreIndex);
				return 0;
			}

			CosComponentClass *pClass = &_aClasses[pCostume->aComponents[pTrack->compIndex].classIdx];

			for (int k = 0; k < pTrack->numKeys; k++) {
				CosChoreKey *pKey = &pTrack->aKeys[k];

				if (!readLine(pFile, _lineBuffer, 1024))
					return 0;

				if (pClass->type > kCosTagInt) {
					float value;

					if (sscanf(_lineBuffer, " %d %f", &pKey->time, &value) != 2)
						return 0;

					memcpy(&pKey->value, &value, sizeof(pKey->value));
				} else if (sscanf(_lineBuffer, " %d %d", &pKey->time, &pKey->value) != 2) {
					return 0;
				}
			}

			if (pTrack->numKeys)
				pTrack->length = pTrack->aKeys[pTrack->numKeys - 1].time;
			else
				pTrack->length = 0;
		}
	}

	return 1;
}

} // End of namespace Grim
