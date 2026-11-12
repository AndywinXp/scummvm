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

#include "common/scummsys.h"

#include "engines/grim/std/std_3d_config.h"

#if defined(GRIM_STD3D_TINYGL)

#include "common/endian.h"
#include "common/list.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/util.h"

#include "graphics/pixelformat.h"
#include "graphics/surface.h"

#include "engines/grim/std/std_3d_tinygl.h"

namespace Grim {

enum {
	kCmpCapsGreater = 0x10,
	kBitDepth16 = 0x400
};

enum {
	kMaxTextureSize = 4096,
	kTextureSize = 256,
	kDrawCallMemory = 8 * 1024 * 1024,
	kQueueFlushThreshold = 6 * 1024 * 1024,
	kBytesPerVertex = 152,
	kBytesPerDrawCall = 512,
	kBytesPerSmallCall = 256,
	kZShift = 14,
	kTexCoordShift = 14,
	kTextureBudget = 2 * 1024 * 1024
};

static const float tinyGlZOffset = (float)(8192.0 / (1073741824.0 - 0.5));

static const float tinyGlTexCoordLimit = (float)(2147483648.0 / (double)((kTextureSize << kTexCoordShift) - 1));

static const byte syntheticDeviceGuid[16] = {
	0x53, 0x63, 0x75, 0x6D, 0x6D, 0x56, 0x4D, 0x20, 0x54, 0x69, 0x6E, 0x79, 0x47, 0x4C, 0x00, 0x02
};

struct Std3DTinyGlSurface {
	byte *cpu;
	TinyGL::ContextHandle *context;
	byte *pixels;
	int pitch;
	int pendingColor;
	uint queuedBytes;
};

static Graphics::PixelFormat displayFormat() {
	return Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0);
}

static Std3DTinyGlSurface *tinyGlData(const Std3DSurface *surface) {
	return (Std3DTinyGlSurface *)surface->backendData;
}

static TGLenum tinyGlDepthFunc(uint d3dFunc) {
	switch (d3dFunc) {
	case 1:
		return TGL_NEVER;
	case 2:
		return TGL_GREATER;
	case 3:
		return TGL_EQUAL;
	case 4:
		return TGL_GEQUAL;
	case 5:
		return TGL_LESS;
	case 6:
		return TGL_NOTEQUAL;
	case 7:
		return TGL_LEQUAL;
	case 8:
		return TGL_ALWAYS;
	default:
		warning("Std3DTinyGlBackend: invalid z compare function %u", d3dFunc);
		return TGL_ALWAYS;
	}
}

static TGLenum tinyGlAlphaFunc(uint d3dFunc) {
	switch (d3dFunc) {
	case 1:
		return TGL_NEVER;
	case 2:
		return TGL_LESS;
	case 3:
		return TGL_EQUAL;
	case 4:
		return TGL_LEQUAL;
	case 5:
		return TGL_GREATER;
	case 6:
		return TGL_NOTEQUAL;
	case 7:
		return TGL_GEQUAL;
	case 8:
		return TGL_ALWAYS;
	default:
		warning("Std3DTinyGlBackend: invalid alpha compare function %u", d3dFunc);
		return TGL_ALWAYS;
	}
}

Std3DTinyGlBackend::Std3DTinyGlBackend() : _modeReady(0), _warnedZFunc(0), _warnedTexCoord(0), _textureContext(nullptr) {
}

Std3DTinyGlBackend::~Std3DTinyGlBackend() {
	while (!_surfaces.empty())
		destroySurface(_surfaces.back());
}

int Std3DTinyGlBackend::openGraphicsMode(int width, int height) {
	Graphics::PixelFormat format = displayFormat();

	_modeReady = 0;
	initGraphics(width, height, &format);

	if (g_system->getScreenFormat() != format) {
		warning("Std3DTinyGlBackend::openGraphicsMode(): the screen does not support the RGB565 pixel format");
		return 0;
	}

	_modeReady = 1;
	return 1;
}

void Std3DTinyGlBackend::fillDeviceDesc(Std3DDeviceDesc *desc) {
	desc->zCmpCaps = kCmpCapsGreater;
	desc->deviceRenderBitDepth = kBitDepth16;
	desc->deviceZBufferBitDepth = kBitDepth16;
}

