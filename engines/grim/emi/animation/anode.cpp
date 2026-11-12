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

#include "engines/grim/emi/animation/ainst.h"
#include "engines/grim/emi/animation/anode.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/core/profilertimer.h"

namespace Grim {

enum {
	kMaxGroups = 5
};

ANode::ANode(EmiEngine *engine) : _engine(engine) {
	_boundTracks = 0;
	_dirty = true;
	_posChannel = new AChannel<CVector>();
	_rotChannel = new AChannel<CQuaternion>();
}

ANode::~ANode() {
	if (_posChannel) {
		delete _posChannel;
		_posChannel = nullptr;
	}

	if (_rotChannel) {
		delete _rotChannel;
		_rotChannel = nullptr;
	}
}

void *ANode::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void ANode::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void ANode::bindTrack(AInst *inst, ATrack *track) {
	_boundTracks++;

	if (track->_dataType == ATrack::kDataVector)
		_posChannel->addTrack(inst, track);
	else if (track->_dataType == ATrack::kDataQuaternion)
		_rotChannel->addTrack(inst, track);
}

void ANode::unbindTrack(AInst *inst, ATrack *track) {
	_boundTracks--;

	if (track->_dataType == ATrack::kDataVector)
		_posChannel->detach(inst);
	else if (track->_dataType == ATrack::kDataQuaternion)
		_rotChannel->detach(inst);
}

void ANode::updateANode() {
	_engine->_updateANodeTimer.start();

	if (!_dirty || _boundTracks <= 0) {
		_engine->_updateANodeTimer.stop();

		return;
	}

	double resultX = 0.0;
	double resultY = 0.0;
	double resultZ = 0.0;
	CQuaternion rotation;

	if (_posChannel->_active._count) {
		CVector groupValue[kMaxGroups];
		float groupSum[kMaxGroups];
		int group = 0;
		ChannelNode *start = _posChannel->_active._head;
		ChannelNode *node = start;
		int groupPriority = start->track->_priority;

		groupSum[0] = 0.0f;

		while (node) {
			ATrackData<CVector> *track = static_cast<ATrackData<CVector> *>(node->track);

			if (track->_priority < groupPriority) {
				if (groupSum[group] >= 1.0f)
					break;

				if (group >= kMaxGroups - 1) {
					warning("ANode::updateANode(): too many priority groups");
					break;
				}

				float groupScale = groupSum[group] <= 0.0f ? 0.0f : (float)(1.0 / groupSum[group]);

				for (ChannelNode *member = start; member != node; member = member->next) {
					ATrackData<CVector> *memberTrack = static_cast<ATrackData<CVector> *>(member->track);
					double factor = (double)member->inst->_weight * groupScale;
					float addedZ = (float)(factor * memberTrack->_value._z);
					float addedY = (float)(factor * memberTrack->_value._y);

					groupValue[group]._x = (float)(factor * memberTrack->_value._x + groupValue[group]._x);
					groupValue[group]._y = (float)((double)addedY + groupValue[group]._y);
					groupValue[group]._z = (float)((double)addedZ + groupValue[group]._z);
				}

				start = node;
				group++;
				groupPriority = track->_priority;
				groupSum[group] = 0.0f;
			}

			AInst *inst = node->inst;

			track->sample(node->lastTime, inst->_time, &track->_value, _posChannel);
			groupSum[group] = inst->_weight + groupSum[group];
			node->lastTime = inst->_time;
			node = node->next;
		}

		float lastScale = groupSum[group] <= 0.0f ? 0.0f : (float)(1.0 / groupSum[group]);

		for (ChannelNode *member = start; member != node; member = member->next) {
			ATrackData<CVector> *memberTrack = static_cast<ATrackData<CVector> *>(member->track);
			double factor = (double)member->inst->_weight * lastScale;
			float addedZ = (float)(factor * memberTrack->_value._z);
			float addedY = (float)(factor * memberTrack->_value._y);

			groupValue[group]._x = (float)(factor * memberTrack->_value._x + groupValue[group]._x);
			groupValue[group]._y = (float)((double)addedY + groupValue[group]._y);
			groupValue[group]._z = (float)((double)addedZ + groupValue[group]._z);
		}

		for (int i = group; i >= 0; i--) {
			if (groupSum[i] >= 1.0f) {
				resultX = groupValue[i]._x;
				resultY = groupValue[i]._y;
				resultZ = groupValue[i]._z;
			} else {
				double weight = groupSum[i];
				float scaledZ = (float)(weight * groupValue[i]._z);
				float scaledY = (float)(weight * groupValue[i]._y);
				float scaledX = (float)(weight * groupValue[i]._x);
				float remaining = (float)(1.0 - groupSum[i]);
				float previousZ = (float)(resultZ * remaining);
				float previousY = (float)(resultY * remaining);

				resultX = resultX * remaining + scaledX;
				resultY = (double)previousY + scaledY;
				resultZ = (double)previousZ + scaledZ;
			}
		}
	}

	_rotChannel->evaluate(&rotation);

	_pos._x = (float)resultX;
	_pos._y = (float)resultY;
	_pos._z = (float)resultZ;
	_nodeFlags |= kPositionValid | kLocalSet;
	updateLocalState();

	_quat = rotation;
	_nodeFlags = (_nodeFlags & ~(kEulerValid | kQuaternionValid | kLocalSet)) | kQuaternionValid | kLocalSet;
	updateLocalState();

	_engine->_updateANodeTimer.stop();
}

} // End of namespace Grim
