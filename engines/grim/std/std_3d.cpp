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

#if defined(USE_OPENGL_SHADERS)

#include "common/str.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "graphics/opengl/context.h"
#include "graphics/opengl/shader.h"

#include "engines/grim/std/std_3d.h"
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

static const byte syntheticDeviceGuid[16] = {
	0x53, 0x63, 0x75, 0x6D, 0x6D, 0x56, 0x4D, 0x20, 0x4F, 0x70, 0x65, 0x6E, 0x47, 0x4C, 0x00, 0x01
};

static const Std3DPixelFormat syntheticTexFormats[3] = {
	{ kDdpfRgb, 16, 0xF800, 0x07E0, 0x001F, 0x0000 },
	{ kDdpfRgb | kDdpfAlphaPixels, 16, 0x7C00, 0x03E0, 0x001F, 0x8000 },
	{ kDdpfRgb | kDdpfAlphaPixels, 16, 0x0F00, 0x00F0, 0x000F, 0xF000 }
};

static const char *const tlAttributes[] = { "position", "color", "texcoord", nullptr };
static const char *const quadAttributes[] = { "position", "texcoord", nullptr };

static const char *const tlVertexSource =
	"attribute vec4 position;\n"
	"attribute vec4 color;\n"
	"attribute vec2 texcoord;\n"
	"uniform vec2 targetSize;\n"
	"varying vec4 vColor;\n"
	"varying vec3 vTexRhw;\n"
	"void main() {\n"
	"	// Like the original TL vertices: position, depth and colour are linear in\n"
	"	// screen space, only tu/tv are perspective corrected through rhw\n"
	"	float rhw = position.w;\n"
	"	if (rhw == 0.0)\n"
	"		rhw = 1.0;\n"
	"	vec2 ndc = (position.xy + vec2(0.5, 0.5)) / targetSize * 2.0 - 1.0;\n"
	"	gl_Position = vec4(ndc, position.z * 2.0 - 1.0, 1.0);\n"
	"	vColor = color;\n"
	"	vTexRhw = vec3(texcoord * rhw, rhw);\n"
	"}\n";

static const char *const tlFragmentSource =
	"uniform sampler2D tex;\n"
	"uniform int textured;\n"
	"uniform int texHasAlpha;\n"
	"uniform int modulateAlpha;\n"
	"uniform int alphaFunc;\n"
	"uniform int fogEnabled;\n"
	"uniform vec3 fogColor;\n"
	"uniform vec2 fogRange;\n"
	"varying vec4 vColor;\n"
	"varying vec3 vTexRhw;\n"
	"void main() {\n"
	"	vec4 diffuse = vColor;\n"
	"	vec4 c = diffuse;\n"
	"	if (textured != 0) {\n"
	"		vec4 t = texture2D(tex, vTexRhw.xy / vTexRhw.z);\n"
	"		c.rgb = t.rgb * diffuse.rgb;\n"
	"		if (modulateAlpha != 0)\n"
	"			c.a = t.a * diffuse.a;\n"
	"		else if (texHasAlpha != 0)\n"
	"			c.a = t.a;\n"
	"	}\n"
	"	bool pass = true;\n"
	"	if (alphaFunc == 1)\n"
	"		pass = false;\n"
	"	else if (alphaFunc == 2)\n"
	"		pass = c.a < 0.0;\n"
	"	else if (alphaFunc == 3)\n"
	"		pass = c.a == 0.0;\n"
	"	else if (alphaFunc == 4)\n"
	"		pass = c.a <= 0.0;\n"
	"	else if (alphaFunc == 5)\n"
	"		pass = c.a > 0.0;\n"
	"	else if (alphaFunc == 6)\n"
	"		pass = c.a != 0.0;\n"
	"	else if (alphaFunc == 7)\n"
	"		pass = c.a >= 0.0;\n"
	"	if (!pass)\n"
	"		discard;\n"
	"	if (fogEnabled != 0) {\n"
	"		float range = fogRange.y - fogRange.x;\n"
	"		float f = 1.0;\n"
	"		if (range != 0.0)\n"
	"			f = clamp((fogRange.y - gl_FragCoord.z) / range, 0.0, 1.0);\n"
	"		c.rgb = mix(fogColor, c.rgb, f);\n"
	"	}\n"
	"	gl_FragColor = c;\n"
	"}\n";

static const char *const quadVertexSource =
	"attribute vec2 position;\n"
	"attribute vec2 texcoord;\n"
	"uniform vec2 targetSize;\n"
	"uniform float flipY;\n"
	"varying vec2 vTexcoord;\n"
	"void main() {\n"
	"	vec2 ndc = position / targetSize * 2.0 - 1.0;\n"
	"	gl_Position = vec4(ndc.x, ndc.y * flipY, 0.0, 1.0);\n"
	"	vTexcoord = texcoord;\n"
	"}\n";

