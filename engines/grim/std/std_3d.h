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

namespace Grim {

class StdBitmap;
class Std3DBackend;

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

enum Std3DBackendKind {
	kStd3DBackendGl = 0,
	kStd3DBackendTinyGl = 1
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
	int hardware;
	int perspectiveCorrect;
	int zBuffer;
	int transparency;
	int alphaTexture;
	int alphaStipple;
	int alphaBlend;
	int squareTexturesOnly;
	int reserved32;
	int colorModelFlags;
	int renderBitDepthMask;
	int zCmpMask;
	int minTexWidth;
	int minTexHeight;
	int maxTexWidth;
	int maxTexHeight;
	int maxBufferSize;
	int maxVertexCount;
	char name[128];
	char description[128];
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
	int idx[3];
	uint flags;
	RdTexCacheEntry *texture;
};

struct Std3DSurface {
	int width;
	int height;
	int gpuDirty;
	int dirtyLeft;
	int dirtyTop;
	int dirtyRight;
	int dirtyBottom;
	int shadowValid;
	byte *reference;
	void *backendData;
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
	int textureAlpha;
	uint firstIndex;
	uint numIndices;
};

class Std3D {
public:
	Std3D(StdDisplay *stdDisplay, StdBitmap *stdBitmap);
	~Std3D();

	static int isBackendCompiled(Std3DBackendKind kind);
	int createBackend(Std3DBackendKind kind);
	int canProbeWithoutMode();
	int openBackendMode(int width, int height);
	int openBackend(Std3DBackendKind kind, int width, int height);
	void closeBackend();
	Std3DBackendKind getBackendKind() { return _backendKind; }
	int hasBackend() { return _backend != nullptr; }

	void setDisplayMode(const StdVBufferTexFmt *tex);
	void setRenderTarget(Std3DSurface *surface);
	Std3DSurface *getRenderTarget() { return _renderTarget; }
	StdVBuffer *getZBuffer() { return &_zBuffer; }
	uint getNumDevices() { return _numDevices; }
	Std3DDevice *getDevices() { return _devices; }
	Std3DDevice *getCurDevice() { return _curDevice; }
	int getCurDeviceIndex() { return _curDeviceIndex; }
	int getHardware() { return _hardware; }
	uint getZBufferMode() { return _zBufferMode; }
	uint getMaxVertices() { return _maxVertices; }

	int enumerateDevices();
	void releaseDirect3D();
	int isAvailable();
	int startup(uint deviceIndex, int hardware);
	void shutdown();
	void setDimensionLimits(uint minWidth, uint minHeight, uint maxWidth, uint maxHeight);
	int beginScene();
	int endScene();
	int resetRenderList();
	int addRenderListVertices(const Std3DVertex *vertices, uint count);
	int beginRenderList();
	int addRenderListTris(const Std3DTri *tris, uint count);
	int drawRenderList();
	void setMaterialRenderState(uint flags);
	int setCurrentPalette(const byte *palette, byte alpha);
	void getValidDimensions(uint width, uint height, uint *widthPointer, uint *heightPointer);
	void purgeTextureCache();
	int addToTextureCache(StdVBuffer *src, RdTexCacheEntry *entry, int alpha, int colorKey);
	void flushTextureCache();
	void cacheListAppend(RdTexCacheEntry *entry);
	void cacheListRemove(RdTexCacheEntry *entry);
	int getTextureMemory(uint *total, uint *free);
	void cacheListTouch(RdTexCacheEntry *entry);
	int clearZBuffer();
	uint findClosestDevice(const Std3DDevice *wanted, const Std3DDevice *devices, uint numDevices);
	uint findClosestMode(const StdColorFormat *wanted, const Std3DTexFormat *formats, uint numFormats);
	int setActiveRect(const StdRect *rect);
	int createExecuteBuffer();
	int createViewport(int width, int height);
	int createDevice(int width, int height);
	int enumDevicesCallback(const byte *guid, const char *description, const char *name, const Std3DDeviceDesc *hwDesc, const Std3DDeviceDesc *helDesc);
	int enumTextureFormatsCallback(const Std3DPixelFormat *pixelFormat);
	uint convertBitDepthCaps(uint bitDepthFlags);
	uint convertZCmpCaps(uint zCmpCaps);
	uint zCmpMaskToFunc(uint zCmpMask);
	int evictTextures(uint size);

	void releaseTexture(RdTexCacheEntry *entry);

	Std3DSurface *createSurface(int width, int height, int hasDepth);
	void destroySurface(Std3DSurface *&surface);
	int uploadPixels(Std3DSurface *surface, const StdRect *rectPointer, const byte *pixels, int pitchInBytes);
	int readPixels(Std3DSurface *surface, const StdRect *rectPointer, byte *out, int pitchInBytes);
	int fillRect(Std3DSurface *surface, const StdRect *rectPointer, uint16 color);
	int blit(Std3DSurface *dst, Std3DSurface *src, const StdRect *srcRectPointer, int dstX, int dstY, int colorKeyFlag, uint16 colorKey);
	int uploadDepth(Std3DSurface *surface, const StdRect *rectPointer, const byte *zData, int pitchInBytes);
	int presentSurface(Std3DSurface *surface);

private:
	int ensureBackend();
	static void markGpuDirty(Std3DSurface *surface, int x, int y, int width, int height);

	StdDisplay *_stdDisplay;
	StdBitmap *_stdBitmap;
	Std3DBackend *_backend;
	Std3DBackendKind _backendKind;
	const StdVBufferTexFmt *_displayModeTex;
	Std3DSurface *_renderTarget;
	int _depthFromDevice;

	int _available;
	int _started;
	int _hardware;
	int _sceneBegun;
	uint _enumFlags;
	uint _numDevices;
	Std3DDevice _devices[kStd3DMaxDevices];
	Std3DDevice *_curDevice;
	int _curDeviceIndex;
	uint _zBufferMode;
	int _zBufferBitDepth;
	StdVBuffer _zBuffer;
	uint _numTexFormats;
	Std3DTexFormat _texFormats[kStd3DMaxTexFormats];
	uint _opaqueFormatIndex;
	uint _alphaFormatIndex;
	uint _colorKeyFormatIndex;
	Std3DTexFormat *_texFormatOpaque;
	Std3DTexFormat *_texFormatAlpha;
	Std3DTexFormat *_texFormatColorKey;
	StdVBuffer *_frameBuffer;
	uint _maxVertices;
	uint _frameCounter;
	uint _numCachedTextures;
	uint _minTexWidth;
	uint _minTexHeight;
	uint _maxTexWidth;
	uint _maxTexHeight;
	RdTexCacheEntry *_cacheHead;
	RdTexCacheEntry *_cacheTail;
	byte _palette[768];
	uint16 _opaquePaletteTable[256];
	uint16 _alphaPaletteTable[256];
	uint16 _colorKeyPaletteTable[256];
	StdRect _activeRect;
	uint _curRenderState;
	float _fogStart;
	float _fogEnd;
	uint _fogRed;
	uint _fogGreen;
	uint _fogBlue;
	uint _alphaFunc;
	int _zEnable;
	int _texMagLinear;
	int _texMinLinear;

	uint _numVertices;
	uint _numTriangles;
	Std3DVertex *_vertices;
	Common::Array<Std3DCommand> _commands;
	Common::Array<uint16> _indices;
	uint _listStart;
	uint _listStartRenderState;
	RdTexCacheEntry *_curTexture;
	int _curTextureValid;
};

} // End of namespace Grim

#endif
