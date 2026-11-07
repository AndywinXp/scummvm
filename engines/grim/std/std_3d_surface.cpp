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

#include "common/endian.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "graphics/opengl/context.h"
#include "graphics/opengl/shader.h"

#include "engines/grim/std/std_3d.h"

namespace Grim {

static byte expand5(uint value) {
	return (byte)((value << 3) | (value >> 2));
}

static byte expand6(uint value) {
	return (byte)((value << 2) | (value >> 4));
}

static int resolveRect(const Std3DSurface *pSurface, const StdRect *pRect, StdRect *pResult) {
	if (pRect) {
		*pResult = *pRect;
	} else {
		pResult->x = 0;
		pResult->y = 0;
		pResult->width = pSurface->width;
		pResult->height = pSurface->height;
	}

	if (pResult->x < 0 || pResult->y < 0 || pResult->width < 0 || pResult->height < 0 ||
		pResult->x + pResult->width > pSurface->width || pResult->y + pResult->height > pSurface->height) {
		warning("Std3D: rectangle outside the surface");
		return 0;
	}

	return 1;
}

static void markGpuDirty(Std3DSurface *pSurface, int x, int y, int width, int height) {
	int left = MAX(x, 0);
	int top = MAX(y, 0);
	int right = MIN(x + width, pSurface->width);
	int bottom = MIN(y + height, pSurface->height);

	if (right <= left || bottom <= top)
		return;

	if (!pSurface->gpuDirty) {
		pSurface->gpuDirty = 1;
		pSurface->dirtyLeft = left;
		pSurface->dirtyTop = top;
		pSurface->dirtyRight = right;
		pSurface->dirtyBottom = bottom;
		return;
	}

	pSurface->dirtyLeft = MIN(pSurface->dirtyLeft, left);
	pSurface->dirtyTop = MIN(pSurface->dirtyTop, top);
	pSurface->dirtyRight = MAX(pSurface->dirtyRight, right);
	pSurface->dirtyBottom = MAX(pSurface->dirtyBottom, bottom);
}

void Std3D::saveGlState(Std3DGlState *pState) {
	GLint framebuffer = 0;
	GLint viewport[4];

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);
	glGetIntegerv(GL_VIEWPORT, viewport);

	pState->framebuffer = framebuffer;

	for (int i = 0; i < 4; i++)
		pState->viewport[i] = viewport[i];
}

void Std3D::restoreGlState(const Std3DGlState *pState) {
	glBindFramebuffer(GL_FRAMEBUFFER, pState->framebuffer);
	glViewport(pState->viewport[0], pState->viewport[1], pState->viewport[2], pState->viewport[3]);
}

void Std3D::bindSurface(Std3DSurface *pSurface, int bMarkDirty) {
	if (bMarkDirty)
		markGpuDirty(pSurface, 0, 0, pSurface->width, pSurface->height);

	glBindFramebuffer(GL_FRAMEBUFFER, pSurface->framebuffer);
	glViewport(0, 0, pSurface->width, pSurface->height);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

int Std3D::attachDepth(Std3DSurface *pSurface) {
	Std3DGlState savedState;
	GLuint renderbuffer = 0;

	saveGlState(&savedState);
	glBindFramebuffer(GL_FRAMEBUFFER, pSurface->framebuffer);
	glGenRenderbuffers(1, &renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);

#if defined(GL_DEPTH_COMPONENT24)
	if (_zBufferBitDepth == 32) {
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, pSurface->width, pSurface->height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, pSurface->width, pSurface->height);
	} else {
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, pSurface->width, pSurface->height);
	}
#else
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, pSurface->width, pSurface->height);
#endif

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		warning("Std3D::attachDepth(): incomplete framebuffer");
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
		glDeleteRenderbuffers(1, &renderbuffer);
		restoreGlState(&savedState);
		return 0;
	}

	pSurface->depthRenderbuffer = renderbuffer;
	restoreGlState(&savedState);
	return 1;
}

