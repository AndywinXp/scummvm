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

#include "common/endian.h"
#include "common/list.h"
#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/ctextfont.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lresourcehandle.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/textevent.h"

namespace Grim {

static const int kColorTag = MKTAG('R', 'L', 'O', 'C');
static const int kTextTag = MKTAG('T', 'X', 'E', 'T');
static const int kLocalizedBufferSize = 640;
static const float kHalf = 0.5f;
static const float kMillisecondsToSeconds = 0.001f;

int Lua_V2::truncateToInt(double value) {
	if (!(value > -9223372036854775809.0 && value < 9223372036854775808.0)) {
		warning("Lua_V2::truncateToInt(): value out of range");

		return 0;
	}

	int64 result = (int64)value;

	if (result < -2147483648LL || result > 2147483647LL)
		warning("Lua_V2::truncateToInt(): value out of range");

	return (int)result;
}

void Lua_V2::applyTextTable(lua_Object table, TextEvent *event) {
	if (!event)
		return;

	int screenWidth = TextEvent::screenRect[2];
	int screenHeight = TextEvent::screenRect[3];
	int x = event->x;
	int y = event->y;
	bool xChanged = false;
	bool yChanged = false;
	lua_Object valueObj;

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldX]));

	valueObj = lua_gettable();

	if (valueObj && lua_isnumber(valueObj)) {
		x = truncateToInt((double)lua_getnumber(valueObj) * (double)screenWidth * (double)kHalf + (double)(screenWidth / 2));
		xChanged = true;
	}

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldY]));

	valueObj = lua_gettable();

	if (valueObj && lua_isnumber(valueObj)) {
		float halfHeight = (float)(screenHeight / 2);

		y = truncateToInt((double)halfHeight - (double)lua_getnumber(valueObj) * (double)screenHeight * (double)kHalf);
		yChanged = true;
	}

	if (xChanged || yChanged) {
		event->x = x;
		event->y = y;
		event->geometryReady = false;
	}

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldFont]));

	valueObj = lua_gettable();

	if (valueObj && lua_isstring(valueObj)) {
		LString fontName = lua_getstring(valueObj);
		LResourceHandle<CTextFont> fontHandle(fontName);
		bool locked = fontHandle.lock();
		LResourceHandle<CTextFont> lockedFont;

		if (locked)
			lockedFont.setResource(fontHandle.typed);
		else
			lockedFont.setResource(nullptr);

		event->setFont(lockedFont);
		fontHandle.unlock();
	}

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldWidth]));

	valueObj = lua_gettable();

	if (valueObj && lua_isnumber(valueObj))
		event->clipRect[2] = truncateToInt((double)lua_getnumber(valueObj) * (double)screenWidth);

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldHeight]));

	valueObj = lua_gettable();

	if (valueObj && lua_isnumber(valueObj))
		event->clipRect[3] = truncateToInt((double)lua_getnumber(valueObj) * (double)screenHeight);

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldFgcolor]));

	valueObj = lua_gettable();

	if (valueObj && lua_isuserdata(valueObj) && lua_tag(valueObj) == kColorTag) {
		event->color = Color((uint)lua_getuserdata(valueObj));
		event->geometryReady = false;
	}

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldBgcolor]));

	valueObj = lua_gettable();

	if (valueObj && lua_isuserdata(valueObj) && lua_tag(valueObj) == kColorTag) {
		event->outlineColor = Color((uint)lua_getuserdata(valueObj));
		event->geometryReady = false;
	}

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldCenter]));

	valueObj = lua_gettable();

	if (valueObj && !lua_isnil(valueObj)) {
		event->justify = 6;
		event->geometryReady = false;
	}

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldLjustify]));

	valueObj = lua_gettable();

	if (valueObj && !lua_isnil(valueObj)) {
		event->justify = 4;
		event->geometryReady = false;
	}

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldRjustify]));

	valueObj = lua_gettable();

	if (valueObj && !lua_isnil(valueObj)) {
		event->justify = 5;
		event->geometryReady = false;
	}

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldLayer]));

	valueObj = lua_gettable();

	if (valueObj && lua_isnumber(valueObj)) {
		event->layer = luaFtol(lua_getnumber(valueObj));
		event->geometryReady = false;
	}

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldDuration]));

	valueObj = lua_gettable();

	if (valueObj && lua_isnumber(valueObj))
		event->duration = (float)((double)luaFtol(lua_getnumber(valueObj)) * (double)kMillisecondsToSeconds);
}

