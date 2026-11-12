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

#ifndef GRIM_EMI_CSKELETON_H
#define GRIM_EMI_CSKELETON_H

#include "common/array.h"

#include "engines/grim/emi/cquaternion.h"
#include "engines/grim/emi/cvector.h"
#include "engines/grim/emi/lfile.h"

namespace Grim {

class EmiEngine;

class SkeletonNode {
public:
	enum {
		kNameSize = 32
	};

	SkeletonNode();

	char name[kNameSize];
	char parent[kNameSize];
	CVector pos;
	CQuaternion quat;
};

class CSkeleton {
public:
	explicit CSkeleton(EmiEngine *engine);
	virtual ~CSkeleton();

	virtual bool save(LFile *file);
	virtual bool restore(LFile *file);

	bool loadBinary(const char *path);
	bool saveBinary(const char *path);

	Common::Array<SkeletonNode> nodes;

protected:
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
