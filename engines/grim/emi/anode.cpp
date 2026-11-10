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

#include "engines/grim/emi/ainst.h"
#include "engines/grim/emi/anode.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/profilertimer.h"

namespace Grim {

enum {
	kMaxGroups = 5
};

static ProfilerTimer updateANodeTimer("ANode::UpdateANode()", ProfilerTimer::kTrackNesting);

ANode::ANode() {
	boundTracks = 0;
	dirty = true;
	posChannel = new AChannel<CVector>();
	rotChannel = new AChannel<CQuaternion>();
}

ANode::~ANode() {
	if (posChannel) {
		delete posChannel;
		posChannel = nullptr;
	}

	if (rotChannel) {
		delete rotChannel;
		rotChannel = nullptr;
	}
}

void *ANode::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void ANode::operator delete(void *block) {
	gpMemoryServices->free(block);
}

void ANode::bindTrack(AInst *inst, ATrack *track) {
	boundTracks++;

	if (track->dataType == ATrack::kDataVector)
		posChannel->addTrack(inst, track);
	else if (track->dataType == ATrack::kDataQuaternion)
		rotChannel->addTrack(inst, track);
}

void ANode::unbindTrack(AInst *inst, ATrack *track) {
	boundTracks--;

	if (track->dataType == ATrack::kDataVector)
		posChannel->detach(inst);
	else if (track->dataType == ATrack::kDataQuaternion)
		rotChannel->detach(inst);
}

void ANode::updateANode() {
	updateANodeTimer.start();

	if (!dirty || boundTracks <= 0) {
		updateANodeTimer.stop();

		return;
	}

	double resultX = 0.0;
	double resultY = 0.0;
	double resultZ = 0.0;
	CQuaternion rotation;

	if (posChannel->active.count) {
		CVector groupValue[kMaxGroups];
		float groupSum[kMaxGroups];
		int group = 0;
		ChannelNode *start = posChannel->active.head;
		ChannelNode *node = start;
		int groupPriority = start->track->priority;

		groupSum[0] = 0.0f;

		while (node) {
			ATrackData<CVector> *track = static_cast<ATrackData<CVector> *>(node->track);

			if (track->priority < groupPriority) {
				if (groupSum[group] >= 1.0f)
					break;

				if (group >= kMaxGroups - 1) {
					warning("ANode::updateANode(): too many priority groups");
					break;
				}

				float groupScale = groupSum[group] <= 0.0f ? 0.0f : (float)(1.0 / groupSum[group]);

				for (ChannelNode *member = start; member != node; member = member->next) {
					ATrackData<CVector> *memberTrack = static_cast<ATrackData<CVector> *>(member->track);
					double factor = (double)member->inst->weight * groupScale;
					float addedZ = (float)(factor * memberTrack->value.z);
					float addedY = (float)(factor * memberTrack->value.y);

					groupValue[group].x = (float)(factor * memberTrack->value.x + groupValue[group].x);
					groupValue[group].y = (float)((double)addedY + groupValue[group].y);
					groupValue[group].z = (float)((double)addedZ + groupValue[group].z);
				}

				start = node;
				group++;
				groupPriority = track->priority;
				groupSum[group] = 0.0f;
			}

			AInst *inst = node->inst;

			track->sample(node->lastTime, inst->time, &track->value, posChannel);
			groupSum[group] = inst->weight + groupSum[group];
			node->lastTime = inst->time;
			node = node->next;
		}

		float lastScale = groupSum[group] <= 0.0f ? 0.0f : (float)(1.0 / groupSum[group]);

		for (ChannelNode *member = start; member != node; member = member->next) {
			ATrackData<CVector> *memberTrack = static_cast<ATrackData<CVector> *>(member->track);
			double factor = (double)member->inst->weight * lastScale;
			float addedZ = (float)(factor * memberTrack->value.z);
			float addedY = (float)(factor * memberTrack->value.y);

			groupValue[group].x = (float)(factor * memberTrack->value.x + groupValue[group].x);
			groupValue[group].y = (float)((double)addedY + groupValue[group].y);
			groupValue[group].z = (float)((double)addedZ + groupValue[group].z);
		}

		for (int i = group; i >= 0; i--) {
			if (groupSum[i] >= 1.0f) {
				resultX = groupValue[i].x;
				resultY = groupValue[i].y;
				resultZ = groupValue[i].z;
			} else {
				double weight = groupSum[i];
				float scaledZ = (float)(weight * groupValue[i].z);
				float scaledY = (float)(weight * groupValue[i].y);
				float scaledX = (float)(weight * groupValue[i].x);
				float remaining = (float)(1.0 - groupSum[i]);
				float previousZ = (float)(resultZ * remaining);
				float previousY = (float)(resultY * remaining);

				resultX = resultX * remaining + scaledX;
				resultY = (double)previousY + scaledY;
				resultZ = (double)previousZ + scaledZ;
			}
		}
	}

	rotChannel->evaluate(&rotation);

	pos.x = (float)resultX;
	pos.y = (float)resultY;
	pos.z = (float)resultZ;
	nodeFlags |= kPositionValid | kLocalSet;
	updateLocalState();

	quat = rotation;
	nodeFlags = (nodeFlags & ~(kEulerValid | kQuaternionValid | kLocalSet)) | kQuaternionValid | kLocalSet;
	updateLocalState();

	updateANodeTimer.stop();
}

} // End of namespace Grim