int Std3DTinyGlBackend::isSupported() {
	if (_modeReady)
		return 1;

	Graphics::PixelFormat format = displayFormat();
	Common::List<Graphics::PixelFormat> formats = g_system->getSupportedFormats();

	for (Common::List<Graphics::PixelFormat>::const_iterator it = formats.begin(); it != formats.end(); ++it) {
		if (*it == format)
			return 1;
	}

	return 0;
}

int Std3DTinyGlBackend::canProbeWithoutMode() {
	return 1;
}

int Std3DTinyGlBackend::ensureReady() {
	return _modeReady;
}

const char *Std3DTinyGlBackend::getName() {
	return "TinyGL";
}

const char *Std3DTinyGlBackend::getDescription() {
	return "ScummVM TinyGL";
}

const byte *Std3DTinyGlBackend::getGuid() {
	return syntheticDeviceGuid;
}

int Std3DTinyGlBackend::getMaxTextureSize() {
	return kMaxTextureSize;
}

uint Std3DTinyGlBackend::getTextureBudget() {
	return kTextureBudget;
}

void Std3DTinyGlBackend::removeSurface(Std3DSurface *surface) {
	for (uint i = 0; i < _surfaces.size(); i++) {
		if (_surfaces[i] == surface) {
			_surfaces[i] = _surfaces.back();
			_surfaces.pop_back();
			return;
		}
	}
}

void Std3DTinyGlBackend::flushQueue(Std3DTinyGlSurface *data) {
	TinyGL::setContext(data->context);
	TinyGL::presentBuffer();
	data->pendingColor = 0;
	data->queuedBytes = 0;
}

void Std3DTinyGlBackend::prepareColorAccess(Std3DTinyGlSurface *data) {
	if (data->context && data->pendingColor)
		flushQueue(data);
}

void Std3DTinyGlBackend::reserveQueue(Std3DTinyGlSurface *data, uint bytes) {
	if (data->queuedBytes + bytes > kQueueFlushThreshold)
		flushQueue(data);

	data->queuedBytes += bytes;
}

void Std3DTinyGlBackend::releaseContext(Std3DTinyGlSurface *data, int width, int height, int keepPixels) {
	TinyGL::ContextHandle *context = data->context;

	TinyGL::setContext(context);

	if (_textureContext == context) {
		for (uint i = 0; i < _textures.size(); i++) {
			TGLuint texture = _textures[i];

			tglDeleteTextures(1, &texture);
		}

		_textures.clear();
		_textureContext = nullptr;
	}

	if (keepPixels) {
		flushQueue(data);

		int pitch = width * 2;
		byte *cpu = new byte[pitch * height];

		for (int y = 0; y < height; y++)
			memcpy(cpu + y * pitch, data->pixels + y * data->pitch, pitch);

		data->cpu = cpu;
		data->pixels = cpu;
		data->pitch = pitch;
	} else {
		data->pixels = nullptr;
	}

	TinyGL::destroyContext(context);
	data->context = nullptr;
	data->pendingColor = 0;
	data->queuedBytes = 0;
}

int Std3DTinyGlBackend::createSurface(Std3DSurface *surface, int hasDepth, int zBufferBitDepth) {
	Std3DTinyGlSurface *data = new Std3DTinyGlSurface;

	memset(data, 0, sizeof(Std3DTinyGlSurface));
	data->pitch = surface->width * 2;
	data->cpu = new byte[data->pitch * surface->height];
	memset(data->cpu, 0, data->pitch * surface->height);
	data->pixels = data->cpu;

	surface->backendData = data;
	_surfaces.push_back(surface);

	if (hasDepth && !attachDepth(surface, zBufferBitDepth)) {
		destroySurface(surface);
		return 0;
	}

	return 1;
}

void Std3DTinyGlBackend::destroySurface(Std3DSurface *surface) {
	Std3DTinyGlSurface *data = tinyGlData(surface);

	if (!data)
		return;

	if (data->context)
		releaseContext(data, surface->width, surface->height, 0);

	delete[] data->cpu;
	delete data;
	surface->backendData = nullptr;
	removeSurface(surface);
}

int Std3DTinyGlBackend::uploadPixels(Std3DSurface *surface, const StdRect *rectPointer, const byte *pixels, int pitchInBytes) {
	Std3DTinyGlSurface *data = tinyGlData(surface);

	prepareColorAccess(data);

	for (int y = 0; y < rectPointer->height; y++) {
		const byte *in = pixels + y * pitchInBytes;
		uint16 *out = (uint16 *)(data->pixels + (rectPointer->y + y) * data->pitch) + rectPointer->x;

		for (int x = 0; x < rectPointer->width; x++)
			out[x] = READ_LE_UINT16(in + 2 * x);
	}

	return 1;
}

