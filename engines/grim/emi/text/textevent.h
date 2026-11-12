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

#ifndef GRIM_EMI_TEXTEVENT_H
#define GRIM_EMI_TEXTEVENT_H

#include "common/array.h"
#include "common/list.h"
#include "common/scummsys.h"

#include "engines/grim/emi/graphics/color.h"
#include "engines/grim/emi/text/ctextfont.h"
#include "engines/grim/emi/files/lfile.h"
#include "engines/grim/emi/resources/lresourcehandle.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

class CDrawContext;
class EmiEngine;

class TextEvent {
public:
	struct TextRun {
		int page;
		int firstVertex;
		int vertexCount;
	};

	enum {
		kRestoreBufferSize = 512,
		kTagBufferSize = 32
	};

	explicit TextEvent(EmiEngine *engine);
	TextEvent(EmiEngine *engine, const LString &newText);
	TextEvent(EmiEngine *engine, const char *newText);
	~TextEvent();

	void *operator new(size_t size);
	void operator delete(void *block);

	void init();
	int activate(float newDuration, int newFlags);
	void freeGeometry();
	void buildGeometry();
	const char *layoutLine(const char *lineStart, int *outWidth, int *outNumChars);
	byte nextChar(const char **cursor);
	int getWidth();
	int getHeight();
	int getCharWidth(uint16 character);
	void draw(CDrawContext *drawContext);
	bool save(tSaveRestoreAccess *access);
	bool restore(tSaveRestoreAccess *access);
	bool setFont(const char *fontName);
	bool setFont(LResourceHandle<CTextFont> newFont);

	static int update(EmiEngine *engine, CDrawContext *drawContext, bool drawAll);
	static bool startup(EmiEngine *engine, CDrawContext *drawContext);
	static void shutdown(EmiEngine *engine);
	static TextEvent *getEvent(EmiEngine *engine, int eventId);
	static const char *translate(EmiEngine *engine, const char *msg, char *buffer);
	static void holdAll(EmiEngine *engine);
	static void releaseAll(EmiEngine *engine);
	static bool saveAll(EmiEngine *engine, tSaveRestoreAccess *access);
	static bool restoreAll(EmiEngine *engine, tSaveRestoreAccess *access);

	LString text;
	float duration;
	uint flags;
	int actorId;
	int clipRect[4];
	LResourceHandle<CTextFont> font;
	int x;
	int y;
	int layer;
	int hold;
	Color color;
	Color outlineColor;
	float scaleX;
	float scaleY;
	byte justify;
	bool geometryReady;
	int id;
	Common::Array<TextRun> runs;
	int vertexCount;
	float *positions;
	float *normals;
	byte *colors;
	float *uvs;

private:
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
