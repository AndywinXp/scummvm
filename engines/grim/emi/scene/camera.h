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

#ifndef GRIM_EMI_CAMERA_H
#define GRIM_EMI_CAMERA_H

#include "engines/grim/emi/graphics/cdrawlist.h"
#include "engines/grim/emi/nodes/cnode.h"
#include "engines/grim/emi/math/ctransform.h"
#include "engines/grim/emi/math/irect.h"

namespace Grim {

class CDrawContext;

class Camera : public CNode {
public:
	Camera(CDrawContext *drawContext);
	virtual ~Camera();

	CDrawContext *getDrawContext() const;

	void applyProjection();
	void updateViewMatrix();
	void setFov(float newFovX, float newFovY);
	void setViewport(const IRect *rect, float newFovX);
	IRect getViewportRect();

	void drawLineLoop(DrawVertex *vertices, int count);

	float nearClip;
	float farClip;
	float fovX;
	float invTanHalfFovX;
	float halfWidth;
	float focalX;
	float fovY;
	float invTanHalfFovY;
	float halfHeight;
	float focalY;
	CTransform view;
	void *reserved368;
	bool reserved372;

private:
	CDrawContext *_drawContext;
};

} // End of namespace Grim

#endif
