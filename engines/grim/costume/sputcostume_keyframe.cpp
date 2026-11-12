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

void KeyframeHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	KeyframeInst *pInst = static_cast<KeyframeInst *>(pInstData);
	RenderDroid *pRd = _pSputCostume->_pRd;
	RdPuppet *pPuppet = pInst->pModelInst->pThing->pPuppet;

	if (!pInst->ref.valid)
		warning("KeyframeHandler::change(): Bad keyframe Reference");

	void *handle = pInst->ref.handle;
	int bNewTrack = 0;
	int trackNum = 0;

	while (trackNum < 6 && pPuppet->aTracks[trackNum].pKeyframe != handle)
		trackNum++;

	if (keyValue < 0 || keyValue >= 15) {
		warning("KeyframeHandler::change(): invalid key %d", keyValue);
		return;
	}

	int action = keyframeKeyValues[keyValue];

	if (trackNum == 6) {
		if ((uint)action > 7)
			return;

		trackNum = pRd->_puppet.addTrack(pPuppet, static_cast<RdKeyframe *>(handle), pInst->priority, pInst->priorityMasked);

		if (trackNum < 0)
			return;

		pInst->pModelInst->pThing->userArg = pInst->pModelInst;
		_pSputCostume->_pRes->setReference(&pInst->pModelInst->aTrackRefs[trackNum], handle);
		pInst->pModelInst->trackUserArg = hInst;
		pRd->_puppet.setCallback(pPuppet, trackNum, SputCostume::puppetHookCallback);
		bNewTrack = 1;
	}

	float delta = (float)((double)(uint)timeOffset * (double)0.001f);

	switch (action) {
	case 0:
		if (bNewTrack)
			pRd->_puppet.advanceTrack(pPuppet, trackNum, delta);

		pRd->_puppet.resetTrack(pPuppet, trackNum);
		pRd->_puppet.setStatus(pPuppet, trackNum, 0x20);
		pRd->_puppet.playTrack(pPuppet, trackNum);
		break;
	case 1:
		if (bNewTrack)
			pRd->_puppet.advanceTrack(pPuppet, trackNum, delta);

		if ((pPuppet->aTracks[trackNum].status & 0xC) == 0)
			pRd->_puppet.playTrack(pPuppet, trackNum);

		break;
	case 2:
		if (bNewTrack)
			pRd->_puppet.advanceTrack(pPuppet, trackNum, delta);

		pRd->_puppet.resetTrack(pPuppet, trackNum);
		pRd->_puppet.setStatus(pPuppet, trackNum, 0x40);
		pRd->_puppet.playTrack(pPuppet, trackNum);
		break;
	case 3:
		if (bNewTrack)
			pRd->_puppet.advanceTrack(pPuppet, trackNum, delta);

		pRd->_puppet.resetTrack(pPuppet, trackNum);
		pRd->_puppet.setStatus(pPuppet, trackNum, 0x80);
		pRd->_puppet.playTrack(pPuppet, trackNum);
		break;
	case 4:
		pRd->_puppet.fadeInTrack(pPuppet, trackNum, 1.0f);

		if (bNewTrack)
			pRd->_puppet.advanceTrack(pPuppet, trackNum, delta);

		break;
	case 5:
		pRd->_puppet.fadeInTrack(pPuppet, trackNum, 0.5f);

		if (bNewTrack)
			pRd->_puppet.advanceTrack(pPuppet, trackNum, delta);

		break;
	case 6:
		pRd->_puppet.fadeInTrack(pPuppet, trackNum, 0.25f);

		if (bNewTrack)
			pRd->_puppet.advanceTrack(pPuppet, trackNum, delta);

		break;
	case 7:
		pRd->_puppet.fadeInTrack(pPuppet, trackNum, 0.125f);

		if (bNewTrack)
			pRd->_puppet.advanceTrack(pPuppet, trackNum, delta);

		break;
	case 8:
		if ((pPuppet->aTracks[trackNum].status & 8) == 0)
			pRd->_puppet.removeTrack(pPuppet, trackNum);

		break;
	case 9:
		pRd->_puppet.setStatus(pPuppet, trackNum, 0x10);
		break;
	case 10:
		pPuppet->aTracks[trackNum].status = (pPuppet->aTracks[trackNum].status & 0xFFFFFFED) | 2;

		if (bNewTrack)
			pRd->_puppet.advanceTrack(pPuppet, trackNum, delta);

		break;
	case 11:
		pRd->_puppet.fadeOutTrack(pPuppet, trackNum, 1.0f);
		break;
	case 12:
		pRd->_puppet.fadeOutTrack(pPuppet, trackNum, 0.5f);
		break;
	case 13:
		pRd->_puppet.fadeOutTrack(pPuppet, trackNum, 0.25f);
		break;
	case 14:
		pRd->_puppet.fadeOutTrack(pPuppet, trackNum, 0.125f);
		break;
	default:
		break;
	}
}

void DemoKeyframeHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	KeyframeInst *pInst = static_cast<KeyframeInst *>(pInstData);
	RenderDroid *pRd = _pSputCostume->_pRd;
	RdPuppet *pPuppet = pInst->pModelInst->pThing->pPuppet;
	int numTracks = pRd->_puppet.getNumTracks();

	if (!pInst->ref.valid)
		warning("DemoKeyframeHandler::change(): Bad keyframe Reference");

	void *handle = pInst->ref.handle;
	int trackNum = 0;

	while (trackNum < numTracks && pPuppet->aTracks[trackNum].pKeyframe != handle)
		trackNum++;

	if (keyValue < 0 || keyValue >= 15) {
		warning("DemoKeyframeHandler::change(): invalid key %d", keyValue);
		return;
	}

	int action = keyframeKeyValues[keyValue];

	if (trackNum == numTracks) {
		if ((uint)action > 7)
			return;

		trackNum = pRd->_puppet.addTrack(pPuppet, static_cast<RdKeyframe *>(handle), pInst->priority, pInst->priorityMasked);

		if (trackNum < 0) {
			warning("Unable to add track for keyframe %p", handle);
			return;
		}

		pInst->pModelInst->pThing->userArg = pInst->pModelInst;
		_pSputCostume->_pRes->setReference(&pInst->pModelInst->aTrackRefs[trackNum], handle);
		pInst->pModelInst->trackUserArg = hInst;
		pRd->_puppet.setCallback(pPuppet, trackNum, SputCostume::puppetHookCallback);
	}

	switch (action) {
	case 0:
		pRd->_puppet.resetTrack(pPuppet, trackNum);
		pRd->_puppet.setStatus(pPuppet, trackNum, 0x20);
		pRd->_puppet.playTrack(pPuppet, trackNum);
		break;
	case 1:
		if ((pPuppet->aTracks[trackNum].status & 0xC) == 0)
			pRd->_puppet.playTrack(pPuppet, trackNum);

		break;
	case 2:
		pRd->_puppet.resetTrack(pPuppet, trackNum);
		pRd->_puppet.setStatus(pPuppet, trackNum, 0x40);
		pRd->_puppet.playTrack(pPuppet, trackNum);
		break;
	case 3:
		pRd->_puppet.resetTrack(pPuppet, trackNum);
		pRd->_puppet.setStatus(pPuppet, trackNum, 0x80);
		pRd->_puppet.playTrack(pPuppet, trackNum);
		break;
	case 4:
		pRd->_puppet.fadeInTrack(pPuppet, trackNum, 1.0f);
		break;
	case 5:
		pRd->_puppet.fadeInTrack(pPuppet, trackNum, 0.5f);
		break;
	case 6:
		pRd->_puppet.fadeInTrack(pPuppet, trackNum, 0.25f);
		break;
	case 7:
		pRd->_puppet.fadeInTrack(pPuppet, trackNum, 0.125f);
		break;
	case 8:
		if ((pPuppet->aTracks[trackNum].status & 8) == 0)
			pRd->_puppet.removeTrack(pPuppet, trackNum);

		break;
	case 9:
		pRd->_puppet.setStatus(pPuppet, trackNum, 0x10);
		break;
	case 10:
		pPuppet->aTracks[trackNum].status = (pPuppet->aTracks[trackNum].status & 0xFFFFFFED) | 2;
		break;
	case 11:
		pRd->_puppet.fadeOutTrack(pPuppet, trackNum, 1.0f);
		break;
	case 12:
		pRd->_puppet.fadeOutTrack(pPuppet, trackNum, 0.5f);
		break;
	case 13:
		pRd->_puppet.fadeOutTrack(pPuppet, trackNum, 0.25f);
		break;
	case 14:
		pRd->_puppet.fadeOutTrack(pPuppet, trackNum, 0.125f);
		break;
	default:
		break;
	}
}

void DemoKeyframeHandler::stop(void *pInstData) {
	KeyframeInst *pInst = static_cast<KeyframeInst *>(pInstData);
	RdPuppet *pPuppet = pInst->pModelInst->pThing->pPuppet;

	if (!pInst->ref.valid)
		return;

	int trackNum = 0;

	while (pPuppet->aTracks[trackNum].pKeyframe != pInst->ref.handle) {
		trackNum++;

		if (trackNum >= _pSputCostume->_pRd->_puppet.getNumTracks())
			return;
	}

	if ((pPuppet->aTracks[trackNum].status & 8) == 0)
		_pSputCostume->_pRd->_puppet.removeTrack(pPuppet, trackNum);
}

int KeyframeHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	char name[32];
	int priority = 1;
	int priorityMasked = 5;

	memset(name, 0, sizeof(name));

	if (parentFourcc != kSputCostumeTagThing && parentFourcc != kSputCostumeTagMainThing)
		return 0;

	if (!pParentId)
		return 0;

	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = pComp->name;

	Common::strlcpy(name, args, 32);

	char *pKeyframeName = strtok(name, ",");

	if (!pKeyframeName) {
		warning("KeyframeHandler::request(): empty keyframe name");
		return 0;
	}

	void *hKeyframe = _pSputCostume->_pSputResource->alloc(kSputTagKeyf, pKeyframeName);
	char *pPriority = strtok(nullptr, ",");

	if (pPriority) {
		priority = atoi(pPriority);

		char *pPriorityMasked = strtok(nullptr, ",");

		if (pPriorityMasked)
			priorityMasked = atoi(pPriorityMasked);
		else
			warning("KeyframeHandler::request(): missing masked priority in %s", args);
	}

	if (!hKeyframe)
		return 0;

	KeyframeCompRes *pRes = new KeyframeCompRes();

	_pSputCostume->_pRes->setReference(&pRes->ref, hKeyframe);
	pRes->priority = priority;
	pRes->priorityMasked = priorityMasked;

	return _pSputCostume->_cos.registerComponent(hCostume, args, pRes, fourcc, pParentId, parentFourcc, &keyframeKeyDesc) != 0;
}

void KeyframeHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	KeyframeCompRes *pRes = static_cast<KeyframeCompRes *>(pCompRes);

	_pSputCostume->_pRes->removeReference(&pRes->ref);
	delete pRes;
}

void KeyframeHandler::stop(void *pInstData) {
	KeyframeInst *pInst = static_cast<KeyframeInst *>(pInstData);
	RdPuppet *pPuppet = pInst->pModelInst->pThing->pPuppet;

	if (!pInst->ref.valid)
		return;

	int trackNum = 0;

	while (pPuppet->aTracks[trackNum].pKeyframe != pInst->ref.handle) {
		trackNum++;

		if (trackNum >= 6)
			return;
	}

	if ((pPuppet->aTracks[trackNum].status & 8) == 0)
		_pSputCostume->_pRd->_puppet.removeTrack(pPuppet, trackNum);
}

int KeyframeHandler::adjust(void **ppCompRes, char *args, int argsSize) {
	KeyframeCompRes *pRes = static_cast<KeyframeCompRes *>(*ppCompRes);
	int aValues[2];

	aValues[0] = pRes->priorityMasked;
	aValues[1] = pRes->priority;

	int accepted = 0; // TODO: grimPlat_ControlPanel("Tagged Priority", "Normal Priority")

	if (!accepted)
		return 0;

	char *pToken = strtok(args, ",");

	if (!pToken) {
		warning("KeyframeHandler::adjust(): empty component name");
		return 0;
	}

	pRes->priorityMasked = aValues[0];
	pRes->priority = aValues[1];

	char text[32];

	snprintf(text, sizeof(text), "%s,%d,%d", pToken, aValues[1], aValues[0]);
	Common::strlcpy(pToken, text, 32);

	return 1;
}

void *KeyframeHandler::instance(int hInst, int compIndex, int bCreate) {
	Cos *pCos = &_pSputCostume->_cos;

	if (!bCreate) {
		KeyframeInst *pInst = static_cast<KeyframeInst *>(pCos->instGetInstanceData(hInst, compIndex, nullptr));

		_pSputCostume->_pRes->removeReference(&pInst->ref);
		delete pInst;

		return nullptr;
	}

	int fourcc = 0;
	void *pId = nullptr;
	int parentIndex = -1;

	pCos->instComponentInfo(hInst, compIndex, &fourcc, &pId, &parentIndex);

	KeyframeCompRes *pRes = static_cast<KeyframeCompRes *>(pId);

	if (!pRes->ref.valid)
		warning("KeyframeHandler::instance(): Bad keyframe Reference");

	KeyframeInst *pInst = new KeyframeInst();

	_pSputCostume->_pRes->setReference(&pInst->ref, pRes->ref.handle);
	pInst->priorityMasked = pRes->priorityMasked;
	pInst->priority = pRes->priority;
	pInst->pModelInst = static_cast<ModelInst *>(pCos->instGetInstanceData(hInst, parentIndex, nullptr));

	return pInst;
}

} // End of namespace Grim