int Std3DTinyGlBackend::readPixels(Std3DSurface *surface, const StdRect *rectPointer, byte *out, int pitchInBytes) {
	Std3DTinyGlSurface *data = tinyGlData(surface);

	prepareColorAccess(data);

	for (int y = 0; y < rectPointer->height; y++) {
		const uint16 *in = (const uint16 *)(data->pixels + (rectPointer->y + y) * data->pitch) + rectPointer->x;
		byte *row = out + y * pitchInBytes;

		for (int x = 0; x < rectPointer->width; x++)
			WRITE_LE_UINT16(row + 2 * x, in[x]);
	}

	return 1;
}

int Std3DTinyGlBackend::fillRect(Std3DSurface *surface, const StdRect *rectPointer, uint16 color) {
	Std3DTinyGlSurface *data = tinyGlData(surface);

	prepareColorAccess(data);

	for (int y = 0; y < rectPointer->height; y++) {
		uint16 *out = (uint16 *)(data->pixels + (rectPointer->y + y) * data->pitch) + rectPointer->x;

		for (int x = 0; x < rectPointer->width; x++)
			out[x] = color;
	}

	return 1;
}

int Std3DTinyGlBackend::canBlitOntoSelf() {
	return 1;
}

int Std3DTinyGlBackend::blit(Std3DSurface *dst, Std3DSurface *src, int srcX, int srcY, int width, int height, int dstX, int dstY, int colorKeyFlag, uint16 colorKey) {
	Std3DTinyGlSurface *dstData = tinyGlData(dst);
	Std3DTinyGlSurface *srcData = tinyGlData(src);

	prepareColorAccess(dstData);

	if (srcData != dstData)
		prepareColorAccess(srcData);

	int reverseRows = dst == src && dstY > srcY;
	int reverseColumns = dst == src && dstY == srcY && dstX > srcX;

	for (int i = 0; i < height; i++) {
		int y = reverseRows ? height - 1 - i : i;
		const uint16 *in = (const uint16 *)(srcData->pixels + (srcY + y) * srcData->pitch) + srcX;
		uint16 *out = (uint16 *)(dstData->pixels + (dstY + y) * dstData->pitch) + dstX;

		if (!colorKeyFlag) {
			memmove(out, in, width * 2);
		} else if (reverseColumns) {
			for (int x = width - 1; x >= 0; x--) {
				if (in[x] != colorKey)
					out[x] = in[x];
			}
		} else {
			for (int x = 0; x < width; x++) {
				if (in[x] != colorKey)
					out[x] = in[x];
			}
		}
	}

	return 1;
}

int Std3DTinyGlBackend::presentSurface(Std3DSurface *surface) {
	Std3DTinyGlSurface *data = tinyGlData(surface);

	prepareColorAccess(data);
	g_system->copyRectToScreen(data->pixels, data->pitch, 0, 0, surface->width, surface->height);
	return 1;
}

int Std3DTinyGlBackend::attachDepth(Std3DSurface *surface, int zBufferBitDepth) {
	Std3DTinyGlSurface *data = tinyGlData(surface);

	(void)zBufferBitDepth;

	if (data->context)
		return 1;

	int width = surface->width;
	int height = surface->height;
	TinyGL::ContextHandle *context = TinyGL::createContext(width, height, displayFormat(), kTextureSize, false, false, kDrawCallMemory);

	tglDisable(TGL_LIGHTING);
	tglDisable(TGL_CULL_FACE);
	tglDisable(TGL_FOG);
	tglDisable(TGL_STENCIL_TEST);
	tglDisable(TGL_POLYGON_OFFSET_FILL);
	tglDisable(TGL_SCISSOR_TEST);
	tglDisable(TGL_POLYGON_STIPPLE);
	tglShadeModel(TGL_SMOOTH);
	tglEnable(TGL_ALPHA_TEST);
	tglAlphaFunc(TGL_GREATER, 0.0f);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_S, TGL_REPEAT);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_T, TGL_REPEAT);
	tglTexEnvi(TGL_TEXTURE_ENV, TGL_TEXTURE_ENV_MODE, TGL_MODULATE);

	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglOrtho(0.0, width - 0.5, height - 0.5, 0.0, -1.0, 0.0);
	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();
	tglMatrixMode(TGL_TEXTURE);
	tglLoadIdentity();
	tglMatrixMode(TGL_MODELVIEW);
	tglViewport(0, 0, width, height);

	Graphics::Surface frame;

	TinyGL::getSurfaceRef(frame);

	byte *framePixels = (byte *)frame.getPixels();
	int framePitch = frame.pitch;

	for (int y = 0; y < height; y++)
		memcpy(framePixels + y * framePitch, data->cpu + y * data->pitch, width * 2);

	delete[] data->cpu;
	data->cpu = nullptr;
	data->context = context;
	data->pixels = framePixels;
	data->pitch = framePitch;
	data->pendingColor = 0;
	data->queuedBytes = 0;

	if (!_textureContext)
		_textureContext = context;

	return 1;
}

