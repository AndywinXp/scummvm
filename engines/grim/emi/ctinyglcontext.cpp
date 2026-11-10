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

#if defined(USE_TINYGL)

#include "common/system.h"
#include "common/textconsole.h"

#include "engines/util.h"

#include "graphics/pixelformat.h"

#include "engines/grim/emi/coglvertexarray.h"
#include "engines/grim/emi/ctinyglcontext.h"
#include "engines/grim/emi/ctinygltexture.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/profilertimer.h"

namespace Grim {

static const float kPi = 3.1415927f;
static const double kDegreesToHalfRadians = 0.002777777777777778;
static const uint kRawReadWidth = 640;
static const uint kRawReadHeight = 480;
static const int kTextureSize = 256;
static const uint kDrawCallMemory = 8 * 1024 * 1024;
static const uint kQueueFlushThreshold = 6 * 1024 * 1024;
static const uint kBytesPerVertex = 152;
static const uint kBytesPerDrawCall = 512;

static ProfilerTimer outputDrawPolyTimer("TinyGL::OutputDrawPoly()", ProfilerTimer::kTrackNesting);
static ProfilerTimer setPolyOptionsTimer("TinyGL::SetPolyOptions()", ProfilerTimer::kTrackNesting);
static ProfilerTimer setTexturesTimer("TinyGL::SetTextures()", ProfilerTimer::kTrackNesting);
static ProfilerTimer submitVerticesTimer("TinyGL::SubmitVertices()", ProfilerTimer::kTrackNesting);
static ProfilerTimer drawTrianglesTimer("TinyGL::DrawTriangles()", ProfilerTimer::kTrackNesting);
static ProfilerTimer drawQuadsTimer("TinyGL::DrawQuads()", ProfilerTimer::kTrackNesting);
static ProfilerTimer drawElementsTimer("tglDrawElements()", ProfilerTimer::kTrackNesting);

CTinyGLContext::CTinyGLContext() {
	currentVertexArray = nullptr;
	numClipPlanes = 0;
	swapCount = 0;
	flushCount = 0;
	depthTestEnabled = false;
	blendEnabled = false;
	alphaTestEnabled = false;
	shadeModel = 0;
	texture2dEnabled = false;
	texEnvMode = 0;
	_contextHandle = nullptr;
	_gammaActive = false;
	_vertexColors = nullptr;
	_lockedVertices = nullptr;
	_queuedBytes = 0;

	for (int i = 0; i < 256; i++)
		_gammaRamp[i] = i * 257;
}

CTinyGLContext::~CTinyGLContext() {
	close();
}

bool CTinyGLContext::open(const DisplayMode *mode) {
	displayMode = *mode;

	Graphics::PixelFormat format;

	if (displayMode.bpp == 32)
		format = Graphics::PixelFormat::createFormatRGBA32();
	else
		format = Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0);

	initGraphics(displayMode.width, displayMode.height, &format);

	Graphics::PixelFormat screenFormat = g_system->getScreenFormat();

	if (screenFormat.bytesPerPixel < 2) {
		warning("CTinyGLContext::open(): the screen format is not supported");

		return false;
	}

	_contextHandle = TinyGL::createContext(displayMode.width, displayMode.height, screenFormat, kTextureSize, false, false, kDrawCallMemory);
	_queuedBytes = 0;

	setViewport(0, 0, displayMode.width, displayMode.height);
	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();
	tglScalef(1.0f, 1.0f, -1.0f);

	initGLState();

	drawListA.ctx = this;
	drawListB.ctx = this;

	return true;
}

bool CTinyGLContext::close() {
	_gammaActive = false;
	_gammaFrame.free();

	if (_contextHandle) {
		// TinyGL needs its context to delete the textures
		for (int i = 0; i < kNumCachedTextures; i++) {
			delete textureCache[i];
			textureCache[i] = nullptr;
		}

		delete defaultTexture;
		defaultTexture = nullptr;

		if (context == this)
			context = nullptr;

		TinyGL::destroyContext(_contextHandle);
		_contextHandle = nullptr;
	}

	return true;
}