static const char *const blitFragmentSource =
	"uniform sampler2D tex;\n"
	"uniform int useKey;\n"
	"uniform vec3 keyColor;\n"
	"varying vec2 vTexcoord;\n"
	"void main() {\n"
	"	vec4 c = texture2D(tex, vTexcoord);\n"
	"	if (useKey != 0 && all(lessThan(abs(c.rgb - keyColor), vec3(0.5 / 255.0))))\n"
	"		discard;\n"
	"	gl_FragColor = vec4(c.rgb, 1.0);\n"
	"}\n";

static const char *const depthFragmentSource =
	"uniform sampler2D tex;\n"
	"varying vec2 vTexcoord;\n"
	"void main() {\n"
	"	vec4 c = texture2D(tex, vTexcoord);\n"
	"	DEPTH_OUT = (floor(c.r * 255.0 + 0.5) + floor(c.g * 255.0 + 0.5) * 256.0) / 65535.0;\n"
	"	gl_FragColor = vec4(0.0);\n"
	"}\n";

static int log2Int(uint value) {
	int result = 0;

	while (value > 1) {
		value >>= 1;
		result++;
	}

	return result;
}

static int readMask(uint mask, int *pShift, int *pLoss, int *pBits) {
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

	*pShift = shift;
	*pLoss = log2Int(0xFF / (mask >> shift));

	int bits = 0;

	while ((shifted & 1) != 0) {
		shifted >>= 1;
		bits++;
	}

	*pBits = bits;
	return 1;
}

Std3D::Std3D(StdDisplay *pStdDisplay, StdBitmap *pStdBitmap) : _pStdDisplay(pStdDisplay), _pStdBitmap(pStdBitmap), _pDisplayModeTex(nullptr),
	_pRenderTarget(nullptr), _bDepthFromDevice(0), _bAvailable(0), _bStarted(0), _bHardware(1), _bSceneBegun(0), _enumFlags(0), _numDevices(0),
	_pCurDevice(nullptr), _curDeviceIndex(0), _zBufferMode(0), _zBufferBitDepth(0), _numTexFormats(0), _opaqueFormatIndex(0), _alphaFormatIndex(0),
	_colorKeyFormatIndex(0), _pTexFormatOpaque(nullptr), _pTexFormatAlpha(nullptr), _pTexFormatColorKey(nullptr), _pFrameBuffer(nullptr),
	_maxVertices(0), _frameCounter(1), _numCachedTextures(0), _minTexWidth(4), _minTexHeight(4), _maxTexWidth(256), _maxTexHeight(256),
	_pCacheHead(nullptr), _pCacheTail(nullptr), _curRenderState(0), _fogStart(0.0f), _fogEnd(0.0f), _fogRed(0), _fogGreen(0), _fogBlue(0),
	_alphaFunc(0), _bZEnable(0), _bTexMagLinear(0), _bTexMinLinear(0), _numVertices(0), _numTriangles(0), _aVertices(nullptr), _listStart(0),
	_listStartRenderState(0), _pCurTexture(nullptr), _bCurTextureValid(0), _bGlReady(0), _bFragDepthSupported(0), _pTlShader(nullptr),
	_pBlitShader(nullptr), _pDepthShader(nullptr), _vertexVbo(0), _indexVbo(0), _quadVbo(0), _depthTexture(0) {
	memset(_aDevices, 0, sizeof(_aDevices));
	memset(&_zBuffer, 0, sizeof(_zBuffer));
	memset(_aTexFormats, 0, sizeof(_aTexFormats));
	memset(_aPalette, 0, sizeof(_aPalette));
	memset(_aOpaquePaletteTable, 0, sizeof(_aOpaquePaletteTable));
	memset(_aAlphaPaletteTable, 0, sizeof(_aAlphaPaletteTable));
	memset(_aColorKeyPaletteTable, 0, sizeof(_aColorKeyPaletteTable));
	memset(&_activeRect, 0, sizeof(_activeRect));
}

Std3D::~Std3D() {
	delete[] _aVertices;
	_aVertices = nullptr;

	if (_pFrameBuffer)
		_pStdDisplay->vBufferFree(_pFrameBuffer);

	if (_bGlReady && OpenGLContext.type != OpenGL::kContextNone) {
		delete _pTlShader;
		_pTlShader = nullptr;
		delete _pBlitShader;
		_pBlitShader = nullptr;
		delete _pDepthShader;
		_pDepthShader = nullptr;

		OpenGL::Shader::freeBuffer(_vertexVbo);
		OpenGL::Shader::freeBuffer(_indexVbo);
		OpenGL::Shader::freeBuffer(_quadVbo);

		if (_depthTexture)
			glDeleteTextures(1, &_depthTexture);
	}
}