void Lua_V2::readSayLineTable(lua_Object table, int *volume, int *pan, int *x, int *y) {
	int screenWidth = TextEvent::screenRect[2];
	lua_Object valueObj;

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldX]));

	valueObj = lua_gettable();

	if (lua_isnumber(valueObj) && *x)
		*x = truncateToInt((double)screenWidth * (double)lua_getnumber(valueObj) * (double)kHalf + (double)(screenWidth / 2));

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldY]));

	valueObj = lua_gettable();

	// The disasm uses the screen width for y too
	if (lua_isnumber(valueObj) && *y)
		*y = truncateToInt((double)screenWidth * (double)lua_getnumber(valueObj) * (double)kHalf + (double)(screenWidth / 2));

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldVolume]));

	valueObj = lua_gettable();

	if (lua_isnumber(valueObj) && *volume)
		*volume = luaFtol(lua_getnumber(valueObj));

	lua_pushobject(table);
	lua_pushobject(lua_getref(_fieldRefs[kFieldPan]));

	valueObj = lua_gettable();

	if (lua_isnumber(valueObj) && *pan)
		*pan = luaFtol(lua_getnumber(valueObj));
}

void Lua_V2::SetTextSpeed() {
	lua_Object speedObj = lua_getparam(1);

	if (lua_isnumber(speedObj))
		Actor::setActorTextRate(luaFtol(lua_getnumber(speedObj)));
}

void Lua_V2::GetTextSpeed() {
	lua_pushnumber((float)Actor::getActorTextRate());
}

void Lua_V2::IsMessageGoing() {
	lua_Object actorObj = lua_getparam(1);

	if (actorObj && !isActor(actorObj) && !lua_isnil(actorObj))
		return;

	if (isActor(actorObj)) {
		Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

		if (actor && actor->extra && actor->extra->talking) {
			lua_pushnumber(1.0f);

			return;
		}
	} else {
		if (Actor::getNumActorsTalking()) {
			lua_pushnumber(1.0f);

			return;
		}

		Common::List<TextEvent *>::iterator it;

		for (it = TextEvent::events.begin(); it != TextEvent::events.end(); ++it) {
			if (!((*it)->flags & 1)) {
				lua_pushnumber(1.0f);

				return;
			}
		}
	}

	lua_pushnil();
}

void Lua_V2::SetSayLineDefaults() {
	lua_Object tableObj = lua_getparam(1);

	if (tableObj && lua_istable(tableObj))
		applyTextTable(tableObj, TextEvent::defaultEvent);
}

void Lua_V2::SetActorTalkColor() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object colorObj = lua_getparam(2);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor) {
		if (gpConsoleServices)
			gpConsoleServices->logError("SetActorTalkColor: Actor is invalid.\n");
		else
			warning("SetActorTalkColor: Actor is invalid.");

		return;
	}

	if (!actor->extra)
		actor->allocateActorExtra();

	if (lua_isuserdata(colorObj) && lua_tag(colorObj) == kColorTag)
		actor->extra->talkColor = Color((uint)lua_getuserdata(colorObj));
}

