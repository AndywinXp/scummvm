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

#ifndef GRIM_STD_STD_3D_H
#define GRIM_STD_STD_3D_H

#include "common/array.h"
#include "common/scummsys.h"

#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/std/std_display.h"

namespace OpenGL {
class Shader;
}

namespace Grim {

class StdBitmap;

enum Std3DRenderFlags {
	kStd3DRsPerspective = 0x1,
	kStd3DRsDither = 0x2,
	kStd3DRsSpecular = 0x4,
	kStd3DRsAntialias = 0x8,
	kStd3DRsSubpixel = 0x10,
	kStd3DRsSubpixelX = 0x20,
	kStd3DRsFog = 0x40,
	kStd3DRsTexMagLinear = 0x80,
	kStd3DRsTexMinLinear = 0x100,
	kStd3DRsAlphaBlend = 0x200,
	kStd3DRsAlphaTexture = 0x400,
	kStd3DRsZTest = 0x800,
	kStd3DRsZWrite = 0x1000,
	kStd3DRsNotMono = 0x8000
};

enum Std3DZBufferMode {
	kStd3DZBufferLess = 2,
	kStd3DZBufferGreater = 16
};

enum Std3DLimits {
	kStd3DMaxDevices = 4,
	kStd3DMaxTexFormats = 8,
	kStd3DDefaultMaxVertices = 512,
	kStd3DTextureMemory = 0x800000
};

struct Std3DDeviceDesc {
	int colorModel;
	uint textureCaps;
	uint shadeCaps;
	uint textureBlendCaps;
	uint zCmpCaps;
	uint deviceRenderBitDepth;
	uint deviceZBufferBitDepth;
	uint minTextureWidth;
	uint minTextureHeight;
	uint maxTextureWidth;
	uint maxTextureHeight;
	uint maxBufferSize;
	uint maxVertexCount;
};

struct Std3DDevice {
	int bHardware;
	int bPerspectiveCorrect;
	int bZBuffer;
	int bTransparency;
	int bAlphaTexture;
	int bAlphaStipple;
	int bAlphaBlend;
	int bSquareTexturesOnly;
	int unk_32;
	int colorModelFlags;
	int renderBitDepthMask;
	int zCmpMask;
	int minTexWidth;
	int minTexHeight;
	int maxTexWidth;
	int maxTexHeight;
	int maxBufferSize;
	int maxVertexCount;
	char aName[128];
	char aDescription[128];
	uint texMemTotal;
	uint texMemFree;
	Std3DDeviceDesc desc;
	byte guid[16];
};

struct Std3DPixelFormat {
	uint flags;
	uint rgbBitCount;
	uint rBitMask;
	uint gBitMask;
	uint bBitMask;
	uint rgbAlphaBitMask;
};

struct Std3DTexFormat {
	StdColorFormat format;
	Std3DPixelFormat pixelFormat;
};

struct Std3DVertex {
	float sx;
	float sy;
	float sz;
	float rhw;
	uint32 color;
	uint32 specular;
	float tu;
	float tv;
};

struct Std3DTri {
	int aIdx[3];
	uint flags;
	RdTexCacheEntry *pTexture;
};

struct Std3DSurface {
	int width;
	int height;
	uint colorTexture;
	uint framebuffer;
	uint depthRenderbuffer;
	int gpuDirty;
	int dirtyLeft;
	int dirtyTop;
	int dirtyRight;
	int dirtyBottom;
	int shadowValid;
	byte *pReference;
};

struct Std3DGlVertex {
	float x;
	float y;
	float z;
	float w;
	byte r;
	byte g;
	byte b;
	byte a;
	float u;
	float v;
};

struct Std3DGlState {
	int framebuffer;
	int viewport[4];
};

enum Std3DCommandType {
	kStd3DCommandRenderState,
	kStd3DCommandTexture,
	kStd3DCommandTriangles
};

struct Std3DCommand {
	int type;
	uint flags;
	uint texture;
	int bTextureAlpha;
	uint firstIndex;
	uint numIndices;
};

class Std3D {
public:
	Std3D(StdDisplay *pStdDisplay, StdBitmap *pStdBitmap);
	~Std3D();

	void setDisplayMode(const StdVBufferTexFmt *pTex);
	void setRenderTarget(Std3DSurface *pSurface);
	Std3DSurface *getRenderTarget() { return _pRenderTarget; }
	StdVBuffer *getZBuffer() { return &_zBuffer; }
	uint getNumDevices() { return _numDevices; }
	Std3DDevice *getDevices() { return _aDevices; }
	Std3DDevice *getCurDevice() { return _pCurDevice; }
	int getCurDeviceIndex() { return _curDeviceIndex; }
	int getHardware() { return _bHardware; }
	uint getZBufferMode() { return _zBufferMode; }
	uint getMaxVertices() { return _maxVertices; }

