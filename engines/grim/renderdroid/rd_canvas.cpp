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

RdCanvas *RenderDroid::CanvasModule::create(uint flags, StdVBuffer *pVBuffer, StdVBuffer *pZBuffer, int xStart, int yStart, int xEnd, int yEnd, int a8) {
	RdCanvas *pCanvas = new RdCanvas;

	newEntry(pCanvas, flags, pVBuffer, pZBuffer, xStart, yStart, xEnd, yEnd, a8);

	return pCanvas;
}

int RenderDroid::CanvasModule::newEntry(RdCanvas *pCanvas, uint flags, StdVBuffer *pVBuffer, StdVBuffer *pZBuffer, int xStart, int yStart, int xEnd, int yEnd, int a9) {
	pCanvas->pZBuffer = pZBuffer;
	pCanvas->flags = flags;
	pCanvas->pVBuffer = pVBuffer;
	pCanvas->zScaleShift = a9;

	if (flags & 1) {
		pCanvas->xStart = xStart;
		pCanvas->yStart = yStart;
		pCanvas->xEnd = xEnd;
		pCanvas->yEnd = yEnd;
	} else {
		pCanvas->xStart = 0;
		pCanvas->yStart = 0;
		pCanvas->xEnd = pVBuffer->tex.width - 1;
		pCanvas->yEnd = pVBuffer->tex.height - 1;
	}

	double yCenter = (double)(pCanvas->yEnd - pCanvas->yStart + 1) * 0.5 + (double)pCanvas->yStart;
	pCanvas->xCenter = (float)((double)(pCanvas->xEnd - pCanvas->xStart + 1) * 0.5 + (double)pCanvas->xStart);
	pCanvas->yCenter = (float)yCenter;

	return 1;
}

void RenderDroid::CanvasModule::destroy(RdCanvas *&pCanvas) {
	if (pCanvas) {
		delete pCanvas;
		pCanvas = nullptr;
	}
}

} // End of namespace Grim