void Lua_V2::GetActorTalkColor() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor) {
		if (gpConsoleServices)
			gpConsoleServices->logError("GetActorTalkColor: Actor is invalid.\n");
		else
			warning("GetActorTalkColor: Actor is invalid.");

		return;
	}

	if (!actor->extra) {
		lua_pushnil();

		return;
	}

	const Color &talkColor = actor->extra->talkColor;
	uint packed = (uint)talkColor.a;

	packed = (packed << 8) | (uint)talkColor.r;
	packed = (packed << 8) | (uint)talkColor.g;
	packed = (packed << 8) | (uint)talkColor.b;

	lua_pushusertag((int32)packed, kColorTag);
}

void Lua_V2::SayLine() {
	const char *text = nullptr;
	int volume = 100;
	int pan = 64;
	int x = -1;
	int y = -1;
	int index = 2;
	lua_Object lineObj = lua_getparam(1);

	if (!isActor(lineObj) && !lua_isstring(lineObj) && !lua_istable(lineObj))
		return;

	Actor *actor = Actor::getSelectedActor();

	if (isActor(lineObj)) {
		actor = Actor::actorDeref(lua_getuserdata(lineObj));
		index = 3;
		lineObj = lua_getparam(2);
	}

	if (!actor || lua_isnil(lineObj))
		return;

	while (lua_isstring(lineObj) || lua_isnumber(lineObj) || lua_istable(lineObj)) {
		if (lua_istable(lineObj))
			readSayLineTable(lineObj, &volume, &pan, &x, &y);
		else
			text = lua_getstring(lineObj);

		lineObj = lua_getparam(index);
		index++;

		if (lua_isnil(lineObj)) {
			if (text)
				actor->sayLine(text, volume, pan, x, y);

			return;
		}
	}
}

void Lua_V2::MakeTextObject() {
	lua_Object textObj = lua_getparam(1);
	char tag[TextEvent::kTagBufferSize];

	if (!lua_isstring(textObj))
		return;

	const char *translated = TextEvent::translate(lua_getstring(textObj), tag);
	TextEvent *event = new TextEvent(LString(translated));
	lua_Object tableObj = lua_getparam(2);

	if (tableObj && lua_istable(tableObj))
		applyTextTable(tableObj, event);

	lua_pushusertag(event->id, kTextTag);
	lua_pushnumber((float)event->getWidth());
	lua_pushnumber((float)event->getHeight());

	event->activate(0.0f, 1);
}

void Lua_V2::KillTextObject() {
	lua_Object textObj = lua_getparam(1);

	if (textObj && lua_isuserdata(textObj) && lua_tag(textObj) == kTextTag) {
		TextEvent *event = TextEvent::getEvent(lua_getuserdata(textObj));

		if (event)
			event->flags &= ~1u;
	}
}

void Lua_V2::ChangeTextObject() {
	lua_Object textObj = lua_getparam(1);
	int index = 2;

	if (!lua_isuserdata(textObj) || lua_tag(textObj) != kTextTag)
		return;

	TextEvent *event = TextEvent::getEvent(lua_getuserdata(textObj));

	if (!event) {
		if (gpConsoleServices)
			gpConsoleServices->logError("ChangeTextObject: text object is invalid");
		else
			warning("ChangeTextObject: text object is invalid");

		return;
	}

	for (int i = 0; i < 2; i++) {
		lua_Object paramObj = lua_getparam(index);

		index++;

		if (!paramObj)
			break;

		if (lua_isstring(paramObj)) {
			event->text = TextEvent::translate(lua_getstring(paramObj), nullptr);
			event->geometryReady = false;
		} else {
			if (!lua_istable(paramObj))
				break;

			applyTextTable(paramObj, event);
		}
	}

	lua_pushnumber((float)event->getWidth());
	lua_pushnumber((float)event->getHeight());
}

