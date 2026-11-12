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

#include "engines/grim/costume/sputcostume.h"
#include "engines/grim/sputresource.h"

namespace Grim {

static const int keyframeKeyValues[15] = { 0, 1, 2, 3, 8, 9, 10, 4, 5, 6, 7, 11, 12, 13, 14 };
static const char *const keyframeKeyNames[15] = {
	"Play Once", "Play Looping", "Play and EndPause", "Play and EndFade", "Stop", "Pause", "Unpause",
	"1.0 Fade in", "0.5 Fade in", "0.25 Fade in", "0.125 Fade in",
	"1.0 Fade out", "0.5 Fade out", "0.25 Fade out", "0.125 Fade out"
};
static const CosComponentNames keyframeKeyDesc = { 15, keyframeKeyValues, keyframeKeyNames, nullptr, nullptr };

void KeyframeHandler::change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) {
	KeyframeInst *inst = static_cast<KeyframeInst *>(instData);
	RenderDroid *rd = _sputCostume->_rd;
	RdPuppet *puppet = inst->modelInst->thing->puppet;

	if (!inst->ref.valid)
		warning("KeyframeHandler::change(): Bad keyframe Reference");

	void *handle = inst->ref.handle;
	int newTrack = 0;
	int trackNum = 0;

	while (trackNum < 6 && puppet->tracks[trackNum].keyframe != handle)
		trackNum++;

	if (keyValue < 0 || keyValue >= 15) {
		warning("KeyframeHandler::change(): invalid key %d", keyValue);
		return;
	}

	int action = keyframeKeyValues[keyValue];

	if (trackNum == 6) {
		if ((uint)action > 7)
			return;

		trackNum = rd->_puppet.addTrack(puppet, static_cast<RdKeyframe *>(handle), inst->priority, inst->priorityMasked);

		if (trackNum < 0)
			return;

		inst->modelInst->thing->userArg = inst->modelInst;
		_sputCostume->_res->setReference(&inst->modelInst->trackRefs[trackNum], handle);
		inst->modelInst->trackUserArg = instHandle;
		rd->_puppet.setCallback(puppet, trackNum, SputCostume::puppetHookCallback);
		newTrack = 1;
	}

	float delta = (float)((double)(uint)timeOffset * (double)0.001f);

	switch (action) {
	case 0:
		if (newTrack)
			rd->_puppet.advanceTrack(puppet, trackNum, delta);

		rd->_puppet.resetTrack(puppet, trackNum);
		rd->_puppet.setStatus(puppet, trackNum, 0x20);
		rd->_puppet.playTrack(puppet, trackNum);
		break;
	case 1:
		if (newTrack)
			rd->_puppet.advanceTrack(puppet, trackNum, delta);

		if ((puppet->tracks[trackNum].status & 0xC) == 0)
			rd->_puppet.playTrack(puppet, trackNum);

		break;
	case 2:
		if (newTrack)
			rd->_puppet.advanceTrack(puppet, trackNum, delta);

		rd->_puppet.resetTrack(puppet, trackNum);
		rd->_puppet.setStatus(puppet, trackNum, 0x40);
		rd->_puppet.playTrack(puppet, trackNum);
		break;
	case 3:
		if (newTrack)
			rd->_puppet.advanceTrack(puppet, trackNum, delta);

		rd->_puppet.resetTrack(puppet, trackNum);
		rd->_puppet.setStatus(puppet, trackNum, 0x80);
		rd->_puppet.playTrack(puppet, trackNum);
		break;
	case 4:
		rd->_puppet.fadeInTrack(puppet, trackNum, 1.0f);

		if (newTrack)
			rd->_puppet.advanceTrack(puppet, trackNum, delta);

		break;
	case 5:
		rd->_puppet.fadeInTrack(puppet, trackNum, 0.5f);

		if (newTrack)
			rd->_puppet.advanceTrack(puppet, trackNum, delta);

		break;
	case 6:
		rd->_puppet.fadeInTrack(puppet, trackNum, 0.25f);

		if (newTrack)
			rd->_puppet.advanceTrack(puppet, trackNum, delta);

		break;
	case 7:
		rd->_puppet.fadeInTrack(puppet, trackNum, 0.125f);

		if (newTrack)
			rd->_puppet.advanceTrack(puppet, trackNum, delta);

		break;
	case 8:
		if ((puppet->tracks[trackNum].status & 8) == 0)
			rd->_puppet.removeTrack(puppet, trackNum);

		break;
	case 9:
		rd->_puppet.setStatus(puppet, trackNum, 0x10);
		break;
	case 10:
		puppet->tracks[trackNum].status = (puppet->tracks[trackNum].status & 0xFFFFFFED) | 2;

		if (newTrack)
			rd->_puppet.advanceTrack(puppet, trackNum, delta);

		break;
	case 11:
		rd->_puppet.fadeOutTrack(puppet, trackNum, 1.0f);
		break;
	case 12:
		rd->_puppet.fadeOutTrack(puppet, trackNum, 0.5f);
		break;
	case 13:
		rd->_puppet.fadeOutTrack(puppet, trackNum, 0.25f);
		break;
	case 14:
		rd->_puppet.fadeOutTrack(puppet, trackNum, 0.125f);
		break;
	default:
		break;
	}
}

