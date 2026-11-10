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

#include <math.h>

#include "common/scummsys.h"
#include "common/textconsole.h"

#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/cdrawcontext.h"

namespace Grim {

static const float kPi = 3.14159274f;
static const float kHalfTurnDegrees = 180.0f;
static const float kDegreesToRadians = 3.14159274f * 0.00555555569f;
static const float kRadiansToDegrees = kHalfTurnDegrees / kPi;
static const int kDefaultViewportWidth = 640;
static const int kDefaultViewportHeight = 480;
static const float kDefaultFov = 90.0f;

Camera::Camera() {
	nearClip = 0.0f;
	farClip = 0.0f;

	view.rot.setIdentity();
	view.pos.x = 0.0f;
	view.pos.y = 0.0f;
	view.pos.z = 0.0f;

	IRect viewport;

	viewport.x = 0;
	viewport.y = 0;
	viewport.w = kDefaultViewportWidth;
	viewport.h = kDefaultViewportHeight;

	setViewport(&viewport, kDefaultFov);
	updateViewMatrix();

	reserved368 = nullptr;
	reserved372 = false;
}

Camera::~Camera() {
}

void Camera::applyProjection() {
	if (CDrawContext::context)
		CDrawContext::context->setPerspective(fovX, fovY, nearClip, farClip);
}

void Camera::updateViewMatrix() {
	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	const float *m = world.rot.m;
	float negX = -world.pos.x;
	float negY = -world.pos.y;
	float negZ = -world.pos.z;
	float posX = (float)((double)negZ * m[2] + (double)negY * m[1] + (double)negX * m[0]);
	float posY = (float)((double)negX * m[3] + (double)negZ * m[5] + (double)negY * m[4]);
	float posZ = (float)((double)negZ * m[8] + (double)negY * m[7] + (double)negX * m[6]);

	view.rot.m[0] = m[0];
	view.rot.m[1] = m[3];
	view.rot.m[2] = m[6];
	view.rot.m[3] = m[1];
	view.rot.m[4] = m[4];
	view.rot.m[5] = m[7];
	view.rot.m[6] = m[2];
	view.rot.m[7] = m[5];
	view.rot.m[8] = m[8];
	view.pos.x = posX;
	view.pos.y = posY;
	view.pos.z = posZ;
}

void Camera::setFov(float newFovX, float newFovY) {
	fovX = newFovX;
	fovY = newFovY;

	double tanHalfX = tan((double)newFovX * 0.5 * kDegreesToRadians);
	double tanHalfY = tan((double)newFovY * 0.5 * kDegreesToRadians);

	if (tanHalfX == 0.0) {
		// The original could divide by zero here, let's avoid that...
		warning("Camera::setFov(): invalid horizontal field of view");
		invTanHalfFovX = 0.0f;
	} else {
		invTanHalfFovX = (float)(1.0 / tanHalfX);
	}

	if (tanHalfY == 0.0) {
		// The original could divide by zero here, let's avoid that...
		warning("Camera::setFov(): invalid vertical field of view");
		invTanHalfFovY = 0.0f;
	} else {
		invTanHalfFovY = (float)(1.0 / tanHalfY);
	}

	focalX = (float)((double)halfWidth * invTanHalfFovX);
	focalY = (float)((double)halfHeight * invTanHalfFovY);
}

void Camera::setViewport(const IRect *rect, float newFovX) {
	halfWidth = (float)((double)rect->w * 0.5);
	halfHeight = (float)((double)rect->h * 0.5);

	if (newFovX != 0.0f) {
		if (rect->w == 0) {
			// The original could divide by zero here, let's avoid that...
			warning("Camera::setViewport(): the viewport has no width");
			return;
		}

		double angle = atan2(tan((double)newFovX * 0.5 * kDegreesToRadians) * ((double)rect->h / (double)rect->w), 1.0);
		float newFovY = (float)(angle * kRadiansToDegrees * 2.0);

		setFov(newFovX, newFovY);
	}
}

IRect Camera::getViewportRect() {
	int rect[4] = { 0, 0, 0, 0 };
	IRect result;

	if (CDrawContext::context)
		CDrawContext::context->getViewportRect(rect);
	else
		warning("Camera::getViewportRect(): no draw context");

	result.x = rect[0];
	result.y = rect[1];
	result.w = rect[2];
	result.h = rect[3];

	return result;
}

void Camera::drawLineLoop(DrawVertex *vertices, int count) {
	if (CDrawContext::context)
		CDrawContext::context->drawPrim(4, vertices, count, false);
}

} // End of namespace Grim
