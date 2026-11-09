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
#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_3d_config.h"

namespace Grim {

RdCamera *RenderDroid::CameraModule::create(float fov, int bClipFar, float zNear, float zFar, float aspectRatio) {
	RdCamera *pCamera = new RdCamera;

	if (!newEntry(pCamera, fov, bClipFar, zNear, zFar, aspectRatio))
		return nullptr;

	return pCamera;
}

int RenderDroid::CameraModule::newEntry(RdCamera *pCamera, float fov, int bClipFar, float zNear, float zFar, float aspectRatio) {
	pCamera->pClipFrustum = new RdClipFrustum;

	double clampedFov;
	if (fov < 5.0)
		clampedFov = 5.0;
	else if (fov > 179.0)
		clampedFov = 179.0;
	else
		clampedFov = fov;

	pCamera->fov = (float)clampedFov;
	pCamera->pClipFrustum->bClipFar = bClipFar;
	pCamera->pClipFrustum->zNear = zNear;
	pCamera->pClipFrustum->zFar = zFar;
	pCamera->aspectRatio = aspectRatio;
	pCamera->orthoScale = 1.0f;
	pCamera->pCanvas = nullptr;
	pCamera->ambientLight = 0.0f;
	pCamera->numLights = 0;
	pCamera->attenuationMin = 0.2f;
	pCamera->attenuationMax = 0.1f;
	setProjectType(pCamera, 1);

	return 1;
}

void RenderDroid::CameraModule::destroy(RdCamera *&pCamera) {
	if (pCamera) {
		freeEntry(pCamera);
		delete pCamera;
		pCamera = nullptr;
	}
}

void RenderDroid::CameraModule::freeEntry(RdCamera *pCamera) {
	if (pCamera->pClipFrustum) {
		delete pCamera->pClipFrustum;
		pCamera->pClipFrustum = nullptr;
	}
}

int RenderDroid::CameraModule::setCanvas(RdCamera *pCamera, RdCanvas *pCanvas) {
	pCamera->pCanvas = pCanvas;
	buildFOV(pCamera);

	return 1;
}

int RenderDroid::CameraModule::setCurrent(RdCamera *pCamera) {
	if (_pCurCamera != pCamera)
		_pCurCamera = pCamera;

	return 1;
}

int RenderDroid::CameraModule::setFOV(RdCamera *pCamera, float fov) {
	double clampedFov;

	if (fov < 5.0)
		clampedFov = 5.0;
	else if (fov > 179.0)
		clampedFov = 179.0;
	else
		clampedFov = fov;

	pCamera->fov = (float)clampedFov;
	buildFOV(pCamera);

	return 1;
}

int RenderDroid::CameraModule::setProjectType(RdCamera *pCamera, int projectType) {
	pCamera->projectType = projectType;

	if (projectType == 0) {
		if (pCamera->aspectRatio == 1.0f) {
			pCamera->pfProject = &CameraModule::orthoProjectSquare;
			pCamera->pfProjectLst = &CameraModule::orthoProjectSquareLst;
		} else {
			pCamera->pfProject = &CameraModule::orthoProject;
			pCamera->pfProjectLst = &CameraModule::orthoProjectLst;
		}
	} else if (projectType == 1) {
		if (pCamera->aspectRatio == 1.0f) {
			pCamera->pfProject = &CameraModule::perspProjectSquare;
			pCamera->pfProjectLst = &CameraModule::perspProjectSquareLst;
		} else {
			pCamera->pfProject = &CameraModule::perspProject;
			pCamera->pfProjectLst = &CameraModule::perspProjectLst;
		}
	}

	if (pCamera->pCanvas)
		buildFOV(pCamera);

	return 1;
}

int RenderDroid::CameraModule::setAspectRatio(RdCamera *pCamera, float aspectRatio) {
	pCamera->aspectRatio = aspectRatio;

	if (pCamera->projectType == 0) {
		if (aspectRatio == 1.0) {
			pCamera->pfProject = &CameraModule::orthoProjectSquare;
			pCamera->pfProjectLst = &CameraModule::orthoProjectSquareLst;
		} else {
			pCamera->pfProject = &CameraModule::orthoProject;
			pCamera->pfProjectLst = &CameraModule::orthoProjectLst;
		}
	} else if (pCamera->projectType == 1) {
		if (aspectRatio == 1.0) {
			pCamera->pfProject = &CameraModule::perspProjectSquare;
			pCamera->pfProjectLst = &CameraModule::perspProjectSquareLst;
		} else {
			pCamera->pfProject = &CameraModule::perspProject;
			pCamera->pfProjectLst = &CameraModule::perspProjectLst;
		}
	}

	buildFOV(pCamera);

	return 1;
}

int RenderDroid::CameraModule::buildFOV(RdCamera *pCamera) {
	RdCanvas *pCanvas = pCamera->pCanvas;

	if (!pCanvas)
		return 0;

	if (pCamera->projectType == 0) {
		int height = pCanvas->yEnd - pCanvas->yStart;
		double halfWidth = (double)(pCanvas->xEnd - pCanvas->xStart) * 0.5;

		pCamera->pClipFrustum->orthoLeft = (float)-(halfWidth / pCamera->orthoScale);
		float halfHeight = (float)((double)height * 0.5);
		pCamera->pClipFrustum->orthoTop = (float)((double)halfHeight / pCamera->orthoScale / pCamera->aspectRatio);
		pCamera->pClipFrustum->orthoRight = (float)(halfWidth / pCamera->orthoScale);
		pCamera->pClipFrustum->orthoBottom = (float)(-((double)halfHeight / pCamera->orthoScale) / pCamera->aspectRatio);
		pCamera->focalLen = 0.0f;
		pCamera->pClipFrustum->top = 0.0f;
		pCamera->pClipFrustum->bottom = 0.0f;
		pCamera->pClipFrustum->left = 0.0f;
		pCamera->pClipFrustum->right = 0.0f;
	} else if (pCamera->projectType == 1) {
		float halfWidth = (float)((double)(pCanvas->xEnd - pCanvas->xStart) * 0.5);
		float halfHeight = (float)((double)(pCanvas->yEnd - pCanvas->yStart) * 0.5);
		float halfFov = (float)((double)pCamera->fov * 0.5);
		float tangent = _rd->_stdMath->tan(halfFov);
		float focalLen = (float)((double)halfWidth / tangent);

		pCamera->focalLen = focalLen;
		pCamera->pClipFrustum->top = (float)((double)halfHeight / focalLen / pCamera->aspectRatio);
		pCamera->pClipFrustum->left = (float)(-(double)halfWidth / pCamera->focalLen);
		pCamera->pClipFrustum->bottom = (float)(-(double)halfHeight / pCamera->focalLen / pCamera->aspectRatio);
		pCamera->pClipFrustum->right = (float)((double)halfWidth / pCamera->focalLen);
		pCamera->pClipFrustum->guardTop = (float)(((double)halfHeight - -1.0) / pCamera->focalLen / pCamera->aspectRatio);
		pCamera->pClipFrustum->guardLeft = (float)(-((double)halfWidth - -1.0) / pCamera->focalLen);
	}

	return 1;
}

void RenderDroid::CameraModule::update(RdMatrix34 mat) {
	_rd->invertOrtho34(_pCurCamera->viewMatrix, mat);
	memcpy(_camMatrix, mat, sizeof(RdMatrix34));
	_rd->extractAngles34(_camMatrix, _camPYR);
}

void RenderDroid::CameraModule::orthoProject(RdVector3 pOut, RdVector3 pIn) {
	RdCanvas *pCanvas = _pCurCamera->pCanvas;

	pOut[0] = (float)((double)_pCurCamera->orthoScale * pIn[0] + pCanvas->xCenter);
	pOut[1] = (float)(-((double)pIn[2] * _pCurCamera->orthoScale) * _pCurCamera->aspectRatio + pCanvas->yCenter);
	pOut[2] = pIn[1];
}

void RenderDroid::CameraModule::orthoProjectLst(RdVector3 *aOut, RdVector3 *aIn, int num) {
	RdCanvas *pCanvas = _pCurCamera->pCanvas;
	float orthoScale = _pCurCamera->orthoScale;
	float yCenter = pCanvas->yCenter;
	float xCenter = pCanvas->xCenter;
	float scaledAspect = (float)((double)_pCurCamera->aspectRatio * orthoScale);

	for (int i = 0; i < num; i++) {
		double scaledX = (double)orthoScale * aIn[i][0];

		aOut[i][0] = (float)(scaledX + xCenter);
		aOut[i][1] = (float)((double)yCenter - (double)aIn[i][2] * scaledAspect);
		aOut[i][2] = aIn[i][1];
	}
}

void RenderDroid::CameraModule::orthoProjectSquare(RdVector3 pOut, RdVector3 pIn) {
	RdCanvas *pCanvas = _pCurCamera->pCanvas;

	pOut[0] = (float)((double)_pCurCamera->orthoScale * pIn[0] + pCanvas->xCenter);
	pOut[1] = (float)((double)pCanvas->yCenter - (double)pIn[2] * _pCurCamera->orthoScale);
	pOut[2] = pIn[1];
}

void RenderDroid::CameraModule::orthoProjectSquareLst(RdVector3 *aOut, RdVector3 *aIn, int num) {
	RdCanvas *pCanvas = _pCurCamera->pCanvas;
	float orthoScale = _pCurCamera->orthoScale;
	float yCenter = pCanvas->yCenter;
	float xCenter = pCanvas->xCenter;

	for (int i = 0; i < num; i++) {
		double scaledX = (double)orthoScale * aIn[i][0];

		aOut[i][0] = (float)(scaledX + xCenter);
		aOut[i][1] = (float)((double)yCenter - (double)aIn[i][2] * orthoScale);
		aOut[i][2] = aIn[i][1];
	}
}

void RenderDroid::CameraModule::perspProject(RdVector3 pOut, RdVector3 pIn) {
	double scale = (double)_pCurCamera->focalLen / pIn[1];
	RdCanvas *pCanvas = _pCurCamera->pCanvas;
	double scaledX = scale * pIn[0];
	double scaledY = scale * _pCurCamera->aspectRatio;

	pOut[0] = (float)(scaledX + pCanvas->xCenter);
	pOut[1] = (float)((double)pCanvas->yCenter - (double)pIn[2] * scaledY);
	pOut[2] = pIn[1];
}

void RenderDroid::CameraModule::perspProjectLst(RdVector3 *aOut, RdVector3 *aIn, int num) {
	RdCanvas *pCanvas = _pCurCamera->pCanvas;
	float xCenter = pCanvas->xCenter;
	float yCenter = pCanvas->yCenter;

	for (int i = 0; i < num; i++) {
		double scale = (double)_pCurCamera->focalLen / aIn[i][1];
		double scaledX = scale * aIn[i][0];
		double scaledY = scale * _pCurCamera->aspectRatio;

		aOut[i][0] = (float)(scaledX + xCenter);
		aOut[i][1] = (float)((double)yCenter - (double)aIn[i][2] * scaledY);
		aOut[i][2] = aIn[i][1];
	}
}

void RenderDroid::CameraModule::perspProjectSquare(RdVector3 pOut, RdVector3 pIn) {
	double scale = (double)_pCurCamera->focalLen / pIn[1];
	RdCanvas *pCanvas = _pCurCamera->pCanvas;

	pOut[0] = (float)(scale * pIn[0] + pCanvas->xCenter);
	pOut[1] = (float)((double)pCanvas->yCenter - (double)pIn[2] * scale);
	pOut[2] = pIn[1];
}

void RenderDroid::CameraModule::perspProjectSquareLst(RdVector3 *aOut, RdVector3 *aIn, int num) {
	RdCanvas *pCanvas = _pCurCamera->pCanvas;
	float xCenter = pCanvas->xCenter;
	float yCenter = pCanvas->yCenter;

	for (int i = 0; i < num; i++) {
		double scale = (double)_pCurCamera->focalLen / aIn[i][1];

		aOut[i][0] = (float)(scale * aIn[i][0] + xCenter);
		aOut[i][1] = (float)((double)yCenter - (double)aIn[i][2] * scale);
		aOut[i][2] = aIn[i][1];
	}
}

void RenderDroid::CameraModule::setAmbientLight(RdCamera *pCamera, float ambient) {
	pCamera->ambientLight = ambient;
}

int RenderDroid::CameraModule::addLight(RdCamera *pCamera, RdLight *pLight, RdVector3 *pPos) {
	if (pCamera->numLights >= 64) {
		warning("RenderDroid::CameraModule::addLight(): too many lights");
		return 0;
	}

	pLight->id = pCamera->numLights;
	pCamera->apLights[pCamera->numLights] = pLight;
	pCamera->aLightPos[pCamera->numLights][0] = (*pPos)[0];
	pCamera->aLightPos[pCamera->numLights][1] = (*pPos)[1];
	pCamera->aLightPos[pCamera->numLights][2] = (*pPos)[2];
	pLight->falloffMin = (float)((double)pLight->intensity / pCamera->attenuationMin);
	pLight->falloffMax = (float)((double)pLight->intensity / pCamera->attenuationMax);
	pCamera->numLights++;

	return 1;
}

void RenderDroid::CameraModule::advanceFrame() {
	if (!_pCurCamera)
		return;

	RdCanvas *pCanvas = _pCurCamera->pCanvas;

	if ((_rd->_curRenderOptions & 0x100) && (pCanvas->flags & 2)) {
		if (_rd->_curAcceleration > 0) {
#if defined(GRIM_STD3D)
			_rd->_pStd3D->clearZBuffer();
#endif
			return;
		}

		if (pCanvas->flags & 1) {
			StdRect rect;

			rect.x = pCanvas->xStart;
			rect.y = pCanvas->yStart;
			rect.width = pCanvas->xEnd - pCanvas->xStart + 1;
			rect.height = pCanvas->yEnd - pCanvas->yStart + 1;
			_rd->_stdDisplay->vBufferFill(pCanvas->pZBuffer, 0, &rect);
		} else {
			_rd->_stdDisplay->vBufferFill(pCanvas->pZBuffer, 0, nullptr);
		}
	}
}

} // End of namespace Grim
