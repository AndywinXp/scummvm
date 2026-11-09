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
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/renderdroid/rd.h"

namespace Grim {

void RenderDroid::KeyframeModule::newEntry(RdKeyframe *pKeyframe) {
	memset(pKeyframe, 0, sizeof(RdKeyframe));
	Common::strlcpy(pKeyframe->aName, "UNKNOWN", 32);
}

RdKeyframe *RenderDroid::KeyframeModule::load(char *path) {
	Common::SeekableReadStream *pFile = _rd->_pHS->openFile(path);

	if (!pFile)
		return nullptr;

	RdKeyframe *pKeyframe = new RdKeyframe;
	uint32 magic = pFile->readUint32LE();

	if (magic != 0x4B455946) {
		delete pFile;

		if (loadEntry(path, pKeyframe))
			return pKeyframe;

		destroy(pKeyframe);
		return nullptr;
	}

	if (loadHeader(pFile, pKeyframe)) {
		Common::strlcpy(pKeyframe->aName, _rd->_stdFile->fromPath(path), 32);
		delete pFile;
		return pKeyframe;
	}

	delete pFile;
	destroy(pKeyframe);

	return nullptr;
}

int RenderDroid::KeyframeModule::loadEntry(char *path, RdKeyframe *pKeyframe) {
	StdConffile *conffile = _rd->_stdConffile;
	char section[1024];
	char meshName[32];
	bool success = false;

	newEntry(pKeyframe);
	Common::strlcpy(pKeyframe->aName, _rd->_stdFile->fromPath(path), 32);

	if (!conffile->open(path))
		return 0;

	do {
		if (!conffile->readLine() || sscanf(conffile->getLine(), " section: %s", section) != 1)
			break;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " flags %x", &pKeyframe->flags) != 1)
			break;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " type %x", &pKeyframe->type) != 1)
			break;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " frames %d", &pKeyframe->numFrames) != 1)
			break;

		float fps;
		if (!conffile->readLine() || sscanf(conffile->getLine(), " fps %f", &fps) != 1)
			break;
		pKeyframe->fps = fps;

		int numJoints;
		if (!conffile->readLine() || sscanf(conffile->getLine(), " joints %d", &numJoints) != 1)
			break;

		if (numJoints < 0) {
			warning("RenderDroid::KeyframeModule::loadEntry(): negative joint count in %s", path);
			break;
		}

		pKeyframe->paNodes = new RdKeyframeNode[numJoints];
		memset(pKeyframe->paNodes, 0, sizeof(RdKeyframeNode) * numJoints);
		pKeyframe->numJoints = numJoints;
		pKeyframe->numNodes = numJoints;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " section: %s", section) != 1)
			break;

		if (!strcmp(section, "markers")) {
			uint numMarkers;
			if (!conffile->readLine() || sscanf(conffile->getLine(), " markers %d", &numMarkers) != 1)
				break;

			if (numMarkers > 8)
				break;

			pKeyframe->numMarkers = numMarkers;
			bool markersOk = true;

			for (uint i = 0; i < numMarkers; i++) {
				if (!conffile->readLine() || sscanf(conffile->getLine(), "%f %d", &pKeyframe->aMarkerFrame[i], &pKeyframe->aMarkerType[i]) != 2) {
					markersOk = false;
					break;
				}
			}

			if (!markersOk)
				break;

			if (!conffile->readLine() || sscanf(conffile->getLine(), " section: %s", section) != 1)
				break;
		}

		uint numNodes;
		if (!conffile->readLine() || sscanf(conffile->getLine(), " nodes %d", &numNodes) != 1)
			break;

		bool nodesOk = true;

		for (uint i = 0; i < numNodes && nodesOk; i++) {
			int nodeIndex;
			int numEntries;

			if (!conffile->readLine() || sscanf(conffile->getLine(), " node %d", &nodeIndex) != 1) {
				nodesOk = false;
				break;
			}

			if (!conffile->readLine() || sscanf(conffile->getLine(), " mesh name %31s", meshName) != 1) {
				nodesOk = false;
				break;
			}

			if (nodeIndex < 0 || nodeIndex >= (int)pKeyframe->numJoints) {
				warning("RenderDroid::KeyframeModule::loadEntry(): node index %d out of range in %s", nodeIndex, path);
				nodesOk = false;
				break;
			}

			RdKeyframeNode *pNode = &pKeyframe->paNodes[nodeIndex];
			Common::strlcpy(pNode->aName, meshName, 32);

			if (!conffile->readLine() || sscanf(conffile->getLine(), " entries %d", &numEntries) != 1) {
				nodesOk = false;
				break;
			}

			if (numEntries < 0) {
				warning("RenderDroid::KeyframeModule::loadEntry(): negative entry count in %s", path);
				nodesOk = false;
				break;
			}

			pNode->nodeNum = nodeIndex;
			pNode->numEntries = numEntries;
			pNode->paEntries = new RdKeyframeEntry[numEntries];

			for (int j = 0; j < numEntries; j++) {
				RdKeyframeEntry *pEntry = &pNode->paEntries[j];
				int entryIndex;
				float values[6];

				if (!conffile->readLine() || sscanf(conffile->getLine(), " %d: %f %x %f %f %f %f %f %f", &entryIndex, &pEntry->frame, &pEntry->flags,
				        &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 9) {
					nodesOk = false;
					break;
				}

				pEntry->pos[0] = values[0];
				pEntry->pos[1] = values[1];
				pEntry->pos[2] = values[2];
				pEntry->pyr[0] = values[3];
				pEntry->pyr[1] = values[4];
				pEntry->pyr[2] = values[5];

				if (!conffile->readLine() || sscanf(conffile->getLine(), " %f %f %f %f %f %f", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 6) {
					nodesOk = false;
					break;
				}

				pEntry->dpos[0] = values[0];
				pEntry->dpos[1] = values[1];
				pEntry->dpos[2] = values[2];
				pEntry->dpyr[0] = values[3];
				pEntry->dpyr[1] = values[4];
				pEntry->dpyr[2] = values[5];
			}
		}

		if (!nodesOk)
			break;

		success = true;
	} while (false);

	conffile->close();

	return success ? 1 : 0;
}