void Std3DTinyGlBackend::detachDepth(Std3DSurface *surface) {
	Std3DTinyGlSurface *data = tinyGlData(surface);

	if (!data || !data->context)
		return;

	releaseContext(data, surface->width, surface->height, 1);
}

int Std3DTinyGlBackend::hasDepth(const Std3DSurface *surface) {
	Std3DTinyGlSurface *data = tinyGlData(surface);

	return data && data->context;
}

int Std3DTinyGlBackend::supportsDepthUpload(const Std3DSurface *surface) {
	return hasDepth(surface);
}

int Std3DTinyGlBackend::clearDepth(Std3DSurface *surface, const StdRect *rect, float depthValue) {
	Std3DTinyGlSurface *data = tinyGlData(surface);

	if (depthValue != 0.0f) {
		warning("Std3DTinyGlBackend::clearDepth(): only the greater z compare function is supported");
		return 0;
	}

	reserveQueue(data, kBytesPerSmallCall);
	TinyGL::setContext(data->context);

	tglEnable(TGL_SCISSOR_TEST);
	tglScissor(rect->x, surface->height - rect->y - rect->height, rect->width, rect->height);
	tglClearDepth(0.0);
	tglClear(TGL_DEPTH_BUFFER_BIT);
	tglDisable(TGL_SCISSOR_TEST);
	return 1;
}

int Std3DTinyGlBackend::uploadDepth(Std3DSurface *surface, const StdRect *rectPointer, const byte *zData, int pitchInBytes) {
	Std3DTinyGlSurface *data = tinyGlData(surface);

	reserveQueue(data, kBytesPerSmallCall);
	TinyGL::setContext(data->context);

	Graphics::Surface zSurface;

	zSurface.create((int16)rectPointer->width, (int16)rectPointer->height, Graphics::PixelFormat::createFormatRGBA32());

	for (int y = 0; y < rectPointer->height; y++) {
		const byte *row = zData + y * pitchInBytes;
		uint32 *out = (uint32 *)zSurface.getBasePtr(0, y);

		for (int x = 0; x < rectPointer->width; x++)
			out[x] = (uint32)READ_LE_UINT16(row + 2 * x) << kZShift;
	}

	TinyGL::BlitImage *image = tglGenBlitImage();

	tglUploadBlitImage(image, zSurface, 0, false, true);
	tglBlitZBuffer(image, rectPointer->x, rectPointer->y);
	tglDeleteBlitImage(image);

	zSurface.free();
	return 1;
}

uint Std3DTinyGlBackend::createTexture(const byte *rgba, int width, int height, int magLinear, int minLinear) {
	if (!_textureContext) {
		warning("Std3DTinyGlBackend::createTexture(): no rendering context");
		return 0;
	}

	TinyGL::setContext(_textureContext);

	TGLuint texture = 0;

	tglGenTextures(1, &texture);

	if (!texture)
		return 0;

	tglBindTexture(TGL_TEXTURE_2D, texture);
	TGLint minFilter = TGL_NEAREST;
	TGLint magFilter = TGL_NEAREST;

	if (minLinear)
		minFilter = TGL_LINEAR;

	if (magLinear)
		magFilter = TGL_LINEAR;

	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MIN_FILTER, minFilter);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MAG_FILTER, magFilter);
	tglTexImage2D(TGL_TEXTURE_2D, 0, TGL_RGBA, width, height, 0, TGL_RGBA, TGL_UNSIGNED_BYTE, rgba);
	tglBindTexture(TGL_TEXTURE_2D, 0);

	_textures.push_back(texture);
	return texture;
}

