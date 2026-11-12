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

#ifndef GRIM_EMI_ANODE_H
#define GRIM_EMI_ANODE_H

#include "engines/grim/emi/achannel.h"
#include "engines/grim/emi/cnode.h"

namespace Grim {

class EmiEngine;

class ANode : public CNode {
public:
	explicit ANode(EmiEngine *engine);
	virtual ~ANode();

	void *operator new(size_t size);
	void operator delete(void *block);

	void bindTrack(AInst *inst, ATrack *track);
	void unbindTrack(AInst *inst, ATrack *track);
	void updateANode();

	int boundTracks;
	bool dirty;
	AChannel<CVector> *posChannel;
	AChannel<CQuaternion> *rotChannel;

private:
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
