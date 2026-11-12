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

#include <limits.h>
#include <math.h>

#include "common/textconsole.h"

#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/polyglot_v2.h"
#include "engines/grim/emi/profilertimer.h"
#include "engines/grim/emi/textevent.h"

namespace Grim {

static const int kQuadsFlags = 0x20004;

static Glyph *findGlyph(CTextFont *textFont, int character) {
	if (!textFont || !textFont->charIndex || !textFont->glyphs)
		return nullptr;

	if (character > textFont->lastChar)
		return nullptr;

	if (character < textFont->firstChar) {
		warning("TextEvent: character %d is below the first character of the font", character);

		return nullptr;
	}

	int index = textFont->charIndex[character - textFont->firstChar];

	if (index >= textFont->numGlyphs) {
		warning("TextEvent: glyph index %d is out of range", index);

		return nullptr;
	}

	return &textFont->glyphs[index];
}

TextEvent::TextEvent(EmiEngine *engine) : font(engine), _engine(engine) {
	clipRect[0] = 0;
	clipRect[1] = 0;
	clipRect[2] = 0;
	clipRect[3] = 0;

	init();
}

TextEvent::TextEvent(EmiEngine *engine, const LString &newText) : font(engine), _engine(engine) {
	clipRect[0] = 0;
	clipRect[1] = 0;
	clipRect[2] = 0;
	clipRect[3] = 0;

	init();

	text = newText;
	geometryReady = false;
}

TextEvent::TextEvent(EmiEngine *engine, const char *newText) : font(engine), _engine(engine) {
	clipRect[0] = 0;
	clipRect[1] = 0;
	clipRect[2] = 0;
	clipRect[3] = 0;

	init();

	if (newText)
		text = newText;
	else
		warning("TextEvent::TextEvent(): no text");

	geometryReady = false;
}

TextEvent::~TextEvent() {
	Common::List<TextEvent *>::iterator it = _engine->_textEvents.begin();

	while (it != _engine->_textEvents.end()) {
		if (*it == this)
			it = _engine->_textEvents.erase(it);
		else
			++it;
	}

	freeGeometry();
}

void *TextEvent::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void TextEvent::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void TextEvent::init() {
	// The disasm leaves flags and layer uninitialized
	flags = 0;
	layer = 0;
	actorId = 0;
	hold = 0;
	id = _engine->_nextTextEventId;
	_engine->_nextTextEventId++;

	TextEvent *defaultEvent = _engine->_defaultTextEvent;

	if (defaultEvent) {
		clipRect[0] = defaultEvent->clipRect[0];
		clipRect[1] = defaultEvent->clipRect[1];
		clipRect[2] = defaultEvent->clipRect[2];
		clipRect[3] = defaultEvent->clipRect[3];
		x = defaultEvent->x;
		y = defaultEvent->y;
		geometryReady = false;

		LResourceHandle<CTextFont> defaultFont(_engine);

		defaultFont.setResource(defaultEvent->font.resource);
		setFont(defaultFont);

		color = defaultEvent->color;
		geometryReady = false;
		outlineColor = defaultEvent->outlineColor;
		geometryReady = false;
		justify = defaultEvent->justify;
		geometryReady = false;

		double durationMs = defaultEvent->duration;
		int truncatedMs = INT_MIN;

		if (durationMs > -2147483648.0 && durationMs < 2147483648.0)
			truncatedMs = (int)durationMs;
		else
			warning("TextEvent::init(): the default duration is out of range");

		duration = (float)((double)truncatedMs * (double)0.001f);
	} else {
		clipRect[0] = 0;
		clipRect[1] = 0;
		clipRect[2] = _engine->_textScreenRect[2];
		clipRect[3] = _engine->_textScreenRect[3];
		x = 0;
		y = 0;
		geometryReady = false;
		font.setResource(nullptr);
		color = Color::kWhite;
		geometryReady = false;
		outlineColor = Color();
		justify = 4;
		geometryReady = false;
		duration = (float)-0.001;
	}

	text = "";
	geometryReady = false;
	scaleX = 1.0f;
	scaleY = 1.0f;
	geometryReady = false;
	vertexCount = 0;
	positions = nullptr;
	normals = nullptr;
	colors = nullptr;
	uvs = nullptr;
}

int TextEvent::activate(float newDuration, int newFlags) {
	duration = newDuration;
	flags = newFlags;
	_engine->_textEvents.push_back(this);

	return _engine->_textEvents.size();
}

void TextEvent::freeGeometry() {
	vertexCount = 0;

	if (positions)
		_engine->getMemoryServices()->freeImpl(positions);

	positions = nullptr;

	if (normals)
		_engine->getMemoryServices()->freeImpl(normals);

	normals = nullptr;

	if (colors)
		_engine->getMemoryServices()->freeImpl(colors);

	colors = nullptr;

	if (uvs)
		_engine->getMemoryServices()->freeImpl(uvs);

	uvs = nullptr;
	runs.clear();
}

void TextEvent::buildGeometry() {
	freeGeometry();

	uint length = text.size();
	int numVertices = 0;
	int lastPage = -1;

	vertexCount = 4 * length;

	positions = (float *)_engine->getMemoryServices()->mallocImpl(vertexCount * 3 * sizeof(float));
	colors = (byte *)_engine->getMemoryServices()->mallocImpl(vertexCount * 4 * sizeof(byte));
	uvs = (float *)_engine->getMemoryServices()->mallocImpl(vertexCount * 2 * sizeof(float));

	// The disasm never checks, an empty text gets no memory (size 0) and builds nothing
	if (vertexCount != 0 && (!positions || !colors || !uvs)) {
		warning("TextEvent::buildGeometry(): out of memory");

		return;
	}

	if (vertexCount != 0) {
		memset(positions, 0, vertexCount * 3 * sizeof(float));
		memset(colors, 0, vertexCount * 4 * sizeof(byte));
		memset(uvs, 0, vertexCount * 2 * sizeof(float));
	}

	const char *cursor = text.c_str();
	CTextFont *lockedFont = nullptr;

	if (font.lock())
		lockedFont = font.typed;

	if (!lockedFont) {
		warning("TextEvent::buildGeometry(): no font");

		return;
	}

	int lineHeight = (int)((double)lockedFont->lineHeight * (double)scaleY);
	int cursorX = x;
	int cursorY = y;
	int previousChar = 0;
	const char *lineStart = cursor;

	while (lineStart) {
		int lineWidth;
		int lineChars;
		const char *nextLine = layoutLine(lineStart, &lineWidth, &lineChars);
		int justification = justify & 3;

		if (justification == 2)
			cursorX += -(lineWidth / 2);
		else if (justification == 1)
			cursorX -= lineWidth;

		for (int i = 0; i < lineChars; i++) {
			int character = nextChar(&cursor);
			int skippedChar = previousChar;

			previousChar = character;

			if ((skippedChar == 10 && character == 13) || !character)
				continue;

			CTextFont *glyphFont = nullptr;

			if (font.lock())
				glyphFont = font.typed;

			Glyph *glyph = findGlyph(glyphFont, character);

			font.unlock();

			if (!glyph) {
				warning("TextEvent::buildGeometry(): missing glyph for character %d", character);

				continue;
			}

			if (character != 32) {
				if (glyph->page != lastPage || runs.empty()) {
					TextRun run;

					run.page = glyph->page;
					run.firstVertex = numVertices;
					run.vertexCount = 0;
					runs.push_back(run);
					lastPage = glyph->page;
				}

				float left = (float)((double)glyph->offsetX * (double)scaleX + (double)cursorX);
				float top = (float)((double)glyph->offsetY * (double)scaleY + (double)cursorY);
				float right = (float)((double)(uint)glyph->alpha.getWidth() * (double)scaleX + (double)left);
				float bottom = (float)((double)(uint)glyph->alpha.getHeight() * (double)scaleY + (double)top);

				if (left < right && top < bottom) {
					float halfWidth = (float)(uint)(_engine->_textScreenRect[2] / 2);
					float halfHeight = (float)(uint)(_engine->_textScreenRect[3] / 2);

					if (halfWidth == 0.0f || halfHeight == 0.0f) {
						// The original could divide by zero here, let's avoid that...
						warning("TextEvent::buildGeometry(): the screen size is zero");

						continue;
					}

					if (numVertices + 4 > vertexCount) {
						warning("TextEvent::buildGeometry(): too many glyphs");

						continue;
					}

					double normalLeft = ((double)left - (double)halfWidth) / (double)halfWidth;
					float normalTop = (float)(((double)top - (double)halfHeight) / (double)halfHeight);
					double normalRight = ((double)right - (double)halfWidth) / (double)halfWidth;
					float normalBottom = (float)(((double)bottom - (double)halfHeight) / (double)halfHeight);
					float *vertex = positions + numVertices * 3;

					vertex[0] = (float)normalLeft;
					vertex[1] = -normalTop;
					vertex[2] = 1.0f;
					vertex[3] = (float)normalRight;
					vertex[4] = -normalTop;
					vertex[5] = 1.0f;
					vertex[6] = (float)normalRight;
					vertex[7] = -normalBottom;
					vertex[8] = 1.0f;
					vertex[9] = (float)normalLeft;
					vertex[10] = -normalBottom;
					vertex[11] = 1.0f;

					byte red = (uint)color.r <= 0xFF ? (byte)color.r : 0xFF;
					byte green = (uint)color.g <= 0xFF ? (byte)color.g : 0xFF;
					byte blue = (uint)color.b <= 0xFF ? (byte)color.b : 0xFF;
					byte alpha = (uint)color.a <= 0xFF ? (byte)color.a : 0xFF;

					for (int corner = 0; corner < 4; corner++) {
						byte *vertexColor = colors + (numVertices + corner) * 4;

						vertexColor[2] = red;
						vertexColor[1] = green;
						vertexColor[0] = blue;
						vertexColor[3] = alpha;
					}

					float *vertexUvs = uvs + numVertices * 2;

					vertexUvs[0] = glyph->u0;
					vertexUvs[1] = glyph->v0;
					vertexUvs[2] = glyph->u1;
					vertexUvs[3] = glyph->v0;
					vertexUvs[4] = glyph->u1;
					vertexUvs[5] = glyph->v1;
					vertexUvs[6] = glyph->u0;
					vertexUvs[7] = glyph->v1;

					runs[runs.size() - 1].vertexCount += 4;
					numVertices += 4;
				}
			}

			float scaledAdvance = (float)((double)glyph->advance * (double)scaleX);

			cursorX += (int)lrint((double)scaledAdvance);
		}

		if (nextLine) {
			cursorX = x;
			cursorY += lineHeight;
		}

		lineStart = nextLine;
		cursor = nextLine;
	}

	geometryReady = true;
	font.unlock();
}

const char *TextEvent::layoutLine(const char *lineStart, int *outWidth, int *outNumChars) {
	int clipLeft = clipRect[0];
	int clipRight = clipRect[2];
	int justification = justify & 3;
	int leftSpace = x - clipLeft;
	int rightSpace = clipLeft + (clipRight - x);
	int maxWidth;

	if (justification == 2) {
		if (leftSpace < rightSpace)
			rightSpace = leftSpace;

		maxWidth = rightSpace * 2;
	} else if (justification == 1) {
		maxWidth = leftSpace;
	} else {
		maxWidth = rightSpace;
	}

	const char *cursor = lineStart;
	const char *wrapPoint = lineStart;
	const char *result;
	int numSpaces = 0;
	int pendingWidth = 0;
	int wordWidth = 0;
	int totalWidth = 0;
	int wordChars = 0;
	int totalChars = 0;
	int character = nextChar(&cursor);

	if (character) {
		while (true) {
			if ((character == 32 || character == 254) && (justify & 4)) {
				if (wordWidth + pendingWidth + totalWidth > maxWidth) {
					if (totalChars) {
						result = wrapPoint;
					} else {
						totalWidth += wordWidth + pendingWidth;
						totalChars = numSpaces + wordChars;
						result = cursor;
					}

					if (outNumChars)
						*outNumChars = totalChars;

					if (outWidth)
						*outWidth = totalWidth;

					return result;
				}

				totalWidth += wordWidth + pendingWidth;
				totalChars += wordChars + numSpaces;
				pendingWidth = getCharWidth(character);
				wordChars = 0;
				numSpaces = 1;
				wordWidth = 0;
				wrapPoint = cursor;
				character = nextChar(&cursor);

				if (!character)
					break;

				while (character == 32 || character == 254) {
					int spaceWidth = getCharWidth(character);

					wrapPoint = cursor;
					pendingWidth += spaceWidth;
					numSpaces++;
					character = nextChar(&cursor);

					if (!character)
						break;
				}

				if (!character)
					break;
			}

			if (character == 13 || character == 10) {
				if (totalWidth + wordWidth + pendingWidth > maxWidth && (justify & 4) && totalChars) {
					result = wrapPoint;
				} else {
					totalWidth += wordWidth + pendingWidth;
					totalChars += numSpaces + wordChars;
					result = cursor;

					if (character == 13 && *cursor == 10)
						result = cursor + 1;
				}

				if (outNumChars)
					*outNumChars = totalChars;

				if (outWidth)
					*outWidth = totalWidth;

				return result;
			}

			wordWidth += getCharWidth(character);
			wordChars++;
			character = nextChar(&cursor);

			if (!character)
				break;
		}
	}

	int lineWidth = wordWidth + pendingWidth;

	if (lineWidth + totalWidth > maxWidth && (justify & 4) && totalChars) {
		result = wrapPoint;
	} else {
		totalWidth += lineWidth;
		result = nullptr;
		totalChars += wordChars + numSpaces;
	}

	if (outNumChars)
		*outNumChars = totalChars;

	if (outWidth)
		*outWidth = totalWidth;

	return result;
}

byte TextEvent::nextChar(const char **cursor) {
	const char *current = *cursor;
	byte character = (byte)*current;

	*cursor = current + 1;

	return character;
}

int TextEvent::getWidth() {
	int width = 0;
	int lineWidth = 0;
	const char *cursor = text.c_str();
	CTextFont *lockedFont = nullptr;

	if (font.lock())
		lockedFont = font.typed;

	if (lockedFont) {
		do {
			cursor = layoutLine(cursor, &lineWidth, nullptr);

			if (width <= lineWidth)
				width = lineWidth;
		} while (cursor);

		font.unlock();
	}

	return width;
}

int TextEvent::getHeight() {
	int height = 0;
	CTextFont *lockedFont = nullptr;

	if (font.lock())
		lockedFont = font.typed;

	if (!lockedFont) {
		warning("TextEvent::getHeight(): no font");

		return 0;
	}

	int lineHeight = (int)((double)lockedFont->lineHeight * (double)scaleY);
	const char *cursor = text.c_str();

	do {
		cursor = layoutLine(cursor, nullptr, nullptr);
		height += lineHeight;
	} while (cursor);

	font.unlock();

	return height;
}

int TextEvent::getCharWidth(uint16 character) {
	int width = 0;
	CTextFont *lockedFont = nullptr;

	if (font.lock())
		lockedFont = font.typed;

	if (lockedFont) {
		Glyph *glyph = findGlyph(lockedFont, character);

		if (glyph)
			width = (int)((double)glyph->advance * (double)scaleX);

		font.unlock();
	} else {
		warning("TextEvent::getCharWidth(): no font");
	}

	return width;
}

void TextEvent::draw(CDrawContext *drawContext) {
	CTextFont *lockedFont = nullptr;

	if (font.lock())
		lockedFont = font.typed;

	if (!lockedFont) {
		warning("TextEvent::draw(): no font");

		return;
	}

	if (lockedFont->pages.empty())
		lockedFont->buildTextures(drawContext);

	if (!geometryReady)
		buildGeometry();

	drawContext->setOrtho(-1.0f, 1.0f, -1.0f, 1.0f);

	VertexStreams streams;

	streams.count = vertexCount;
	streams.positions = positions;
	streams.normals = normals;
	streams.colors = reinterpret_cast<const uint32 *>(colors);
	streams.uvs = uvs;

	for (uint i = 0; i < runs.size(); i++) {
		CTexture *texture = lockedFont->getTexture(drawContext, runs[i].page);

		drawContext->drawQuads(kQuadsFlags, texture, runs[i].firstVertex, runs[i].vertexCount, &streams);
	}

	font.unlock();
}

bool TextEvent::save(tSaveRestoreAccess *access) {
	int length = (int)text.size() + 1;

	access(&length, sizeof(length));
	access((void *)text.c_str(), length);
	access(&x, sizeof(x));
	access(&y, sizeof(y));
	access(&duration, sizeof(duration));
	access(&actorId, sizeof(actorId));
	access(&flags, sizeof(flags));
	access(&id, sizeof(id));

	for (int i = 0; i < 4; i++)
		access(&clipRect[i], sizeof(int));

	LString fontName;

	if (font.resource)
		fontName = font.resource->name;
	else
		warning("TextEvent::save(): no font");

	length = (int)fontName.size() + 1;

	access(&length, sizeof(length));
	access((void *)fontName.c_str(), length);
	access(&layer, sizeof(layer));
	access(&hold, sizeof(hold));
	access(&color, sizeof(color));
	access(&outlineColor, sizeof(outlineColor));
	access(&scaleX, sizeof(scaleX));
	access(&scaleY, sizeof(scaleY));
	access(&justify, sizeof(justify));

	return true;
}

bool TextEvent::restore(tSaveRestoreAccess *access) {
	char buffer[kRestoreBufferSize + 1];
	int length;

	access(&length, sizeof(length));

	if (length < 1 || length > kRestoreBufferSize) {
		warning("TextEvent::restore(): invalid text length %d", length);

		return false;
	}

	access(buffer, length);
	buffer[length] = 0;
	text = buffer;

	access(&x, sizeof(x));
	access(&y, sizeof(y));
	access(&duration, sizeof(duration));
	access(&actorId, sizeof(actorId));
	access(&flags, sizeof(flags));
	access(&id, sizeof(id));

	for (int i = 0; i < 4; i++)
		access(&clipRect[i], sizeof(int));

	access(&length, sizeof(length));

	if (length < 1 || length > kRestoreBufferSize) {
		warning("TextEvent::restore(): invalid font name length %d", length);

		return false;
	}

	access(buffer, length);
	buffer[length] = 0;

	LString fontName = buffer;

	if (fontName.empty()) {
		font.setResource(nullptr);
	} else {
		LResourceHandle<CTextFont> restoredFont(_engine, fontName);

		font.setResource(restoredFont.resource);
	}

	access(&layer, sizeof(layer));
	access(&hold, sizeof(hold));
	access(&color, sizeof(color));
	access(&outlineColor, sizeof(outlineColor));
	access(&scaleX, sizeof(scaleX));
	access(&scaleY, sizeof(scaleY));
	access(&justify, sizeof(justify));
	geometryReady = false;

	return true;
}

bool TextEvent::setFont(const char *fontName) {
	if (!fontName) {
		warning("TextEvent::setFont(): no font name");

		return false;
	}

	LString fontLabel = fontName;
	LResourceHandle<CTextFont> newFont(_engine, fontLabel);

	if (newFont.lock() && newFont.typed) {
		newFont.unlock();
		font.setResource(nullptr);
		font.setResource(newFont.resource);
		geometryReady = false;

		return true;
	}

	return false;
}

bool TextEvent::setFont(LResourceHandle<CTextFont> newFont) {
	if (!newFont.resource)
		return false;

	if (!newFont.lock())
		return false;

	if (!newFont.typed)
		return false;

	newFont.unlock();
	font.setResource(nullptr);
	font.setResource(newFont.resource);
	geometryReady = false;

	return true;
}

int TextEvent::update(EmiEngine *engine, CDrawContext *drawContext, bool drawAll) {
	if (engine->isDumpingFrames())
		return 0;

	engine->_textUpdateTimer.start();

	Common::List<TextEvent *>::iterator it = engine->_textEvents.begin();

	while (it != engine->_textEvents.end()) {
		TextEvent *event = *it;

		++it;

		if (!event->hold) {
			if (event->duration > 0.0f || (event->flags & 1)) {
				uint eventFlags = event->flags;

				if (((eventFlags & 8) || drawAll) && !(eventFlags & 2))
					event->draw(drawContext);

				event->duration = (float)((double)event->duration - (double)engine->getFrameTime());
			} else {
				delete event;
			}
		}
	}

	int result = drawContext->flushDrawLists();

	engine->_textUpdateTimer.stop();

	return result;
}

bool TextEvent::startup(EmiEngine *engine, CDrawContext *drawContext) {
	if (engine->_textEventsStarted)
		return false;

	int viewport[4];
	int *rect = drawContext->getViewportRect(viewport);

	engine->_textScreenRect[0] = rect[0];
	engine->_textScreenRect[1] = rect[1];
	engine->_textScreenRect[2] = rect[2];
	engine->_textScreenRect[3] = rect[3];

	engine->_textEvents.clear();
	engine->_defaultTextEvent = new TextEvent(engine);
	engine->_defaultTextEvent->flags = 0;
	engine->_textEventsStarted = true;

	return true;
}

void TextEvent::shutdown(EmiEngine *engine) {
	Common::List<TextEvent *>::iterator it = engine->_textEvents.begin();

	while (it != engine->_textEvents.end()) {
		TextEvent *event = *it;

		++it;

		if (event)
			delete event;
	}

	engine->_textEvents.clear();

	if (engine->_defaultTextEvent) {
		delete engine->_defaultTextEvent;
		engine->_defaultTextEvent = nullptr;
	}

	engine->_textEventsStarted = false;
}

TextEvent *TextEvent::getEvent(EmiEngine *engine, int eventId) {
	Common::List<TextEvent *>::iterator it;

	for (it = engine->_textEvents.begin(); it != engine->_textEvents.end(); ++it) {
		if ((*it)->id == eventId)
			return *it;
	}

	return nullptr;
}

const char *TextEvent::translate(EmiEngine *engine, const char *msg, char *buffer) {
	const char *tagEnd = nullptr;
	const char *translated;

	if (buffer)
		*buffer = 0;

	Polyglot_v2 *polyglot = engine->getPolyglot();

	if (polyglot) {
		translated = polyglot->getTranslation(msg);
	} else {
		warning("TextEvent::translate(): no translation table");

		translated = msg;
	}

	if (buffer && msg && *msg == '/') {
		tagEnd = strchr(msg + 1, '/');

		if (tagEnd) {
			int tagLength = (int)(tagEnd - msg) - 1;

			if (tagLength >= kTagBufferSize) {
				warning("TextEvent::translate(): the tag is too long");

				tagLength = kTagBufferSize - 1;
			}

			memcpy(buffer, msg + 1, tagLength);
			buffer[tagLength] = 0;
		}
	}

	if (engine->_textMode == 1) {
		if (tagEnd)
			return tagEnd + 1;

		warning("TextEvent::translate(): the text has no tag");

		return translated;
	}

	if (engine->_textMode != 2)
		return translated;

	return msg;
}

void TextEvent::holdAll(EmiEngine *engine) {
	Common::List<TextEvent *>::iterator it;

	for (it = engine->_textEvents.begin(); it != engine->_textEvents.end(); ++it)
		(*it)->hold++;
}

void TextEvent::releaseAll(EmiEngine *engine) {
	Common::List<TextEvent *>::iterator it;

	for (it = engine->_textEvents.begin(); it != engine->_textEvents.end(); ++it) {
		TextEvent *event = *it;

		event->hold--;

		if (event->hold < 0)
			event->hold = 0;
	}
}

bool TextEvent::saveAll(EmiEngine *engine, tSaveRestoreAccess *access) {
	uint count = engine->_textEvents.size();

	access(&count, sizeof(count));

	Common::List<TextEvent *>::iterator it;

	for (it = engine->_textEvents.begin(); it != engine->_textEvents.end(); ++it)
		(*it)->save(access);

	access(&engine->_nextTextEventId, sizeof(engine->_nextTextEventId));
	access(&engine->_textMode, sizeof(engine->_textMode));

	if (engine->_defaultTextEvent)
		engine->_defaultTextEvent->save(access);
	else
		warning("TextEvent::saveAll(): no default event");

	return true;
}

bool TextEvent::restoreAll(EmiEngine *engine, tSaveRestoreAccess *access) {
	Common::List<TextEvent *>::iterator it = engine->_textEvents.begin();

	while (it != engine->_textEvents.end()) {
		TextEvent *event = *it;

		++it;

		if (event)
			delete event;
	}

	engine->_textEvents.clear();

	int count;

	access(&count, sizeof(count));

	for (int i = 0; i < count; i++) {
		TextEvent *event = new TextEvent(engine);

		if (!event->restore(access)) {
			delete event;

			return false;
		}

		engine->_textEvents.push_back(event);
	}

	access(&engine->_nextTextEventId, sizeof(engine->_nextTextEventId));
	access(&engine->_textMode, sizeof(engine->_textMode));

	if (engine->_defaultTextEvent)
		engine->_defaultTextEvent->restore(access);
	else
		warning("TextEvent::restoreAll(): no default event");

	return true;
}

} // End of namespace Grim