void Std3DTinyGlBackend::deleteTexture(uint textureHandle) {
	if (!textureHandle || !_textureContext)
		return;

	for (uint i = 0; i < _textures.size(); i++) {
		if (_textures[i] == textureHandle) {
			_textures[i] = _textures.back();
			_textures.pop_back();
			break;
		}
	}

	TGLuint texture = textureHandle;

	TinyGL::setContext(_textureContext);
	tglDeleteTextures(1, &texture);
}

void Std3DTinyGlBackend::applyRenderState(const Std3DDrawList *list, uint flags) {
	if (flags & (kStd3DRsAlphaBlend | kStd3DRsAlphaTexture)) {
		tglEnable(TGL_BLEND);
		tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
	} else {
		tglDisable(TGL_BLEND);
	}

	if (list->zEnable) {
		tglEnable(TGL_DEPTH_TEST);

		if (flags & kStd3DRsZTest)
			tglDepthFunc(tinyGlDepthFunc(list->zCompareFunc));
		else
			tglDepthFunc(TGL_ALWAYS);

		tglDepthMask((flags & kStd3DRsZWrite) != 0 ? TGL_TRUE : TGL_FALSE);
	} else {
		tglDisable(TGL_DEPTH_TEST);
		tglDepthMask(TGL_FALSE);
	}
}

int Std3DTinyGlBackend::drawList(const Std3DDrawList *list) {
	Std3DTinyGlSurface *data = tinyGlData(list->target);

	if (!data || !data->context) {
		warning("Std3DTinyGlBackend::drawList(): the target has no rendering context");
		return 0;
	}

	if (list->zCompareFunc != 5 && !_warnedZFunc) {
		warning("Std3DTinyGlBackend::drawList(): only the greater z compare function is supported");
		_warnedZFunc = 1;
	}

	TinyGL::setContext(data->context);

	int textured = 0;

	tglEnable(TGL_ALPHA_TEST);
	tglAlphaFunc(tinyGlAlphaFunc(list->alphaFunc), 0.0f);
	tglDisable(TGL_TEXTURE_2D);
	tglBindTexture(TGL_TEXTURE_2D, 0);
	applyRenderState(list, list->initialRenderState);

	for (uint c = 0; c < list->numCommands; c++) {
		const Std3DCommand &command = list->commands[c];

		switch (command.type) {
		case kStd3DCommandRenderState:
			applyRenderState(list, command.flags);
			break;
		case kStd3DCommandTexture:
			if (command.texture) {
				tglEnable(TGL_TEXTURE_2D);
				tglBindTexture(TGL_TEXTURE_2D, command.texture);
				textured = 1;
			} else {
				tglDisable(TGL_TEXTURE_2D);
				tglBindTexture(TGL_TEXTURE_2D, 0);
				textured = 0;
			}

			break;
		case kStd3DCommandTriangles:
			reserveQueue(data, kBytesPerDrawCall + kBytesPerVertex * command.numIndices);
			tglBegin(TGL_TRIANGLES);

			for (uint i = 0; i < command.numIndices; i++) {
				const Std3DVertex &vertex = list->vertices[list->indices[command.firstIndex + i]];

				tglColor4ub((vertex.color >> 16) & 0xFF, (vertex.color >> 8) & 0xFF, vertex.color & 0xFF, vertex.color >> 24);

				if (textured) {
					if (!_warnedTexCoord && (ABS(vertex.tu) >= tinyGlTexCoordLimit || ABS(vertex.tv) >= tinyGlTexCoordLimit)) {
						warning("Std3DTinyGlBackend::drawList(): texture coordinates beyond +-%.0f overflow in TinyGL", tinyGlTexCoordLimit);
						_warnedTexCoord = 1;
					}

					tglTexCoord2f(vertex.tu, vertex.tv);
				}

				tglVertex3f(vertex.sx + 0.5f, vertex.sy + 0.5f, vertex.sz - tinyGlZOffset);
			}

			tglEnd();
			data->pendingColor = 1;
			break;
		default:
			break;
		}
	}

	tglDisable(TGL_TEXTURE_2D);
	tglBindTexture(TGL_TEXTURE_2D, 0);
	tglDisable(TGL_BLEND);
	tglDisable(TGL_DEPTH_TEST);
	tglDepthMask(TGL_TRUE);
	return 1;
}

Std3DBackend *createTinyGlBackend() {
	return new Std3DTinyGlBackend();
}

} // End of namespace Grim

#endif
