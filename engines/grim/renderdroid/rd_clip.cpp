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

#include "engines/grim/renderdroid/rd.h"

namespace Grim {

int RenderDroid::ClipModule::calcOutcode2(RdCanvas *pCanvas, int x, int y) {
	int outcode = 0;

	if (x >= pCanvas->xStart) {
		if (x > pCanvas->xEnd)
			outcode = 0x10;
	} else {
		outcode = 1;
	}

	if (y >= pCanvas->yStart) {
		if (y > pCanvas->yEnd)
			outcode |= 0x100;
	} else {
		outcode |= 0x1000;
	}

	return outcode;
}

int RenderDroid::ClipModule::line2(RdCanvas *pCanvas, int *x0, int *y0, int *x1, int *y1) {
	int outcode0 = calcOutcode2(pCanvas, *x0, *y0);
	int outcode1 = calcOutcode2(pCanvas, *x1, *y1);

	if (!outcode0 && !outcode1)
		return 1;

	if ((outcode0 & outcode1) != 0)
		return 0;

	float fx0 = (float)*x0;
	float fx1 = (float)*x1;
	float fy0 = (float)*y0;
	float fy1 = (float)*y1;
	bool vertical = (double)*x0 == fx1;
	bool horizontal = fy0 == fy1;

	while (true) {
		int outcode = outcode0 ? outcode0 : outcode1;
		double newX;
		double newY;

		if ((outcode & 0x1000) != 0) {
			if (vertical)
				newX = fx0;
			else
				newX = ((double)fx1 - fx0) / ((double)fy1 - fy0) * ((double)pCanvas->yStart - fy0) + fx0;
			newY = (double)pCanvas->yStart;
		} else if ((outcode & 0x100) != 0) {
			if (vertical)
				newX = fx0;
			else
				newX = ((double)fx1 - fx0) / ((double)fy1 - fy0) * ((double)pCanvas->yEnd - fy0) + fx0;
			newY = (double)pCanvas->yEnd;
		} else {
			float clippedY;

			if ((outcode & 0x10) != 0) {
				if (horizontal)
					clippedY = fy0;
				else
					clippedY = (float)(((double)fy1 - fy0) / ((double)fx1 - fx0) * ((double)pCanvas->xEnd - fx0) + fy0);
				newX = (double)pCanvas->xEnd;
			} else {
				if (horizontal)
					clippedY = fy0;
				else
					clippedY = (float)(((double)fy1 - fy0) / ((double)fx1 - fx0) * ((double)pCanvas->xStart - fx0) + fy0);
				newX = (double)pCanvas->xStart;
			}

			newY = clippedY;
		}

		if (outcode == outcode0) {
			fx0 = (float)newX;
			fy0 = (float)newY;
			outcode0 = calcOutcode2(pCanvas, (int)newX, (int)newY);
		} else {
			fx1 = (float)newX;
			fy1 = (float)newY;
			outcode1 = calcOutcode2(pCanvas, (int)newX, (int)newY);
		}

		if (!outcode0 && !outcode1)
			break;

		if ((outcode0 & outcode1) != 0)
			return 0;
	}

	*x0 = (int)fx0;
	*y0 = (int)fy0;
	*x1 = (int)fx1;
	*y1 = (int)fy1;

	return 1;
}

int RenderDroid::ClipModule::point3(RdClipFrustum *pClip, RdVector3 pt) {
	RdCamera *pCamera = _rd->_camera.getCurrent();
	int projectType = pCamera->projectType;

	if (pt[1] < (double)pClip->zNear)
		return 0;

	if (pClip->bClipFar && pt[1] > (double)pClip->zFar)
		return 0;

	double limit;

	if (projectType == 1)
		limit = (double)pClip->left * pt[1];
	else
		limit = pClip->orthoLeft;
	if (limit > pt[0])
		return 0;

	if (projectType == 1)
		limit = (double)pClip->right * pt[1];
	else
		limit = pClip->orthoRight;
	if (limit < pt[0])
		return 0;

	if (projectType == 1)
		limit = (double)pClip->top * pt[1];
	else
		limit = pClip->orthoTop;
	if (limit < pt[2])
		return 0;

	if (projectType == 1)
		limit = (double)pClip->bottom * pt[1];
	else
		limit = pClip->orthoBottom;

	return limit <= pt[2];
}

int RenderDroid::ClipModule::line3(RdClipFrustum *pClip, RdVector3 v0, RdVector3 v1, int *pClipped0, int *pClipped1) {
	if (v0[1] < (double)pClip->zNear && v1[1] < (double)pClip->zNear)
		return 0;

	if (v0[1] >= (double)pClip->zNear) {
		if (v1[1] < (double)pClip->zNear) {
			double t = ((double)pClip->zNear - v0[1]) / ((double)v1[1] - v0[1]);
			float x = v1[0];

			v1[1] = pClip->zNear;
			v1[2] = (float)(((double)v1[2] - v0[2]) * t + v0[2]);
			v1[0] = (float)(((double)x - v0[0]) * t + v0[0]);
			if (pClipped1)
				*pClipped1 = 1;
		}
	} else {
		double t = ((double)pClip->zNear - v0[1]) / ((double)v1[1] - v0[1]);

		v0[1] = pClip->zNear;
		v0[2] = (float)(((double)v1[2] - v0[2]) * t + v0[2]);
		v0[0] = (float)(((double)v1[0] - v0[0]) * t + v0[0]);
		if (pClipped0)
			*pClipped0 = 1;
	}

	if (pClip->bClipFar) {
		if (v0[1] > (double)pClip->zFar && v1[1] > (double)pClip->zFar)
			return 0;

		if (v0[1] <= (double)pClip->zFar) {
			if (v1[1] > (double)pClip->zFar) {
				double t = ((double)pClip->zFar - v0[1]) / ((double)v1[1] - v0[1]);
				float x = v1[0];

				v1[1] = pClip->zFar;
				v1[2] = (float)(((double)v1[2] - v0[2]) * t + v0[2]);
				v1[0] = (float)(((double)x - v0[0]) * t + v0[0]);
				if (pClipped1)
					*pClipped1 = 1;
			}
		} else {
			double t = ((double)pClip->zFar - v0[1]) / ((double)v1[1] - v0[1]);

			v0[1] = pClip->zFar;
			v0[2] = (float)(((double)v1[2] - v0[2]) * t + v0[2]);
			v0[0] = (float)(((double)v1[0] - v0[0]) * t + v0[0]);
			if (pClipped0)
				*pClipped0 = 1;
		}
	}

	float leftLimit = (float)((double)pClip->left * v1[1]);

	if (v0[0] < (double)pClip->left * v0[1] && v1[0] < (double)leftLimit)
		return 0;

	if ((double)pClip->left * v0[1] <= v0[0]) {
		if (v1[0] < (double)leftLimit) {
			float dy = (float)((double)v1[1] - v0[1]);
			float dx = (float)((double)v1[0] - v0[0]);
			double c = (double)v1[1] * v0[0] - (double)v0[1] * v1[0];
			double denominator = (double)pClip->left * dy - dx;

			if (denominator != 0.0)
				c = c / (float)denominator;

			double s = (double)pClip->left * c;
			double t;

			if (fabs((double)dy) <= fabs((double)dx))
				t = (s - v0[0]) / dx;
			else
				t = (c - v0[1]) / dy;

			float z = v1[2];
			v1[0] = (float)s;
			v1[1] = (float)c;
			v1[2] = (float)(((double)z - v0[2]) * t + v0[2]);
			if (pClipped1)
				*pClipped1 = 1;
		}
	} else {
		float dy = (float)((double)v1[1] - v0[1]);
		float dx = (float)((double)v1[0] - v0[0]);
		double c = (double)v1[1] * v0[0] - (double)v0[1] * v1[0];
		double denominator = (double)pClip->left * dy - dx;

		if (denominator != 0.0)
			c = c / (float)denominator;

		float s = (float)((double)pClip->left * c);
		double t;

		if (fabs((double)dy) <= fabs((double)dx))
			t = ((double)s - v0[0]) / dx;
		else
			t = (c - v0[1]) / dy;

		v0[0] = s;
		v0[1] = (float)c;
		v0[2] = (float)(((double)v1[2] - v0[2]) * t + v0[2]);
		if (pClipped0)
			*pClipped0 = 1;
	}

	float rightLimit = (float)((double)pClip->right * v1[1]);

	if (v0[0] > (double)pClip->right * v0[1] && v1[0] > (double)rightLimit)
		return 0;

	if ((double)pClip->right * v0[1] >= v0[0]) {
		if (v1[0] > (double)rightLimit) {
			float dy = (float)((double)v1[1] - v0[1]);
			float dx = (float)((double)v1[0] - v0[0]);
			double c = (double)v1[1] * v0[0] - (double)v0[1] * v1[0];
			double denominator = (double)pClip->right * dy - dx;

			if (denominator != 0.0)
				c = c / (float)denominator;

			double s = (double)pClip->right * c;
			double t;

			if (fabs((double)dy) <= fabs((double)dx))
				t = (s - v0[0]) / dx;
			else
				t = (c - v0[1]) / dy;

			float z = v1[2];
			v1[0] = (float)s;
			v1[1] = (float)c;
			v1[2] = (float)(((double)z - v0[2]) * t + v0[2]);
			if (pClipped1)
				*pClipped1 = 1;
		}
	} else {
		float dy = (float)((double)v1[1] - v0[1]);
		float dx = (float)((double)v1[0] - v0[0]);
		double c = (double)v1[1] * v0[0] - (double)v0[1] * v1[0];
		double denominator = (double)pClip->right * dy - dx;

		if (denominator != 0.0)
			c = c / (float)denominator;

		float s = (float)((double)pClip->right * c);
		double t;

		if (fabs((double)dy) <= fabs((double)dx))
			t = ((double)s - v0[0]) / dx;
		else
			t = (c - v0[1]) / dy;

		v0[0] = s;
		v0[1] = (float)c;
		v0[2] = (float)(((double)v1[2] - v0[2]) * t + v0[2]);
		if (pClipped0)
			*pClipped0 = 1;
	}

	float topLimit = (float)((double)pClip->top * v1[1]);

	if (v0[2] > (double)pClip->top * v0[1] && v1[2] > (double)topLimit)
		return 0;

	if ((double)pClip->top * v0[1] >= v0[2]) {
		if (v1[2] > (double)topLimit) {
			float dy = (float)((double)v1[1] - v0[1]);
			float dz = (float)((double)v1[2] - v0[2]);
			double denominator = (double)pClip->top * dy - dz;
			double c = (double)v1[1] * v0[2] - (double)v0[1] * v1[2];

			if (denominator != 0.0)
				c = c / (float)denominator;

			double s = (double)pClip->top * c;
			double t;

			if (fabs((double)dy) <= fabs((double)dz))
				t = (s - v0[2]) / dz;
			else
				t = (c - v0[1]) / dy;

			double x = ((double)v1[0] - v0[0]) * t + v0[0];
			v1[1] = (float)c;
			v1[2] = (float)s;
			v1[0] = (float)x;
			if (pClipped1)
				*pClipped1 = 1;
		}
	} else {
		float dy = (float)((double)v1[1] - v0[1]);
		float dz = (float)((double)v1[2] - v0[2]);
		double denominator = (double)pClip->top * dy - dz;
		double c = (double)v1[1] * v0[2] - (double)v0[1] * v1[2];

		if (denominator != 0.0)
			c = c / (float)denominator;

		double s = (double)pClip->top * c;
		double t;

		if (fabs((double)dy) <= fabs((double)dz))
			t = (s - v0[2]) / dz;
		else
			t = (c - v0[1]) / dy;

		double x = ((double)v1[0] - v0[0]) * t;
		v0[1] = (float)c;
		x = x + v0[0];
		v0[2] = (float)s;
		v0[0] = (float)x;
		if (pClipped0)
			*pClipped0 = 1;
	}

	float bottomLimit = (float)((double)pClip->bottom * v1[1]);

	if (v0[2] < (double)pClip->bottom * v0[1] && v1[2] < (double)bottomLimit)
		return 0;

	if ((double)pClip->bottom * v0[1] <= v0[2]) {
		if (v1[2] < (double)bottomLimit) {
			float dy = (float)((double)v1[1] - v0[1]);
			float dz = (float)((double)v1[2] - v0[2]);
			double denominator = (double)pClip->bottom * dy - dz;
			double c = (double)v1[1] * v0[2] - (double)v0[1] * v1[2];

			if (denominator != 0.0)
				c = c / (float)denominator;

			double s = (double)pClip->bottom * c;
			double t;

			if (fabs((double)dy) <= fabs((double)dz))
				t = (s - v0[2]) / dz;
			else
				t = (c - v0[1]) / dy;

			double x = ((double)v1[0] - v0[0]) * t + v0[0];
			v1[1] = (float)c;
			v1[2] = (float)s;
			v1[0] = (float)x;
			if (pClipped1)
				*pClipped1 = 1;
		}
	} else {
		float dy = (float)((double)v1[1] - v0[1]);
		float dz = (float)((double)v1[2] - v0[2]);
		double denominator = (double)pClip->bottom * dy - dz;
		double c = (double)v1[1] * v0[2] - (double)v0[1] * v1[2];

		if (denominator != 0.0)
			c = c / (float)denominator;

		double s = (double)pClip->bottom * c;
		double t;

		if (fabs((double)dy) <= fabs((double)dz))
			t = (s - v0[2]) / dz;
		else
			t = (c - v0[1]) / dy;

		double x = ((double)v1[0] - v0[0]) * t;
		v0[1] = (float)c;
		x = x + v0[0];
		v0[2] = (float)s;
		v0[0] = (float)x;
		if (pClipped0)
			*pClipped0 = 1;
	}

	return 1;
}

int RenderDroid::ClipModule::sphereInFrustrum(RdClipFrustum *pClip, RdVector3 center, float radius) {
	RdCamera *pCamera = _rd->_camera.getCurrent();
	float farEdge = (float)((double)radius + center[1]);
	float nearEdge = (float)((double)center[1] - radius);

	if (farEdge < (double)pClip->zNear)
		return 2;

	int inside = nearEdge >= (double)pClip->zNear;

	if (pClip->bClipFar) {
		if (nearEdge > (double)pClip->zFar)
			return 2;

		if (farEdge > (double)pClip->zFar)
			inside = 0;
	}

	int projectType = pCamera->projectType;
	float topEdge = (float)((double)radius + center[2]);
	float bottomEdge = (float)((double)center[2] - radius);
	double limitNear;
	float limitFar;

	if (projectType == 1) {
		limitNear = (double)pClip->top * nearEdge;
		limitFar = (float)((double)pClip->top * farEdge);
	} else {
		limitNear = pClip->orthoTop;
		limitFar = pClip->orthoTop;
	}

	if (bottomEdge > limitNear && bottomEdge > (double)limitFar)
		return 2;

	if (topEdge > limitNear || topEdge > (double)limitFar)
		inside = 0;

	if (projectType == 1) {
		limitNear = (double)pClip->bottom * nearEdge;
		limitFar = (float)((double)pClip->bottom * farEdge);
	} else {
		limitNear = pClip->orthoBottom;
		limitFar = pClip->orthoBottom;
	}

	if (topEdge < limitNear && topEdge < (double)limitFar)
		return 2;

	if (bottomEdge < limitNear || bottomEdge < (double)limitFar)
		inside = 0;

	float rightEdge = (float)((double)radius + center[0]);
	float leftEdge = (float)((double)center[0] - radius);

	if (projectType == 1) {
		limitNear = (double)pClip->left * nearEdge;
		limitFar = (float)((double)pClip->left * farEdge);
	} else {
		limitNear = pClip->orthoLeft;
		limitFar = pClip->orthoLeft;
	}

	if (rightEdge < limitNear && rightEdge < (double)limitFar)
		return 2;

	if (leftEdge < limitNear || leftEdge < (double)limitFar)
		inside = 0;

	if (projectType == 1) {
		limitNear = (double)pClip->right * nearEdge;
		limitFar = (float)((double)pClip->right * farEdge);
	} else {
		limitNear = pClip->orthoRight;
		limitFar = pClip->orthoRight;
	}

	if (leftEdge > limitNear && leftEdge > (double)limitFar)
		return 2;

	if (rightEdge > limitNear || rightEdge > (double)limitFar)
		inside = 0;

	return !inside;
}

int RenderDroid::ClipModule::face3W(RdClipFrustum *pClip, RdVector3 *aVerts, int numVerts) {
	return clipFace(pClip, aVerts, nullptr, nullptr, numVerts, false, false);
}

int RenderDroid::ClipModule::face3WOrtho(RdClipFrustum *pClip, RdVector3 *aVerts, int numVerts) {
	return clipFace(pClip, aVerts, nullptr, nullptr, numVerts, false, true);
}

int RenderDroid::ClipModule::face3S(RdClipFrustum *pClip, RdVector3 *aVerts, int numVerts) {
	return clipFace(pClip, aVerts, nullptr, nullptr, numVerts, true, false);
}

int RenderDroid::ClipModule::face3SOrtho(RdClipFrustum *pClip, RdVector3 *aVerts, int numVerts) {
	return clipFace(pClip, aVerts, nullptr, nullptr, numVerts, true, true);
}

int RenderDroid::ClipModule::face3GS(RdClipFrustum *pClip, RdVector3 *aVerts, float *aIntensities, int numVerts) {
	return clipFace(pClip, aVerts, nullptr, aIntensities, numVerts, true, false);
}

int RenderDroid::ClipModule::face3GSOrtho(RdClipFrustum *pClip, RdVector3 *aVerts, float *aIntensities, int numVerts) {
	return clipFace(pClip, aVerts, nullptr, aIntensities, numVerts, true, true);
}

int RenderDroid::ClipModule::face3T(RdClipFrustum *pClip, RdVector3 *aVerts, RdVector2 *aTexVerts, int numVerts) {
	return clipFace(pClip, aVerts, aTexVerts, nullptr, numVerts, true, false);
}

int RenderDroid::ClipModule::face3TOrtho(RdClipFrustum *pClip, RdVector3 *aVerts, RdVector2 *aTexVerts, int numVerts) {
	return clipFace(pClip, aVerts, aTexVerts, nullptr, numVerts, true, true);
}

int RenderDroid::ClipModule::face3GT(RdClipFrustum *pClip, RdVector3 *aVerts, RdVector2 *aTexVerts, float *aIntensities, int numVerts) {
	return clipFace(pClip, aVerts, aTexVerts, aIntensities, numVerts, true, false);
}

int RenderDroid::ClipModule::face3GTOrtho(RdClipFrustum *pClip, RdVector3 *aVerts, RdVector2 *aTexVerts, float *aIntensities, int numVerts) {
	return clipFace(pClip, aVerts, aTexVerts, aIntensities, numVerts, true, true);
}

int RenderDroid::ClipModule::clipFace(RdClipFrustum *pClip, RdVector3 *aVerts, RdVector2 *aTexVerts, float *aIntensities, int numVerts, bool bGuard, bool bOrtho) {
	RdClipPoly caller;
	RdClipPoly work;
	RdClipPoly *pSrc = &caller;
	RdClipPoly *pDst = &work;
	RdClipPoly *pTemp;
	int count;

	_faceStatus = 0;

	if (numVerts > 32) {
		warning("RenderDroid::ClipModule::clipFace(): too many vertices (%d)", numVerts);
		return 0;
	}

	caller.aVerts = aVerts;
	caller.aTexVerts = aTexVerts;
	caller.aIntensities = aIntensities;
	work.aVerts = _aVertBufferA;
	work.aTexVerts = aTexVerts ? _workTVerts : nullptr;
	work.aIntensities = aIntensities ? _workIVerts : nullptr;

	if (bOrtho)
		count = clipOrtho(pSrc, numVerts, pDst, 0, pClip->orthoLeft, true, 0x10);
	else
		count = clipPerspectiveX(pSrc, numVerts, pDst, bGuard ? pClip->guardLeft : pClip->left, true, 0x10);

	if (count < 3)
		return count;

	pTemp = pSrc;
	pSrc = pDst;
	pDst = pTemp;

	if (bOrtho)
		count = clipOrtho(pSrc, count, pDst, 0, pClip->orthoRight, false, 0x20);
	else
		count = clipPerspectiveX(pSrc, count, pDst, pClip->right, false, 0x20);

	if (count < 3)
		return count;

	pTemp = pSrc;
	pSrc = pDst;
	pDst = pTemp;

	if (bOrtho)
		count = clipOrtho(pSrc, count, pDst, 2, pClip->orthoTop, false, 4);
	else
		count = clipPerspectiveZ(pSrc, count, pDst, bGuard ? pClip->guardTop : pClip->top, false, 4);

	if (count < 3)
		return count;

	pTemp = pSrc;
	pSrc = pDst;
	pDst = pTemp;

	if (bOrtho)
		count = clipOrtho(pSrc, count, pDst, 2, pClip->orthoBottom, true, 8);
	else
		count = clipPerspectiveZ(pSrc, count, pDst, pClip->bottom, true, 8);

	if (count < 3)
		return count;

	pTemp = pSrc;
	pSrc = pDst;
	pDst = pTemp;

	count = clipDepth(pSrc, count, pDst, pClip->zNear, true, 1);

	if (count < 3) {
		_faceStatus |= 0x40;
		return count;
	}

	if (pClip->bClipFar) {
		pTemp = pSrc;
		pSrc = pDst;
		pDst = pTemp;

		count = clipDepth(pSrc, count, pDst, pClip->zFar, false, 2);

		if (count < 3)
			return count;
	}

	if (pDst->aVerts != aVerts) {
		memcpy(aVerts, pDst->aVerts, sizeof(RdVector3) * count);

		if (aTexVerts)
			memcpy(aTexVerts, pDst->aTexVerts, sizeof(RdVector2) * count);

		if (aIntensities)
			memcpy(aIntensities, pDst->aIntensities, sizeof(float) * count);
	}

	return count;
}

int RenderDroid::ClipModule::faceToPlane(RdVector3 norm, RdVector3 pt, RdVector3 *aVerts, RdVector2 *aTexVerts, float *aIntensities, int numVerts) {
	int outCount = 0;

	if (numVerts <= 0 || numVerts > 32) {
		warning("RenderDroid::ClipModule::faceToPlane(): invalid vertex count (%d)", numVerts);
		return 0;
	}

	const float *prev = aVerts[numVerts - 1];
	const float *prevTex = aTexVerts ? aTexVerts[numVerts - 1] : nullptr;
	float prevIntensity = aIntensities ? aIntensities[numVerts - 1] : 0.0f;
	float prevDist = (float)(((double)prev[2] - pt[2]) * norm[2] + ((double)prev[0] - pt[0]) * norm[0] + ((double)prev[1] - pt[1]) * norm[1]);

	for (int i = 0; i < numVerts; i++) {
		const float *cur = aVerts[i];
		const float *curTex = aTexVerts ? aTexVerts[i] : nullptr;
		float curIntensity = aIntensities ? aIntensities[i] : 0.0f;
		double dist = ((double)cur[2] - pt[2]) * norm[2] + ((double)cur[0] - pt[0]) * norm[0] + ((double)cur[1] - pt[1]) * norm[1];

		if (prevDist < 0.0f || dist < 0.0) {
			if (prevDist >= 0.0f || dist >= 0.0) {
				double t = -((double)prevDist / (dist - prevDist));

				if (!canAddVertex(outCount))
					return 0;

				_aVertBufferA[outCount][0] = (float)(((double)cur[0] - prev[0]) * t + prev[0]);
				_aVertBufferA[outCount][1] = (float)(((double)cur[1] - prev[1]) * t + prev[1]);
				_aVertBufferA[outCount][2] = (float)(((double)cur[2] - prev[2]) * t + prev[2]);

				if (aTexVerts) {
					_workTVerts[outCount][0] = (float)(((double)curTex[0] - prevTex[0]) * t + prevTex[0]);
					_workTVerts[outCount][1] = (float)(((double)curTex[1] - prevTex[1]) * t + prevTex[1]);
				}

				if (aIntensities)
					_workIVerts[outCount] = (float)(((double)curIntensity - prevIntensity) * t + prevIntensity);

				outCount++;

				if (prevDist < 0.0f) {
					if (!canAddVertex(outCount))
						return 0;

					_aVertBufferA[outCount][0] = cur[0];
					_aVertBufferA[outCount][1] = cur[1];
					_aVertBufferA[outCount][2] = cur[2];

					if (aTexVerts) {
						_workTVerts[outCount][0] = curTex[0];
						_workTVerts[outCount][1] = curTex[1];
					}

					if (aIntensities)
						_workIVerts[outCount] = curIntensity;

					outCount++;
				}
			}
		} else {
			if (!canAddVertex(outCount))
				return 0;

			_aVertBufferA[outCount][0] = cur[0];
			_aVertBufferA[outCount][1] = cur[1];
			_aVertBufferA[outCount][2] = cur[2];

			if (aTexVerts) {
				_workTVerts[outCount][0] = curTex[0];
				_workTVerts[outCount][1] = curTex[1];
			}

			if (aIntensities)
				_workIVerts[outCount] = curIntensity;

			outCount++;
		}

		prev = cur;
		prevTex = curTex;
		prevIntensity = curIntensity;
		prevDist = (float)dist;
	}

	for (int i = 0; i < outCount; i++) {
		aVerts[i][0] = _aVertBufferA[i][0];
		aVerts[i][1] = _aVertBufferA[i][1];
		aVerts[i][2] = _aVertBufferA[i][2];

		if (aTexVerts) {
			aTexVerts[i][0] = _workTVerts[i][0];
			aTexVerts[i][1] = _workTVerts[i][1];
		}

		if (aIntensities)
			aIntensities[i] = _workIVerts[i];
	}

	return outCount;
}

bool RenderDroid::ClipModule::canAddVertex(int count) {
	if (count >= 32) {
		warning("RenderDroid::ClipModule::canAddVertex(): clipped face has too many vertices");
		return false;
	}

	return true;
}

void RenderDroid::ClipModule::copyVertex(const RdClipPoly *pSrc, int srcIdx, RdClipPoly *pDst, int dstIdx) {
	pDst->aVerts[dstIdx][0] = pSrc->aVerts[srcIdx][0];
	pDst->aVerts[dstIdx][1] = pSrc->aVerts[srcIdx][1];
	pDst->aVerts[dstIdx][2] = pSrc->aVerts[srcIdx][2];

	if (pSrc->aTexVerts) {
		pDst->aTexVerts[dstIdx][0] = pSrc->aTexVerts[srcIdx][0];
		pDst->aTexVerts[dstIdx][1] = pSrc->aTexVerts[srcIdx][1];
	}

	if (pSrc->aIntensities)
		pDst->aIntensities[dstIdx] = pSrc->aIntensities[srcIdx];
}

void RenderDroid::ClipModule::interpolateAttributes(const RdClipPoly *pSrc, int prevIdx, int curIdx, RdClipPoly *pDst, int dstIdx, double t) {
	if (pSrc->aTexVerts) {
		pDst->aTexVerts[dstIdx][0] = (float)(((double)pSrc->aTexVerts[curIdx][0] - pSrc->aTexVerts[prevIdx][0]) * t + pSrc->aTexVerts[prevIdx][0]);
		pDst->aTexVerts[dstIdx][1] = (float)(((double)pSrc->aTexVerts[curIdx][1] - pSrc->aTexVerts[prevIdx][1]) * t + pSrc->aTexVerts[prevIdx][1]);
	}

	if (pSrc->aIntensities)
		pDst->aIntensities[dstIdx] = (float)(((double)pSrc->aIntensities[curIdx] - pSrc->aIntensities[prevIdx]) * t + pSrc->aIntensities[prevIdx]);
}

int RenderDroid::ClipModule::clipPerspectiveX(const RdClipPoly *pSrc, int count, RdClipPoly *pDst, float slope, bool keepGreater, uint status) {
	int outCount = 0;
	int prevIdx = count - 1;

	for (int curIdx = 0; curIdx < count; curIdx++) {
		const float *prev = pSrc->aVerts[prevIdx];
		const float *cur = pSrc->aVerts[curIdx];

		float prevBound = (float)((double)prev[1] * slope);
		double curBound = (double)cur[1] * slope;
		bool prevInside = keepGreater ? !(prev[0] < prevBound) : (prev[0] <= prevBound);
		bool curInside = keepGreater ? (curBound <= cur[0]) : (curBound >= cur[0]);

		if (prevInside || curInside) {
			bool prevOutside = keepGreater ? (prev[0] < (double)prevBound) : (prev[0] > (double)prevBound);
			bool curOutside = keepGreater ? (curBound > cur[0]) : (curBound < cur[0]);

			if (prev[0] != prevBound && curBound != cur[0] && (prevOutside || curOutside)) {
				float dy = (float)((double)cur[1] - prev[1]);
				float dx = (float)((double)cur[0] - prev[0]);
				double c = (double)cur[1] * prev[0] - (double)prev[1] * cur[0];
				double denominator = (double)slope * dy - dx;

				if (denominator != 0.0)
					c = c / (float)denominator;

				float x = (float)((double)slope * c);
				double t;

				if (fabs((double)dy) <= fabs((double)dx))
					t = ((double)x - prev[0]) / dx;
				else
					t = (c - prev[1]) / dy;

				if (!canAddVertex(outCount))
					return 0;

				pDst->aVerts[outCount][0] = x;
				pDst->aVerts[outCount][1] = (float)c;
				pDst->aVerts[outCount][2] = (float)(((double)cur[2] - prev[2]) * t + prev[2]);
				interpolateAttributes(pSrc, prevIdx, curIdx, pDst, outCount, t);
				outCount++;
				_faceStatus |= status;
			}

			if (curInside) {
				if (!canAddVertex(outCount))
					return 0;

				copyVertex(pSrc, curIdx, pDst, outCount);
				outCount++;
			}
		}

		prevIdx = curIdx;
	}

	return outCount;
}

int RenderDroid::ClipModule::clipPerspectiveZ(const RdClipPoly *pSrc, int count, RdClipPoly *pDst, float slope, bool keepGreater, uint status) {
	int outCount = 0;
	int prevIdx = count - 1;

	for (int curIdx = 0; curIdx < count; curIdx++) {
		const float *prev = pSrc->aVerts[prevIdx];
		const float *cur = pSrc->aVerts[curIdx];

		float prevBound = (float)((double)slope * prev[1]);
		double curBound = (double)cur[1] * slope;
		bool prevInside = keepGreater ? !(prev[2] < prevBound) : (prev[2] <= prevBound);
		bool curInside = keepGreater ? (curBound <= cur[2]) : (curBound >= cur[2]);

		if (prevInside || curInside) {
			bool prevOutside = keepGreater ? (prev[2] < (double)prevBound) : (prev[2] > (double)prevBound);
			bool curOutside = keepGreater ? (curBound > cur[2]) : (curBound < cur[2]);

			if (prev[2] != prevBound && curBound != cur[2] && (prevOutside || curOutside)) {
				float dy = (float)((double)cur[1] - prev[1]);
				float dz = (float)((double)cur[2] - prev[2]);
				double denominator = (double)slope * dy - dz;
				double c = (double)cur[1] * prev[2] - (double)cur[2] * prev[1];

				if (denominator != 0.0)
					c = c / (float)denominator;

				double z = (double)slope * c;
				double t;

				if (fabs((double)dy) <= fabs((double)dz))
					t = (z - prev[2]) / dz;
				else
					t = (c - prev[1]) / dy;

				if (!canAddVertex(outCount))
					return 0;

				pDst->aVerts[outCount][0] = (float)(((double)cur[0] - prev[0]) * t + prev[0]);
				pDst->aVerts[outCount][1] = (float)c;
				pDst->aVerts[outCount][2] = (float)z;
				interpolateAttributes(pSrc, prevIdx, curIdx, pDst, outCount, t);
				outCount++;
				_faceStatus |= status;
			}

			if (curInside) {
				if (!canAddVertex(outCount))
					return 0;

				copyVertex(pSrc, curIdx, pDst, outCount);
				outCount++;
			}
		}

		prevIdx = curIdx;
	}

	return outCount;
}

int RenderDroid::ClipModule::clipOrtho(const RdClipPoly *pSrc, int count, RdClipPoly *pDst, int axis, float bound, bool keepGreater, uint status) {
	int outCount = 0;
	int prevIdx = count - 1;

	for (int curIdx = 0; curIdx < count; curIdx++) {
		const float *prev = pSrc->aVerts[prevIdx];
		const float *cur = pSrc->aVerts[curIdx];

		bool prevInside = keepGreater ? !(prev[axis] < bound) : (prev[axis] <= bound);
		bool curInside = keepGreater ? (bound <= cur[axis]) : (bound >= cur[axis]);

		if (prevInside || curInside) {
			bool prevOutside = keepGreater ? (prev[axis] < (double)bound) : (prev[axis] > (double)bound);
			bool curOutside = keepGreater ? (bound > cur[axis]) : (bound < cur[axis]);

			if (prev[axis] != bound && bound != cur[axis] && (prevOutside || curOutside)) {
				double t = ((double)bound - prev[axis]) / ((double)cur[axis] - prev[axis]);

				if (!canAddVertex(outCount))
					return 0;

				for (int k = 0; k < 3; k++) {
					if (k == axis)
						pDst->aVerts[outCount][k] = bound;
					else
						pDst->aVerts[outCount][k] = (float)(((double)cur[k] - prev[k]) * t + prev[k]);
				}

				interpolateAttributes(pSrc, prevIdx, curIdx, pDst, outCount, t);
				outCount++;
				_faceStatus |= status;
			}

			if (curInside) {
				if (!canAddVertex(outCount))
					return 0;

				copyVertex(pSrc, curIdx, pDst, outCount);
				outCount++;
			}
		}

		prevIdx = curIdx;
	}

	return outCount;
}

int RenderDroid::ClipModule::clipDepth(const RdClipPoly *pSrc, int count, RdClipPoly *pDst, float plane, bool keepGreater, uint status) {
	int outCount = 0;
	int prevIdx = count - 1;

	for (int curIdx = 0; curIdx < count; curIdx++) {
		const float *prev = pSrc->aVerts[prevIdx];
		const float *cur = pSrc->aVerts[curIdx];

		bool prevInside = keepGreater ? (prev[1] >= (double)plane) : (prev[1] <= (double)plane);
		bool curInside = keepGreater ? (cur[1] >= (double)plane) : (cur[1] <= (double)plane);

		if (prevInside || curInside) {
			bool prevOutside = keepGreater ? (prev[1] < (double)plane) : (prev[1] > (double)plane);
			bool curOutside = keepGreater ? (cur[1] < (double)plane) : (cur[1] > (double)plane);

			if (prev[1] != plane && cur[1] != plane && (prevOutside || curOutside)) {
				double t = ((double)plane - prev[1]) / ((double)cur[1] - prev[1]);

				if (!canAddVertex(outCount))
					return 0;

				pDst->aVerts[outCount][1] = plane;
				pDst->aVerts[outCount][2] = (float)(((double)cur[2] - prev[2]) * t + prev[2]);
				pDst->aVerts[outCount][0] = (float)(((double)cur[0] - prev[0]) * t + prev[0]);
				interpolateAttributes(pSrc, prevIdx, curIdx, pDst, outCount, t);
				outCount++;
				_faceStatus |= status;
			}

			if (curInside) {
				if (!canAddVertex(outCount))
					return 0;

				copyVertex(pSrc, curIdx, pDst, outCount);
				outCount++;
			}
		}

		prevIdx = curIdx;
	}

	return outCount;
}

} // End of namespace Grim