	int enumerateDevices();
	void releaseDirect3D();
	int isAvailable();
	int startup(uint deviceIndex, int bHardware);
	void shutdown();
	void setDimensionLimits(uint minWidth, uint minHeight, uint maxWidth, uint maxHeight);
	int beginScene();
	int endScene();
	int resetRenderList();
	int addRenderListVertices(const Std3DVertex *pVertices, uint count);
	int beginRenderList();
	int addRenderListTris(const Std3DTri *pTris, uint count);
	int drawRenderList();
	void setMaterialRenderState(uint flags);
	int setCurrentPalette(const byte *pPalette, byte alpha);
	void getValidDimensions(uint width, uint height, uint *pWidth, uint *pHeight);
	void purgeTextureCache();
	int addToTextureCache(StdVBuffer *pSrc, RdTexCacheEntry *pEntry, int bAlpha, int bColorKey);
	void flushTextureCache();
	void cacheListAppend(RdTexCacheEntry *pEntry);
	void cacheListRemove(RdTexCacheEntry *pEntry);
	int getTextureMemory(uint *pTotal, uint *pFree);
	void cacheListTouch(RdTexCacheEntry *pEntry);
	int clearZBuffer();
	uint findClosestDevice(const Std3DDevice *pWanted, const Std3DDevice *aDevices, uint numDevices);
	uint findClosestMode(const StdColorFormat *pWanted, const Std3DTexFormat *aFormats, uint numFormats);
	int setActiveRect(const StdRect *pRect);
	int createExecuteBuffer();
	int createViewport(int width, int height);
	int createDevice(int width, int height);
	int enumDevicesCallback(const byte *pGuid, const char *pDescription, const char *pName, const Std3DDeviceDesc *pHWDesc, const Std3DDeviceDesc *pHELDesc);
	int enumTextureFormatsCallback(const Std3DPixelFormat *pPixelFormat);
	uint convertBitDepthCaps(uint bitDepthFlags);
	uint convertZCmpCaps(uint zCmpCaps);
	uint zCmpMaskToFunc(uint zCmpMask);
	int evictTextures(uint size);

	void releaseTexture(RdTexCacheEntry *pEntry);

	Std3DSurface *createSurface(int width, int height, int bHasDepth);
	void destroySurface(Std3DSurface *&pSurface);
	int uploadPixels(Std3DSurface *pSurface, const StdRect *pRect, const byte *pPixels, int pitchInBytes);
	int readPixels(Std3DSurface *pSurface, const StdRect *pRect, byte *pOut, int pitchInBytes);
	int fillRect(Std3DSurface *pSurface, const StdRect *pRect, uint16 color);
	int blit(Std3DSurface *pDst, Std3DSurface *pSrc, const StdRect *pSrcRect, int dstX, int dstY, int bColorKey, uint16 colorKey);
	int uploadDepth(Std3DSurface *pSurface, const StdRect *pRect, const byte *pZData, int pitchInBytes);
	int presentSurface(Std3DSurface *pSurface);

private:
	int isGlAvailable();
	int ensureGl();
	OpenGL::Shader *createShader(const char *name, const char *vertexSource, const char *fragmentSource, const char *const *attributes, int bDepthOutput);
	void saveGlState(Std3DGlState *pState);
	void restoreGlState(const Std3DGlState *pState);
	void bindSurface(Std3DSurface *pSurface, int bMarkDirty = 1);
	int attachDepth(Std3DSurface *pSurface);
	void detachDepth(Std3DSurface *pSurface);
	void applyRenderState(uint flags);
	uint glCompareFunc(uint d3dFunc);
	void drawQuad(const float *pQuad);

	StdDisplay *_pStdDisplay;
	StdBitmap *_pStdBitmap;
	const StdVBufferTexFmt *_pDisplayModeTex;
	Std3DSurface *_pRenderTarget;
	int _bDepthFromDevice;

	int _bAvailable;
	int _bStarted;
	int _bHardware;
	int _bSceneBegun;
	uint _enumFlags;
	uint _numDevices;
	Std3DDevice _aDevices[kStd3DMaxDevices];
	Std3DDevice *_pCurDevice;
	int _curDeviceIndex;
	uint _zBufferMode;
	int _zBufferBitDepth;
	StdVBuffer _zBuffer;
	uint _numTexFormats;
	Std3DTexFormat _aTexFormats[kStd3DMaxTexFormats];
	uint _opaqueFormatIndex;
	uint _alphaFormatIndex;
	uint _colorKeyFormatIndex;
	Std3DTexFormat *_pTexFormatOpaque;
	Std3DTexFormat *_pTexFormatAlpha;
	Std3DTexFormat *_pTexFormatColorKey;
	StdVBuffer *_pFrameBuffer;
	uint _maxVertices;
	uint _frameCounter;
	uint _numCachedTextures;
	uint _minTexWidth;
	uint _minTexHeight;
	uint _maxTexWidth;
	uint _maxTexHeight;
	RdTexCacheEntry *_pCacheHead;
	RdTexCacheEntry *_pCacheTail;
	byte _aPalette[768];
	uint16 _aOpaquePaletteTable[256];
	uint16 _aAlphaPaletteTable[256];
	uint16 _aColorKeyPaletteTable[256];
	StdRect _activeRect;
	uint _curRenderState;
	float _fogStart;
	float _fogEnd;
	uint _fogRed;
	uint _fogGreen;
	uint _fogBlue;
	uint _alphaFunc;
	int _bZEnable;
	int _bTexMagLinear;
	int _bTexMinLinear;

	uint _numVertices;
	uint _numTriangles;
	Std3DVertex *_aVertices;
	Common::Array<Std3DCommand> _commands;
	Common::Array<uint16> _indices;
	uint _listStart;
	uint _listStartRenderState;
	RdTexCacheEntry *_pCurTexture;
	int _bCurTextureValid;

	int _bGlReady;
	int _bFragDepthSupported;
	OpenGL::Shader *_pTlShader;
	OpenGL::Shader *_pBlitShader;
	OpenGL::Shader *_pDepthShader;
	uint _vertexVbo;
	uint _indexVbo;
	uint _quadVbo;
	uint _depthTexture;
};

} // End of namespace Grim

#endif