void Std3D::setDisplayMode(const StdVBufferTexFmt *pTex) {
	_pDisplayModeTex = pTex;
}

void Std3D::setRenderTarget(Std3DSurface *pSurface) {
	_pRenderTarget = pSurface;
}

int Std3D::isGlAvailable() {
	return OpenGLContext.type != OpenGL::kContextNone && OpenGLContext.enginesShadersSupported && OpenGLContext.framebufferObjectSupported;
}

OpenGL::Shader *Std3D::createShader(const char *name, const char *vertexSource, const char *fragmentSource, const char *const *attributes, int bDepthOutput) {
	Common::String vertexHeader;
	Common::String fragmentHeader;

	if (OpenGLContext.type == OpenGL::kContextGLES2) {
		vertexHeader = "#version 100\n";
		fragmentHeader = "#version 100\n";

		if (bDepthOutput)
			fragmentHeader += "#extension GL_EXT_frag_depth : enable\n#define DEPTH_OUT gl_FragDepthEXT\n";

		fragmentHeader += "#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\n";
	} else {
		vertexHeader = "#version 120\n";
		fragmentHeader = "#version 120\n";

		if (bDepthOutput)
			fragmentHeader += "#define DEPTH_OUT gl_FragDepth\n";
	}

	const char *vertexParts[2] = { vertexHeader.c_str(), vertexSource };
	const char *fragmentParts[2] = { fragmentHeader.c_str(), fragmentSource };

	OpenGL::Shader *pShader = new OpenGL::Shader();

	if (!pShader->loadFromStringsArray(name, 2, vertexParts, 2, fragmentParts, attributes)) {
		warning("Std3D::createShader(): could not create shader %s", name);
		delete pShader;
		return nullptr;
	}

	return pShader;
}

int Std3D::ensureGl() {
	if (_bGlReady)
		return 1;

	if (!isGlAvailable())
		return 0;

	_pTlShader = createShader("std3d_tl", tlVertexSource, tlFragmentSource, tlAttributes, 0);
	_pBlitShader = createShader("std3d_blit", quadVertexSource, blitFragmentSource, quadAttributes, 0);

	if (!_pTlShader || !_pBlitShader) {
		delete _pTlShader;
		_pTlShader = nullptr;
		delete _pBlitShader;
		_pBlitShader = nullptr;
		return 0;
	}

	_bFragDepthSupported = 1;

	if (OpenGLContext.type == OpenGL::kContextGLES2) {
		const char *pExtensions = (const char *)glGetString(GL_EXTENSIONS);
		_bFragDepthSupported = pExtensions && strstr(pExtensions, "GL_EXT_frag_depth") != nullptr;
	}

	if (_bFragDepthSupported)
		_pDepthShader = createShader("std3d_depth", quadVertexSource, depthFragmentSource, quadAttributes, 1);

	_vertexVbo = OpenGL::Shader::createBuffer(GL_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);
	_indexVbo = OpenGL::Shader::createBuffer(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);
	_quadVbo = OpenGL::Shader::createBuffer(GL_ARRAY_BUFFER, 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

	_pTlShader->enableVertexAttribute("position", _vertexVbo, 4, GL_FLOAT, GL_FALSE, sizeof(Std3DGlVertex), 0);
	_pTlShader->enableVertexAttribute("color", _vertexVbo, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Std3DGlVertex), 16);
	_pTlShader->enableVertexAttribute("texcoord", _vertexVbo, 2, GL_FLOAT, GL_FALSE, sizeof(Std3DGlVertex), 20);

	_pBlitShader->enableVertexAttribute("position", _quadVbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	_pBlitShader->enableVertexAttribute("texcoord", _quadVbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float));

	if (_pDepthShader) {
		_pDepthShader->enableVertexAttribute("position", _quadVbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		_pDepthShader->enableVertexAttribute("texcoord", _quadVbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float));
	}

	_bGlReady = 1;
	return 1;
}

int Std3D::enumerateDevices() {
	if (!isGlAvailable())
		return 0;

	_enumFlags = kStd3DRsPerspective | kStd3DRsDither | kStd3DRsSubpixel | kStd3DRsSubpixelX | kStd3DRsTexMagLinear | kStd3DRsTexMinLinear | kStd3DRsZTest | kStd3DRsZWrite;
	_bHardware = 1;
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

	enumDevicesCallback(syntheticDeviceGuid, "ScummVM OpenGL", "OpenGL", &hwDesc, &helDesc);

	if (_numDevices) {
		_bAvailable = 1;
		return 1;
	}

	return 0;
}