void Lua_V2::GetTextObjectDimensions() {
	lua_Object textObj = lua_getparam(1);

	if (!lua_isuserdata(textObj) || lua_tag(textObj) != kTextTag)
		return;

	TextEvent *event = TextEvent::getEvent(lua_getuserdata(textObj));

	if (!event) {
		if (gpConsoleServices)
			gpConsoleServices->logError("GetTextObjectDimensions: text object is invalid");
		else
			warning("GetTextObjectDimensions: text object is invalid");

		return;
	}

	int screenWidth = TextEvent::screenRect[2];

	lua_pushnumber((float)((double)event->getWidth() / (double)(screenWidth / 2)));

	int screenHeight = TextEvent::screenRect[3];

	lua_pushnumber((float)((double)event->getHeight() / (double)(screenHeight / 2)));
}

void Lua_V2::BlastText() {
	lua_Object textObj = lua_getparam(1);
	char tag[TextEvent::kTagBufferSize];

	if (!lua_isstring(textObj))
		return;

	const char *translated = TextEvent::translate(lua_getstring(textObj), tag);
	TextEvent *event = new TextEvent(LString(translated));
	lua_Object tableObj = lua_getparam(2);

	if (tableObj && lua_istable(tableObj))
		applyTextTable(tableObj, event);

	delete event;
}

void Lua_V2::ExpireText() {
	Common::List<TextEvent *>::iterator it = TextEvent::events.begin();

	while (it != TextEvent::events.end()) {
		TextEvent *event = *it;

		++it;

		event->duration = -1.0f;

		if (event->flags & 4) {
			event->flags &= ~5u;

			Actor *actor = Actor::actorDeref(event->actorId);

			if (actor && actor->extra)
				actor->extra->textEventId = 0;

			delete event;
		}
	}
}

void Lua_V2::PurgeText() {
	lua_Object keepHeldObj = lua_getparam(1);
	Common::List<TextEvent *>::iterator it;

	for (it = TextEvent::events.begin(); it != TextEvent::events.end(); ++it) {
		TextEvent *event = *it;

		if (lua_isnil(keepHeldObj) || !event->hold) {
			event->duration = -1.0f;
			event->flags &= ~1u;
		}
	}
}

void Lua_V2::PushText() {
	TextEvent::holdAll();
}

void Lua_V2::PopText() {
	TextEvent::releaseAll();
}

void Lua_V2::MakeColor() {
	lua_Object redObj = lua_getparam(1);
	lua_Object greenObj = lua_getparam(2);
	lua_Object blueObj = lua_getparam(3);
	lua_Object alphaObj = lua_getparam(4);
	int red;
	int green;
	int blue;
	int alpha;

	if (lua_isnumber(alphaObj)) {
		alpha = luaFtol(lua_getnumber(alphaObj));

		if (alpha < 0)
			alpha = 0;
		else if (alpha > 255)
			alpha = 255;
	} else {
		alpha = 255;
	}

	if (lua_isnumber(blueObj)) {
		blue = luaFtol(lua_getnumber(blueObj));

		if (blue < 0)
			blue = 0;
		else if (blue > 255)
			blue = 255;
	} else {
		blue = 0;
	}

	if (lua_isnumber(greenObj)) {
		green = luaFtol(lua_getnumber(greenObj));

		if (green < 0)
			green = 0;
		else if (green > 255)
			green = 255;
	} else {
		green = 0;
	}

	if (lua_isnumber(redObj)) {
		red = luaFtol(lua_getnumber(redObj));

		if (red < 0)
			red = 0;
		else if (red > 255)
			red = 255;
	} else {
		red = 0;
	}

	uint packed = (uint)alpha;

	packed = (packed << 8) | (uint)red;
	packed = (packed << 8) | (uint)green;
	packed = (packed << 8) | (uint)blue;

	lua_pushusertag((int32)packed, kColorTag);
}

void Lua_V2::GetColorComponents() {
	lua_Object colorObj = lua_getparam(1);
	Color color((uint)lua_getuserdata(colorObj));

	lua_pushnumber((float)(uint)color.r);
	lua_pushnumber((float)(uint)color.g);
	lua_pushnumber((float)(uint)color.b);
}

