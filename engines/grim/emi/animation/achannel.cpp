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

#include "engines/grim/emi/animation/achannel.h"
#include "engines/grim/emi/animation/ainst.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

enum {
	kMaxGroups = 5
};

void *ChannelNode::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void ChannelNode::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

static void removeChannel(AInst *inst, AChannelHandle *channel) {
	Common::List<AChannelHandle *>::iterator it = inst->_channels.begin();

	while (it != inst->_channels.end()) {
		if (*it == channel)
			it = inst->_channels.erase(it);
		else
			++it;
	}
}

static void accumulateGroup(CQuaternion *accumulator, ChannelNode *from, ChannelNode *to, float scale) {
	for (ChannelNode *node = from; node != to; node = node->next) {
		ATrackData<CQuaternion> *track = static_cast<ATrackData<CQuaternion> *>(node->track);
		double factor = (double)node->inst->_weight * scale;
		CQuaternion contribution((float)(factor * track->_value._x), (float)(factor * track->_value._y), (float)(factor * track->_value._z), (float)(factor * track->_value._w));

		if (accumulator->dot(contribution) < 0.0f)
			contribution = contribution * -1.0f;

		*accumulator += contribution;
	}
}

ChannelList::ChannelList() {
	_tail = nullptr;
	_head = nullptr;
	_count = 0;
}

void ChannelList::popFront() {
	ChannelNode *oldHead = _head;

	_head = oldHead->next;

	if (_head)
		_head->prev = nullptr;
	else
		_tail = nullptr;

	oldHead->prev = nullptr;
	oldHead->next = nullptr;
	_count--;
}

void ChannelList::popBack() {
	ChannelNode *oldTail = _tail;

	_tail = oldTail->prev;

	if (!_tail)
		_head = nullptr;
	else
		_tail->next = nullptr;

	oldTail->prev = nullptr;
	oldTail->next = nullptr;
	_count--;
}

void ChannelList::remove(ChannelNode *node) {
	if (node == _head) {
		popFront();
	} else if (node == _tail) {
		popBack();
	} else if (node->next || node->prev) {
		node->next->prev = node->prev;
		node->prev->next = node->next;
		_count--;
	} else {
		return;
	}

	node->prev = nullptr;
	node->next = nullptr;
}

void ChannelList::pushBack(ChannelNode *node) {
	if (_tail)
		_tail->next = node;

	node->next = nullptr;
	node->prev = _tail;

	if (!_head)
		_head = node;

	_tail = node;
	_count++;
}

void ChannelList::insertByPriority(ChannelNode *node) {
	ChannelNode *cursor = _head;

	if (cursor) {
		while (cursor->track->_priority > node->track->_priority) {
			cursor = cursor->next;

			if (!cursor) {
				pushBack(node);

				return;
			}
		}

		if (cursor == _head) {
			_head->prev = node;
			node->prev = nullptr;
			node->next = _head;
			_head = node;

			if (!_tail)
				_tail = node;
		} else {
			node->next = cursor;
			node->prev = cursor->prev;
			cursor->prev->next = node;
			cursor->prev = node;
		}

		_count++;
	} else {
		pushBack(node);
	}
}

template<class T>
AChannel<T>::AChannel() {
	_dirty = false;
}

template<class T>
AChannel<T>::~AChannel() {
	for (ChannelNode *node = _active._head; node; node = node->next)
		removeChannel(node->inst, this);

	for (ChannelNode *node = _inactive._head; node; node = node->next)
		removeChannel(node->inst, this);

	while (_active._head) {
		ChannelNode *node = _active._head;

		_active.popFront();
		delete node;
	}

	while (_inactive._head) {
		ChannelNode *node = _inactive._head;

		_inactive.popFront();
		delete node;
	}
}

template<class T>
bool AChannel<T>::detach(AInst *inst) {
	ChannelList *list = (inst->_flags & AInst::kAttached) == 0 ? &_inactive : &_active;
	ChannelNode *node = list->_head;

	while (node) {
		ChannelNode *next = node->next;

		if (node->inst == inst) {
			list->remove(node);
			removeChannel(inst, this);
			delete node;
		}

		node = next;
	}

	return true;
}

