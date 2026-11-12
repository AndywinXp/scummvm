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

#include "engines/grim/emi/graphics/coglvertexarray.h"

namespace Grim {

COGLVertexArray::COGLVertexArray(CDCVertexArray *source) {
	numVerts = 0;
	data = nullptr;

	if (!source)
		return;

	int vertexCount = source->numVerts;

	if (vertexCount < 0) {
		warning("COGLVertexArray::COGLVertexArray(): negative vertex count");
		vertexCount = 0;
	}

	data = new CDCVertex[vertexCount]();
	memcpy(data, source->lock(), vertexCount * sizeof(CDCVertex));
	source->unlock();
	numVerts = vertexCount;
}

COGLVertexArray::COGLVertexArray(const VertexStreams *streams) {
	numVerts = 0;
	data = nullptr;

	if (!streams)
		return;

	int vertexCount = streams->count;

	if (vertexCount < 0) {
		warning("COGLVertexArray::COGLVertexArray(): negative vertex count");
		vertexCount = 0;
	}

	data = new CDCVertex[vertexCount]();

	for (int i = 0; i < vertexCount; i++) {
		if (streams->positions) {
			data[i].x = streams->positions[3 * i];
			data[i].y = streams->positions[3 * i + 1];
			data[i].z = streams->positions[3 * i + 2];
		}

		if (streams->colors)
			data[i].color = streams->colors[i];

		if (streams->uvs) {
			data[i].u = streams->uvs[2 * i];
			data[i].v = streams->uvs[2 * i + 1];
		}
	}

	numVerts = vertexCount;
}

COGLVertexArray::COGLVertexArray(int vertexCount, const float *positions, const uint32 *colors, const float *uvs) {
	if (vertexCount < 0) {
		warning("COGLVertexArray::COGLVertexArray(): negative vertex count");
		vertexCount = 0;
	}

	numVerts = vertexCount;
	data = new CDCVertex[vertexCount]();

	for (int i = 0; i < vertexCount; i++) {
		if (positions) {
			data[i].x = positions[3 * i];
			data[i].y = positions[3 * i + 1];
			data[i].z = positions[3 * i + 2];
		}

		if (colors)
			data[i].color = colors[i];

		if (uvs) {
			data[i].u = uvs[2 * i];
			data[i].v = uvs[2 * i + 1];
		}
	}
}

COGLVertexArray::~COGLVertexArray() {
	delete[] data;
	data = nullptr;
}

void *COGLVertexArray::lock() {
	lockCount++;

	return data;
}

void COGLVertexArray::unlock() {
	lockCount--;
}

} // End of namespace Grim