int RenderDroid::KeyframeModule::loadHeader(Common::SeekableReadStream *pFile, RdKeyframe *pKeyframe) {
	pFile->read(pKeyframe->aName, 32);
	pKeyframe->reserved32 =pFile->readSint32LE();
	pKeyframe->flags = pFile->readUint32LE();
	pKeyframe->numJoints = pFile->readUint32LE();
	pKeyframe->type = pFile->readUint32LE();
	pKeyframe->fps = pFile->readFloatLE();
	pKeyframe->numFrames = pFile->readUint32LE();
	pKeyframe->numNodes = pFile->readUint32LE();
	pFile->skip(4);
	pKeyframe->paNodes = nullptr;
	pKeyframe->numMarkers = pFile->readUint32LE();

	for (int i = 0; i < 8; i++)
		pKeyframe->aMarkerFrame[i] = pFile->readFloatLE();

	for (int i = 0; i < 8; i++)
		pKeyframe->aMarkerType[i] = pFile->readSint32LE();

	if (pFile->err() || pFile->eos() || (int64)pKeyframe->numNodes * 44 > pFile->size() - pFile->pos()) {
		warning("RenderDroid::KeyframeModule::loadHeader(): invalid keyframe header");
		pKeyframe->numNodes = 0;
		return 0;
	}

	pKeyframe->paNodes = new RdKeyframeNode[pKeyframe->numNodes];
	memset(pKeyframe->paNodes, 0, sizeof(RdKeyframeNode) * pKeyframe->numNodes);

	for (uint i = 0; i < pKeyframe->numNodes; i++) {
		RdKeyframeNode *pNode = &pKeyframe->paNodes[i];

		pFile->read(pNode->aName, 32);
		pNode->nodeNum = pFile->readUint32LE();
		pNode->numEntries = pFile->readUint32LE();
		pFile->skip(4);
		pNode->paEntries = nullptr;

		if (!pNode->numEntries)
			continue;

		if ((int64)pNode->numEntries * 56 > pFile->size() - pFile->pos()) {
			warning("RenderDroid::KeyframeModule::loadHeader(): invalid entry count");
			pNode->numEntries = 0;
			return 0;
		}

		pNode->paEntries = new RdKeyframeEntry[pNode->numEntries];

		for (uint j = 0; j < pNode->numEntries; j++) {
			RdKeyframeEntry *pEntry = &pNode->paEntries[j];

			pEntry->frame = pFile->readFloatLE();
			pEntry->flags = pFile->readUint32LE();

			for (int k = 0; k < 3; k++)
				pEntry->pos[k] = pFile->readFloatLE();

			for (int k = 0; k < 3; k++)
				pEntry->pyr[k] = pFile->readFloatLE();

			for (int k = 0; k < 3; k++)
				pEntry->dpos[k] = pFile->readFloatLE();

			for (int k = 0; k < 3; k++)
				pEntry->dpyr[k] = pFile->readFloatLE();
		}
	}

	return 1;
}

void RenderDroid::KeyframeModule::destroy(RdKeyframe *&pKeyframe) {
	if (pKeyframe) {
		freeEntry(pKeyframe);
		delete pKeyframe;
		pKeyframe = nullptr;
	}
}

void RenderDroid::KeyframeModule::freeEntry(RdKeyframe *pKeyframe) {
	if (pKeyframe->paNodes) {
		for (uint i = 0; i < pKeyframe->numNodes; i++) {
			if (pKeyframe->paNodes[i].paEntries) {
				delete[] pKeyframe->paNodes[i].paEntries;
				pKeyframe->paNodes[i].paEntries = nullptr;
			}
		}

		delete[] pKeyframe->paNodes;
		pKeyframe->paNodes = nullptr;
	}
}

} // End of namespace Grim
