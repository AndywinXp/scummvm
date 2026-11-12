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

#if defined(GRIM_STD3D_GL)

#include "common/endian.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/util.h"

#include "graphics/opengl/context.h"
#include "graphics/opengl/shader.h"

#include "engines/grim/std/std_3d_gl.h"

namespace Grim {

struct Std3DGlSurface {
	uint colorTexture;
	uint framebuffer;
	uint depthRenderbuffer;
};

static const byte syntheticDeviceGuid[16] = {
	0x53, 0x63, 0x75, 0x6D, 0x6D, 0x56, 0x4D, 0x20, 0x4F, 0x70, 0x65, 0x6E, 0x47, 0x4C, 0x00, 0x01
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

static byte expand5(uint value) {
	return (byte)((value << 3) | (value >> 2));
}

static byte expand6(uint value) {
	return (byte)((value << 2) | (value >> 4));
}

static Std3DGlSurface *glData(const Std3DSurface *surface) {
	return (Std3DGlSurface *)surface->backendData;
}

Std3DGlBackend::Std3DGlBackend() : _glReady(0), _fragDepthSupported(0), _tlShader(nullptr), _blitShader(nullptr), _depthShader(nullptr),
	_vertexVbo(0), _indexVbo(0), _quadVbo(0), _depthTexture(0) {
}

Std3DGlBackend::~Std3DGlBackend() {
	if (_glReady && OpenGLContext.type != OpenGL::kContextNone) {
		delete _tlShader;
		_tlShader = nullptr;
		delete _blitShader;
		_blitShader = nullptr;
		delete _depthShader;
		_depthShader = nullptr;

		OpenGL::Shader::freeBuffer(_vertexVbo);
		OpenGL::Shader::freeBuffer(_indexVbo);
		OpenGL::Shader::freeBuffer(_quadVbo);

		if (_depthTexture)
			glDeleteTextures(1, &_depthTexture);
	}
}

int Std3DGlBackend::openGraphicsMode(int width, int height) {
	initGraphics3d(width, height);

	if (!isSupported()) {
		warning("Std3DGlBackend::openGraphicsMode(): the OpenGL context with shaders is not available");
		return 0;
	}

	return 1;
}

void Std3DGlBackend::fillDeviceDesc(Std3DDeviceDesc *desc) {
	(void)desc;
}

int Std3DGlBackend::isSupported() {
	return OpenGLContext.type != OpenGL::kContextNone && OpenGLContext.enginesShadersSupported && OpenGLContext.framebufferObjectSupported;
}

int Std3DGlBackend::canProbeWithoutMode() {
	return 0;
}

const char *Std3DGlBackend::getName() {
	return "OpenGL";
}

const char *Std3DGlBackend::getDescription() {
	return "ScummVM OpenGL";
}

const byte *Std3DGlBackend::getGuid() {
	return syntheticDeviceGuid;
}

int Std3DGlBackend::getMaxTextureSize() {
	return OpenGLContext.maxTextureSize;
}

uint Std3DGlBackend::getTextureBudget() {
	return kStd3DTextureMemory;
}

OpenGL::Shader *Std3DGlBackend::createShader(const char *name, const char *vertexSource, const char *fragmentSource, const char *const *attributes, int depthOutput) {
	Common::String vertexHeader;
	Common::String fragmentHeader;

	if (OpenGLContext.type == OpenGL::kContextGLES2) {
		vertexHeader = "#version 100\n";
		fragmentHeader = "#version 100\n";

		if (depthOutput)
			fragmentHeader += "#extension GL_EXT_frag_depth : enable\n#define DEPTH_OUT gl_FragDepthEXT\n";

		fragmentHeader += "#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\n";
	} else {
		vertexHeader = "#version 120\n";
		fragmentHeader = "#version 120\n";

		if (depthOutput)
			fragmentHeader += "#define DEPTH_OUT gl_FragDepth\n";
	}

	const char *vertexParts[2] = { vertexHeader.c_str(), vertexSource };
	const char *fragmentParts[2] = { fragmentHeader.c_str(), fragmentSource };

	OpenGL::Shader *shader = new OpenGL::Shader();

	if (!shader->loadFromStringsArray(name, 2, vertexParts, 2, fragmentParts, attributes)) {
		warning("Std3DGlBackend::createShader(): could not create shader %s", name);
		delete shader;
		return nullptr;
	}

	return shader;
}

int Std3DGlBackend::ensureReady() {
	if (_glReady)
		return 1;

	if (!isSupported())
		return 0;

	_tlShader = createShader("std3d_tl", tlVertexSource, tlFragmentSource, tlAttributes, 0);
	_blitShader = createShader("std3d_blit", quadVertexSource, blitFragmentSource, quadAttributes, 0);

	if (!_tlShader || !_blitShader) {
		delete _tlShader;
		_tlShader = nullptr;
		delete _blitShader;
		_blitShader = nullptr;
		return 0;
	}

	_fragDepthSupported = 1;

	if (OpenGLContext.type == OpenGL::kContextGLES2) {
		const char *extensions = (const char *)glGetString(GL_EXTENSIONS);
		_fragDepthSupported = extensions && strstr(extensions, "GL_EXT_frag_depth") != nullptr;
	}

	if (_fragDepthSupported)
		_depthShader = createShader("std3d_depth", quadVertexSource, depthFragmentSource, quadAttributes, 1);

	_vertexVbo = OpenGL::Shader::createBuffer(GL_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);
	_indexVbo = OpenGL::Shader::createBuffer(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);
	_quadVbo = OpenGL::Shader::createBuffer(GL_ARRAY_BUFFER, 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

	_tlShader->enableVertexAttribute("position", _vertexVbo, 4, GL_FLOAT, GL_FALSE, sizeof(Std3DGlVertex), 0);
	_tlShader->enableVertexAttribute("color", _vertexVbo, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Std3DGlVertex), 16);
	_tlShader->enableVertexAttribute("texcoord", _vertexVbo, 2, GL_FLOAT, GL_FALSE, sizeof(Std3DGlVertex), 20);

	_blitShader->enableVertexAttribute("position", _quadVbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	_blitShader->enableVertexAttribute("texcoord", _quadVbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float));

	if (_depthShader) {
		_depthShader->enableVertexAttribute("position", _quadVbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		_depthShader->enableVertexAttribute("texcoord", _quadVbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float));
	}

	_glReady = 1;
	return 1;
}

void Std3DGlBackend::saveGlState(Std3DGlState *state) {
	GLint framebuffer = 0;
	GLint viewport[4];

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);
	glGetIntegerv(GL_VIEWPORT, viewport);

	state->framebuffer = framebuffer;

	for (int i = 0; i < 4; i++)
		state->viewport[i] = viewport[i];
}

void Std3DGlBackend::restoreGlState(const Std3DGlState *state) {
	glBindFramebuffer(GL_FRAMEBUFFER, state->framebuffer);
	glViewport(state->viewport[0], state->viewport[1], state->viewport[2], state->viewport[3]);
}

void Std3DGlBackend::bindSurface(Std3DSurface *surface) {
	glBindFramebuffer(GL_FRAMEBUFFER, glData(surface)->framebuffer);
	glViewport(0, 0, surface->width, surface->height);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

int Std3DGlBackend::attachDepth(Std3DSurface *surface, int zBufferBitDepth) {
	Std3DGlState savedState;
	GLuint renderbuffer = 0;

	saveGlState(&savedState);
	glBindFramebuffer(GL_FRAMEBUFFER, glData(surface)->framebuffer);
	glGenRenderbuffers(1, &renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);

#if defined(GL_DEPTH_COMPONENT24)
	if (zBufferBitDepth == 32) {
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, surface->width, surface->height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, surface->width, surface->height);
	} else {
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, surface->width, surface->height);
	}
#else
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, surface->width, surface->height);
#endif

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		warning("Std3DGlBackend::attachDepth(): incomplete framebuffer");
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
		glDeleteRenderbuffers(1, &renderbuffer);
		restoreGlState(&savedState);
		return 0;
	}

	glData(surface)->depthRenderbuffer = renderbuffer;
	restoreGlState(&savedState);
	return 1;
}