void Std3D::detachDepth(Std3DSurface *pSurface) {
	if (!pSurface->depthRenderbuffer)
		return;

	Std3DGlState savedState;
	GLuint renderbuffer = pSurface->depthRenderbuffer;

	saveGlState(&savedState);
	glBindFramebuffer(GL_FRAMEBUFFER, pSurface->framebuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glDeleteRenderbuffers(1, &renderbuffer);
	pSurface->depthRenderbuffer = 0;
	restoreGlState(&savedState);
}

void Std3D::drawQuad(const float *pQuad) {
	glBindBuffer(GL_ARRAY_BUFFER, _quadVbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 16 * sizeof(float), pQuad);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

Std3DSurface *Std3D::createSurface(int width, int height, int bHasDepth) {
	if (!ensureGl())
		return nullptr;

	if (width <= 0 || height <= 0 || width > OpenGLContext.maxTextureSize || height > OpenGLContext.maxTextureSize) {
		warning("Std3D::createSurface(): invalid surface size");
		return nullptr;
	}

	Std3DSurface *pSurface = new Std3DSurface;
	Std3DGlState savedState;
	GLuint texture = 0;
	GLuint framebuffer = 0;

	memset(pSurface, 0, sizeof(Std3DSurface));
	pSurface->width = width;
	pSurface->height = height;

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
	pSurface->colorTexture = texture;

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	pSurface->framebuffer = framebuffer;

	int bComplete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

	if (!bComplete)
		warning("Std3D::createSurface(): incomplete framebuffer");

	if (bComplete && bHasDepth && !attachDepth(pSurface))
		bComplete = 0;

	if (!bComplete) {
		glDeleteFramebuffers(1, &framebuffer);
		glDeleteTextures(1, &texture);
		restoreGlState(&savedState);
		delete pSurface;
		return nullptr;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, width, height);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	restoreGlState(&savedState);
	return pSurface;
}

void Std3D::destroySurface(Std3DSurface *&pSurface) {
	if (!pSurface)
		return;

	if (pSurface == _pRenderTarget)
		_pRenderTarget = nullptr;

	if (_bGlReady && OpenGLContext.type != OpenGL::kContextNone) {
		detachDepth(pSurface);

		GLuint framebuffer = pSurface->framebuffer;
		GLuint texture = pSurface->colorTexture;

		glDeleteFramebuffers(1, &framebuffer);
		glDeleteTextures(1, &texture);
	}

	delete[] pSurface->pReference;
	delete pSurface;
	pSurface = nullptr;
}

int Std3D::uploadPixels(Std3DSurface *pSurface, const StdRect *pRect, const byte *pPixels, int pitchInBytes) {
	if (!pSurface || !pPixels || !ensureGl())
		return 0;

	StdRect rect;

	if (!resolveRect(pSurface, pRect, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	byte *pRgba = new byte[rect.width * rect.height * 4];

	for (int y = 0; y < rect.height; y++) {
		const byte *pRow = pPixels + y * pitchInBytes;
		byte *pOut = &pRgba[y * rect.width * 4];

		for (int x = 0; x < rect.width; x++) {
			uint value = READ_LE_UINT16(pRow + 2 * x);

			pOut[0] = expand5((value >> 11) & 0x1F);
			pOut[1] = expand6((value >> 5) & 0x3F);
			pOut[2] = expand5(value & 0x1F);
			pOut[3] = 255;
			pOut += 4;
		}
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pSurface->colorTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x, rect.y, rect.width, rect.height, GL_RGBA, GL_UNSIGNED_BYTE, pRgba);
	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] pRgba;
	return 1;
}

int Std3D::readPixels(Std3DSurface *pSurface, const StdRect *pRect, byte *pOut, int pitchInBytes) {
	if (!pSurface || !pOut || !ensureGl())
		return 0;

	StdRect rect;

	if (!resolveRect(pSurface, pRect, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	byte *pRgba = new byte[rect.width * rect.height * 4];
	Std3DGlState savedState;

	saveGlState(&savedState);
	glBindFramebuffer(GL_FRAMEBUFFER, pSurface->framebuffer);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadPixels(rect.x, rect.y, rect.width, rect.height, GL_RGBA, GL_UNSIGNED_BYTE, pRgba);
	restoreGlState(&savedState);

	for (int y = 0; y < rect.height; y++) {
		const byte *pIn = &pRgba[y * rect.width * 4];
		byte *pRow = pOut + y * pitchInBytes;

		for (int x = 0; x < rect.width; x++) {
			uint value = ((pIn[0] >> 3) << 11) | ((pIn[1] >> 2) << 5) | (pIn[2] >> 3);

			WRITE_LE_UINT16(pRow + 2 * x, value);
			pIn += 4;
		}
	}

	delete[] pRgba;
	return 1;
}

int Std3D::fillRect(Std3DSurface *pSurface, const StdRect *pRect, uint16 color) {
	if (!pSurface || !ensureGl())
		return 0;

	StdRect rect;

	if (!resolveRect(pSurface, pRect, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	Std3DGlState savedState;

	saveGlState(&savedState);
	bindSurface(pSurface, 0);
	markGpuDirty(pSurface, rect.x, rect.y, rect.width, rect.height);
	glEnable(GL_SCISSOR_TEST);
	glScissor(rect.x, rect.y, rect.width, rect.height);
	glClearColor(expand5((color >> 11) & 0x1F) / 255.0f, expand6((color >> 5) & 0x3F) / 255.0f, expand5(color & 0x1F) / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
	restoreGlState(&savedState);
	return 1;
}

int Std3D::blit(Std3DSurface *pDst, Std3DSurface *pSrc, const StdRect *pSrcRect, int dstX, int dstY, int bColorKey, uint16 colorKey) {
	if (!pDst || !pSrc || !ensureGl())
		return 0;

	int srcX = 0;
	int srcY = 0;
	int width = pSrc->width;
	int height = pSrc->height;

	if (pSrcRect) {
		srcX = pSrcRect->x;
		srcY = pSrcRect->y;
		width = pSrcRect->width;
		height = pSrcRect->height;
	}

	if (srcX < 0) {
		dstX -= srcX;
		width += srcX;
		srcX = 0;
	}

	if (srcY < 0) {
		dstY -= srcY;
		height += srcY;
		srcY = 0;
	}

	if (srcX + width > pSrc->width)
		width = pSrc->width - srcX;

	if (srcY + height > pSrc->height)
		height = pSrc->height - srcY;

	if (dstX < 0) {
		srcX -= dstX;
		width += dstX;
		dstX = 0;
	}

	if (dstY < 0) {
		srcY -= dstY;
		height += dstY;
		dstY = 0;
	}

	if (dstX + width > pDst->width)
		width = pDst->width - dstX;

	if (dstY + height > pDst->height)
		height = pDst->height - dstY;

	if (width <= 0 || height <= 0)
		return 1;

	if (pDst == pSrc) {
		Std3DSurface *pScratch = createSurface(width, height, 0);

		if (!pScratch)
			return 0;

		StdRect srcRect;

		srcRect.x = srcX;
		srcRect.y = srcY;
		srcRect.width = width;
		srcRect.height = height;

		int result = blit(pScratch, pSrc, &srcRect, 0, 0, 0, 0) && blit(pDst, pScratch, nullptr, dstX, dstY, bColorKey, colorKey);

		destroySurface(pScratch);
		return result;
	}

	Std3DGlState savedState;

	saveGlState(&savedState);
	bindSurface(pDst, 0);
	markGpuDirty(pDst, dstX, dstY, width, height);

	float left = (float)dstX;
	float top = (float)dstY;
	float right = (float)(dstX + width);
	float bottom = (float)(dstY + height);
	float s0 = (float)srcX / (float)pSrc->width;
	float t0 = (float)srcY / (float)pSrc->height;
	float s1 = (float)(srcX + width) / (float)pSrc->width;
	float t1 = (float)(srcY + height) / (float)pSrc->height;
	float quad[16] = {
		left, top, s0, t0,
		right, top, s1, t0,
		left, bottom, s0, t1,
		right, bottom, s1, t1
	};

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pSrc->colorTexture);

	_pBlitShader->use(true);
	_pBlitShader->setUniform("targetSize", Math::Vector2d((float)pDst->width, (float)pDst->height));
	_pBlitShader->setUniform1f("flipY", 1.0f);
	_pBlitShader->setUniform("tex", 0u);
	_pBlitShader->setUniform("useKey", bColorKey != 0 ? 1u : 0u);
	_pBlitShader->setUniform("keyColor", Math::Vector3d(expand5((colorKey >> 11) & 0x1F) / 255.0f, expand6((colorKey >> 5) & 0x3F) / 255.0f, expand5(colorKey & 0x1F) / 255.0f));

	drawQuad(quad);

	glBindTexture(GL_TEXTURE_2D, 0);
	_pBlitShader->unbind();
	restoreGlState(&savedState);
	return 1;
}

int Std3D::uploadDepth(Std3DSurface *pSurface, const StdRect *pRect, const byte *pZData, int pitchInBytes) {
	if (!pSurface || !pZData || !ensureGl())
		return 0;

	if (!pSurface->depthRenderbuffer || !_pDepthShader) {
		warning("Std3D::uploadDepth(): depth upload is not available");
		return 0;
	}

	StdRect rect;

	if (!resolveRect(pSurface, pRect, &rect))
		return 0;

	if (rect.width == 0 || rect.height == 0)
		return 1;

	byte *pData = new byte[rect.width * rect.height * 4];

	for (int y = 0; y < rect.height; y++) {
		const byte *pRow = pZData + y * pitchInBytes;
		byte *pOut = &pData[y * rect.width * 4];

		for (int x = 0; x < rect.width; x++) {
			uint value = READ_LE_UINT16(pRow + 2 * x);

			pOut[0] = value & 0xFF;
			pOut[1] = value >> 8;
			pOut[2] = 0;
			pOut[3] = 255;
			pOut += 4;
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rect.width, rect.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pData);

	delete[] pData;

	Std3DGlState savedState;

	saveGlState(&savedState);
	bindSurface(pSurface, 0);
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

	_pDepthShader->use(true);
	_pDepthShader->setUniform("targetSize", Math::Vector2d((float)pSurface->width, (float)pSurface->height));
	_pDepthShader->setUniform1f("flipY", 1.0f);
	_pDepthShader->setUniform("tex", 0u);

	drawQuad(quad);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, 0);
	_pDepthShader->unbind();
	restoreGlState(&savedState);
	return 1;
}

int Std3D::presentSurface(Std3DSurface *pSurface) {
	if (!pSurface || !ensureGl())
		return 0;

	Std3DGlState savedState;

	saveGlState(&savedState);
	glViewport(0, 0, g_system->getWidth(), g_system->getHeight());
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	float right = (float)pSurface->width;
	float bottom = (float)pSurface->height;
	float quad[16] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		right, 0.0f, 1.0f, 0.0f,
		0.0f, bottom, 0.0f, 1.0f,
		right, bottom, 1.0f, 1.0f
	};

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pSurface->colorTexture);

	_pBlitShader->use(true);
	_pBlitShader->setUniform("targetSize", Math::Vector2d(right, bottom));
	_pBlitShader->setUniform1f("flipY", -1.0f);
	_pBlitShader->setUniform("tex", 0u);
	_pBlitShader->setUniform("useKey", 0u);
	_pBlitShader->setUniform("keyColor", Math::Vector3d(0.0f, 0.0f, 0.0f));

	drawQuad(quad);

	glBindTexture(GL_TEXTURE_2D, 0);
	_pBlitShader->unbind();
	restoreGlState(&savedState);
	return 1;
}

} // End of namespace Grim

#endif