void Lua_V2::SetTranslationMode() {
	lua_Object modeObj = lua_getparam(1);

	if (lua_isnil(modeObj)) {
		TextEvent::textMode = 0;

		return;
	}

	if (!lua_isnumber(modeObj))
		return;

	int mode = luaFtol(lua_getnumber(modeObj));

	if (mode < 0)
		TextEvent::textMode = 0;
	else if (mode > 2)
		TextEvent::textMode = 2;
	else
		TextEvent::textMode = mode;
}

void Lua_V2::GetTranslationMode() {
	lua_pushnumber((float)TextEvent::textMode);
}

void Lua_V2::LocalizeString() {
	lua_Object textObj = lua_getparam(1);
	char tag[TextEvent::kTagBufferSize];

	memset(tag, 0, sizeof(tag));

	if (!lua_isstring(textObj))
		return;

	const char *translated = TextEvent::translate(lua_getstring(textObj), tag);
	Common::String result;

	if (tag[0])
		result = Common::String::format("/%s/%s", tag, translated);
	else
		result = translated;

	// The original could overflow its buffer here, let's avoid that...
	if (result.size() >= (uint)kLocalizedBufferSize) {
		warning("Lua_V2::LocalizeString(): the string is too long");

		result = result.substr(0, kLocalizedBufferSize - 1);
	}

	lua_pushstring(result.c_str());
}

void Lua_V2::GetFontDimensions() {
	lua_Object fontObj = lua_getparam(1);

	if (!lua_isstring(fontObj))
		return;

	LString fontName = lua_getstring(fontObj);
	LResourceHandle<CTextFont> fontHandle(fontName);
	CTextFont *font = nullptr;

	if (fontHandle.lock())
		font = fontHandle.typed;

	if (!font) {
		// The original reads the dimensions of the missing font
		warning("Lua_V2::GetFontDimensions(): the font %s cannot be loaded", fontName.c_str());
		fontHandle.unlock();

		return;
	}

	int lineHeight = font->lineHeight;
	int header52 = font->header52;
	int header56 = font->header56;
	int header60 = font->header60;

	fontHandle.unlock();

	lua_pushnumber((float)lineHeight);
	lua_pushnumber((float)header52);
	lua_pushnumber((float)header56);
	lua_pushnumber((float)header60);
}

void Lua_V2::GetTextCharPosition() {
	lua_Object textObj = lua_getparam(1);
	lua_Object countObj = lua_getparam(2);

	if (!lua_isuserdata(textObj) || lua_tag(textObj) != kTextTag)
		return;

	TextEvent *event = TextEvent::getEvent(lua_getuserdata(textObj));

	if (!event) {
		if (gpConsoleServices)
			gpConsoleServices->logError("GetTextCharPosition: text object is invalid");
		else
			warning("GetTextCharPosition: text object is invalid");

		return;
	}

	int count = luaFtol(lua_getnumber(countObj));
	int width = 0;

	for (int i = 0; i < count; i++) {
		if ((uint)i >= event->text.size())
			break;

		width += event->getCharWidth((byte)event->text[i]);
	}

	int screenWidth = TextEvent::screenRect[2];

	lua_pushnumber((float)((double)width / (double)(screenWidth / 2)));
}

void Lua_V2::SetOffscreenTextPos() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	float x;
	double y;

	if (lua_isnumber(xObj))
		x = lua_getnumber(xObj);
	else
		x = 0.0f;

	if (lua_isnumber(yObj))
		y = lua_getnumber(yObj);
	else
		y = 0.875;

	int screenWidth = TextEvent::screenRect[2];
	int screenHeight = TextEvent::screenRect[3];

	Actor::offscreenTextRect.x = truncateToInt((double)screenWidth * (double)x * (double)kHalf + (double)(screenWidth / 2));
	Actor::offscreenTextRect.y = truncateToInt((double)(screenHeight / 2) - (double)screenHeight * y * (double)kHalf);
}

} // end of namespace Grim