void Std3DGlBackend::detachDepth(Std3DSurface *surface) {
	Std3DGlSurface *data = glData(surface);

	if (!data->depthRenderbuffer)
		return;

	Std3DGlState savedState;
	GLuint renderbuffer = data->depthRenderbuffer;

	saveGlState(&savedState);
	glBindFramebuffer(GL_FRAMEBUFFER, data->framebuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glDeleteRenderbuffers(1, &renderbuffer);
	data->depthRenderbuffer = 0;
	restoreGlState(&savedState);
}

int Std3DGlBackend::hasDepth(const Std3DSurface *surface) {
	Std3DGlSurface *data = glData(surface);

	return data && data->depthRenderbuffer != 0;
}

int Std3DGlBackend::supportsDepthUpload(const Std3DSurface *surface) {
	return hasDepth(surface) && _depthShader;
}

void Std3DGlBackend::drawQuad(const float *quad) {
	glBindBuffer(GL_ARRAY_BUFFER, _quadVbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 16 * sizeof(float), quad);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

int Std3DGlBackend::createSurface(Std3DSurface *surface, int hasDepth, int zBufferBitDepth) {
	Std3DGlSurface *data = new Std3DGlSurface;
	Std3DGlState savedState;
	GLuint texture = 0;
	GLuint framebuffer = 0;
	int width = surface->width;
	int height = surface->height;

	memset(data, 0, sizeof(Std3DGlSurface));
	surface->backendData = data;

	saveGlState(&savedState);

	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
	data->colorTexture = texture;

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	data->framebuffer = framebuffer;

	int complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

	if (!complete)
		warning("Std3DGlBackend::createSurface(): incomplete framebuffer");

	if (complete && hasDepth && !attachDepth(surface, zBufferBitDepth))
		complete = 0;

	if (!complete) {
		glDeleteFramebuffers(1, &framebuffer);
		glDeleteTextures(1, &texture);
		restoreGlState(&savedState);
		delete data;
		surface->backendData = nullptr;
		return 0;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, width, height);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	restoreGlState(&savedState);
	return 1;
}

void Std3DGlBackend::destroySurface(Std3DSurface *surface) {
	Std3DGlSurface *data = glData(surface);

	if (!data)
		return;

	if (_glReady && OpenGLContext.type != OpenGL::kContextNone) {
		detachDepth(surface);

		GLuint framebuffer = data->framebuffer;
		GLuint texture = data->colorTexture;

		glDeleteFramebuffers(1, &framebuffer);
		glDeleteTextures(1, &texture);
	}

	delete data;
	surface->backendData = nullptr;
}

int Std3DGlBackend::uploadPixels(Std3DSurface *surface, const StdRect *rectPointer, const byte *pixels, int pitchInBytes) {
	const StdRect &rect = *rectPointer;
	byte *rgba = new byte[rect.width * rect.height * 4];

	for (int y = 0; y < rect.height; y++) {
		const byte *row = pixels + y * pitchInBytes;
		byte *out = &rgba[y * rect.width * 4];

		for (int x = 0; x < rect.width; x++) {
			uint value = READ_LE_UINT16(row + 2 * x);

			out[0] = expand5((value >> 11) & 0x1F);
			out[1] = expand6((value >> 5) & 0x3F);
			out[2] = expand5(value & 0x1F);
			out[3] = 255;
			out += 4;
		}
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glData(surface)->colorTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x, rect.y, rect.width, rect.height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] rgba;
	return 1;
}

int Std3DGlBackend::readPixels(Std3DSurface *surface, const StdRect *rectPointer, byte *out, int pitchInBytes) {
	const StdRect &rect = *rectPointer;
	byte *rgba = new byte[rect.width * rect.height * 4];
	Std3DGlState savedState;

	saveGlState(&savedState);
	glBindFramebuffer(GL_FRAMEBUFFER, glData(surface)->framebuffer);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadPixels(rect.x, rect.y, rect.width, rect.height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	restoreGlState(&savedState);

	for (int y = 0; y < rect.height; y++) {
		const byte *in = &rgba[y * rect.width * 4];
		byte *row = out + y * pitchInBytes;

		for (int x = 0; x < rect.width; x++) {
			uint value = ((in[0] >> 3) << 11) | ((in[1] >> 2) << 5) | (in[2] >> 3);

			WRITE_LE_UINT16(row + 2 * x, value);
			in += 4;
		}
	}

	delete[] rgba;
	return 1;
}

int Std3DGlBackend::fillRect(Std3DSurface *surface, const StdRect *rectPointer, uint16 color) {
	const StdRect &rect = *rectPointer;
	Std3DGlState savedState;

	saveGlState(&savedState);
	bindSurface(surface);
	glEnable(GL_SCISSOR_TEST);
	glScissor(rect.x, rect.y, rect.width, rect.height);
	glClearColor(expand5((color >> 11) & 0x1F) / 255.0f, expand6((color >> 5) & 0x3F) / 255.0f, expand5(color & 0x1F) / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
	restoreGlState(&savedState);
	return 1;
}

int Std3DGlBackend::canBlitOntoSelf() {
	return 0;
}

int Std3DGlBackend::blit(Std3DSurface *dst, Std3DSurface *src, int srcX, int srcY, int width, int height, int dstX, int dstY, int colorKeyFlag, uint16 colorKey) {
	Std3DGlState savedState;

	saveGlState(&savedState);
	bindSurface(dst);

	float left = (float)dstX;
	float top = (float)dstY;
	float right = (float)(dstX + width);
	float bottom = (float)(dstY + height);
	float s0 = (float)srcX / (float)src->width;
	float t0 = (float)srcY / (float)src->height;
	float s1 = (float)(srcX + width) / (float)src->width;
	float t1 = (float)(srcY + height) / (float)src->height;
	float quad[16] = {
		left, top, s0, t0,
		right, top, s1, t0,
		left, bottom, s0, t1,
		right, bottom, s1, t1
	};

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glData(src)->colorTexture);

	_blitShader->use(true);
	_blitShader->setUniform("targetSize", Math::Vector2d((float)dst->width, (float)dst->height));
	_blitShader->setUniform1f("flipY", 1.0f);
	_blitShader->setUniform("tex", 0u);
	_blitShader->setUniform("useKey", colorKeyFlag != 0 ? 1u : 0u);
	_blitShader->setUniform("keyColor", Math::Vector3d(expand5((colorKey >> 11) & 0x1F) / 255.0f, expand6((colorKey >> 5) & 0x3F) / 255.0f, expand5(colorKey & 0x1F) / 255.0f));

	drawQuad(quad);

	glBindTexture(GL_TEXTURE_2D, 0);
	_blitShader->unbind();
	restoreGlState(&savedState);
	return 1;
}

int Std3DGlBackend::clearDepth(Std3DSurface *surface, const StdRect *rect, float depthValue) {
	Std3DGlState savedState;

	saveGlState(&savedState);
	bindSurface(surface);
	glEnable(GL_SCISSOR_TEST);
	glScissor(rect->x, rect->y, rect->width, rect->height);
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

int Std3DGlBackend::uploadDepth(Std3DSurface *surface, const StdRect *rectPointer, const byte *zData, int pitchInBytes) {
	const StdRect &rect = *rectPointer;
	byte *data = new byte[rect.width * rect.height * 4];

	for (int y = 0; y < rect.height; y++) {
		const byte *row = zData + y * pitchInBytes;
		byte *out = &data[y * rect.width * 4];

		for (int x = 0; x < rect.width; x++) {
			uint value = READ_LE_UINT16(row + 2 * x);

			out[0] = value & 0xFF;
			out[1] = value >> 8;
			out[2] = 0;
			out[3] = 255;
			out += 4;
		}
	}

	if (!_depthTexture)
		glGenTextures(1, &_depthTexture);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _depthTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rect.width, rect.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	delete[] data;

	Std3DGlState savedState;

	saveGlState(&savedState);
	bindSurface(surface);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_TRUE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	float left = (float)rect.x;
	float top = (float)rect.y;
	float right = (float)(rect.x + rect.width);
	float bottom = (float)(rect.y + rect.height);
	float quad[16] = {
		left, top, 0.0f, 0.0f,
		right, top, 1.0f, 0.0f,
		left, bottom, 0.0f, 1.0f,
		right, bottom, 1.0f, 1.0f
	};

	_depthShader->use(true);
	_depthShader->setUniform("targetSize", Math::Vector2d((float)surface->width, (float)surface->height));
	_depthShader->setUniform1f("flipY", 1.0f);
	_depthShader->setUniform("tex", 0u);

	drawQuad(quad);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, 0);
	_depthShader->unbind();
	restoreGlState(&savedState);
	return 1;
}

int Std3DGlBackend::presentSurface(Std3DSurface *surface) {
	Std3DGlState savedState;

	saveGlState(&savedState);
	glViewport(0, 0, g_system->getWidth(), g_system->getHeight());
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	float right = (float)surface->width;
	float bottom = (float)surface->height;
	float quad[16] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		right, 0.0f, 1.0f, 0.0f,
		0.0f, bottom, 0.0f, 1.0f,
		right, bottom, 1.0f, 1.0f
	};

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glData(surface)->colorTexture);

	_blitShader->use(true);
	_blitShader->setUniform("targetSize", Math::Vector2d(right, bottom));
	_blitShader->setUniform1f("flipY", -1.0f);
	_blitShader->setUniform("tex", 0u);
	_blitShader->setUniform("useKey", 0u);
	_blitShader->setUniform("keyColor", Math::Vector3d(0.0f, 0.0f, 0.0f));

	drawQuad(quad);

	glBindTexture(GL_TEXTURE_2D, 0);
	_blitShader->unbind();
	restoreGlState(&savedState);
	return 1;
}

uint Std3DGlBackend::createTexture(const byte *rgba, int width, int height, int magLinear, int minLinear) {
	GLuint texture = 0;

	glGenTextures(1, &texture);

	if (!texture)
		return 0;

	int powerOfTwo = ((uint)width & ((uint)width - 1)) == 0 && ((uint)height & ((uint)height - 1)) == 0;
	GLint wrapMode = GL_REPEAT;

	if (!powerOfTwo && OpenGLContext.type == OpenGL::kContextGLES2)
		wrapMode = GL_CLAMP_TO_EDGE;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minLinear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magLinear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	glBindTexture(GL_TEXTURE_2D, 0);
	return texture;
}

void Std3DGlBackend::deleteTexture(uint textureHandle) {
	if (textureHandle && _glReady && OpenGLContext.type != OpenGL::kContextNone) {
		GLuint texture = textureHandle;
		glDeleteTextures(1, &texture);
	}
}

uint Std3DGlBackend::glCompareFunc(uint d3dFunc) {
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
		warning("Std3DGlBackend::glCompareFunc(): invalid compare function %u", d3dFunc);
		return GL_ALWAYS;
	}
}

