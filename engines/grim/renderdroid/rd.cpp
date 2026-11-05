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

RenderDroid::RenderDroid(StdMath *stdMath, StdDisplay *stdDisplay, StdConffile *stdConffile, StdFile *stdFile) :
		_camera(this), _canvas(this), _clip(this), _colormap(this), _colorEffects(this),
		_face(this), _keyframe(this), _light(this), _material(this), _model3(this),
		_primit2(this), _primit3(this), _puppet(this), _sprite(this), _thing(this),
		_active(this), _cache(this), _raster(this), _afRaster(this), _nRaster(this),
		_zRaster(this), _stdMath(stdMath), _stdDisplay(stdDisplay), _stdConffile(stdConffile), _stdFile(stdFile),
		_pHS(nullptr), _bStarted(0), _bOpened(0),
		_frameTrue(0), _curAcceleration(0), _curRenderOptions(0), _curGeometryMode(0),
		_curLightingMode(0), _curTextureMode(0), _curSortingMethod(0), _curOcclusionMethod(0),
		_curZBufferMethod(0), _curVertexColorMode(0), _curCullFlags(0), _curProcFaceUserData(0) {
	memset(&_colorInfo, 0, sizeof(StdColorFormat));
	memset(&_curColorEffects, 0, sizeof(RdColorEffects));

	_aMipDistances[0] = 10.0f;
	_aMipDistances[1] = 20.0f;
	_aMipDistances[2] = 40.0f;
	_aMipDistances[3] = 80.0f;

	memset(_identMatrix34, 0, sizeof(RdMatrix34));
	_identMatrix34[0][0] = 1.0f;
	_identMatrix34[1][1] = 1.0f;
	_identMatrix34[2][2] = 1.0f;
}

RenderDroid::~RenderDroid() {
}

void RenderDroid::setColorInfo(const StdColorFormat &colorInfo) {
	_colorInfo = colorInfo;
}

int RenderDroid::startup(SysStruct *pHS) {
	if (_bStarted)
		return 1;

	_pHS = pHS;
	_active.startup();
	_cache.startup();
	_raster.startup();
	_bStarted = 1;

	return 1;
}

void RenderDroid::shutdown() {
	if (_bStarted == 1)
		_bStarted = 0;
}

int RenderDroid::open(int acceleration) {
	if (!_bOpened) {
		_curGeometryMode = 5;
		_curLightingMode = 5;
		_curTextureMode = 3;
		_curRenderOptions = 1;
		_curSortingMethod = 0;
		_curOcclusionMethod = 0;
		_curCullFlags = 1;
		_curProcFaceUserData = 0;
		_colorEffects.clear(&_curColorEffects);
		_curVertexColorMode = 0;
		_frameTrue = 0;
		clearPreStatistics();
		_bOpened = 1;
		_curAcceleration = acceleration;
	}

	return 1;
}

int RenderDroid::close() {
	int wasOpened = _bOpened;

	if (_bOpened)
		_bOpened = 0;

	return wasOpened;
}

int RenderDroid::setRenderOptions(int options) {
	_curRenderOptions = options;
	return options;
}

int RenderDroid::setGeometryMode(int mode) {
	_curGeometryMode = mode;
	return mode;
}

int RenderDroid::setLightingMode(int mode) {
	_curLightingMode = mode;
	return mode;
}

int RenderDroid::setTextureMode(int mode) {
	_curTextureMode = mode;
	return mode;
}

int RenderDroid::setSortingMethod(int method) {
	_curSortingMethod = method;
	return method;
}

int RenderDroid::setOcclusionMethod(int method) {
	_curOcclusionMethod = method;
	return method;
}

int RenderDroid::setZBufferMethod(int method) {
	_curZBufferMethod = method;
	return method;
}

int RenderDroid::advanceFrame() {
	clearPreStatistics();
	_frameTrue++;
	_camera.advanceFrame();

	return beginHardwareScene();
}

void RenderDroid::finishFrame() {
	_cache.flush();
	_cache.finishFrame();
	_primit3.clearFrameCounters();
}

void RenderDroid::clearPreStatistics() {
	_cache.clearFrameCounters();
	_active.clearFrameCounters();
	_model3.clearFrameCounters();
}

int RenderDroid::beginHardwareScene() {
	return _curAcceleration;
}

} // End of namespace Grim
