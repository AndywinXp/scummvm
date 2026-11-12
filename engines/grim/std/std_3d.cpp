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
#include "engines/grim/std/std_3d.h"

namespace Grim {

int Std3D::isBackendCompiled(Std3DBackendKind kind) {
	if (kind == kStd3DBackendGl) {
#if defined(GRIM_STD3D_GL)
		return 1;
#endif
	}

	if (kind == kStd3DBackendTinyGl) {
#if defined(GRIM_STD3D_TINYGL)
		return 1;
#endif
	}

	return 0;
}

} // End of namespace Grim

#if defined(GRIM_STD3D)

#include "common/str.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/std/std_3d_backend.h"
#include "engines/grim/std/std_bitmap.h"

namespace Grim {

enum {
	kDdpfAlphaPixels = 0x1,
	kDdpfPaletteIndexed4 = 0x8,
	kDdpfPaletteIndexed8 = 0x20,
	kDdpfRgb = 0x40
};

enum {
	kBitDepth1 = 0x4000,
	kBitDepth2 = 0x2000,
	kBitDepth4 = 0x1000,
	kBitDepth8 = 0x800,
	kBitDepth16 = 0x400,
	kBitDepth24 = 0x200,
	kBitDepth32 = 0x100
};

enum {
	kCmpCapsNever = 0x1,
	kCmpCapsLess = 0x2,
	kCmpCapsEqual = 0x4,
	kCmpCapsLessEqual = 0x8,
	kCmpCapsGreater = 0x10,
	kCmpCapsNotEqual = 0x20,
	kCmpCapsGreaterEqual = 0x40,
	kCmpCapsAlways = 0x80
};

enum {
	kTexCapsPerspective = 0x1,
	kTexCapsAlpha = 0x4,
	kTexCapsTransparency = 0x8,
	kTexCapsSquareOnly = 0x20
};

enum {
	kShadeCapsAlphaFlatBlend = 0x1000,
	kShadeCapsAlphaFlatStippled = 0x2000,
	kShadeCapsAlphaGouraudBlend = 0x4000
};

enum {
	kTexBlendCapsModulate = 0x4,
	kTexBlendCapsModulateAlpha = 0x8
};

enum {
	kColorModelMono = 0x1,
	kColorModelRgb = 0x2
};

static const Std3DPixelFormat syntheticTexFormats[3] = {
	{ kDdpfRgb, 16, 0xF800, 0x07E0, 0x001F, 0x0000 },
	{ kDdpfRgb | kDdpfAlphaPixels, 16, 0x7C00, 0x03E0, 0x001F, 0x8000 },
	{ kDdpfRgb | kDdpfAlphaPixels, 16, 0x0F00, 0x00F0, 0x000F, 0xF000 }
};

static int log2Int(uint value) {
	int result = 0;

	while (value > 1) {
		value >>= 1;
		result++;
	}

	return result;
}

static int readMask(uint mask, int *shiftPointer, int *loss, int *bitsPointer) {
	if (!mask) {
		warning("Std3D::enumTextureFormatsCallback(): empty color mask");
		return 0;
	}

	uint shifted = mask;
	int shift = 0;

	while ((shifted & 1) == 0) {
		shifted >>= 1;
		shift++;
	}

	*shiftPointer = shift;
	*loss = log2Int(0xFF / (mask >> shift));

	int bits = 0;

	while ((shifted & 1) != 0) {
		shifted >>= 1;
		bits++;
	}

	*bitsPointer = bits;
	return 1;
}

Std3D::Std3D(StdDisplay *stdDisplay, StdBitmap *stdBitmap) : _stdDisplay(stdDisplay), _stdBitmap(stdBitmap), _backend(nullptr), _backendKind(kStd3DBackendGl), _displayModeTex(nullptr),
	_renderTarget(nullptr), _depthFromDevice(0), _available(0), _started(0), _hardware(1), _sceneBegun(0), _enumFlags(0), _numDevices(0),
	_curDevice(nullptr), _curDeviceIndex(0), _zBufferMode(0), _zBufferBitDepth(0), _numTexFormats(0), _opaqueFormatIndex(0), _alphaFormatIndex(0),
	_colorKeyFormatIndex(0), _texFormatOpaque(nullptr), _texFormatAlpha(nullptr), _texFormatColorKey(nullptr), _frameBuffer(nullptr),
	_maxVertices(0), _frameCounter(1), _numCachedTextures(0), _minTexWidth(4), _minTexHeight(4), _maxTexWidth(256), _maxTexHeight(256),
	_cacheHead(nullptr), _cacheTail(nullptr), _curRenderState(0), _fogStart(0.0f), _fogEnd(0.0f), _fogRed(0), _fogGreen(0), _fogBlue(0),
	_alphaFunc(0), _zEnable(0), _texMagLinear(0), _texMinLinear(0), _numVertices(0), _numTriangles(0), _vertices(nullptr), _listStart(0),
	_listStartRenderState(0), _curTexture(nullptr), _curTextureValid(0) {
	memset(_devices, 0, sizeof(_devices));
	memset(&_zBuffer, 0, sizeof(_zBuffer));
	memset(_texFormats, 0, sizeof(_texFormats));
	memset(_palette, 0, sizeof(_palette));
	memset(_opaquePaletteTable, 0, sizeof(_opaquePaletteTable));
	memset(_alphaPaletteTable, 0, sizeof(_alphaPaletteTable));
	memset(_colorKeyPaletteTable, 0, sizeof(_colorKeyPaletteTable));
	memset(&_activeRect, 0, sizeof(_activeRect));
}

Std3D::~Std3D() {
	delete[] _vertices;
	_vertices = nullptr;

	if (_frameBuffer)
		_stdDisplay->vBufferFree(_frameBuffer);

	delete _backend;
	_backend = nullptr;
}

int Std3D::createBackend(Std3DBackendKind kind) {
	closeBackend();

	Std3DBackend *backend = nullptr;

	switch (kind) {
#if defined(GRIM_STD3D_GL)
	case kStd3DBackendGl:
		backend = createGlBackend();
		break;
#endif
#if defined(GRIM_STD3D_TINYGL)
	case kStd3DBackendTinyGl:
		backend = createTinyGlBackend();
		break;
#endif
	default:
		break;
	}

	if (!backend) {
		warning("Std3D::createBackend(): the backend %d is not part of this build", (int)kind);
		return 0;
	}

	_backend = backend;
	_backendKind = kind;
	return 1;
}

int Std3D::canProbeWithoutMode() {
	return _backend && _backend->canProbeWithoutMode();
}

int Std3D::openBackendMode(int width, int height) {
	if (!_backend)
		return 0;

	if (!_backend->openGraphicsMode(width, height) || !_backend->isSupported()) {
		delete _backend;
		_backend = nullptr;
		return 0;
	}

	return 1;
}

int Std3D::openBackend(Std3DBackendKind kind, int width, int height) {
	return createBackend(kind) && openBackendMode(width, height);
}

void Std3D::closeBackend() {
	if (!_backend)
		return;

	if (_started)
		shutdown();

	if (_cacheHead)
		flushTextureCache();

	if (_depthFromDevice && _renderTarget) {
		_backend->detachDepth(_renderTarget);
		_depthFromDevice = 0;
	}

	delete _backend;
	_backend = nullptr;
}

void Std3D::setDisplayMode(const StdVBufferTexFmt *tex) {
	_displayModeTex = tex;
}

void Std3D::setRenderTarget(Std3DSurface *surface) {
	_renderTarget = surface;
}

int Std3D::ensureBackend() {
	return _backend && _backend->ensureReady();
}

int Std3D::enumerateDevices() {
	if (!_backend || !_backend->isSupported())
		return 0;

	_enumFlags = kStd3DRsPerspective | kStd3DRsDither | kStd3DRsSubpixel | kStd3DRsSubpixelX | kStd3DRsTexMagLinear | kStd3DRsTexMinLinear | kStd3DRsZTest | kStd3DRsZWrite;
	_hardware = 1;
	_numDevices = 0;

	Std3DDeviceDesc hwDesc;
	Std3DDeviceDesc helDesc;

	memset(&hwDesc, 0, sizeof(hwDesc));
	memset(&helDesc, 0, sizeof(helDesc));

	hwDesc.colorModel = kColorModelRgb;
	hwDesc.textureCaps = kTexCapsPerspective | kTexCapsAlpha | kTexCapsTransparency;
	hwDesc.shadeCaps = kShadeCapsAlphaFlatBlend | kShadeCapsAlphaGouraudBlend;
	hwDesc.textureBlendCaps = kTexBlendCapsModulate | kTexBlendCapsModulateAlpha;
	hwDesc.zCmpCaps = kCmpCapsNever | kCmpCapsLess | kCmpCapsEqual | kCmpCapsLessEqual | kCmpCapsGreater | kCmpCapsNotEqual | kCmpCapsGreaterEqual | kCmpCapsAlways;
	hwDesc.deviceRenderBitDepth = kBitDepth16 | kBitDepth32;
	hwDesc.deviceZBufferBitDepth = kBitDepth16 | kBitDepth32;
	hwDesc.minTextureWidth = 1;
	hwDesc.minTextureHeight = 1;
	hwDesc.maxTextureWidth = 256;
	hwDesc.maxTextureHeight = 256;
	hwDesc.maxBufferSize = 0;
	hwDesc.maxVertexCount = 0;

	_backend->fillDeviceDesc(&hwDesc);

	enumDevicesCallback(_backend->getGuid(), _backend->getDescription(), _backend->getName(), &hwDesc, &helDesc);

	if (_numDevices) {
		_available = 1;
		return 1;
	}

	return 0;
}

void Std3D::releaseDirect3D() {
	_available = 0;
}

int Std3D::isAvailable() {
	return _available;
}

int Std3D::startup(uint deviceIndex, int hardware) {
	if (_started)
		return 0;

	if (deviceIndex >= _numDevices)
		return 0;

	if (!_displayModeTex) {
		warning("Std3D::startup(): no display mode");
		return 0;
	}

	_curDeviceIndex = deviceIndex;
	_curDevice = &_devices[deviceIndex];

	if (hardware && _curDevice->zBuffer && ((_enumFlags >> 8) & 0x18) != 0) {
		_hardware = 1;

		if (!createDevice(_displayModeTex->width, _displayModeTex->height))
			return 0;

		_zBufferMode = (_curDevice->zCmpMask & 0x10) != 0 ? kStd3DZBufferGreater : kStd3DZBufferLess;
	} else {
		_hardware = 0;
	}

	if (!_renderTarget) {
		warning("Std3D::startup(): no render target");
		return 0;
	}

	_numTexFormats = 0;

	for (int i = 0; i < ARRAYSIZE(syntheticTexFormats); i++) {
		if (!enumTextureFormatsCallback(&syntheticTexFormats[i]))
			break;
	}

	if (!_numTexFormats)
		return 0;

	if (!createViewport(_displayModeTex->width, _displayModeTex->height))
		return 0;

	if (!createExecuteBuffer())
		return 0;

	uint maxVertexCount = _curDevice->maxVertexCount;

	if (!maxVertexCount || maxVertexCount >= 0x200)
		_maxVertices = kStd3DDefaultMaxVertices;
	else
		_maxVertices = maxVertexCount;

	delete[] _vertices;
	_vertices = new Std3DVertex[_maxVertices];
	memset(_vertices, 0, sizeof(Std3DVertex) * _maxVertices);

	_frameCounter = 1;
	_numCachedTextures = 0;

	setDimensionLimits(_curDevice->minTexWidth, _curDevice->minTexHeight, _curDevice->maxTexWidth, _curDevice->maxTexHeight);

	_cacheHead = nullptr;
	_cacheTail = nullptr;

	_opaqueFormatIndex = findClosestMode(&_displayModeTex->format, _texFormats, _numTexFormats);
	_texFormatOpaque = &_texFormats[_opaqueFormatIndex];

	if (_curDevice->alphaTexture) {
		StdColorFormat wantedFormat;

		memset(&wantedFormat, 0, sizeof(wantedFormat));
		wantedFormat.colorMode = 2;
		wantedFormat.bpp = 16;
		wantedFormat.redBits = 5;
		wantedFormat.greenBits = 5;
		wantedFormat.blueBits = 5;
		wantedFormat.alphaBits = 1;

		_alphaFormatIndex = findClosestMode(&wantedFormat, _texFormats, _numTexFormats);
		_texFormatAlpha = &_texFormats[_alphaFormatIndex];

		if (!_curDevice->alphaBlend) {
			wantedFormat.colorMode = 2;
			wantedFormat.bpp = 16;
			wantedFormat.redBits = 4;
			wantedFormat.greenBits = 4;
			wantedFormat.blueBits = 4;
			wantedFormat.alphaBits = 4;

			_colorKeyFormatIndex = findClosestMode(&wantedFormat, _texFormats, _numTexFormats);

			StdVBufferTexFmt frameFormat;

			memset(&frameFormat, 0, sizeof(frameFormat));
			frameFormat.width = 32;
			frameFormat.height = 32;

			_texFormatColorKey = &_texFormats[_colorKeyFormatIndex];
			frameFormat.format = _texFormatColorKey->format;
			_frameBuffer = _stdDisplay->vBufferNew(&frameFormat, 1, 0, nullptr);
		}
	}

	getTextureMemory(&_curDevice->texMemTotal, &_curDevice->texMemFree);

	_started = 1;
	return 1;
}

void Std3D::shutdown() {
	if (!_started)
		return;

	if (_frameBuffer)
		_stdDisplay->vBufferFree(_frameBuffer);

	delete[] _vertices;
	_vertices = nullptr;

	flushTextureCache();

	if (_depthFromDevice && _renderTarget && _backend) {
		_backend->detachDepth(_renderTarget);
		_depthFromDevice = 0;
	}

	_curDevice = nullptr;
	_started = 0;
}

void Std3D::setDimensionLimits(uint minWidth, uint minHeight, uint maxWidth, uint maxHeight) {
	_minTexWidth = minWidth;
	_minTexHeight = minHeight;
	_maxTexWidth = maxWidth;
	_maxTexHeight = maxHeight;
}

int Std3D::beginScene() {
	int result = _sceneBegun;

	if (!_sceneBegun) {
		result = 0;
		_sceneBegun = 1;
	}

	return result;
}

int Std3D::endScene() {
	int result = _sceneBegun;

	if (_sceneBegun) {
		result = 0;
		_sceneBegun = 0;
	}

	return result;
}

int Std3D::resetRenderList() {
	_numVertices = 0;
	_numTriangles = 0;
	_frameCounter++;
	_curTexture = nullptr;
	_curTextureValid = 0;

	_commands.clear();
	_indices.clear();
	_listStart = 0;

	if (!_vertices) {
		warning("Std3D::resetRenderList(): device not started");
		return 0;
	}

	return 1;
}

int Std3D::addRenderListVertices(const Std3DVertex *vertices, uint count) {
	if (count > _maxVertices || count + _numVertices > _maxVertices)
		return 0;

	if (!_vertices) {
		warning("Std3D::addRenderListVertices(): device not started");
		return 0;
	}

	if (&_vertices[_numVertices] != vertices)
		memcpy(&_vertices[_numVertices], vertices, sizeof(Std3DVertex) * count);

	_numVertices += count;
	return 1;
}

int Std3D::beginRenderList() {
	_listStart = _commands.size();
	_listStartRenderState = _curRenderState;
	return 1;
}

int Std3D::addRenderListTris(const Std3DTri *tris, uint count) {
	if (!tris && count) {
		warning("Std3D::addRenderListTris(): no triangles");
		return 0;
	}

	uint numDone = 0;

	while (numDone < count) {
		const Std3DTri *tri = &tris[numDone];
		RdTexCacheEntry *texture = tri->texture;
		uint flags = tri->flags;
		uint runLength = 0;
		uint scanIndex = numDone;
		const Std3DTri *scan = tri;

		while (scanIndex < count && scan->texture == texture && scan->flags == flags) {
			runLength++;
			scanIndex++;
			scan++;
		}

		setMaterialRenderState(tri->flags);

		if (!_curTextureValid || _curTexture != texture) {
			Std3DCommand command;

			memset(&command, 0, sizeof(command));
			command.type = kStd3DCommandTexture;

			if (texture) {
				command.texture = texture->texture;
				command.textureAlpha = texture->colorKey;
			}

			_commands.push_back(command);
			_curTexture = texture;
			_curTextureValid = 1;
		}

		Std3DCommand triangles;

		memset(&triangles, 0, sizeof(triangles));
		triangles.type = kStd3DCommandTriangles;
		triangles.firstIndex = _indices.size();
		triangles.numIndices = runLength * 3;

		for (uint i = 0; i < runLength; i++) {
			_indices.push_back((uint16)tri[i].idx[0]);
			_indices.push_back((uint16)tri[i].idx[1]);
			_indices.push_back((uint16)tri[i].idx[2]);
		}

		_commands.push_back(triangles);
		numDone += runLength;
	}

	_numTriangles += count;
	return 1;
}

int Std3D::drawRenderList() {
	if (!_renderTarget || !_vertices) {
		warning("Std3D::drawRenderList(): device not started");
		return 0;
	}

	if (!ensureBackend())
		return 0;

	markGpuDirty(_renderTarget, 0, 0, _renderTarget->width, _renderTarget->height);

	Std3DDrawList list;

	memset(&list, 0, sizeof(list));
	list.target = _renderTarget;
	list.vertices = _vertices;
	list.numVertices = _numVertices;
	list.indices = _indices.empty() ? nullptr : &_indices[0];
	list.numIndices = _indices.size();

	if (_listStart < _commands.size()) {
		list.commands = &_commands[_listStart];
		list.numCommands = _commands.size() - _listStart;
	}

	list.initialRenderState = _listStartRenderState;
	list.zEnable = _zEnable;
	list.zCompareFunc = zCmpMaskToFunc(_zBufferMode);
	list.alphaFunc = _alphaFunc;
	list.fogStart = _fogStart;
	list.fogEnd = _fogEnd;
	list.fogRed = _fogRed;
	list.fogGreen = _fogGreen;
	list.fogBlue = _fogBlue;

	return _backend->drawList(&list);
}

void Std3D::setMaterialRenderState(uint flags) {
	if (_curRenderState != flags) {
		Std3DCommand command;

		memset(&command, 0, sizeof(command));
		command.type = kStd3DCommandRenderState;
		command.flags = flags;
		_commands.push_back(command);

		_curRenderState = flags;
	}
}

int Std3D::clearZBuffer() {
	if (!_renderTarget || !_backend || !_backend->hasDepth(_renderTarget)) {
		warning("Std3D::clearZBuffer(): no z buffer");
		return 0;
	}

	if (!ensureBackend())
		return 0;

	int left = _activeRect.x;
	int top = _activeRect.y;
	int right = _activeRect.x + _activeRect.width;
	int bottom = _activeRect.y + _activeRect.height;

	if (left < 0)
		left = 0;

	if (top < 0)
		top = 0;

	if (right > _renderTarget->width)
		right = _renderTarget->width;

	if (bottom > _renderTarget->height)
		bottom = _renderTarget->height;

	if (right <= left || bottom <= top)
		return 1;

	StdRect clearRect;
	float depthValue = _zBufferMode != kStd3DZBufferGreater ? 1.0f : 0.0f;

	clearRect.x = left;
	clearRect.y = top;
	clearRect.width = right - left;
	clearRect.height = bottom - top;

	return _backend->clearDepth(_renderTarget, &clearRect, depthValue);
}

uint Std3D::findClosestDevice(const Std3DDevice *wanted, const Std3DDevice *devices, uint numDevices) {
	uint index = 0;

	if (!numDevices)
		return index;

	int bestScore = 0;
	uint bestIndex = 0;
	int perspectiveCorrect = wanted->perspectiveCorrect;

	do {
		const Std3DDevice *candidate = &devices[index];
		int score = 0;

		if (!perspectiveCorrect || candidate->perspectiveCorrect == perspectiveCorrect) {
			int wantedZBuffer = wanted->zBuffer;

			score = 1;

			if (!wantedZBuffer || candidate->zBuffer == wantedZBuffer) {
				score = 2;

				if ((wanted->colorModelFlags & candidate->colorModelFlags) != 0) {
					score = 3;

					if (candidate->hardware == wanted->hardware)
						return index;
				}
			}
		}

		if (score > bestScore) {
			bestIndex = index;
			bestScore = score;
		}

		index++;
	} while (index < numDevices);

	return bestIndex;
}

uint Std3D::findClosestMode(const StdColorFormat *wanted, const Std3DTexFormat *formats, uint numFormats) {
	uint index = 0;

	if (!numFormats)
		return index;

	int bestScore = 0;
	uint bestIndex = 0;
	int colorMode = wanted->colorMode;

	do {
		const StdColorFormat *candidate = &formats[index].format;
		int score = 0;

		if (candidate->colorMode == colorMode) {
			score = 1;

			if (candidate->bpp == wanted->bpp) {
				score = 2;

				if (colorMode == 1) {
					if (candidate->redBits == wanted->redBits && candidate->greenBits == wanted->greenBits && candidate->blueBits == wanted->blueBits)
						return index;
				} else {
					if (colorMode != 2)
						return index;

					if (candidate->colorMode == 2)
						score = 3;

					if (candidate->redBits == wanted->redBits && candidate->greenBits == wanted->greenBits &&
						candidate->blueBits == wanted->blueBits && candidate->alphaBits == wanted->alphaBits)
						return index;
				}
			}
		}

		if (score > bestScore) {
			bestIndex = index;
			bestScore = score;
		}

		index++;
	} while (index < numFormats);

	return bestIndex;
}

int Std3D::setActiveRect(const StdRect *rect) {
	_activeRect = *rect;
	return 0;
}

int Std3D::createExecuteBuffer() {
	_texMagLinear = (_enumFlags & kStd3DRsTexMagLinear) != 0;
	_texMinLinear = (_enumFlags & kStd3DRsTexMinLinear) != 0;
	_alphaFunc = zCmpMaskToFunc(_zBufferMode);
	_zEnable = _hardware != 0;
	_curRenderState = _enumFlags;
	return 1;
}

int Std3D::createViewport(int width, int height) {
	StdRect rect;

	rect.x = 0;
	rect.y = 0;
	rect.width = width;
	rect.height = height;
	setActiveRect(&rect);
	return 1;
}

int Std3D::createDevice(int width, int height) {
	if (!_displayModeTex || !_renderTarget) {
		warning("Std3D::createDevice(): no display mode or render target");
		return 0;
	}

	_zBuffer.type = 1;
	_zBuffer.videoMemory = 0;
	_zBuffer.tex = *_displayModeTex;
	_zBuffer.paletteRGB = nullptr;
	_zBuffer.pixels = nullptr;

	if ((_curDevice->desc.deviceZBufferBitDepth & kBitDepth32) != 0)
		_zBufferBitDepth = 32;
	else if ((_curDevice->desc.deviceZBufferBitDepth & kBitDepth16) != 0)
		_zBufferBitDepth = 16;
	else if ((_curDevice->desc.deviceZBufferBitDepth & kBitDepth8) != 0)
		_zBufferBitDepth = 8;
	else
		return 0;

	if (_renderTarget->width != width || _renderTarget->height != height) {
		warning("Std3D::createDevice(): render target size mismatch");
		return 0;
	}

	if (!_backend) {
		warning("Std3D::createDevice(): no backend");
		return 0;
	}

	if (!_backend->hasDepth(_renderTarget)) {
		if (!_backend->attachDepth(_renderTarget, _zBufferBitDepth))
			return 0;

		_depthFromDevice = 1;
	}

	if (_curDevice->hardware)
		_zBuffer.videoMemory = 1;

	return 1;
}

int Std3D::enumDevicesCallback(const byte *guid, const char *description, const char *name, const Std3DDeviceDesc *hwDesc, const Std3DDeviceDesc *helDesc) {
	if (_numDevices >= kStd3DMaxDevices)
		return 0;

	Std3DDevice *device = &_devices[_numDevices];

	memcpy(device->guid, guid, 16);
	Common::strlcpy(device->description, description, sizeof(device->description));
	Common::strlcpy(device->name, name, sizeof(device->name));

	if (hwDesc->colorModel) {
		device->hardware = 1;
		device->desc = *hwDesc;
	} else {
		device->hardware = 0;
		device->desc = *helDesc;
	}

	int colorModel = device->desc.colorModel;

	device->colorModelFlags = 0;

	if ((colorModel & kColorModelRgb) != 0)
		device->colorModelFlags = 2;

	if ((colorModel & kColorModelMono) != 0)
		device->colorModelFlags |= 1;

	uint textureCaps = device->desc.textureCaps;
	uint shadeCaps = device->desc.shadeCaps;

	device->perspectiveCorrect = (textureCaps & kTexCapsPerspective) != 0;
	device->zBuffer = device->desc.deviceZBufferBitDepth != 0;
	device->alphaTexture = (textureCaps & kTexCapsAlpha) != 0;
	device->squareTexturesOnly = (textureCaps & kTexCapsSquareOnly) != 0;
	device->alphaStipple = (shadeCaps & kShadeCapsAlphaFlatBlend) == 0 && (shadeCaps & kShadeCapsAlphaFlatStippled) != 0;
	device->alphaBlend = ((device->desc.textureBlendCaps & kTexBlendCapsModulateAlpha) != 0 && (shadeCaps & kShadeCapsAlphaGouraudBlend) != 0) || device->alphaStipple;
	device->transparency = (textureCaps & kTexCapsTransparency) != 0;
	device->renderBitDepthMask = convertBitDepthCaps(device->desc.deviceRenderBitDepth);
	device->zCmpMask = convertZCmpCaps(device->desc.zCmpCaps);

	uint minTextureWidth = device->desc.minTextureWidth;

	if (minTextureWidth <= 4)
		minTextureWidth = 4;

	device->minTexWidth = minTextureWidth;

	uint minTextureHeight = device->desc.minTextureHeight;

	if (minTextureHeight <= 4)
		minTextureHeight = 4;

	device->minTexHeight = minTextureHeight;

	uint maxTextureWidth = device->desc.maxTextureWidth;

	if (!maxTextureWidth)
		maxTextureWidth = 256;

	device->maxTexWidth = maxTextureWidth;

	uint maxTextureHeight = device->desc.maxTextureHeight;

	if (!maxTextureHeight)
		maxTextureHeight = 256;

	device->maxTexHeight = maxTextureHeight;
	device->maxBufferSize = device->desc.maxBufferSize;
	device->maxVertexCount = device->desc.maxVertexCount;

	_numDevices++;
	return 1;
}

int Std3D::enumTextureFormatsCallback(const Std3DPixelFormat *pixelFormat) {
	if (_numTexFormats >= kStd3DMaxTexFormats)
		return 0;

	Std3DTexFormat *format = &_texFormats[_numTexFormats];
	StdColorFormat *color = &format->format;

	format->pixelFormat = *pixelFormat;

	if ((pixelFormat->flags & kDdpfPaletteIndexed8) != 0) {
		color->bpp = 8;
		color->colorMode = 0;
		color->redShift = 0;
		color->redLoss = 0;
		color->redBits = 0;
		color->greenShift = 0;
		color->greenLoss = 0;
		color->greenBits = 0;
		color->blueShift = 0;
		color->blueLoss = 0;
		color->blueBits = 0;
	} else {
		if ((pixelFormat->flags & kDdpfPaletteIndexed4) != 0)
			return 1;

		color->colorMode = (pixelFormat->flags & kDdpfAlphaPixels) != 0 ? 2 : 1;
		color->bpp = pixelFormat->rgbBitCount;

		if (!readMask(pixelFormat->rBitMask, &color->redShift, &color->redLoss, &color->redBits) ||
			!readMask(pixelFormat->gBitMask, &color->greenShift, &color->greenLoss, &color->greenBits) ||
			!readMask(pixelFormat->bBitMask, &color->blueShift, &color->blueLoss, &color->blueBits))
			return 1;

		if (color->colorMode == 2 && !readMask(pixelFormat->rgbAlphaBitMask, &color->alphaShift, &color->alphaLoss, &color->alphaBits))
			return 1;
	}

	if (color->colorMode != 2) {
		color->alphaShift = 0;
		color->alphaLoss = 0;
		color->alphaBits = 0;
	}

	_numTexFormats++;
	return 1;
}

uint Std3D::convertBitDepthCaps(uint bitDepthFlags) {
	uint result = (bitDepthFlags & kBitDepth1) != 0;

	if ((bitDepthFlags & kBitDepth2) != 0)
		result |= 2;

	if ((bitDepthFlags & kBitDepth4) != 0)
		result |= 4;

	if ((bitDepthFlags & kBitDepth8) != 0)
		result |= 8;

	if ((bitDepthFlags & kBitDepth16) != 0)
		result |= 0x10;

	if ((bitDepthFlags & kBitDepth24) != 0)
		result |= 0x20;

	if ((bitDepthFlags & kBitDepth32) != 0)
		result |= 0x40;

	return result;
}

uint Std3D::convertZCmpCaps(uint zCmpCaps) {
	uint result = (zCmpCaps & kCmpCapsNever) != 0;

	if ((zCmpCaps & kCmpCapsEqual) != 0)
		result |= 4;

	if ((zCmpCaps & kCmpCapsLess) != 0)
		result |= 2;

	if ((zCmpCaps & kCmpCapsLessEqual) != 0)
		result |= 8;

	if ((zCmpCaps & kCmpCapsGreater) != 0)
		result |= 0x10;

	if ((zCmpCaps & kCmpCapsNotEqual) != 0)
		result |= 0x20;

	if ((zCmpCaps & kCmpCapsGreaterEqual) != 0)
		result |= 0x40;

	if ((zCmpCaps & kCmpCapsAlways) != 0)
		result |= 0x80;

	return result;
}

uint Std3D::zCmpMaskToFunc(uint zCmpMask) {
	uint result = (zCmpMask & 1) != 0;

	if ((zCmpMask & 4) != 0)
		result |= 3;

	if ((zCmpMask & 2) != 0)
		result |= 2;

	if ((zCmpMask & 8) != 0)
		result |= 4;

	if ((zCmpMask & 0x10) != 0)
		result |= 5;

	if ((zCmpMask & 0x20) != 0)
		result |= 6;

	if ((zCmpMask & 0x40) != 0)
		result |= 7;

	if ((zCmpMask & 0x80) != 0)
		result |= 8;

	return result;
}

} // End of namespace Grim

#endif