void DemoKeyframeHandler::change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) {
	KeyframeInst *inst = static_cast<KeyframeInst *>(instData);
	RenderDroid *rd = _sputCostume->_rd;
	RdPuppet *puppet = inst->modelInst->thing->puppet;
	int numTracks = rd->_puppet.getNumTracks();

	if (!inst->ref.valid)
		warning("DemoKeyframeHandler::change(): Bad keyframe Reference");

	void *handle = inst->ref.handle;
	int trackNum = 0;

	while (trackNum < numTracks && puppet->tracks[trackNum].keyframe != handle)
		trackNum++;

	if (keyValue < 0 || keyValue >= 15) {
		warning("DemoKeyframeHandler::change(): invalid key %d", keyValue);
		return;
	}

	int action = keyframeKeyValues[keyValue];

	if (trackNum == numTracks) {
		if ((uint)action > 7)
			return;

		trackNum = rd->_puppet.addTrack(puppet, static_cast<RdKeyframe *>(handle), inst->priority, inst->priorityMasked);

		if (trackNum < 0) {
			warning("Unable to add track for keyframe %p", handle);
			return;
		}

		inst->modelInst->thing->userArg = inst->modelInst;
		_sputCostume->_res->setReference(&inst->modelInst->trackRefs[trackNum], handle);
		inst->modelInst->trackUserArg = instHandle;
		rd->_puppet.setCallback(puppet, trackNum, SputCostume::puppetHookCallback);
	}

	switch (action) {
	case 0:
		rd->_puppet.resetTrack(puppet, trackNum);
		rd->_puppet.setStatus(puppet, trackNum, 0x20);
		rd->_puppet.playTrack(puppet, trackNum);
		break;
	case 1:
		if ((puppet->tracks[trackNum].status & 0xC) == 0)
			rd->_puppet.playTrack(puppet, trackNum);

		break;
	case 2:
		rd->_puppet.resetTrack(puppet, trackNum);
		rd->_puppet.setStatus(puppet, trackNum, 0x40);
		rd->_puppet.playTrack(puppet, trackNum);
		break;
	case 3:
		rd->_puppet.resetTrack(puppet, trackNum);
		rd->_puppet.setStatus(puppet, trackNum, 0x80);
		rd->_puppet.playTrack(puppet, trackNum);
		break;
	case 4:
		rd->_puppet.fadeInTrack(puppet, trackNum, 1.0f);
		break;
	case 5:
		rd->_puppet.fadeInTrack(puppet, trackNum, 0.5f);
		break;
	case 6:
		rd->_puppet.fadeInTrack(puppet, trackNum, 0.25f);
		break;
	case 7:
		rd->_puppet.fadeInTrack(puppet, trackNum, 0.125f);
		break;
	case 8:
		if ((puppet->tracks[trackNum].status & 8) == 0)
			rd->_puppet.removeTrack(puppet, trackNum);

		break;
	case 9:
		rd->_puppet.setStatus(puppet, trackNum, 0x10);
		break;
	case 10:
		puppet->tracks[trackNum].status = (puppet->tracks[trackNum].status & 0xFFFFFFED) | 2;
		break;
	case 11:
		rd->_puppet.fadeOutTrack(puppet, trackNum, 1.0f);
		break;
	case 12:
		rd->_puppet.fadeOutTrack(puppet, trackNum, 0.5f);
		break;
	case 13:
		rd->_puppet.fadeOutTrack(puppet, trackNum, 0.25f);
		break;
	case 14:
		rd->_puppet.fadeOutTrack(puppet, trackNum, 0.125f);
		break;
	default:
		break;
	}
}

