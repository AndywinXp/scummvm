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

#include "engines/grim/emi/achannel.h"
#include "engines/grim/emi/ainst.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

enum {
	kMaxGroups = 5
};

void *ChannelNode::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void ChannelNode::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

static void removeChannel(AInst *inst, AChannelHandle *channel) {
	Common::List<AChannelHandle *>::iterator it = inst->channels.begin();

	while (it != inst->channels.end()) {
		if (*it == channel)
			it = inst->channels.erase(it);
		else
			++it;
	}
}

static void accumulateGroup(CQuaternion *accumulator, ChannelNode *from, ChannelNode *to, float scale) {
	for (ChannelNode *node = from; node != to; node = node->next) {
		ATrackData<CQuaternion> *track = static_cast<ATrackData<CQuaternion> *>(node->track);
		double factor = (double)node->inst->weight * scale;
		CQuaternion contribution((float)(factor * track->value.x), (float)(factor * track->value.y), (float)(factor * track->value.z), (float)(factor * track->value.w));

		if (accumulator->dot(contribution) < 0.0f)
			contribution = contribution * -1.0f;

		*accumulator += contribution;
	}
}

ChannelList::ChannelList() {
	tail = nullptr;
	head = nullptr;
	count = 0;
}

void ChannelList::popFront() {
	ChannelNode *oldHead = head;

	head = oldHead->next;

	if (head)
		head->prev = nullptr;
	else
		tail = nullptr;

	oldHead->prev = nullptr;
	oldHead->next = nullptr;
	count--;
}

void ChannelList::popBack() {
	ChannelNode *oldTail = tail;

	tail = oldTail->prev;

	if (!tail)
		head = nullptr;
	else
		tail->next = nullptr;

	oldTail->prev = nullptr;
	oldTail->next = nullptr;
	count--;
}

void ChannelList::remove(ChannelNode *node) {
	if (node == head) {
		popFront();
	} else if (node == tail) {
		popBack();
	} else if (node->next || node->prev) {
		node->next->prev = node->prev;
		node->prev->next = node->next;
		count--;
	} else {
		return;
	}

	node->prev = nullptr;
	node->next = nullptr;
}

void ChannelList::pushBack(ChannelNode *node) {
	if (tail)
		tail->next = node;

	node->next = nullptr;
	node->prev = tail;

	if (!head)
		head = node;

	tail = node;
	count++;
}

void ChannelList::insertByPriority(ChannelNode *node) {
	ChannelNode *cursor = head;

	if (cursor) {
		while (cursor->track->priority > node->track->priority) {
			cursor = cursor->next;

			if (!cursor) {
				pushBack(node);

				return;
			}
		}

		if (cursor == head) {
			head->prev = node;
			node->prev = nullptr;
			node->next = head;
			head = node;

			if (!tail)
				tail = node;
		} else {
			node->next = cursor;
			node->prev = cursor->prev;
			cursor->prev->next = node;
			cursor->prev = node;
		}

		count++;
	} else {
		pushBack(node);
	}
}

template<class T>
AChannel<T>::AChannel() {
	dirty = false;
}

template<class T>
AChannel<T>::~AChannel() {
	for (ChannelNode *node = active.head; node; node = node->next)
		removeChannel(node->inst, this);

	for (ChannelNode *node = inactive.head; node; node = node->next)
		removeChannel(node->inst, this);

	while (active.head) {
		ChannelNode *node = active.head;

		active.popFront();
		delete node;
	}

	while (inactive.head) {
		ChannelNode *node = inactive.head;

		inactive.popFront();
		delete node;
	}
}

template<class T>
bool AChannel<T>::detach(AInst *inst) {
	ChannelList *list = (inst->flags & AInst::kAttached) == 0 ? &inactive : &active;
	ChannelNode *node = list->head;

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
	ChannelNode *node = inactive.head;

	while (node && node->inst != inst)
		node = node->next;

	if (!node) {
		warning("AChannel::activate(): the instance is not bound to this channel");

		return;
	}

	inactive.remove(node);
	active.insertByPriority(node);
}

template<class T>
void AChannel<T>::deactivate(AInst *inst) {
	ChannelNode *node = active.head;

	while (node && node->inst != inst)
		node = node->next;

	if (!node) {
		warning("AChannel::deactivate(): the instance is not active on this channel");

		return;
	}

	active.remove(node);
	inactive.pushBack(node);
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

	if ((inst->flags & AInst::kAttached) != 0)
		active.insertByPriority(node);
	else
		inactive.pushBack(node);

	inst->channels.push_back(this);

	return true;
}

template<>
bool AChannel<CQuaternion>::evaluate(CQuaternion *out) {
	if (active.count == 0)
		return true;

	CQuaternion groupValue[kMaxGroups];
	float groupSum[kMaxGroups];
	int group = 0;
	ChannelNode *start = active.head;
	ChannelNode *node = start;
	int groupPriority = start->track->priority;

	groupSum[0] = 0.0f;

	while (node) {
		ATrackData<CQuaternion> *track = static_cast<ATrackData<CQuaternion> *>(node->track);

		if (track->priority < groupPriority) {
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
			groupPriority = track->priority;
			groupSum[group] = 0.0f;
		}

		AInst *inst = node->inst;

		track->sample(node->lastTime, inst->time, &track->value, this);
		groupSum[group] = inst->weight + groupSum[group];
		node->lastTime = inst->time;
		node = node->next;
	}

	float lastScale = groupSum[group] <= 0.0f ? 0.0f : (float)(1.0 / groupSum[group]);

	accumulateGroup(&groupValue[group], start, node, lastScale);

	for (int i = group; i >= 0; i--) {
		if (groupSum[i] >= 1.0f) {
			*out = groupValue[i];
		} else {
			double weight = groupSum[i];
			CQuaternion scaled((float)(weight * groupValue[i].x), (float)(weight * groupValue[i].y), (float)(weight * groupValue[i].z), (float)(weight * groupValue[i].w));
			double remaining = 1.0 - groupSum[i];
			float remainingW = (float)(remaining * out->w);
			float remainingZ = (float)(remaining * out->z);
			float remainingY = (float)(remaining * out->y);
			float remainingX = (float)(remaining * out->x);

			*out = CQuaternion(remainingX, remainingY, remainingZ, remainingW);

			if (out->dot(scaled) < 0.0f)
				scaled = CQuaternion((float)(scaled.x * -1.0), (float)(scaled.y * -1.0), (float)(scaled.z * -1.0), (float)(scaled.w * -1.0));

			out->x = scaled.x + out->x;
			out->y = scaled.y + out->y;
			out->z = scaled.z + out->z;
			out->w = scaled.w + out->w;
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
