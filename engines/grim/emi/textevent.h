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

#include "engines/grim/emi/color.h"
#include "engines/grim/emi/ctextfont.h"
#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lresourcehandle.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

class CDrawContext;

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

	TextEvent();
	explicit TextEvent(const LString &newText);
	explicit TextEvent(const char *newText);
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

	static int update(CDrawContext *drawContext, bool drawAll);
	static bool startup(CDrawContext *drawContext);
	static void shutdown();
	static TextEvent *getEvent(int eventId);
	static const char *translate(const char *msg, char *buffer);
	static void holdAll();
	static void releaseAll();
	static bool saveAll(tSaveRestoreAccess *access);
	static bool restoreAll(tSaveRestoreAccess *access);

	LString text;
	float duration;
	uint flags;
	int reserved24;
	int clipRect[4];
	LResourceHandle<CTextFont> font;
	int x;
	int y;
	int reserved68;
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

	static Common::List<TextEvent *> events;
	static TextEvent *defaultEvent;
	static int textMode;
	static int screenRect[4];

private:
	static int _nextId;
	static bool _started;
};

} // End of namespace Grim

#endif