void DemoKeyframeHandler::stop(void *instData) {
	KeyframeInst *inst = static_cast<KeyframeInst *>(instData);
	RdPuppet *puppet = inst->modelInst->thing->puppet;

	if (!inst->ref.valid)
		return;

	int trackNum = 0;

	while (puppet->tracks[trackNum].keyframe != inst->ref.handle) {
		trackNum++;

		if (trackNum >= _sputCostume->_rd->_puppet.getNumTracks())
			return;
	}

	if ((puppet->tracks[trackNum].status & 8) == 0)
		_sputCostume->_rd->_puppet.removeTrack(puppet, trackNum);
}

int KeyframeHandler::request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) {
	char name[32];
	int priority = 1;
	int priorityMasked = 5;

	memset(name, 0, sizeof(name));

	if (parentFourcc != kSputCostumeTagThing && parentFourcc != kSputCostumeTagMainThing)
		return 0;

	if (!parentId)
		return 0;

	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = comp->name;

	Common::strlcpy(name, args, 32);

	char *keyframeName = strtok(name, ",");

	if (!keyframeName) {
		warning("KeyframeHandler::request(): empty keyframe name");
		return 0;
	}

	void *keyframe = _sputCostume->_sputResource->alloc(kSputTagKeyf, keyframeName);
	char *priorityPointer = strtok(nullptr, ",");

	if (priorityPointer) {
		priority = atoi(priorityPointer);

		char *priorityMaskedPointer = strtok(nullptr, ",");

		if (priorityMaskedPointer)
			priorityMasked = atoi(priorityMaskedPointer);
		else
			warning("KeyframeHandler::request(): missing masked priority in %s", args);
	}

	if (!keyframe)
		return 0;

	KeyframeCompRes *res = new KeyframeCompRes();

	_sputCostume->_res->setReference(&res->ref, keyframe);
	res->priority = priority;
	res->priorityMasked = priorityMasked;

	return _sputCostume->_cos.registerComponent(costume, args, res, fourcc, parentId, parentFourcc, &keyframeKeyDesc) != 0;
}

void KeyframeHandler::freeComponent(int costume, int fourcc, void *compRes) {
	KeyframeCompRes *res = static_cast<KeyframeCompRes *>(compRes);

	_sputCostume->_res->removeReference(&res->ref);
	delete res;
}

void KeyframeHandler::stop(void *instData) {
	KeyframeInst *inst = static_cast<KeyframeInst *>(instData);
	RdPuppet *puppet = inst->modelInst->thing->puppet;

	if (!inst->ref.valid)
		return;

	int trackNum = 0;

	while (puppet->tracks[trackNum].keyframe != inst->ref.handle) {
		trackNum++;

		if (trackNum >= 6)
			return;
	}

	if ((puppet->tracks[trackNum].status & 8) == 0)
		_sputCostume->_rd->_puppet.removeTrack(puppet, trackNum);
}

int KeyframeHandler::adjust(void **compRes, char *args, int argsSize) {
	KeyframeCompRes *res = static_cast<KeyframeCompRes *>(*compRes);
	int values[2];

	values[0] = res->priorityMasked;
	values[1] = res->priority;

	int accepted = 0; // TODO: grimPlat_ControlPanel("Tagged Priority", "Normal Priority")

	if (!accepted)
		return 0;

	char *token = strtok(args, ",");

	if (!token) {
		warning("KeyframeHandler::adjust(): empty component name");
		return 0;
	}

	res->priorityMasked = values[0];
	res->priority = values[1];

	char text[32];

	snprintf(text, sizeof(text), "%s,%d,%d", token, values[1], values[0]);
	Common::strlcpy(token, text, 32);

	return 1;
}

void *KeyframeHandler::instance(int instHandle, int compIndex, int create) {
	Cos *cos = &_sputCostume->_cos;

	if (!create) {
		KeyframeInst *inst = static_cast<KeyframeInst *>(cos->instGetInstanceData(instHandle, compIndex, nullptr));

		_sputCostume->_res->removeReference(&inst->ref);
		delete inst;

		return nullptr;
	}

	int fourcc = 0;
	void *id = nullptr;
	int parentIndex = -1;

	cos->instComponentInfo(instHandle, compIndex, &fourcc, &id, &parentIndex);

	KeyframeCompRes *res = static_cast<KeyframeCompRes *>(id);

	if (!res->ref.valid)
		warning("KeyframeHandler::instance(): Bad keyframe Reference");

	KeyframeInst *inst = new KeyframeInst();

	_sputCostume->_res->setReference(&inst->ref, res->ref.handle);
	inst->priorityMasked = res->priorityMasked;
	inst->priority = res->priority;
	inst->modelInst = static_cast<ModelInst *>(cos->instGetInstanceData(instHandle, parentIndex, nullptr));

	return inst;
}

} // End of namespace Grim