bool CTinyGLContext::beginScene() {
	return CDrawContext::beginScene();
}

bool CTinyGLContext::endScene() {
	bool result = CDrawContext::endScene();

	if (result) {
		// TinyGL has no glFinish: the queued draws are executed instead
		flushDrawCalls();

		return true;
	}

	return result;
}

int CTinyGLContext::present(bool vsync) {
	flushDrawCalls();

	Graphics::Surface frame;

	TinyGL::getSurfaceRef(frame);

	if (_gammaActive) {
		applyGamma(frame);
		g_system->copyRectToScreen(_gammaFrame.getPixels(), _gammaFrame.pitch, 0, 0, _gammaFrame.w, _gammaFrame.h);
	} else {
		g_system->copyRectToScreen(frame.getPixels(), frame.pitch, 0, 0, frame.w, frame.h);
	}

	g_system->updateScreen();
	swapCount++;

	return swapCount;
}

void CTinyGLContext::clear(bool color, bool depth) {
	tglClear((depth ? TGL_DEPTH_BUFFER_BIT : 0) | (color ? TGL_COLOR_BUFFER_BIT : 0));
}

void CTinyGLContext::setCameraMatrix(const float *matrix) {
	TGLfloat m[16];

	tglFrontFace(TGL_CW);
	tglMatrixMode(TGL_MODELVIEW);

	m[3] = 0.0f;
	m[7] = 0.0f;
	m[11] = 0.0f;
	m[0] = matrix[0];
	m[1] = matrix[1];
	m[2] = -matrix[2];
	m[4] = matrix[3];
	m[5] = matrix[4];
	m[6] = -matrix[5];
	m[8] = matrix[6];
	m[9] = matrix[7];
	m[15] = 1.0f;
	m[10] = -matrix[8];
	m[14] = -matrix[11];
	m[12] = matrix[9];
	m[13] = matrix[10];

	tglLoadMatrixf(m);
}

void CTinyGLContext::resetCameraMatrix() {
	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();
	tglScalef(1.0f, 1.0f, -1.0f);
}

void CTinyGLContext::setViewport(int x, int y, int width, int height) {
	tglViewport(x, y, width, height);
}

void CTinyGLContext::setPerspective(float fovX, float fovY, float nearZ, float farZ) {
	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();

	double top = tan((double)kPi * fovY * kDegreesToHalfRadians) * nearZ;
	double right = tan((double)kPi * fovX * kDegreesToHalfRadians) * nearZ;

	tglFrustum(-right, right, -top, top, nearZ, farZ);
}

void CTinyGLContext::setOrtho(float left, float right, float bottom, float top) {
	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglOrtho(left, right, bottom, top, 0.0, 1.0);
}

void CTinyGLContext::setGamma(float gamma) {
	if (!hasGammaCtl)
		return;

	for (int i = 0; i < 256; i++) {
		double value = pow((double)i * 0.00392156862745098, (double)gamma) * 65535.0 + 0.5;

		if (value > 65535.0)
			value = 65535.0;

		_gammaRamp[i] = (uint16)(int64)value;
	}

	_gammaActive = gamma != 1.0f;
}

int CTinyGLContext::getMaxClipPlanes() {
	return kMaxClipPlanes;
}

void CTinyGLContext::addClipPlane(float a, float b, float c, float d) {
	if (numClipPlanes >= kMaxClipPlanes) {
		warning("CTinyGLContext::addClipPlane(): too many clip planes");

		return;
	}

	// TinyGL has no clip planes: drawTriangles applies them by hand
	_clipPlanes[numClipPlanes].a = a;
	_clipPlanes[numClipPlanes].b = b;
	_clipPlanes[numClipPlanes].c = c;
	_clipPlanes[numClipPlanes].d = d;
	numClipPlanes++;
}

void CTinyGLContext::clearClipPlanes() {
	numClipPlanes = 0;
}