void Std3DGlBackend::applyRenderState(const Std3DDrawList *list, uint flags) {
	if (flags & (kStd3DRsAlphaBlend | kStd3DRsAlphaTexture)) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glDisable(GL_BLEND);
	}

	_tlShader->setUniform("modulateAlpha", (flags & kStd3DRsAlphaTexture) != 0 ? 1u : 0u);

	if (list->zEnable) {
		glEnable(GL_DEPTH_TEST);

		if (flags & kStd3DRsZTest)
			glDepthFunc(glCompareFunc(list->zCompareFunc));
		else
			glDepthFunc(GL_ALWAYS);

		glDepthMask((flags & kStd3DRsZWrite) != 0 ? GL_TRUE : GL_FALSE);
	} else {
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}

	_tlShader->setUniform("fogEnabled", (flags & kStd3DRsFog) != 0 ? 1u : 0u);
}

int Std3DGlBackend::drawList(const Std3DDrawList *list) {
	Std3DGlState savedState;

	saveGlState(&savedState);
	bindSurface(list->target);
	glDisable(GL_DITHER);

	Common::Array<Std3DGlVertex> glVertices;

	for (uint i = 0; i < list->numVertices; i++) {
		const Std3DVertex &vertex = list->vertices[i];
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
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, list->numIndices * sizeof(uint16), list->numIndices == 0 ? nullptr : list->indices, GL_STREAM_DRAW);

	_tlShader->use(true);
	_tlShader->setUniform("targetSize", Math::Vector2d((float)list->target->width, (float)list->target->height));
	_tlShader->setUniform("tex", 0u);
	_tlShader->setUniform("textured", 0u);
	_tlShader->setUniform("texHasAlpha", 0u);
	_tlShader->setUniform("alphaFunc", list->alphaFunc);
	_tlShader->setUniform("fogColor", Math::Vector3d(list->fogRed / 255.0f, list->fogGreen / 255.0f, list->fogBlue / 255.0f));
	_tlShader->setUniform("fogRange", Math::Vector2d(list->fogStart, list->fogEnd));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	applyRenderState(list, list->initialRenderState);

	for (uint i = 0; i < list->numCommands; i++) {
		const Std3DCommand &command = list->commands[i];

		switch (command.type) {
		case kStd3DCommandRenderState:
			applyRenderState(list, command.flags);
			break;
		case kStd3DCommandTexture:
			glBindTexture(GL_TEXTURE_2D, command.texture);
			_tlShader->setUniform("textured", command.texture != 0 ? 1u : 0u);
			_tlShader->setUniform("texHasAlpha", command.textureAlpha != 0 ? 1u : 0u);
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
	_tlShader->unbind();
	restoreGlState(&savedState);
	return 1;
}

Std3DBackend *createGlBackend() {
	return new Std3DGlBackend();
}

} // End of namespace Grim

#endif