template<class T>
void AChannel<T>::activate(AInst *inst) {
	ChannelNode *node = _inactive._head;

	while (node && node->inst != inst)
		node = node->next;

	if (!node) {
		warning("AChannel::activate(): the instance is not bound to this channel");

		return;
	}

	_inactive.remove(node);
	_active.insertByPriority(node);
}

template<class T>
void AChannel<T>::deactivate(AInst *inst) {
	ChannelNode *node = _active._head;

	while (node && node->inst != inst)
		node = node->next;

	if (!node) {
		warning("AChannel::deactivate(): the instance is not active on this channel");

		return;
	}

	_active.remove(node);
	_inactive.pushBack(node);
}

template<class T>
void AChannel<T>::onKey(ATrack *track, float time, void *key) {
}

template<class T>
bool AChannel<T>::addTrack(AInst *inst, ATrack *track) {
	ChannelNode *node = new ChannelNode();

	node->prev = nullptr;
	node->next = nullptr;
	node->lastTime = 0.0f;
	node->inst = inst;
	node->track = track;

	if ((inst->_flags & AInst::kAttached) != 0)
		_active.insertByPriority(node);
	else
		_inactive.pushBack(node);

	inst->_channels.push_back(this);

	return true;
}

template<>
bool AChannel<CQuaternion>::evaluate(CQuaternion *out) {
	if (_active._count == 0)
		return true;

	CQuaternion groupValue[kMaxGroups];
	float groupSum[kMaxGroups];
	int group = 0;
	ChannelNode *start = _active._head;
	ChannelNode *node = start;
	int groupPriority = start->track->_priority;

	groupSum[0] = 0.0f;

	while (node) {
		ATrackData<CQuaternion> *track = static_cast<ATrackData<CQuaternion> *>(node->track);

		if (track->_priority < groupPriority) {
			if (groupSum[group] >= 1.0f)
				break;

			if (group >= kMaxGroups - 1) {
				warning("AChannel::evaluate(): too many priority groups");
				break;
			}

			float scale = groupSum[group] <= 0.0f ? 0.0f : (float)(1.0 / groupSum[group]);

			accumulateGroup(&groupValue[group], start, node, scale);
			start = node;
			group++;
			groupPriority = track->_priority;
			groupSum[group] = 0.0f;
		}

		AInst *inst = node->inst;

		track->sample(node->lastTime, inst->_time, &track->_value, this);
		groupSum[group] = inst->_weight + groupSum[group];
		node->lastTime = inst->_time;
		node = node->next;
	}

	float lastScale = groupSum[group] <= 0.0f ? 0.0f : (float)(1.0 / groupSum[group]);

	accumulateGroup(&groupValue[group], start, node, lastScale);

	for (int i = group; i >= 0; i--) {
		if (groupSum[i] >= 1.0f) {
			*out = groupValue[i];
		} else {
			double weight = groupSum[i];
			CQuaternion scaled((float)(weight * groupValue[i]._x), (float)(weight * groupValue[i]._y), (float)(weight * groupValue[i]._z), (float)(weight * groupValue[i]._w));
			double remaining = 1.0 - groupSum[i];
			float remainingW = (float)(remaining * out->_w);
			float remainingZ = (float)(remaining * out->_z);
			float remainingY = (float)(remaining * out->_y);
			float remainingX = (float)(remaining * out->_x);

			*out = CQuaternion(remainingX, remainingY, remainingZ, remainingW);

			if (out->dot(scaled) < 0.0f)
				scaled = CQuaternion((float)(scaled._x * -1.0), (float)(scaled._y * -1.0), (float)(scaled._z * -1.0), (float)(scaled._w * -1.0));

			out->_x = scaled._x + out->_x;
			out->_y = scaled._y + out->_y;
			out->_z = scaled._z + out->_z;
			out->_w = scaled._w + out->_w;
		}
	}

	return true;
}

template AChannel<CVector>::AChannel();
template AChannel<CVector>::~AChannel();
template bool AChannel<CVector>::detach(AInst *inst);
template void AChannel<CVector>::activate(AInst *inst);
template void AChannel<CVector>::deactivate(AInst *inst);
template void AChannel<CVector>::onKey(ATrack *track, float time, void *key);
template bool AChannel<CVector>::addTrack(AInst *inst, ATrack *track);

template class AChannel<CQuaternion>;

} // End of namespace Grim
