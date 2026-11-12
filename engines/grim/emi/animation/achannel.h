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

#ifndef GRIM_EMI_ACHANNEL_H
#define GRIM_EMI_ACHANNEL_H

#include "engines/grim/emi/animation/atrack.h"
#include "engines/grim/emi/math/cquaternion.h"
#include "engines/grim/emi/math/cvector.h"

namespace Grim {

class AInst;

struct ChannelNode {
	void *operator new(size_t size);
	void operator delete(void *block);

	ChannelNode *prev;
	ChannelNode *next;
	ATrack *track;
	AInst *inst;
	float lastTime;
};

class ChannelList {
public:
	ChannelList();

	void popFront();
	void popBack();
	void remove(ChannelNode *node);
	void pushBack(ChannelNode *node);
	void insertByPriority(ChannelNode *node);

	int count;
	ChannelNode *head;
	ChannelNode *tail;
};

template<class T>
class AChannel : public AChannelHandle {
public:
	AChannel();
	virtual ~AChannel();

	virtual bool detach(AInst *inst);
	virtual void activate(AInst *inst);
	virtual void deactivate(AInst *inst);
	virtual void onKey(ATrack *track, float time, void *key);

	bool addTrack(AInst *inst, ATrack *track);
	bool evaluate(T *out);

	bool dirty;
	ChannelList active;
	ChannelList inactive;
};

template<> bool AChannel<CQuaternion>::evaluate(CQuaternion *out);

} // End of namespace Grim

#endif
