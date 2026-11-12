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

#include "engines/grim/renderdroid/rd.h"

namespace Grim {

int RenderDroid::FaceModule::newEntry(RdFace3 *face) {
	face->num = 0;
	face->type = 0;
	face->numVertices = 0;
	face->vertices = nullptr;
	face->texVertices = nullptr;
	face->material = nullptr;
	face->curCel = -1;
	face->normal[0] = 0.0f;
	face->normal[1] = 0.0f;
	face->normal[2] = 0.0f;
	face->uvDelta[0] = 0.0f;
	face->uvDelta[1] = 0.0f;
	face->extraLight = 0.0f;

	return 1;
}

void RenderDroid::FaceModule::freeEntry(RdFace3 *face) {
	if (face->vertices) {
		delete[] face->vertices;
		face->vertices = nullptr;
	}

	if (face->texVertices) {
		delete[] face->texVertices;
		face->texVertices = nullptr;
	}
}

} // End of namespace Grim
