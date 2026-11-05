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

#include "common/textconsole.h"

#include "engines/grim/renderdroid/rd.h"

namespace Grim {

RdPuppet *RenderDroid::PuppetModule::create(RdThing *pThing) {
	RdPuppet *pPuppet = new RdPuppet;

	memset(pPuppet, 0, sizeof(RdPuppet));
	if (!newEntry(pPuppet, pThing))
		return nullptr;

	pThing->pPuppet = pPuppet;
	return pPuppet;
}

int RenderDroid::PuppetModule::newEntry(RdPuppet *pPuppet, RdThing *pThing) {
	pPuppet->bPaused = 0;
	pPuppet->pThing = pThing;

	for (int i = 0; i < 6; i++) {
		pPuppet->aTracks[i].curFrame = 0.0f;
		pPuppet->aTracks[i].prevFrame = 0.0f;
		removeTrack(pPuppet, i);
	}

	return 1;
}

int RenderDroid::PuppetModule::addTrack(RdPuppet *pPuppet, RdKeyframe *pKeyframe, int priority, int priorityMasked) {
	int slot = 0;

	while (slot < 6 && pPuppet->aTracks[slot].status != 0)
		slot++;

	if (slot >= 6) {
		slot = 0;
		while (slot < 6 && ((pPuppet->aTracks[slot].status & 8) == 0 || (pPuppet->aTracks[slot].status & 0x140) != 0))
			slot++;

		if (slot >= 6)
			return -1;

		removeTrack(pPuppet, slot);
	}

	RdPuppetTrack *track = &pPuppet->aTracks[slot];
	track->pKeyframe = pKeyframe;
	track->priority = priority;
	track->fps = pKeyframe->fps;
	track->fade = 0.0f;
	track->priorityMasked = priorityMasked;
	track->status |= 1;
	resetTrack(pPuppet, slot);

	return slot;
}

int RenderDroid::PuppetModule::removeTrack(RdPuppet *pPuppet, int trackNum) {
	RdPuppetTrackCallback callback = pPuppet->aTracks[trackNum].pfCallback;

	if (callback)
		callback(pPuppet->pThing->userArg, trackNum, 0);

	pPuppet->aTracks[trackNum].status = 0;
	pPuppet->aTracks[trackNum].pKeyframe = nullptr;
	pPuppet->aTracks[trackNum].pfCallback = nullptr;

	return 1;
}

int RenderDroid::PuppetModule::setStatus(RdPuppet *pPuppet, int trackNum, uint status) {
	pPuppet->aTracks[trackNum].status |= status;
	return 1;
}

void RenderDroid::PuppetModule::setCallback(RdPuppet *pPuppet, int trackNum, RdPuppetTrackCallback pfCallback) {
	pPuppet->aTracks[trackNum].pfCallback = pfCallback;
}

int RenderDroid::PuppetModule::playTrack(RdPuppet *pPuppet, int trackNum) {
	RdPuppetTrack *track = &pPuppet->aTracks[trackNum];

	track->status = (track->status & 0xFFFFFFED) | 2;
	track->fade = 1.0f;

	return 1;
}

int RenderDroid::PuppetModule::fadeInTrack(RdPuppet *pPuppet, int trackNum, float time) {
	RdPuppetTrack *track = &pPuppet->aTracks[trackNum];

	track->status = (track->status & 0xFFFFFFF1) | 6;
	if (time <= 0.0)
		track->fadeSpeed = 1.0f;
	else
		track->fadeSpeed = (float)(1.0 / time);

	return 1;
}

int RenderDroid::PuppetModule::fadeOutTrack(RdPuppet *pPuppet, int trackNum, float time) {
	RdPuppetTrack *track = &pPuppet->aTracks[trackNum];

	track->status = (track->status & 0xFFFFFFF3) | 8;
	if (time <= 0.0)
		track->fadeSpeed = 1.0f;
	else
		track->fadeSpeed = (float)(1.0 / time);

	return 1;
}

void RenderDroid::PuppetModule::advanceTrack(RdPuppet *pPuppet, int trackNum, float delta) {
	RdPuppetTrack *track = &pPuppet->aTracks[trackNum];
	RdKeyframe *keyframe = track->pKeyframe;
	float wraps = 0.0f;
	int finished = 0;

	if (!keyframe || delta == 0.0)
		return;

	float newFrame = (float)((double)delta + track->prevFrame);
	track->curFrame = newFrame;

	double numFrames = (double)keyframe->numFrames;
	if (newFrame >= numFrames) {
		uint status = track->status;

		if (status & 0x20) {
			track->curFrame = (float)numFrames;
			finished = 1;
		} else if (status & 0x40) {
			track->curFrame = (float)numFrames;
			track->status = status | 0x10;
		} else if ((status & 0x80) == 0) {
			float turns = (float)((double)newFrame / numFrames);
			wraps = _rd->_stdMath->rint(turns);
			track->curFrame = (float)((double)track->curFrame - (double)track->pKeyframe->numFrames * wraps);

			uint numHNodes = pPuppet->pThing->data.pModel3->numHNodes;
			if (numHNodes >= 64)
				numHNodes = 64;
			memset(track->aEntryIdx, 0, 4 * numHNodes);
		} else {
			track->curFrame = (float)numFrames;
			fadeOutTrack(pPuppet, trackNum, 0.25f);
			track->status |= 0x10;
		}
	}

	if (track->pfCallback) {
		keyframe = track->pKeyframe;

		if (keyframe->numMarkers) {
			if (wraps == 0.0) {
				for (uint i = 0; i < keyframe->numMarkers; i++) {
					if (keyframe->aMarkerFrame[i] > (double)track->curFrame)
						break;

					if (keyframe->aMarkerFrame[i] > (double)track->prevFrame || track->prevFrame == 0.0)
						track->pfCallback(pPuppet->pThing->userArg, trackNum, keyframe->aMarkerType[i]);

					keyframe = track->pKeyframe;
				}
			} else if (wraps <= 1.0) {
				for (uint i = 0; i < keyframe->numMarkers; i++) {
					if (keyframe->aMarkerFrame[i] > (double)track->prevFrame || keyframe->aMarkerFrame[i] <= (double)track->curFrame)
						track->pfCallback(pPuppet->pThing->userArg, trackNum, keyframe->aMarkerType[i]);

					keyframe = track->pKeyframe;
				}
			} else {
				for (uint i = 0; i < keyframe->numMarkers; i++) {
					track->pfCallback(pPuppet->pThing->userArg, trackNum, keyframe->aMarkerType[i]);
					keyframe = track->pKeyframe;
				}
			}
		}
	}

	if (finished)
		removeTrack(pPuppet, trackNum);
	else
		track->prevFrame = track->curFrame;
}

int RenderDroid::PuppetModule::updateTracks(RdPuppet *pPuppet, float delta) {
	int activeTracks = 0;

	if (pPuppet->bPaused)
		return activeTracks;

	for (int i = 0; i < 6; i++) {
		RdPuppetTrack *track = &pPuppet->aTracks[i];

		if (!track->status)
			continue;

		activeTracks++;
		if (track->status & 0x200)
			continue;

		if ((track->status & 0x10) == 0)
			advanceTrack(pPuppet, i, (float)((double)track->fps * delta));

		if (track->status & 4) {
			double fade = (double)track->fadeSpeed * delta + track->fade;
			track->fade = (float)fade;

			if (!(fade < 1.0)) {
				track->fade = 1.0f;
				track->status &= 0xFFFFFFFB;
			}
		} else if (track->status & 8) {
			double fade = (double)track->fade - (double)track->fadeSpeed * delta;
			track->fade = (float)fade;

			if (fade <= 0.0) {
				if (track->status & 0x100)
					track->status = (track->status & 0xFFFFFFF7) | 0x10;
				else
					removeTrack(pPuppet, i);
			}
		}
	}

	return activeTracks;
}

int RenderDroid::PuppetModule::resetTrack(RdPuppet *pPuppet, int trackNum) {
	RdPuppetTrack *track = &pPuppet->aTracks[trackNum];
	uint numHNodes = pPuppet->pThing->data.pModel3->numHNodes;

	if (numHNodes >= 64)
		numHNodes = 64;

	memset(track->aEntryIdx, 0, 4 * numHNodes);
	track->curFrame = 0.0f;
	track->prevFrame = 0.0f;
	track->status = 3;

	return 0;
}

void RenderDroid::PuppetModule::buildJointMatrices(RdThing *pThing, RdMatrix34 pBase) {
	RdModel3 *pModel3 = pThing->data.pModel3;
	RdPuppet *pPuppet = pThing->pPuppet;

	if (pThing->bJointsLocked)
		return;

	if (!pPuppet || pPuppet->bPaused) {
		for (uint i = 0; i < pModel3->numHNodes; i++)
			memcpy(pThing->aJointMatrices[i], pModel3->aHNodes[i].mat, sizeof(RdMatrix34));
	} else {
		for (int t = 0; t < 6; t++) {
			RdPuppetTrack *pTrack = &pPuppet->aTracks[t];

			if (!pTrack->status)
				continue;

			RdKeyframe *pKeyframe = pTrack->pKeyframe;

			for (uint n = 0; n < pKeyframe->numNodes; n++) {
				RdKeyframeNode *pNode = &pKeyframe->paNodes[n];

				if (!pNode->numEntries)
					continue;

				int index = pNode->nodeNum >= 0x40 ? 0 : pTrack->aEntryIdx[pNode->nodeNum];
				int last = pNode->numEntries - 1;

				if (index < 0 || index > last) {
					warning("RenderDroid::PuppetModule::buildJointMatrices(): stale entry index %d for node %s", index, pNode->aName);
					continue;
				}

				if (index == last)
					continue;

				int next = index + 1;

				if (pTrack->curFrame >= (double)pNode->paEntries[next].frame) {
					while (next != last && pTrack->curFrame >= (double)pNode->paEntries[next + 1].frame)
						next++;

					if (n < 0x40)
						pTrack->aEntryIdx[n] = next;
				}
			}
		}

		for (uint i = 0; i < pModel3->numHNodes; i++) {
			RdModel3HNode *pHNode = &pModel3->aHNodes[i];
			float topPos[3] = { 0.0f, 0.0f, 0.0f };
			float topPyr[3] = { 0.0f, 0.0f, 0.0f };
			float secondPos[3] = { 0.0f, 0.0f, 0.0f };
			float secondPyr[3] = { 0.0f, 0.0f, 0.0f };
			float topFade = 0.0f;
			float secondFade = 0.0f;
			int topPriority = 0;
			int secondPriority = 0;

			for (int t = 0; t < 6; t++) {
				RdPuppetTrack *pTrack = &pPuppet->aTracks[t];
				RdKeyframe *pKeyframe = pTrack->pKeyframe;

				if (!pKeyframe)
					continue;

				int priority;

				if (pKeyframe->type & pHNode->type)
					priority = pTrack->priorityMasked;
				else
					priority = pTrack->priority;

				if (!(pTrack->status & 2))
					continue;

				if (pHNode->num >= pKeyframe->numNodes)
					continue;

				RdKeyframeNode *pNode = &pKeyframe->paNodes[pHNode->num];

				if (!pNode->numEntries)
					continue;

				if (priority < secondPriority)
					continue;

				if (!(priority >= topPriority || topFade < 1.0f))
					continue;

				if (pHNode->num >= 0x40) {
					warning("RenderDroid::PuppetModule::buildJointMatrices(): node index %u out of range", pHNode->num);
					continue;
				}

				RdKeyframeEntry *pEntry = &pNode->paEntries[pTrack->aEntryIdx[pHNode->num]];
				double delta;

				if (pKeyframe->flags & 0x100)
					delta = 0.0;
				else
					delta = (double)pTrack->curFrame - pEntry->frame;

				float curPos[3];
				float curPyr[3];

				if (pEntry->flags & 1) {
					curPos[0] = (float)(pEntry->dpos[0] * delta + pEntry->pos[0]);
					curPos[1] = (float)(pEntry->dpos[1] * delta + pEntry->pos[1]);
					curPos[2] = (float)(pEntry->dpos[2] * delta + pEntry->pos[2]);
				} else {
					curPos[0] = pEntry->pos[0];
					curPos[1] = pEntry->pos[1];
					curPos[2] = pEntry->pos[2];
				}

				if (pEntry->flags & 2) {
					curPyr[0] = (float)(pEntry->dpyr[0] * delta + pEntry->pyr[0]);
					curPyr[1] = (float)(pEntry->dpyr[1] * delta + pEntry->pyr[1]);
					curPyr[2] = (float)(pEntry->dpyr[2] * delta + pEntry->pyr[2]);
				} else {
					curPyr[0] = pEntry->pyr[0];
					curPyr[1] = pEntry->pyr[1];
					curPyr[2] = pEntry->pyr[2];
				}

				curPos[0] = (float)((double)curPos[0] - pHNode->xyz[0]);
				curPos[1] = (float)((double)curPos[1] - pHNode->xyz[1]);
				curPos[2] = (float)((double)curPos[2] - pHNode->xyz[2]);
				curPyr[0] = (float)((double)curPyr[0] - pHNode->pyr[0]);
				curPyr[1] = (float)((double)curPyr[1] - pHNode->pyr[1]);
				curPyr[2] = (float)((double)curPyr[2] - pHNode->pyr[2]);
				curPyr[0] = _rd->_stdMath->normalizeAngleAcute(curPyr[0]);
				curPyr[1] = _rd->_stdMath->normalizeAngleAcute(curPyr[1]);
				curPyr[2] = _rd->_stdMath->normalizeAngleAcute(curPyr[2]);

				if (pTrack->fade < 1.0f) {
					for (int k = 0; k < 3; k++) {
						curPos[k] = (float)((double)pTrack->fade * curPos[k]);
						curPyr[k] = (float)((double)pTrack->fade * curPyr[k]);
					}
				}

				if (priority == topPriority) {
					for (int k = 0; k < 3; k++) {
						topPos[k] = (float)((double)curPos[k] + topPos[k]);
						topPyr[k] = (float)((double)curPyr[k] + topPyr[k]);
					}

					topFade = (float)((double)topFade + pTrack->fade);
				} else if (priority < topPriority) {
					if (priority <= secondPriority) {
						for (int k = 0; k < 3; k++) {
							secondPos[k] = (float)((double)curPos[k] + secondPos[k]);
							secondPyr[k] = (float)((double)curPyr[k] + secondPyr[k]);
						}

						secondFade = (float)((double)secondFade + pTrack->fade);
					} else {
						for (int k = 0; k < 3; k++) {
							secondPos[k] = curPos[k];
							secondPyr[k] = curPyr[k];
						}

						secondFade = pTrack->fade;
						secondPriority = priority;
					}
				} else {
					int previousTopPriority = topPriority;

					for (int k = 0; k < 3; k++) {
						secondPos[k] = topPos[k];
						secondPyr[k] = topPyr[k];
						topPos[k] = curPos[k];
						topPyr[k] = curPyr[k];
					}

					secondFade = topFade;
					topPriority = priority;
					secondPriority = previousTopPriority;
					topFade = pTrack->fade;
				}
			}

			if (topFade >= 1.0f || secondFade <= 0.0f) {
				if (topFade > 1.0f) {
					double scale = 1.0 / topFade;

					for (int k = 0; k < 3; k++) {
						topPos[k] = (float)(topPos[k] * scale);
						topPyr[k] = (float)(topPyr[k] * scale);
					}
				}
			} else {
				if (secondFade > 1.0f) {
					double scale = 1.0 / secondFade;

					for (int k = 0; k < 3; k++) {
						secondPos[k] = (float)(secondPos[k] * scale);
						secondPyr[k] = (float)(secondPyr[k] * scale);
					}
				}

				double remaining = 1.0 - topFade;

				for (int k = 0; k < 3; k++) {
					topPos[k] = (float)(secondPos[k] * remaining + topPos[k]);
					topPyr[k] = (float)(secondPyr[k] * remaining + topPyr[k]);
				}
			}

			topPyr[0] = _rd->_stdMath->normalizeAngleAcute(topPyr[0]);
			topPyr[1] = _rd->_stdMath->normalizeAngleAcute(topPyr[1]);
			topPyr[2] = _rd->_stdMath->normalizeAngleAcute(topPyr[2]);

			for (int k = 0; k < 3; k++) {
				topPos[k] = (float)((double)pHNode->xyz[k] + topPos[k]);
				topPyr[k] = (float)((double)pHNode->pyr[k] + topPyr[k]);
			}

			_rd->build34(pThing->aJointMatrices[i], topPyr, topPos);

			const float *jointPyr = pThing->aJointPyr[i];

			if (jointPyr[0] != 0.0f || jointPyr[1] != 0.0f || jointPyr[2] != 0.0f)
				_rd->preRotate34(pThing->aJointMatrices[i], pThing->aJointPyr[i]);
		}
	}

	_rd->_thing.accumulateMatrices(pThing, pModel3->aHNodes, pBase);
	pThing->jointsFrame = _rd->_frameTrue;
}

} // End of namespace Grim
