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

#ifndef GRIM_EMI_RNODE_H
#define GRIM_EMI_RNODE_H

#include "common/array.h"

#include "engines/grim/emi/cnode.h"

namespace Grim {

class Camera;
class RPrimitiveInstance;

class RNode : public CNode {
public:
	enum {
		kDebugEnabled = 0x01,
		kDebugAxes = 0x02,
		kDebugParentLine = 0x04,
		kNoOutput = 0x80
	};

	RNode();
	virtual ~RNode();

	virtual void drawDebug(Camera *camera);
	virtual void draw(Camera *camera);

	int findPrimitiveInstance(RPrimitiveInstance *instance);
	void removePrimitiveInstance(int index);

	Common::Array<RPrimitiveInstance *> instances;
	int lastFrame;
	uint drawFlags;
};

} // End of namespace Grim

#endif