void Std3D::releaseDirect3D() {
	_bAvailable = 0;
}

int Std3D::isAvailable() {
	return _bAvailable;
}

int Std3D::startup(uint deviceIndex, int bHardware) {
	if (_bStarted)
		return 0;

	if (deviceIndex >= _numDevices)
		return 0;

	if (!_pDisplayModeTex) {
		warning("Std3D::startup(): no display mode");
		return 0;
	}

	_curDeviceIndex = deviceIndex;
	_pCurDevice = &_aDevices[deviceIndex];

	if (bHardware && _pCurDevice->bZBuffer && ((_enumFlags >> 8) & 0x18) != 0) {
		_bHardware = 1;

		if (!createDevice(_pDisplayModeTex->width, _pDisplayModeTex->height))
			return 0;

		_zBufferMode = (_pCurDevice->zCmpMask & 0x10) != 0 ? kStd3DZBufferGreater : kStd3DZBufferLess;
	} else {
		_bHardware = 0;
	}

	if (!_pRenderTarget) {
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

	if (!createViewport(_pDisplayModeTex->width, _pDisplayModeTex->height))
		return 0;

	if (!createExecuteBuffer())
		return 0;

	uint maxVertexCount = _pCurDevice->maxVertexCount;

	if (!maxVertexCount || maxVertexCount >= 0x200)
		_maxVertices = kStd3DDefaultMaxVertices;
	else
		_maxVertices = maxVertexCount;

	delete[] _aVertices;
	_aVertices = new Std3DVertex[_maxVertices];
	memset(_aVertices, 0, sizeof(Std3DVertex) * _maxVertices);

	_frameCounter = 1;
	_numCachedTextures = 0;

	setDimensionLimits(_pCurDevice->minTexWidth, _pCurDevice->minTexHeight, _pCurDevice->maxTexWidth, _pCurDevice->maxTexHeight);

	_pCacheHead = nullptr;
	_pCacheTail = nullptr;

	_opaqueFormatIndex = findClosestMode(&_pDisplayModeTex->format, _aTexFormats, _numTexFormats);
	_pTexFormatOpaque = &_aTexFormats[_opaqueFormatIndex];

	if (_pCurDevice->bAlphaTexture) {
		StdColorFormat wantedFormat;

		memset(&wantedFormat, 0, sizeof(wantedFormat));
		wantedFormat.colorMode = 2;
		wantedFormat.bpp = 16;
		wantedFormat.redBits = 5;
		wantedFormat.greenBits = 5;
		wantedFormat.blueBits = 5;
		wantedFormat.alphaBits = 1;

		_alphaFormatIndex = findClosestMode(&wantedFormat, _aTexFormats, _numTexFormats);
		_pTexFormatAlpha = &_aTexFormats[_alphaFormatIndex];

		if (!_pCurDevice->bAlphaBlend) {
			wantedFormat.colorMode = 2;
			wantedFormat.bpp = 16;
			wantedFormat.redBits = 4;
			wantedFormat.greenBits = 4;
			wantedFormat.blueBits = 4;
			wantedFormat.alphaBits = 4;

			_colorKeyFormatIndex = findClosestMode(&wantedFormat, _aTexFormats, _numTexFormats);

			StdVBufferTexFmt frameFormat;

			memset(&frameFormat, 0, sizeof(frameFormat));
			frameFormat.width = 32;
			frameFormat.height = 32;

			_pTexFormatColorKey = &_aTexFormats[_colorKeyFormatIndex];
			frameFormat.format = _pTexFormatColorKey->format;
			_pFrameBuffer = _pStdDisplay->vBufferNew(&frameFormat, 1, 0, nullptr);
		}
	}

	getTextureMemory(&_pCurDevice->texMemTotal, &_pCurDevice->texMemFree);

	_bStarted = 1;
	return 1;
}

void Std3D::shutdown() {
	if (!_bStarted)
		return;

	if (_pFrameBuffer)
		_pStdDisplay->vBufferFree(_pFrameBuffer);

	delete[] _aVertices;
	_aVertices = nullptr;

	flushTextureCache();

	if (_bDepthFromDevice && _pRenderTarget) {
		detachDepth(_pRenderTarget);
		_bDepthFromDevice = 0;
	}

	_pCurDevice = nullptr;
	_bStarted = 0;
}

void Std3D::setDimensionLimits(uint minWidth, uint minHeight, uint maxWidth, uint maxHeight) {
	_minTexWidth = minWidth;
	_minTexHeight = minHeight;
	_maxTexWidth = maxWidth;
	_maxTexHeight = maxHeight;
}

int Std3D::beginScene() {
	int result = _bSceneBegun;

	if (!_bSceneBegun) {
		result = 0;
		_bSceneBegun = 1;
	}

	return result;
}

int Std3D::endScene() {
	int result = _bSceneBegun;

	if (_bSceneBegun) {
		result = 0;
		_bSceneBegun = 0;
	}

	return result;
}

int Std3D::resetRenderList() {
	_numVertices = 0;
	_numTriangles = 0;
	_frameCounter++;
	_pCurTexture = nullptr;
	_bCurTextureValid = 0;

	_commands.clear();
	_indices.clear();
	_listStart = 0;

	if (!_aVertices) {
		warning("Std3D::resetRenderList(): device not started");
		return 0;
	}

	return 1;
}

int Std3D::addRenderListVertices(const Std3DVertex *pVertices, uint count) {
	if (count > _maxVertices || count + _numVertices > _maxVertices)
		return 0;

	if (!_aVertices) {
		warning("Std3D::addRenderListVertices(): device not started");
		return 0;
	}

	if (&_aVertices[_numVertices] != pVertices)
		memcpy(&_aVertices[_numVertices], pVertices, sizeof(Std3DVertex) * count);

	_numVertices += count;
	return 1;
}

int Std3D::beginRenderList() {
	_listStart = _commands.size();
	_listStartRenderState = _curRenderState;
	return 1;
}

int Std3D::addRenderListTris(const Std3DTri *pTris, uint count) {
	if (!pTris && count) {
		warning("Std3D::addRenderListTris(): no triangles");
		return 0;
	}

	uint numDone = 0;

	while (numDone < count) {
		const Std3DTri *pTri = &pTris[numDone];
		RdTexCacheEntry *pTexture = pTri->pTexture;
		uint flags = pTri->flags;
		uint runLength = 0;
		uint scanIndex = numDone;
		const Std3DTri *pScan = pTri;

		while (scanIndex < count && pScan->pTexture == pTexture && pScan->flags == flags) {
			runLength++;
			scanIndex++;
			pScan++;
		}

		setMaterialRenderState(pTri->flags);

		if (!_bCurTextureValid || _pCurTexture != pTexture) {
			Std3DCommand command;

			memset(&command, 0, sizeof(command));
			command.type = kStd3DCommandTexture;

			if (pTexture) {
				command.texture = pTexture->hTexture;
				command.bTextureAlpha = pTexture->bColorKey;
			}

			_commands.push_back(command);
			_pCurTexture = pTexture;
			_bCurTextureValid = 1;
		}

		Std3DCommand triangles;

		memset(&triangles, 0, sizeof(triangles));
		triangles.type = kStd3DCommandTriangles;
		triangles.firstIndex = _indices.size();
		triangles.numIndices = runLength * 3;

		for (uint i = 0; i < runLength; i++) {
			_indices.push_back((uint16)pTri[i].aIdx[0]);
			_indices.push_back((uint16)pTri[i].aIdx[1]);
			_indices.push_back((uint16)pTri[i].aIdx[2]);
		}

		_commands.push_back(triangles);
		numDone += runLength;
	}

	_numTriangles += count;
	return 1;
}

uint Std3D::glCompareFunc(uint d3dFunc) {
	switch (d3dFunc) {
	case 1:
		return GL_NEVER;
	case 2:
		return GL_LESS;
	case 3:
		return GL_EQUAL;
	case 4:
		return GL_LEQUAL;
	case 5:
		return GL_GREATER;
	case 6:
		return GL_NOTEQUAL;
	case 7:
		return GL_GEQUAL;
	case 8:
		return GL_ALWAYS;
	default:
		warning("Std3D::glCompareFunc(): invalid compare function %u", d3dFunc);
		return GL_ALWAYS;
	}
}

void Std3D::applyRenderState(uint flags) {
	if (flags & (kStd3DRsAlphaBlend | kStd3DRsAlphaTexture)) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glDisable(GL_BLEND);
	}

	_pTlShader->setUniform("modulateAlpha", (flags & kStd3DRsAlphaTexture) != 0 ? 1u : 0u);

	if (_bZEnable) {
		glEnable(GL_DEPTH_TEST);

		if (flags & kStd3DRsZTest)
			glDepthFunc(glCompareFunc(zCmpMaskToFunc(_zBufferMode)));
		else
			glDepthFunc(GL_ALWAYS);

		glDepthMask((flags & kStd3DRsZWrite) != 0 ? GL_TRUE : GL_FALSE);
	} else {
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}

	_pTlShader->setUniform("fogEnabled", (flags & kStd3DRsFog) != 0 ? 1u : 0u);
}

