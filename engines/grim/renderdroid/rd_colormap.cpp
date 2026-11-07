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

#include "common/endian.h"
#include "common/str.h"
#include "common/stream.h"

#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/std/std_3d.h"

namespace Grim {

RdColormap *RenderDroid::ColormapModule::load(char *path) {
	RdColormap *pColormap = new RdColormap;
	memset(pColormap, 0, sizeof(RdColormap));

	if (loadEntry(path, pColormap))
		return pColormap;

	destroy(pColormap);
	return nullptr;
}

int RenderDroid::ColormapModule::loadEntry(char *path, RdColormap *pColormap) {
	Common::SeekableReadStream *pFile = _rd->_pHS->openFile(path);

	if (!pFile)
		return 0;

	Common::strlcpy(pColormap->aName, _rd->_stdFile->fromPath(path), 32);

	uint32 magic = pFile->readUint32BE();
	pFile->skip(4);
	pColormap->flags = pFile->readUint32LE();
	pColormap->tint[0] = pFile->readFloatLE();
	pColormap->tint[1] = pFile->readFloatLE();
	pColormap->tint[2] = pFile->readFloatLE();
	pFile->skip(40);

	if (magic != MKTAG('C', 'M', 'P', ' ')) {
		delete pFile;
		buildGrayRamp(pColormap);
		return 1;
	}

	pFile->read(pColormap->aColors, 768);

	byte firstRed = pColormap->aColors[0];
	byte firstGreen = pColormap->aColors[1];
	byte firstBlue = pColormap->aColors[2];

	for (int i = 0; i < 254; i++) {
		if (pColormap->aColors[0] == firstRed)
			pColormap->aColors[0] = pColormap->aColors[0] >= 247 ? pColormap->aColors[0] - 8 : pColormap->aColors[0] + 8;

		if (pColormap->aColors[1] == firstGreen)
			pColormap->aColors[1] = pColormap->aColors[1] >= 247 ? pColormap->aColors[1] - 8 : pColormap->aColors[1] + 8;

		if (pColormap->aColors[2] == firstBlue)
			pColormap->aColors[2] = pColormap->aColors[2] >= 247 ? pColormap->aColors[2] - 8 : pColormap->aColors[2] + 8;
	}

	if (pColormap->flags & 4) {
		pColormap->pRGB16 = new byte[0x8000];
		buildRGB16(pColormap->pRGB16, pColormap->aColors, 0, 0, 0, &_rd->_colorInfo);
	} else {
		pColormap->pRGB16 = nullptr;
	}

	if (!(pColormap->flags & 1)) {
		delete pFile;
		return 1;
	}

	pColormap->pTransparencyAlloc = new byte[65792];
	pColormap->pTransparency = pColormap->pTransparencyAlloc;

	if ((uintptr)pColormap->pTransparencyAlloc & 0xFF)
		pColormap->pTransparency = pColormap->pTransparencyAlloc + 256 - ((uintptr)pColormap->pTransparencyAlloc & 0xFF);

	pFile->read(pColormap->pTransparency, 0x10000);

	if (pColormap->flags & 4) {
		pColormap->pRGB16Alpha = new byte[0x10000];

		if (_rd->_colorInfo.greenBits == 5) {
			pFile->skip(0x10000);
			pFile->read(pColormap->pRGB16Alpha, 0x10000);
		} else {
			pFile->read(pColormap->pRGB16Alpha, 0x10000);
			pFile->skip(0x10000);
		}
	} else {
		pColormap->pRGB16Alpha = nullptr;
	}

	pColormap->unk_832 = 0;
	pColormap->unk_836 = 0;

	delete pFile;
	return 1;
}

void RenderDroid::ColormapModule::destroy(RdColormap *&pColormap) {
	freeEntry(pColormap);
	delete pColormap;
	pColormap = nullptr;
}

void RenderDroid::ColormapModule::freeEntry(RdColormap *pColormap) {
	if (pColormap->pLightLevelAlloc) {
		delete[] pColormap->pLightLevelAlloc;
		pColormap->pLightLevelAlloc = nullptr;
		pColormap->pLightLevel = nullptr;
	}

	if (pColormap->pRGB16) {
		delete[] pColormap->pRGB16;
		pColormap->pRGB16 = nullptr;
	}

	if (pColormap->flags & 1) {
		if (pColormap->pTransparencyAlloc) {
			delete[] pColormap->pTransparencyAlloc;
			pColormap->pTransparencyAlloc = nullptr;
			pColormap->pTransparency = nullptr;
		}

		if (pColormap->pRGB16Alpha) {
			delete[] pColormap->pRGB16Alpha;
			pColormap->pRGB16Alpha = nullptr;
		}
	}
}

int RenderDroid::ColormapModule::setCurrent(RdColormap *pColormap) {
	if (_pCurMap != pColormap)
		_pCurMap = pColormap;

	if (!_pIdentityMap)
		setIdentity(pColormap);

	return 1;
}

int RenderDroid::ColormapModule::setIdentity(RdColormap *pColormap) {
	if (_pIdentityMap != pColormap) {
		_pIdentityMap = pColormap;

#if defined(USE_OPENGL_SHADERS)
		if (_rd->_curAcceleration > 0)
			_rd->_pStd3D->setCurrentPalette(pColormap->aColors, 90);
#endif
	}

	return 1;
}

int RenderDroid::ColormapModule::size(RdColormap *pColormap) {
	int result = 848;

	if (pColormap->pLightLevelAlloc)
		result = 17488;

	if (pColormap->pTransparencyAlloc)
		result += 65792;

	if (pColormap->pRGB16)
		result += 0x8000;

	if (pColormap->pRGB16Alpha)
		result += 0x10000;

	return result;
}

int RenderDroid::ColormapModule::buildGrayRamp(RdColormap *pColormap) {
	Common::strlcpy(pColormap->aName, "GREYRAMP.CMP", 32);
	pColormap->flags = 0;
	pColormap->tint[0] = 1.0f;
	pColormap->tint[1] = 1.0f;
	pColormap->tint[2] = 1.0f;

	pColormap->pLightLevelAlloc = new byte[16640];
	pColormap->pLightLevel = pColormap->pLightLevelAlloc;

	if ((uintptr)pColormap->pLightLevelAlloc & 0xFF)
		pColormap->pLightLevel = pColormap->pLightLevelAlloc + 256 - ((uintptr)pColormap->pLightLevelAlloc & 0xFF);

	for (uint i = 0; i < 256; i++) {
		pColormap->aColors[i * 3] = i;
		pColormap->aColors[i * 3 + 1] = i;
		pColormap->aColors[i * 3 + 2] = i;

		uint level = 0;
		for (uint j = 0; j < 0x4000; j += 256)
			pColormap->pLightLevel[i + j] = i * (level++ >> 6);
	}

	return 1;
}

int RenderDroid::ColormapModule::buildRGB16(byte *pDst, byte *pPalette, byte r, byte g, byte b, StdColorFormat *pColorFormat) {
	uint16 *pOut = (uint16 *)pDst;
	byte *pColor = pPalette;

	double targetR = r;
	double targetG = g;
	double targetB = b;

	for (uint col = 0; col < 8; col++) {
		byte red = pColor[0];
		byte green = pColor[1];
		byte blue = pColor[2];
		pColor += 3;

		for (uint row = 0; row < 64; row++) {
			uint16 value = (byte)(green >> pColorFormat->greenLoss) << pColorFormat->greenShift;
			value |= (byte)(red >> pColorFormat->redLoss) << pColorFormat->redShift;
			value |= (byte)(blue >> pColorFormat->blueLoss) << pColorFormat->blueShift;
			pOut[row * 256 + col] = value;
		}
	}

	pColor = pPalette + 24;

	for (uint col = 8; col < 256; col++) {
		double curR = pColor[0];
		double curG = pColor[1];
		double curB = pColor[2];
		pColor += 3;

		double stepR = (targetR - curR) * 0.015625;
		double stepG = (targetG - curG) * 0.015625;
		float stepB = (float)((targetB - curB) * 0.015625);

		for (uint row = 64; row > 0; row--) {
			curR = curR + stepR;
			curG = curG + stepG;
			curB = curB + stepB;

			int red = (int)(curR > 0.0 ? curR + 0.5 : curR - 0.5);
			int green = (int)(curG > 0.0 ? curG + 0.5 : curG - 0.5);
			int blue = (int)(curB > 0.0 ? curB + 0.5 : curB - 0.5);

			pOut[(row - 1) * 256 + col] = (red >> pColorFormat->redLoss << pColorFormat->redShift) |
				(green >> pColorFormat->greenLoss << pColorFormat->greenShift) |
				(blue >> pColorFormat->blueLoss << pColorFormat->blueShift);
		}
	}

	return 1;
}

} // End of namespace Grim