void CTinyGLContext::submitVertices(CDCVertexArray *vertexArray) {
	submitVerticesTimer.start();

	CDCVertex *vertices = (CDCVertex *)vertexArray->lock();

	// TinyGL reads the indices as signed 16 bit values
	if (vertexArray->numVerts > 32768)
		warning("CTinyGLContext::submitVertices(): too many vertices for TinyGL (%d)", vertexArray->numVerts);

	tglVertexPointer(3, TGL_FLOAT, sizeof(CDCVertex), vertices);
	tglEnableClientState(TGL_VERTEX_ARRAY);
	tglEnableClientState(TGL_COLOR_ARRAY);

	_vertexColors = (byte *)gpMemoryServices->malloc(vertexArray->numVerts * 4 * sizeof(byte));

	if (_vertexColors) {
		for (int i = 0; i < vertexArray->numVerts; i++) {
			uint32 color = vertices[i].color;

			_vertexColors[4 * i] = (color >> 16) & 0xFF;
			_vertexColors[4 * i + 1] = (color >> 8) & 0xFF;
			_vertexColors[4 * i + 2] = color & 0xFF;
			_vertexColors[4 * i + 3] = color >> 24;
		}
	}

	// TinyGL does not take a zero stride as tightly packed colors
	tglColorPointer(4, TGL_UNSIGNED_BYTE, 4, _vertexColors);
	tglTexCoordPointer(2, TGL_FLOAT, sizeof(CDCVertex), &vertices->u);
	tglEnableClientState(TGL_TEXTURE_COORD_ARRAY);

	currentVertexArray = vertexArray;
	_lockedVertices = vertices;
	counter8 += vertexArray->numVerts;

	submitVerticesTimer.stop();
}

void CTinyGLContext::releaseVertices() {
	if (!currentVertexArray) {
		warning("CTinyGLContext::releaseVertices(): no vertex array submitted");

		return;
	}

	currentVertexArray->unlock();
	gpMemoryServices->free(_vertexColors);
	_vertexColors = nullptr;
	_lockedVertices = nullptr;
}

void CTinyGLContext::drawTriangles(int flags, CTexture *tex, int numIndices, const uint16 *indices) {
	drawTrianglesTimer.start();

	if (numIndices) {
		setPolyOptions(flags);
		tglEnable(TGL_CULL_FACE);
		tglFrontFace(TGL_CW);
		setTextures(tex, flags);

		drawElementsTimer.start();

		if (numClipPlanes > 0) {
			drawClippedTriangles(numIndices, indices);
		} else {
			reserveVertices(numIndices);
			tglDrawElements(TGL_TRIANGLES, numIndices, TGL_UNSIGNED_SHORT, indices);
		}

		drawElementsTimer.stop();

		trianglesDrawn += numIndices / 3;
	}

	drawTrianglesTimer.stop();
}