int Std3D::drawRenderList() {
	if (!_pRenderTarget || !_aVertices) {
		warning("Std3D::drawRenderList(): device not started");
		return 0;
	}

	if (!ensureGl())
		return 0;

	Std3DGlState savedState;

	saveGlState(&savedState);
	bindSurface(_pRenderTarget);
	glDisable(GL_DITHER);

	Common::Array<Std3DGlVertex> glVertices;

	for (uint i = 0; i < _numVertices; i++) {
		const Std3DVertex &vertex = _aVertices[i];
		Std3DGlVertex glVertex;

		glVertex.x = vertex.sx;
		glVertex.y = vertex.sy;
		glVertex.z = vertex.sz;
		glVertex.w = vertex.rhw;
		glVertex.r = (vertex.color >> 16) & 0xFF;
		glVertex.g = (vertex.color >> 8) & 0xFF;
		glVertex.b = vertex.color & 0xFF;
		glVertex.a = (vertex.color >> 24) & 0xFF;
		glVertex.u = vertex.tu;
		glVertex.v = vertex.tv;
		glVertices.push_back(glVertex);
	}

	glBindBuffer(GL_ARRAY_BUFFER, _vertexVbo);
	glBufferData(GL_ARRAY_BUFFER, glVertices.size() * sizeof(Std3DGlVertex), glVertices.empty() ? nullptr : &glVertices[0], GL_STREAM_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexVbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(uint16), _indices.empty() ? nullptr : &_indices[0], GL_STREAM_DRAW);

	_pTlShader->use(true);
	_pTlShader->setUniform("targetSize", Math::Vector2d((float)_pRenderTarget->width, (float)_pRenderTarget->height));
	_pTlShader->setUniform("tex", 0u);
	_pTlShader->setUniform("textured", 0u);
	_pTlShader->setUniform("texHasAlpha", 0u);
	_pTlShader->setUniform("alphaFunc", _alphaFunc);
	_pTlShader->setUniform("fogColor", Math::Vector3d(_fogRed / 255.0f, _fogGreen / 255.0f, _fogBlue / 255.0f));
	_pTlShader->setUniform("fogRange", Math::Vector2d(_fogStart, _fogEnd));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	applyRenderState(_listStartRenderState);

	for (uint i = _listStart; i < _commands.size(); i++) {
		const Std3DCommand &command = _commands[i];

		switch (command.type) {
		case kStd3DCommandRenderState:
			applyRenderState(command.flags);
			break;
		case kStd3DCommandTexture:
			glBindTexture(GL_TEXTURE_2D, command.texture);
			_pTlShader->setUniform("textured", command.texture != 0 ? 1u : 0u);			_pTlShader->setUniform("texHasAlpha", command.bTextureAlpha != 0 ? 1u : 0u);
			break;
		case kStd3DCommandTriangles:
			glDrawElements(GL_TRIANGLES, command.numIndices, GL_UNSIGNED_SHORT, (const void *)(uintptr)(command.firstIndex * sizeof(uint16)));
			break;
		default:
			break;
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	_pTlShader->unbind();
	restoreGlState(&savedState);
	return 1;
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
	if (!_pRenderTarget || !_pRenderTarget->depthRenderbuffer) {
		warning("Std3D::clearZBuffer(): no z buffer");
		return 0;
	}

	if (!ensureGl())
		return 0;

	int left = _activeRect.x;
	int top = _activeRect.y;
	int right = _activeRect.x + _activeRect.width;
	int bottom = _activeRect.y + _activeRect.height;

	if (left < 0)
		left = 0;

	if (top < 0)
		top = 0;

	if (right > _pRenderTarget->width)
		right = _pRenderTarget->width;

	if (bottom > _pRenderTarget->height)
		bottom = _pRenderTarget->height;

	if (right <= left || bottom <= top)
		return 1;

	Std3DGlState savedState;
	float depthValue = _zBufferMode != kStd3DZBufferGreater ? 1.0f : 0.0f;

	saveGlState(&savedState);
	bindSurface(_pRenderTarget, 0);
	glEnable(GL_SCISSOR_TEST);
	glScissor(left, top, right - left, bottom - top);
	glDepthMask(GL_TRUE);

#if USE_FORCED_GLES2
	glClearDepthf(depthValue);
#elif defined(USE_GLAD)
	if (OpenGLContext.type == OpenGL::kContextGLES2)
		glClearDepthf(depthValue);
	else
		glClearDepth(depthValue);
#else
	glClearDepth(depthValue);
#endif

	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
	restoreGlState(&savedState);
	return 1;
}

uint Std3D::findClosestDevice(const Std3DDevice *pWanted, const Std3DDevice *aDevices, uint numDevices) {
	uint index = 0;

	if (!numDevices)
		return index;

	int bestScore = 0;
	uint bestIndex = 0;
	int bPerspectiveCorrect = pWanted->bPerspectiveCorrect;

	do {
		const Std3DDevice *pCandidate = &aDevices[index];
		int score = 0;

		if (!bPerspectiveCorrect || pCandidate->bPerspectiveCorrect == bPerspectiveCorrect) {
			int wantedZBuffer = pWanted->bZBuffer;

			score = 1;

			if (!wantedZBuffer || pCandidate->bZBuffer == wantedZBuffer) {
				score = 2;

				if ((pWanted->colorModelFlags & pCandidate->colorModelFlags) != 0) {
					score = 3;

					if (pCandidate->bHardware == pWanted->bHardware)
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

uint Std3D::findClosestMode(const StdColorFormat *pWanted, const Std3DTexFormat *aFormats, uint numFormats) {
	uint index = 0;

	if (!numFormats)
		return index;

	int bestScore = 0;
	uint bestIndex = 0;
	int colorMode = pWanted->colorMode;

	do {
		const StdColorFormat *pCandidate = &aFormats[index].format;
		int score = 0;

		if (pCandidate->colorMode == colorMode) {
			score = 1;

			if (pCandidate->bpp == pWanted->bpp) {
				score = 2;

				if (colorMode == 1) {
					if (pCandidate->redBits == pWanted->redBits && pCandidate->greenBits == pWanted->greenBits && pCandidate->blueBits == pWanted->blueBits)
						return index;
				} else {
					if (colorMode != 2)
						return index;

					if (pCandidate->colorMode == 2)
						score = 3;

					if (pCandidate->redBits == pWanted->redBits && pCandidate->greenBits == pWanted->greenBits &&
						pCandidate->blueBits == pWanted->blueBits && pCandidate->alphaBits == pWanted->alphaBits)
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

int Std3D::setActiveRect(const StdRect *pRect) {
	_activeRect = *pRect;
	return 0;
}

int Std3D::createExecuteBuffer() {
	_bTexMagLinear = (_enumFlags & kStd3DRsTexMagLinear) != 0;
	_bTexMinLinear = (_enumFlags & kStd3DRsTexMinLinear) != 0;
	_alphaFunc = zCmpMaskToFunc(_zBufferMode);
	_bZEnable = _bHardware != 0;
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
	if (!_pDisplayModeTex || !_pRenderTarget) {
		warning("Std3D::createDevice(): no display mode or render target");
		return 0;
	}

	_zBuffer.type = 1;
	_zBuffer.bVideoMemory = 0;
	_zBuffer.tex = *_pDisplayModeTex;
	_zBuffer.pPaletteRGB = nullptr;
	_zBuffer.pPixels = nullptr;

	if ((_pCurDevice->desc.deviceZBufferBitDepth & kBitDepth32) != 0)
		_zBufferBitDepth = 32;
	else if ((_pCurDevice->desc.deviceZBufferBitDepth & kBitDepth16) != 0)
		_zBufferBitDepth = 16;
	else if ((_pCurDevice->desc.deviceZBufferBitDepth & kBitDepth8) != 0)
		_zBufferBitDepth = 8;
	else
		return 0;

	if (_pRenderTarget->width != width || _pRenderTarget->height != height) {
		warning("Std3D::createDevice(): render target size mismatch");
		return 0;
	}

	if (!_pRenderTarget->depthRenderbuffer) {
		if (!attachDepth(_pRenderTarget))
			return 0;

		_bDepthFromDevice = 1;
	}

	if (_pCurDevice->bHardware)
		_zBuffer.bVideoMemory = 1;

	return 1;
}

int Std3D::enumDevicesCallback(const byte *pGuid, const char *pDescription, const char *pName, const Std3DDeviceDesc *pHWDesc, const Std3DDeviceDesc *pHELDesc) {
	if (_numDevices >= kStd3DMaxDevices)
		return 0;

	Std3DDevice *pDevice = &_aDevices[_numDevices];

	memcpy(pDevice->guid, pGuid, 16);
	Common::strlcpy(pDevice->aDescription, pDescription, sizeof(pDevice->aDescription));
	Common::strlcpy(pDevice->aName, pName, sizeof(pDevice->aName));

	if (pHWDesc->colorModel) {
		pDevice->bHardware = 1;
		pDevice->desc = *pHWDesc;
	} else {
		pDevice->bHardware = 0;
		pDevice->desc = *pHELDesc;
	}

	int colorModel = pDevice->desc.colorModel;

	pDevice->colorModelFlags = 0;

	if ((colorModel & kColorModelRgb) != 0)
		pDevice->colorModelFlags = 2;

	if ((colorModel & kColorModelMono) != 0)
		pDevice->colorModelFlags |= 1;

	uint textureCaps = pDevice->desc.textureCaps;
	uint shadeCaps = pDevice->desc.shadeCaps;

	pDevice->bPerspectiveCorrect = (textureCaps & kTexCapsPerspective) != 0;
	pDevice->bZBuffer = pDevice->desc.deviceZBufferBitDepth != 0;
	pDevice->bAlphaTexture = (textureCaps & kTexCapsAlpha) != 0;
	pDevice->bSquareTexturesOnly = (textureCaps & kTexCapsSquareOnly) != 0;
	pDevice->bAlphaStipple = (shadeCaps & kShadeCapsAlphaFlatBlend) == 0 && (shadeCaps & kShadeCapsAlphaFlatStippled) != 0;
	pDevice->bAlphaBlend = ((pDevice->desc.textureBlendCaps & kTexBlendCapsModulateAlpha) != 0 && (shadeCaps & kShadeCapsAlphaGouraudBlend) != 0) || pDevice->bAlphaStipple;
	pDevice->bTransparency = (textureCaps & kTexCapsTransparency) != 0;
	pDevice->renderBitDepthMask = convertBitDepthCaps(pDevice->desc.deviceRenderBitDepth);
	pDevice->zCmpMask = convertZCmpCaps(pDevice->desc.zCmpCaps);

	uint minTextureWidth = pDevice->desc.minTextureWidth;

	if (minTextureWidth <= 4)
		minTextureWidth = 4;

	pDevice->minTexWidth = minTextureWidth;

	uint minTextureHeight = pDevice->desc.minTextureHeight;

	if (minTextureHeight <= 4)
		minTextureHeight = 4;

	pDevice->minTexHeight = minTextureHeight;

	uint maxTextureWidth = pDevice->desc.maxTextureWidth;

	if (!maxTextureWidth)
		maxTextureWidth = 256;

	pDevice->maxTexWidth = maxTextureWidth;

	uint maxTextureHeight = pDevice->desc.maxTextureHeight;

	if (!maxTextureHeight)
		maxTextureHeight = 256;

	pDevice->maxTexHeight = maxTextureHeight;
	pDevice->maxBufferSize = pDevice->desc.maxBufferSize;
	pDevice->maxVertexCount = pDevice->desc.maxVertexCount;

	_numDevices++;
	return 1;
}

int Std3D::enumTextureFormatsCallback(const Std3DPixelFormat *pPixelFormat) {
	if (_numTexFormats >= kStd3DMaxTexFormats)
		return 0;

	Std3DTexFormat *pFormat = &_aTexFormats[_numTexFormats];
	StdColorFormat *pColor = &pFormat->format;

	pFormat->pixelFormat = *pPixelFormat;

	if ((pPixelFormat->flags & kDdpfPaletteIndexed8) != 0) {
		pColor->bpp = 8;
		pColor->colorMode = 0;
		pColor->redShift = 0;
		pColor->redLoss = 0;
		pColor->redBits = 0;
		pColor->greenShift = 0;
		pColor->greenLoss = 0;
		pColor->greenBits = 0;
		pColor->blueShift = 0;
		pColor->blueLoss = 0;
		pColor->blueBits = 0;
	} else {
		if ((pPixelFormat->flags & kDdpfPaletteIndexed4) != 0)
			return 1;

		pColor->colorMode = (pPixelFormat->flags & kDdpfAlphaPixels) != 0 ? 2 : 1;
		pColor->bpp = pPixelFormat->rgbBitCount;

		if (!readMask(pPixelFormat->rBitMask, &pColor->redShift, &pColor->redLoss, &pColor->redBits) ||
			!readMask(pPixelFormat->gBitMask, &pColor->greenShift, &pColor->greenLoss, &pColor->greenBits) ||
			!readMask(pPixelFormat->bBitMask, &pColor->blueShift, &pColor->blueLoss, &pColor->blueBits))
			return 1;

		if (pColor->colorMode == 2 && !readMask(pPixelFormat->rgbAlphaBitMask, &pColor->alphaShift, &pColor->alphaLoss, &pColor->alphaBits))
			return 1;
	}

	if (pColor->colorMode != 2) {
		pColor->alphaShift = 0;
		pColor->alphaLoss = 0;
		pColor->alphaBits = 0;
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
