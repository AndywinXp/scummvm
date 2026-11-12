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

#include "engines/grim/emi/graphics/cdcvertexarray.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

void *CDCVertex::operator new[](size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void CDCVertex::operator delete[](void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

int VertexStreams::getDataSize() const {
	int vertexSize = 0;

	if (positions)
		vertexSize = 3 * sizeof(float);

	if (normals)
		vertexSize += 3 * sizeof(float);

	if (colors)
		vertexSize += sizeof(uint32);

	if (uvs)
		vertexSize += 2 * sizeof(float);

	return vertexSize * count;
}

CDCVertexArray::CDCVertexArray() {
	numVerts = 0;
	lockCount = 0;
}

CDCVertexArray::~CDCVertexArray() {
}

void *CDCVertexArray::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void CDCVertexArray::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

} // End of namespace Grim