void CTinyGLContext::drawQuads(int flags, CTexture *tex, int first, int count, const VertexStreams *streams) {
	drawQuadsTimer.start();

	if (count) {
		reserveVertices(count);
		setPolyOptions(flags);
		tglEnable(TGL_CULL_FACE);
		tglFrontFace(TGL_CW);

		if (tex)
			setTextures(tex, flags);
		else
			setTextures(nullptr, 0);

		tglBegin(TGL_QUADS);

		for (int i = first; i < first + count; i++) {
			if (streams->uvs)
				tglTexCoord2f(streams->uvs[2 * i], streams->uvs[2 * i + 1]);

			if (streams->colors) {
				uint32 color = streams->colors[i];

				tglColor4ub((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, color >> 24);
			} else {
				tglColor4ub(255, 255, 255, 255);
			}

			tglVertex3fv(&streams->positions[3 * i]);
		}

		tglEnd();

		trianglesDrawn += count / 4;
	}

	drawQuadsTimer.stop();
}

int CTinyGLContext::outputDrawPoly(DrawPoly *poly) {
	outputDrawPolyTimer.start();

	reserveVertices(poly->numVerts);

	poly->flags &= 0xFFFFFF0F;

	if (poly->tex)
		poly->flags |= 0x20;
	else
		poly->flags |= 0x10;

	setPolyOptions(poly->flags);

	if (poly->flags & 0x10) {
		setTextures(nullptr, poly->flags);
		tglBegin(TGL_TRIANGLE_FAN);

		for (int i = 0; i < poly->numVerts; i++) {
			DrawVertex *vertex = poly->verts[i];

			if ((poly->flags & 3) == 0) {
				uint32 color = vertex->color;

				tglColor4ub((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, color >> 24);
			}

			tglVertex3fv(&vertex->x);
		}
	} else {
		setTextures(poly->tex, poly->flags);
		tglBegin(TGL_TRIANGLE_FAN);

		for (int i = 0; i < poly->numVerts; i++) {
			DrawVertex *vertex = poly->verts[i];

			tglTexCoord2f(vertex->u, vertex->v);

			if ((poly->flags & 3) == 0) {
				uint32 color = vertex->color;

				tglColor4ub((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, color >> 24);
			}

			tglVertex3fv(&vertex->x);
		}
	}

	tglEnd();
	trianglesDrawn++;

	outputDrawPolyTimer.stop();

	return 1;
}

void CTinyGLContext::drawPrim(int flags, DrawVertex *verts, int count, bool filled) {
	reserveVertices(count);
	setPolyOptions(flags);
	setTextures(nullptr, flags);

	tglBegin(filled ? TGL_POLYGON : TGL_LINE_LOOP);

	for (int i = 0; i < count; i++) {
		tglVertex3fv(&verts[i].x);

		if ((flags & 3) == 0) {
			uint32 color = verts[i].color;

			tglColor4ub((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, color >> 24);
		}
	}

	tglEnd();
}

void CTinyGLContext::drawPrim2D(int flags, DrawVertex *verts, int count, bool filled) {
	tglDisable(TGL_CULL_FACE);
	setOrtho(-1.0f, 1.0f, -1.0f, 1.0f);
	drawPrim(flags, verts, count, filled);
}

void CTinyGLContext::setColor(const Color *color) {
	tglColor4ub(color->r, color->g, color->b, color->a);
}

void CTinyGLContext::setPolyOptions(int flags) {
	setPolyOptionsTimer.start();

	if (flags & 0x100) {
		if (!depthTestEnabled) {
			tglEnable(TGL_DEPTH_TEST);
			tglDepthMask(TGL_TRUE);
			tglDepthFunc(TGL_LEQUAL);
			depthTestEnabled = true;
		}
	} else if (depthTestEnabled) {
		tglDisable(TGL_DEPTH_TEST);
		depthTestEnabled = false;
	}

	if ((flags & 3) != 0 && shadeModel != 1) {
		shadeModel = 1;
		tglShadeModel(TGL_FLAT);
	}

	if (flags & 1)
		tglColor3ub(255, 255, 255);

	if ((flags & 4) != 0 && shadeModel != 2) {
		shadeModel = 2;
		tglShadeModel(TGL_SMOOTH);
	}

	if (flags & 0x10000) {
		if (!blendEnabled) {
			tglEnable(TGL_BLEND);
			blendEnabled = true;
		}

		if (blendMode != 1) {
			tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
			blendMode = 1;
		}
	} else if (blendEnabled) {
		tglDisable(TGL_BLEND);
		blendEnabled = false;
	}

	if (flags & 0x20000) {
		if (!alphaTestEnabled) {
			tglEnable(TGL_ALPHA_TEST);
			alphaTestEnabled = true;
		}
	} else if (alphaTestEnabled) {
		tglDisable(TGL_ALPHA_TEST);
		alphaTestEnabled = false;
	}

	polyOptions = flags;

	setPolyOptionsTimer.stop();
}

void CTinyGLContext::setTextures(CTexture *tex, int flags) {
	setTexturesTimer.start();

	if (!tex) {
		texture2dEnabled = false;
		tglDisable(TGL_TEXTURE_2D);
		setTexturesTimer.stop();

		return;
	}

	uint textureFlags = tex->flags;
	CTinyGLTexture *texture = static_cast<CTinyGLTexture *>(tex);

	tglBindTexture(TGL_TEXTURE_2D, texture->glId);
	texture->boundFlush = flushCount;

	// TinyGL keeps the wrap mode in the context, not in the texture
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_S, (textureFlags & 0x100) ? TGL_CLAMP_TO_EDGE : TGL_REPEAT);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_T, (textureFlags & 0x200) ? TGL_CLAMP_TO_EDGE : TGL_REPEAT);

	if (texEnvMode != 2) {
		texEnvMode = 2;
		tglTexEnvi(TGL_TEXTURE_ENV, TGL_TEXTURE_ENV_MODE, TGL_MODULATE);
	}

	if ((textureFlags & 0x80) || (flags & 0x10000)) {
		if (!blendEnabled) {
			tglEnable(TGL_BLEND);
			blendEnabled = true;
		}

		if (blendMode != 1) {
			tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
			blendMode = 1;
		}
	}

	if (textureFlags & 0x400) {
		if (!blendEnabled) {
			tglEnable(TGL_BLEND);
			blendEnabled = true;
		}

		if (blendMode != 4) {
			tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE);
			blendMode = 4;
		}
	}

	if (!texture2dEnabled) {
		texture2dEnabled = true;
		tglEnable(TGL_TEXTURE_2D);
	}

	setTexturesTimer.stop();
}

void CTinyGLContext::readBackBuffer(LABitmap *bitmap) {
	int width = displayMode.width;
	int height = displayMode.height;
	LColorFormat format(6);

	bitmap->create(width, height, format, nullptr);

	if (!_contextHandle) {
		warning("CTinyGLContext::readBackBuffer(): no frame available");

		return;
	}

	// TinyGL queues the draws: they are executed before reading
	flushDrawCalls();

	Graphics::Surface frame;

	TinyGL::getSurfaceRef(frame);

	byte *destination = (byte *)bitmap->getPixels();

	for (int row = 0; row < height; row++) {
		byte *line = destination + row * bitmap->getStride();

		for (int x = 0; x < width; x++) {
			uint8 red;
			uint8 green;
			uint8 blue;

			frame.format.colorToRGB(frame.getPixel(x, row), red, green, blue);

			line[3 * x] = red;
			line[3 * x + 1] = green;
			line[3 * x + 2] = blue;
		}
	}
}

void CTinyGLContext::readBackBufferRaw(char *dest, uint size) {
	uint bytesPerPixel = displayMode.bpp >> 3;

	if (!_contextHandle || displayMode.width < (int)kRawReadWidth || displayMode.height < (int)kRawReadHeight) {
		warning("CTinyGLContext::readBackBufferRaw(): no frame available");

		return;
	}

	if (size < kRawReadWidth * kRawReadHeight * bytesPerPixel) {
		warning("CTinyGLContext::readBackBufferRaw(): buffer too small");

		return;
	}

	// TinyGL queues the draws: they are executed before reading
	flushDrawCalls();

	Graphics::Surface frame;

	TinyGL::getSurfaceRef(frame);

	for (uint row = 0; row < kRawReadHeight; row++) {
		int frameRow = frame.h - kRawReadHeight + row;
		byte *destination = (byte *)dest;

		for (uint x = 0; x < kRawReadWidth; x++) {
			uint8 red;
			uint8 green;
			uint8 blue;

			frame.format.colorToRGB(frame.getPixel(x, frameRow), red, green, blue);

			if (bytesPerPixel == 4) {
				destination[4 * x] = red;
				destination[4 * x + 1] = green;
				destination[4 * x + 2] = blue;
				destination[4 * x + 3] = 255;
			} else {
				uint16 packed = (((red * 31 + 127) / 255) << 11) | (((green * 63 + 127) / 255) << 5) | ((blue * 31 + 127) / 255);

				memcpy(destination + 2 * x, &packed, sizeof(packed));
			}
		}

		dest += kRawReadWidth * bytesPerPixel;
	}
}

void CTinyGLContext::copyFrontToBack() {
	setOrtho(-1.0f, 1.0f, -1.0f, 1.0f);
	setPolyOptions(18);
	setTextures(nullptr, 18);

	// TinyGL has no glCopyPixels: the last frame is still in its framebuffer
}

CDCVertexArray *CTinyGLContext::createVertexArray(int numVerts, const float *positions, const uint32 *colors, const float *uvs) {
	return new COGLVertexArray(numVerts, positions, colors, uvs);
}

CDCVertexArray *CTinyGLContext::createVertexArray(const VertexStreams *streams) {
	return new COGLVertexArray(streams);
}

CDCVertexArray *CTinyGLContext::createVertexArray(CDCVertexArray *source) {
	return new COGLVertexArray(source);
}

CTexture *CTinyGLContext::createTexture(LABitmap *bitmap) {
	if (!bitmap)
		return nullptr;

	int width = bitmap->getWidth();

	if (((width - 1) & bitmap->getWidth()) != 0) {
		if (gpConsoleServices)
			gpConsoleServices->logError("Texture not a power of two in width: %s\n", bitmap->bitmapName.c_str());
		else
			warning("CTinyGLContext::createTexture(): texture not a power of two in width: %s", bitmap->bitmapName.c_str());

		return defaultTexture;
	}

	int height = bitmap->getHeight();

	if (((height - 1) & bitmap->getHeight()) != 0) {
		if (gpConsoleServices)
			gpConsoleServices->logError("Texture not a power of two in height: %s\n", bitmap->bitmapName.c_str());
		else
			warning("CTinyGLContext::createTexture(): texture not a power of two in height: %s", bitmap->bitmapName.c_str());

		return defaultTexture;
	}

	if (width == 0 || height == 0) {
		// The original could divide by zero here, let's avoid that...
		warning("CTinyGLContext::createTexture(): empty texture: %s", bitmap->bitmapName.c_str());

		return defaultTexture;
	}

	if ((uint)width / (uint)height > 8 || (uint)height / (uint)width > 8) {
		if (gpConsoleServices)
			gpConsoleServices->logError("Texture aspect greater than 8:1 : %s\n", bitmap->bitmapName.c_str());
		else
			warning("CTinyGLContext::createTexture(): texture aspect greater than 8:1: %s", bitmap->bitmapName.c_str());

		return defaultTexture;
	}

	return new CTinyGLTexture(this, bitmap, textureFlag);
}

CTexture *CTinyGLContext::createTexture(int width, int height, const LColorFormat &format, byte *pixels, const char *name) {
	LABitmap bitmap(width, height, format, pixels, true);

	return createTexture(&bitmap);
}

int CTinyGLContext::getTextureCost(CTexture *tex) {
	if (!tex || tex == defaultTexture)
		return 0;

	return sizeof(CTinyGLTexture);
}

void CTinyGLContext::prepareDialog() {
	if (swapCount & 1)
		present(false);
}

void CTinyGLContext::initGLState() {
	tglDisable(TGL_CULL_FACE);
	tglDisable(TGL_LIGHTING);
	tglEnable(TGL_DEPTH_TEST);
	tglClearDepth(1.0);
	tglDepthFunc(TGL_LEQUAL);
	tglAlphaFunc(TGL_GEQUAL, 0.1f);

	depthTestEnabled = true;
	blendEnabled = false;
	alphaTestEnabled = false;
	blendMode = 0;
	shadeModel = 2;
	tglShadeModel(TGL_SMOOTH);
	texture2dEnabled = false;
	texEnvMode = 0;
	hasGammaCtl = true;
}

void CTinyGLContext::flushDrawCalls() {
	TinyGL::presentBuffer();
	_queuedBytes = 0;
	flushCount++;
}

void CTinyGLContext::reserveVertices(int count) {
	// TinyGL stores the queued draws in a fixed memory block
	uint bytes = count * kBytesPerVertex + kBytesPerDrawCall;

	if (_queuedBytes + bytes > kQueueFlushThreshold)
		flushDrawCalls();

	_queuedBytes += bytes;
}

void CTinyGLContext::drawClippedTriangles(int numIndices, const uint16 *indices) {
	if (!currentVertexArray || !_lockedVertices) {
		warning("CTinyGLContext::drawClippedTriangles(): no vertex array submitted");

		return;
	}

	ClipVertex polygonA[kMaxClipVertices];
	ClipVertex polygonB[kMaxClipVertices];

	for (int first = 0; first + 2 < numIndices; first += 3) {
		bool valid = true;

		for (int corner = 0; corner < 3; corner++) {
			int index = indices[first + corner];

			if (index >= currentVertexArray->numVerts) {
				warning("CTinyGLContext::drawClippedTriangles(): vertex index %d out of range", index);
				valid = false;

				break;
			}

			const CDCVertex &vertex = _lockedVertices[index];

			polygonA[corner].x = vertex.x;
			polygonA[corner].y = vertex.y;
			polygonA[corner].z = vertex.z;
			polygonA[corner].red = (vertex.color >> 16) & 0xFF;
			polygonA[corner].green = (vertex.color >> 8) & 0xFF;
			polygonA[corner].blue = vertex.color & 0xFF;
			polygonA[corner].alpha = vertex.color >> 24;
			polygonA[corner].u = vertex.u;
			polygonA[corner].v = vertex.v;
		}

		if (!valid)
			continue;

		ClipVertex *input = polygonA;
		ClipVertex *output = polygonB;
		int count = 3;

		for (int plane = 0; plane < numClipPlanes && count > 0; plane++) {
			count = clipPolygon(input, count, _clipPlanes[plane], output);

			ClipVertex *temp = input;

			input = output;
			output = temp;
		}

		if (count < 3)
			continue;

		reserveVertices(count);
		tglBegin(TGL_TRIANGLE_FAN);

		for (int i = 0; i < count; i++) {
			tglColor4f(input[i].red / 255.0f, input[i].green / 255.0f, input[i].blue / 255.0f, input[i].alpha / 255.0f);
			tglTexCoord2f(input[i].u, input[i].v);
			tglVertex3f(input[i].x, input[i].y, input[i].z);
		}

		tglEnd();
	}
}

int CTinyGLContext::clipPolygon(const ClipVertex *input, int count, const ClipPlane &plane, ClipVertex *output) {
	int outputCount = 0;

	for (int i = 0; i < count; i++) {
		const ClipVertex &current = input[i];
		const ClipVertex &next = input[(i + 1) % count];
		float currentDistance = plane.a * current.x + plane.b * current.y + plane.c * current.z + plane.d;
		float nextDistance = plane.a * next.x + plane.b * next.y + plane.c * next.z + plane.d;
		bool currentInside = currentDistance >= 0.0f;
		bool nextInside = nextDistance >= 0.0f;

		if (currentInside) {
			output[outputCount] = current;
			outputCount++;
		}

		if (currentInside != nextInside) {
			float t = currentDistance / (currentDistance - nextDistance);
			ClipVertex &cut = output[outputCount];

			cut.x = current.x + (next.x - current.x) * t;
			cut.y = current.y + (next.y - current.y) * t;
			cut.z = current.z + (next.z - current.z) * t;
			cut.red = current.red + (next.red - current.red) * t;
			cut.green = current.green + (next.green - current.green) * t;
			cut.blue = current.blue + (next.blue - current.blue) * t;
			cut.alpha = current.alpha + (next.alpha - current.alpha) * t;
			cut.u = current.u + (next.u - current.u) * t;
			cut.v = current.v + (next.v - current.v) * t;
			outputCount++;
		}
	}

	return outputCount;
}

void CTinyGLContext::applyGamma(const Graphics::Surface &frame) {
	if (_gammaFrame.w != frame.w || _gammaFrame.h != frame.h || _gammaFrame.format != frame.format) {
		_gammaFrame.free();
		_gammaFrame.create(frame.w, frame.h, frame.format);
	}

	for (int y = 0; y < frame.h; y++) {
		for (int x = 0; x < frame.w; x++) {
			uint8 red;
			uint8 green;
			uint8 blue;

			frame.format.colorToRGB(frame.getPixel(x, y), red, green, blue);
			_gammaFrame.setPixel(x, y, frame.format.RGBToColor(_gammaRamp[red] >> 8, _gammaRamp[green] >> 8, _gammaRamp[blue] >> 8));
		}
	}
}

} // End of namespace Grim

#endif
