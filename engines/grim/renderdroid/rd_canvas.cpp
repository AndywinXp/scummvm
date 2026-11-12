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

RdCanvas *RenderDroid::CanvasModule::create(uint flags, StdVBuffer *vBuffer, StdVBuffer *zBuffer, int xStart, int yStart, int xEnd, int yEnd, int a8) {
	RdCanvas *canvas = new RdCanvas;

	newEntry(canvas, flags, vBuffer, zBuffer, xStart, yStart, xEnd, yEnd, a8);

	return canvas;
}

int RenderDroid::CanvasModule::newEntry(RdCanvas *canvas, uint flags, StdVBuffer *vBuffer, StdVBuffer *zBuffer, int xStart, int yStart, int xEnd, int yEnd, int a9) {
	canvas->zBuffer = zBuffer;
	canvas->flags = flags;
	canvas->vBuffer = vBuffer;
	canvas->zScaleShift = a9;

	if (flags & 1) {
		canvas->xStart = xStart;
		canvas->yStart = yStart;
		canvas->xEnd = xEnd;
		canvas->yEnd = yEnd;
	} else {
		canvas->xStart = 0;
		canvas->yStart = 0;
		canvas->xEnd = vBuffer->tex.width - 1;
		canvas->yEnd = vBuffer->tex.height - 1;
	}

	double yCenter = (double)(canvas->yEnd - canvas->yStart + 1) * 0.5 + (double)canvas->yStart;
	canvas->xCenter = (float)((double)(canvas->xEnd - canvas->xStart + 1) * 0.5 + (double)canvas->xStart);
	canvas->yCenter = (float)yCenter;

	return 1;
}

void RenderDroid::CanvasModule::destroy(RdCanvas *&canvas) {
	if (canvas) {
		delete canvas;
		canvas = nullptr;
	}
}

} // End of namespace Grim
