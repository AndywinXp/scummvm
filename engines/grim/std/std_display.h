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

#ifndef GRIM_STD_STD_DISPLAY_H
#define GRIM_STD_STD_DISPLAY_H

#include "common/scummsys.h"

namespace Grim {

class StdBitmap;
class Std3D;
struct Std3DSurface;

enum StdDisplayLimits {
	kStdDisplayMaxDevices = 16,
	kStdDisplayMaxModes = 64
};

struct StdRect {
	int x;
	int y;
	int width;
	int height;
};

struct StdColorFormat {
	int colorMode;
	int bpp;
	int redBits;
	int greenBits;
	int blueBits;
	int redShift;
	int greenShift;
	int blueShift;
	int redLoss;
	int greenLoss;
	int blueLoss;
	int alphaBits;
	int alphaShift;
	int alphaLoss;
};

struct StdVBufferTexFmt {
	int width;
	int height;
	int sizeInBytes;
	int pitchInBytes;
	int pitchInPixels;
	StdColorFormat format;
};

struct StdVBuffer {
	int type;
	uint lockCount;
	int bVideoMemory;
	StdVBufferTexFmt tex;
	byte *pPaletteRGB;
	byte *pPixels;
	int colorKey;
	int allocSize;
	Std3DSurface *pSurface;
	int surfaceColorKey;
};

struct StdDisplayDevice {
	char aName[128];
	char aDescription[128];
	int type;
	int bHasGuid;
	int b3D;
	int bPrimary;
	int bHasHardware;
	uint vidMemTotal;
	uint vidMemFree;
	byte guid[16];
};

struct StdDisplayDeviceFlags {
	int type;
	int bHasGuid;
	int b3D;
	int bPrimary;
	int bHasHardware;
};

struct StdDisplayMode {
	int bModeX;
	float aspect;
	StdVBufferTexFmt tex;
};

struct StdDisplayModeDesc {
	int width;
	int height;
	int pitch;
	uint pixelFlags;
	uint rgbBitCount;
	uint rBitMask;
	uint gBitMask;
	uint bBitMask;
};

class StdDisplay {
public:
	StdDisplay();

	void setStdBitmap(StdBitmap *pStdBitmap) { _pStdBitmap = pStdBitmap; }
	void setStd3D(Std3D *pStd3D) { _pStd3D = pStd3D; }
	void setBuffers(StdVBuffer *pMainGfxBuffer, StdVBuffer *pBackBuffer) { _pMainGfxBuffer = pMainGfxBuffer; _pBackBuffer = pBackBuffer; }

	int isDisplayOpen() { return _bDisplayOpen; }
	int isSurfacesCreated() { return _bSurfacesCreated; }
	int getFullscreen() { return _bFullscreen; }
	int getNumDrawBuffers() { return _numDrawBuffers; }
	uint getNumDevices() { return _numDevices; }
	StdDisplayDevice *getDevices() { return _aDevices; }
	StdDisplayDevice *getCurDevice() { return _pCurDevice; }
	uint getCurDeviceIndex() { return _curDeviceIndex; }
	uint getNumModes() { return _numModes; }
	StdDisplayMode *getModes() { return _aModes; }
	StdDisplayMode *getDisplayMode() { return _pDisplayMode; }

	int startup();
	void shutdown();
	int enumerateDevices();
	int enumDevicesCallback(const byte *pGuid, const char *pDescription, const char *pName);
	void restoreDisplayMode();
	int enumerateModes(uint deviceIndex);
	int createDirectDraw();
	int enumModesCallback(const StdDisplayModeDesc *pDesc);
	int enumModesCallback2(const StdDisplayModeDesc *pDesc);
	uint findClosestDevice(const StdDisplayDeviceFlags *pWanted);
	uint findClosestMode(const StdDisplayMode *pWanted, const StdDisplayMode *aModes, uint numModes);
	int openDisplay(uint modeIndex, byte *pPalette, int bFullscreen);
	void closeDisplay();
	int initDisplay(StdDisplayMode *pMode, byte *pPalette);
	void releaseSurfaces();
	void freeBuffers();
	int prepareWindowForDraw();
	void prepareOutputForDraw();
	int update();
	byte *surfaceLock(StdVBuffer *pVBuffer);
	int surfaceUnlock(StdVBuffer *pVBuffer, int bUpload);
	int surfaceBlt(StdVBuffer *pVBuffer, int color, const StdRect *pRect);
	int copyBltB(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey);
	int copyBltC(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey);
	int copyBltD(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey);

	StdVBuffer *vBufferNew(StdVBufferTexFmt *pFormat, int bVideoMemory, int bVideoMemoryOnly, byte *pPalette);
	void vBufferFree(StdVBuffer *&pVBuffer);
	void vBufferFreeEntry(StdVBuffer *pVBuffer);
	int vBufferLock(StdVBuffer *pVBuffer);
	int vBufferUnlock(StdVBuffer *pVBuffer);
	void vBufferFill(StdVBuffer *pVBuffer, int color, StdRect *pRect);
	int vBufferCopy(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey);
	int vBufferSetColorKey(StdVBuffer *pVBuffer, int colorKey);
	void vBufferDeCompress(StdVBuffer *pVBuffer);
	int vBufferDeCompressTo(StdVBuffer *pSrc, StdVBuffer *pDst);
	StdVBuffer *vBufferConvertColorFormat(StdColorFormat *pFormat, StdVBuffer *pVBuffer);
	int copyBltA(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey);
	int lzssDecompress(byte *pDst, byte *pSrc, int size, int outputSize);

private:
	void decompressImpl();
	int readFlagBit(const byte *&pIn, uint &flagBits, int &flagCount);
	int isZBuffer(StdVBuffer *pVBuffer);
	void presentIfPrimary(StdVBuffer *pVBuffer);
	int copyToZBuffer(StdVBuffer *pSrc, int x, int y, const StdRect *pSrcRect);
	void unlockForRead(StdVBuffer *pVBuffer);
	int syncShadow(StdVBuffer *pVBuffer);

	StdBitmap *_pStdBitmap;
	byte *_inputBuffer;
	int _inputBufferIndex;
	int _inputBufferMax;
	byte *_outputBuffer;
	int _outputBufferIndex;
	int _outputBufferMax;

	Std3D *_pStd3D;
	StdVBuffer *_pMainGfxBuffer;
	StdVBuffer *_pBackBuffer;
	int _bStartup;
	int _bEnumerated;
	int _bDisplayOpen;
	int _bSurfacesCreated;
	int _bFullscreen;
	int _numDrawBuffers;
	uint _numDevices;
	StdDisplayDevice _aDevices[kStdDisplayMaxDevices];
	StdDisplayDevice *_pCurDevice;
	uint _curDeviceIndex;
	uint _numModes;
	StdDisplayMode _aModes[kStdDisplayMaxModes];
	uint _curModeIndex;
	StdDisplayMode *_pDisplayMode;
};

} // End of namespace Grim

#endif
