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

#ifndef GRIM_EMI_CTINYGLCONTEXT_H
#define GRIM_EMI_CTINYGLCONTEXT_H

#include "common/scummsys.h"

#if defined(USE_TINYGL)

#include "graphics/surface.h"
#include "graphics/tinygl/tinygl.h"

#include "engines/grim/emi/cdrawcontext.h"

namespace Grim {

class CTinyGLContext : public CDrawContext {
public:
	enum {
		kMaxClipPlanes = 8,
		kMaxClipVertices = kMaxClipPlanes + 4
	};

	CTinyGLContext();
	~CTinyGLContext() override;

	bool open(const DisplayMode *mode) override;
	bool close() override;
	bool beginScene() override;
	bool endScene() override;
	int present(bool vsync) override;
	void clear(bool color, bool depth) override;
	void setViewport(int x, int y, int width, int height) override;
	void setPerspective(float fovX, float fovY, float nearZ, float farZ) override;
	void setOrtho(float left, float right, float bottom, float top) override;
	void setCameraMatrix(const float *matrix) override;
	void resetCameraMatrix() override;
	void setGamma(float gamma) override;
	int getMaxClipPlanes() override;
	void addClipPlane(float a, float b, float c, float d) override;
	void clearClipPlanes() override;
	void submitVertices(CDCVertexArray *vertexArray) override;
	void releaseVertices() override;
	void drawTriangles(int flags, CTexture *tex, int numIndices, const uint16 *indices) override;
	void drawQuads(int flags, CTexture *tex, int first, int count, const VertexStreams *streams) override;
	int outputDrawPoly(DrawPoly *poly) override;
	void drawPrim(int flags, DrawVertex *verts, int count, bool filled) override;
	void drawPrim2D(int flags, DrawVertex *verts, int count, bool filled) override;
	void setColor(const Color *color) override;
	void setPolyOptions(int flags) override;
	void setTextures(CTexture *tex, int flags) override;
	void readBackBuffer(LABitmap *bitmap) override;
	void readBackBufferRaw(char *dest, uint size) override;
	void copyFrontToBack() override;
	CDCVertexArray *createVertexArray(int numVerts, const float *positions, const uint32 *colors, const float *uvs) override;
	CDCVertexArray *createVertexArray(const VertexStreams *streams) override;
	CDCVertexArray *createVertexArray(CDCVertexArray *source) override;
	CTexture *createTexture(LABitmap *bitmap) override;
	CTexture *createTexture(int width, int height, const LColorFormat &format, byte *pixels, const char *name) override;
	int getTextureCost(CTexture *tex) override;
	void prepareDialog() override;
	Graphics::PixelFormat getMovieFormat() override;
	bool drawMovieFrame(const Graphics::Surface &frame) override;

	void initGLState();
	void flushDrawCalls();

	CDCVertexArray *currentVertexArray;
	int numClipPlanes;
	int swapCount;
	int flushCount;
	bool depthTestEnabled;
	bool blendEnabled;
	bool alphaTestEnabled;
	int shadeModel;
	bool texture2dEnabled;
	int texEnvMode;

private:
	struct ClipPlane {
		float a;
		float b;
		float c;
		float d;
	};

	struct ClipVertex {
		float x;
		float y;
		float z;
		float red;
		float green;
		float blue;
		float alpha;
		float u;
		float v;
	};

	void drawClippedTriangles(int numIndices, const uint16 *indices);
	int clipPolygon(const ClipVertex *input, int count, const ClipPlane &plane, ClipVertex *output);
	void reserveVertices(int count);
	void applyGamma(const Graphics::Surface &frame);

	TinyGL::ContextHandle *_contextHandle;
	Graphics::Surface _gammaFrame;
	uint16 _gammaRamp[256];
	bool _gammaActive;
	byte *_vertexColors;
	CDCVertex *_lockedVertices;
	ClipPlane _clipPlanes[kMaxClipPlanes];
	uint _queuedBytes;
};

} // End of namespace Grim

#endif

#endif
