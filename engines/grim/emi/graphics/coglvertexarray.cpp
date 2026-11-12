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
	_numVerts = 0;
	_data = nullptr;

	if (!source)
		return;

	int vertexCount = source->_numVerts;

	if (vertexCount < 0) {
		warning("COGLVertexArray::COGLVertexArray(): negative vertex count");
		vertexCount = 0;
	}

	_data = new CDCVertex[vertexCount]();
	memcpy(_data, source->lock(), vertexCount * sizeof(CDCVertex));
	source->unlock();
	_numVerts = vertexCount;
}

COGLVertexArray::COGLVertexArray(const VertexStreams *streams) {
	_numVerts = 0;
	_data = nullptr;

	if (!streams)
		return;

	int vertexCount = streams->count;

	if (vertexCount < 0) {
		warning("COGLVertexArray::COGLVertexArray(): negative vertex count");
		vertexCount = 0;
	}

	_data = new CDCVertex[vertexCount]();

	for (int i = 0; i < vertexCount; i++) {
		if (streams->positions) {
			_data[i].x = streams->positions[3 * i];
			_data[i].y = streams->positions[3 * i + 1];
			_data[i].z = streams->positions[3 * i + 2];
		}

		if (streams->colors)
			_data[i].color = streams->colors[i];

		if (streams->uvs) {
			_data[i].u = streams->uvs[2 * i];
			_data[i].v = streams->uvs[2 * i + 1];
		}
	}

	_numVerts = vertexCount;
}

COGLVertexArray::COGLVertexArray(int vertexCount, const float *positions, const uint32 *colors, const float *uvs) {
	if (vertexCount < 0) {
		warning("COGLVertexArray::COGLVertexArray(): negative vertex count");
		vertexCount = 0;
	}

	_numVerts = vertexCount;
	_data = new CDCVertex[vertexCount]();

	for (int i = 0; i < vertexCount; i++) {
		if (positions) {
			_data[i].x = positions[3 * i];
			_data[i].y = positions[3 * i + 1];
			_data[i].z = positions[3 * i + 2];
		}

		if (colors)
			_data[i].color = colors[i];

		if (uvs) {
			_data[i].u = uvs[2 * i];
			_data[i].v = uvs[2 * i + 1];
		}
	}
}

COGLVertexArray::~COGLVertexArray() {
	delete[] _data;
	_data = nullptr;
}

void *COGLVertexArray::lock() {
	_lockCount++;

	return _data;
}

void COGLVertexArray::unlock() {
	_lockCount--;
}

} // End of namespace Grim
