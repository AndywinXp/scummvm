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

#ifndef GRIM_EMI_CDRAWCONTEXT_H
#define GRIM_EMI_CDRAWCONTEXT_H

#include "common/scummsys.h"

#include "graphics/pixelformat.h"

#include "engines/grim/emi/cclipvolume.h"
#include "engines/grim/emi/cdcvertexarray.h"
#include "engines/grim/emi/cdrawlist.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/ctexture.h"
#include "engines/grim/emi/labitmap.h"
#include "engines/grim/emi/lcolorformat.h"

namespace Graphics {
struct Surface;
}

namespace Grim {

struct DisplayMode {
	int width;
	int height;
	int bpp;
	int reserved12[2];
	byte flags;
};

class CDrawContext {
public:
	enum {
		kNumCachedTextures = 32
	};

	CDrawContext();
	virtual ~CDrawContext();

	void *operator new(size_t size);
	void operator delete(void *block);

	virtual void enumerateDevices() {}
	virtual bool open(const DisplayMode *mode) { return false; }
	virtual bool close() { return false; }
	virtual void restoreDisplay() {}
	virtual bool beginScene();
	virtual bool endScene();
	virtual int present(bool vsync) { return 0; }
	virtual void clear(bool color, bool depth) {}
	virtual void setViewport(int x, int y, int width, int height) {}
	virtual void setPerspective(float fovX, float fovY, float nearZ, float farZ) {}
	virtual void setOrtho(float left, float right, float bottom, float top) {}
	virtual void setCameraMatrix(const float *matrix) {}
	virtual void resetCameraMatrix() {}
	virtual bool hasGammaControl();
	virtual void setGamma(float gamma) {}
	virtual void enableLighting() {}
	virtual void disableLighting() {}
	virtual int getMaxLights() { return 0; }
	virtual void setAmbientLight(const float *rgb) {}
	virtual void addDirectionalLight(const float *direction, const float *color) {}
	virtual void addPointLight(const float *position, const float *color, float quadraticAttenuation) {}
	virtual void addSpotLight(const float *position, const float *direction, const float *color, float quadraticAttenuation, int unused, float coneAngle, float exponent) {}
	virtual void clearLights() {}
	virtual int getMaxClipPlanes() { return 0; }
	virtual void addClipPlane(float a, float b, float c, float d) {}
	virtual void clearClipPlanes() {}
	virtual void submitVertices(CDCVertexArray *vertexArray) {}
	virtual void releaseVertices() {}
	virtual void drawTriangles(int flags, CTexture *tex, int numIndices, const uint16 *indices) {}
	virtual void drawQuads(int flags, CTexture *tex, int first, int count, const VertexStreams *streams) {}
	virtual void afterFlush() {}
	virtual int outputDrawList(CDrawList *list);
	virtual int outputDrawPoly(DrawPoly *poly) { return 0; }
	virtual void drawTriangle(DrawVertex *verts) {}
	virtual void drawPrim(int flags, DrawVertex *verts, int count, bool filled) {}
	virtual void drawPrim2D(int flags, DrawVertex *verts, int count, bool filled) {}
	virtual void drawLineStrip(int flags, DrawVertex *verts, int count) {}
	virtual void setColor(const Color *color) {}
	virtual void setPolyOptions(int flags) {}
	virtual void setTextures(CTexture *tex, int flags) {}
	virtual void setTextureFlag(bool value);
	virtual bool getTextureFlag();
	virtual void enableFog(const Color *color, float nearZ, float farZ) {}
	virtual void disableFog() {}
	virtual int *getViewportRect(int *rect);
	virtual void setScissor(const int *rect) {}
	virtual void setWindowOrigin(int x, int y) {}
	virtual void applyClipVolume(bool enable) {}
	virtual void drawPixels(int x, int y, int width, int height, int unused, const void *pixels) {}
	virtual void readBackBuffer(LABitmap *bitmap) {}
	virtual void readBackBufferRaw(char *dest, uint size) {}
	virtual void copyFrontToBack() {}
	virtual CDCVertexArray *createVertexArray(int numVerts, const float *positions, const uint32 *colors, const float *uvs) = 0;
	virtual CDCVertexArray *createVertexArray(const VertexStreams *streams) = 0;
	virtual CDCVertexArray *createVertexArray(CDCVertexArray *source) = 0;
	virtual LColorFormat getTextureFormat(int first, int second);
	virtual CTexture *createTexture(LABitmap *bitmap) { return nullptr; }
	virtual CTexture *createTexture(int width, int height, const LColorFormat &format, byte *pixels, const char *name) { return nullptr; }
	virtual void freeTexture(CTexture *tex);
	virtual int getTextureCost(CTexture *tex);
	virtual bool getPixelCenter();
	virtual void setPixelCenter(bool value);
	virtual void prepareDialog() = 0;
	virtual Graphics::PixelFormat getMovieFormat();
	virtual bool drawMovieFrame(const Graphics::Surface &frame) { return false; }

	CDrawList *getDrawList(int index);
	int flushDrawLists();
	bool setCachedTexture(int slot, int width, int height, const LColorFormat &format, byte *pixels, const char *name);
	bool setCachedTexture(int slot, LABitmap *bitmap);
	CTexture *getCachedTexture(int slot);

	static CDrawContext *context;

	int trianglesDrawn;
	int counter8;
	int counter12;
	int counter16;
	int counter20;
	int counter24;
	CDrawList drawListA;
	CDrawList drawListB;
	int polyOptions;
	DisplayMode displayMode;
	CTexture *defaultTexture;
	CTexture *textureCache[kNumCachedTextures];
	int frameCounter;
	int blendMode;
	bool textureFlag;
	CClipVolume clipVolume;
	bool pixelCenter;
	bool sceneBegun;
	bool hasGammaCtl;
};

} // End of namespace Grim

#endif
