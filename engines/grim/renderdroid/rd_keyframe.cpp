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

void RenderDroid::KeyframeModule::newEntry(RdKeyframe *keyframe) {
	memset(keyframe, 0, sizeof(RdKeyframe));
	Common::strlcpy(keyframe->name, "UNKNOWN", 32);
}

RdKeyframe *RenderDroid::KeyframeModule::load(char *path) {
	Common::SeekableReadStream *file = _rd->_hs->openFile(path);

	if (!file)
		return nullptr;

	RdKeyframe *keyframe = new RdKeyframe;
	uint32 magic = file->readUint32LE();

	if (magic != 0x4B455946) {
		delete file;

		if (loadEntry(path, keyframe))
			return keyframe;

		destroy(keyframe);
		return nullptr;
	}

	if (loadHeader(file, keyframe)) {
		Common::strlcpy(keyframe->name, _rd->_stdFile->fromPath(path), 32);
		delete file;
		return keyframe;
	}

	delete file;
	destroy(keyframe);

	return nullptr;
}

int RenderDroid::KeyframeModule::loadEntry(char *path, RdKeyframe *keyframe) {
	StdConffile *conffile = _rd->_stdConffile;
	char section[1024];
	char meshName[32];
	bool success = false;

	newEntry(keyframe);
	Common::strlcpy(keyframe->name, _rd->_stdFile->fromPath(path), 32);

	if (!conffile->open(path))
		return 0;

	do {
		if (!conffile->readLine() || sscanf(conffile->getLine(), " section: %s", section) != 1)
			break;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " flags %x", &keyframe->flags) != 1)
			break;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " type %x", &keyframe->type) != 1)
			break;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " frames %d", &keyframe->numFrames) != 1)
			break;

		float fps;
		if (!conffile->readLine() || sscanf(conffile->getLine(), " fps %f", &fps) != 1)
			break;
		keyframe->fps = fps;

		int numJoints;
		if (!conffile->readLine() || sscanf(conffile->getLine(), " joints %d", &numJoints) != 1)
			break;

		if (numJoints < 0) {
			warning("RenderDroid::KeyframeModule::loadEntry(): negative joint count in %s", path);
			break;
		}

		keyframe->paNodes = new RdKeyframeNode[numJoints];
		memset(keyframe->paNodes, 0, sizeof(RdKeyframeNode) * numJoints);
		keyframe->numJoints = numJoints;
		keyframe->numNodes = numJoints;

		if (!conffile->readLine() || sscanf(conffile->getLine(), " section: %s", section) != 1)
			break;

		if (!strcmp(section, "markers")) {
			uint numMarkers;
			if (!conffile->readLine() || sscanf(conffile->getLine(), " markers %d", &numMarkers) != 1)
				break;

			if (numMarkers > 8)
				break;

			keyframe->numMarkers = numMarkers;
			bool markersOk = true;

			for (uint i = 0; i < numMarkers; i++) {
				if (!conffile->readLine() || sscanf(conffile->getLine(), "%f %d", &keyframe->markerFrame[i], &keyframe->markerType[i]) != 2) {
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

			if (nodeIndex < 0 || nodeIndex >= (int)keyframe->numJoints) {
				warning("RenderDroid::KeyframeModule::loadEntry(): node index %d out of range in %s", nodeIndex, path);
				nodesOk = false;
				break;
			}

			RdKeyframeNode *node = &keyframe->paNodes[nodeIndex];
			Common::strlcpy(node->name, meshName, 32);

			if (!conffile->readLine() || sscanf(conffile->getLine(), " entries %d", &numEntries) != 1) {
				nodesOk = false;
				break;
			}

			if (numEntries < 0) {
				warning("RenderDroid::KeyframeModule::loadEntry(): negative entry count in %s", path);
				nodesOk = false;
				break;
			}

			node->nodeNum = nodeIndex;
			node->numEntries = numEntries;
			node->paEntries = new RdKeyframeEntry[numEntries];

			for (int j = 0; j < numEntries; j++) {
				RdKeyframeEntry *entry = &node->paEntries[j];
				int entryIndex;
				float values[6];

				if (!conffile->readLine() || sscanf(conffile->getLine(), " %d: %f %x %f %f %f %f %f %f", &entryIndex, &entry->frame, &entry->flags,
				        &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 9) {
					nodesOk = false;
					break;
				}

				entry->pos[0] = values[0];
				entry->pos[1] = values[1];
				entry->pos[2] = values[2];
				entry->pyr[0] = values[3];
				entry->pyr[1] = values[4];
				entry->pyr[2] = values[5];

				if (!conffile->readLine() || sscanf(conffile->getLine(), " %f %f %f %f %f %f", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 6) {
					nodesOk = false;
					break;
				}

				entry->dpos[0] = values[0];
				entry->dpos[1] = values[1];
				entry->dpos[2] = values[2];
				entry->dpyr[0] = values[3];
				entry->dpyr[1] = values[4];
				entry->dpyr[2] = values[5];
			}
		}

		if (!nodesOk)
			break;

		success = true;
	} while (false);

	conffile->close();

	return success ? 1 : 0;
}

int RenderDroid::KeyframeModule::loadHeader(Common::SeekableReadStream *file, RdKeyframe *keyframe) {
	file->read(keyframe->name, 32);
	keyframe->reserved32 =file->readSint32LE();
	keyframe->flags = file->readUint32LE();
	keyframe->numJoints = file->readUint32LE();
	keyframe->type = file->readUint32LE();
	keyframe->fps = file->readFloatLE();
	keyframe->numFrames = file->readUint32LE();
	keyframe->numNodes = file->readUint32LE();
	file->skip(4);
	keyframe->paNodes = nullptr;
	keyframe->numMarkers = file->readUint32LE();

	for (int i = 0; i < 8; i++)
		keyframe->markerFrame[i] = file->readFloatLE();

	for (int i = 0; i < 8; i++)
		keyframe->markerType[i] = file->readSint32LE();

	if (file->err() || file->eos() || (int64)keyframe->numNodes * 44 > file->size() - file->pos()) {
		warning("RenderDroid::KeyframeModule::loadHeader(): invalid keyframe header");
		keyframe->numNodes = 0;
		return 0;
	}

	keyframe->paNodes = new RdKeyframeNode[keyframe->numNodes];
	memset(keyframe->paNodes, 0, sizeof(RdKeyframeNode) * keyframe->numNodes);

	for (uint i = 0; i < keyframe->numNodes; i++) {
		RdKeyframeNode *node = &keyframe->paNodes[i];

		file->read(node->name, 32);
		node->nodeNum = file->readUint32LE();
		node->numEntries = file->readUint32LE();
		file->skip(4);
		node->paEntries = nullptr;

		if (!node->numEntries)
			continue;

		if ((int64)node->numEntries * 56 > file->size() - file->pos()) {
			warning("RenderDroid::KeyframeModule::loadHeader(): invalid entry count");
			node->numEntries = 0;
			return 0;
		}

		node->paEntries = new RdKeyframeEntry[node->numEntries];

		for (uint j = 0; j < node->numEntries; j++) {
			RdKeyframeEntry *entry = &node->paEntries[j];

			entry->frame = file->readFloatLE();
			entry->flags = file->readUint32LE();

			for (int k = 0; k < 3; k++)
				entry->pos[k] = file->readFloatLE();

			for (int k = 0; k < 3; k++)
				entry->pyr[k] = file->readFloatLE();

			for (int k = 0; k < 3; k++)
				entry->dpos[k] = file->readFloatLE();

			for (int k = 0; k < 3; k++)
				entry->dpyr[k] = file->readFloatLE();
		}
	}

	return 1;
}

void RenderDroid::KeyframeModule::destroy(RdKeyframe *&keyframe) {
	if (keyframe) {
		freeEntry(keyframe);
		delete keyframe;
		keyframe = nullptr;
	}
}

void RenderDroid::KeyframeModule::freeEntry(RdKeyframe *keyframe) {
	if (keyframe->paNodes) {
		for (uint i = 0; i < keyframe->numNodes; i++) {
			if (keyframe->paNodes[i].paEntries) {
				delete[] keyframe->paNodes[i].paEntries;
				keyframe->paNodes[i].paEntries = nullptr;
			}
		}

		delete[] keyframe->paNodes;
		keyframe->paNodes = nullptr;
	}
}

} // End of namespace Grim
