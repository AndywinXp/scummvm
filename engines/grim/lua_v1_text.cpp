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

#include "common/savefile.h"
#include "common/system.h"

#include "engines/grim/grim.h"
#include "engines/grim/lua_v1.h"
#include "engines/grim/localize.h"
#include "engines/grim/actor.h"
#include "engines/grim/savegame.h"
#include "engines/grim/resource.h"
#include "engines/grim/inputdialog.h"
#include "engines/grim/blasttext.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/text.h"
#include "engines/grim/textobject.h"
#include "engines/grim/lua/lauxlib.h"

namespace Grim {

SputTextEvent *Lua_V1::getTextEvent(lua_Object obj) {
	SputTextEvent *pEvent = g_grim->_pSputText->findSerialNumber(lua_getuserdata(obj));

	if (!pEvent)
		warning("Lua_V1::getTextEvent(): the text object does not exist");

	return pEvent;
}

/* Destroy a text object since we don't need it anymore
 * note that the menu creates more objects than it needs,
 * so it deletes some objects right after creating them
 */
void Lua_V1::KillTextObject() {
	lua_Object textObj = lua_getparam(1);

	if (textObj && lua_isuserdata(textObj) && lua_tag(textObj) == MKTAG('T', 'X', 'E', 'T')) {
		SputTextEvent *pEvent = getTextEvent(textObj);

		if (pEvent)
			pEvent->flags &= ~1;
	}
}

/* Make changes to a text object based on the parameters passed
 * in the table in the LUA parameter 2.
 */
void Lua_V1::ChangeTextObject() {
	lua_Object textObj = lua_getparam(1);
	if (lua_isuserdata(textObj) && lua_tag(textObj) == MKTAG('T', 'X', 'E', 'T')) {
		SputTextEvent *pEvent = getTextEvent(textObj);
		SputText *pSputText = g_grim->_pSputText;
		int paramId = 2;

		if (!pEvent)
			return;

		for (int i = 0; i < 2; i++) {
			lua_Object paramObj = lua_getparam(paramId++);
			if (!paramObj)
				break;

			if (lua_isstring(paramObj)) {
				const char *pText = pSputText->extractSayLineData(lua_getstring(paramObj), nullptr);

				Common::strlcpy(pEvent->text, pText, sizeof(pEvent->text));
			} else {
				if (!lua_istable(paramObj))
					break;
				pSputText->getTextObjectParams(&pEvent->params, paramObj, pEvent);
			}
		}

		lua_pushnumber((float)g_grim->_pText->getStringWidth(&pEvent->params, pEvent->text));
		lua_pushnumber((float)g_grim->_pText->getStringHeight(&pEvent->params, pEvent->text));
	}
}

void Lua_V1::GetTextSpeed() {
	lua_pushnumber((float)g_grim->_pSputActor->getActorTextRate());
}

void Lua_V1::SetTextSpeed() {
	lua_Object speedObj = lua_getparam(1);
	if (!lua_isnumber(speedObj))
		return;

	g_grim->_pSputActor->setActorTextRate(luaFtol(lua_getnumber(speedObj)));
}

void Lua_V1::MakeTextObject() {
	SputText *pSputText = g_grim->_pSputText;
	StdRect clip;

	clip.x = 0;
	clip.y = 0;
	clip.width = g_grim->_pBlastText->getSurface()->width;
	clip.height = g_grim->_pBlastText->getSurface()->height;

	lua_Object textObj = lua_getparam(1);
	if (!lua_isstring(textObj)) {
		return;
	}

	const char *pMsgId = lua_getstring(textObj);
	TextParams params = pSputText->_blastTextParams;

	params.pClip = &clip;

	lua_Object tableObj = lua_getparam(2);
	if (tableObj && lua_istable(tableObj))
		pSputText->getTextObjectParams(&params, tableObj, nullptr);

	char translation[32];
	const char *pText = pSputText->extractSayLineData(pMsgId, translation);
	SputTextEvent *pEvent = pSputText->allocEvent(&params, pText);

	lua_pushusertag(pEvent->serial, MKTAG('T', 'X', 'E', 'T'));
	lua_pushnumber((float)g_grim->_pText->getStringWidth(&pEvent->params, pEvent->text));
	lua_pushnumber((float)g_grim->_pText->getStringHeight(&pEvent->params, pEvent->text));
	pSputText->addToQueue(pEvent, 0, 1);
}

void Lua_V1::GetTextObjectDimensions() {
	lua_Object textObj = lua_getparam(1);

	if (lua_isuserdata(textObj) && lua_tag(textObj) == MKTAG('T', 'X', 'E', 'T')) {
		SputTextEvent *pEvent = getTextEvent(textObj);

		if (pEvent) {
			lua_pushnumber((float)g_grim->_pText->getStringWidth(&pEvent->params, pEvent->text));
			lua_pushnumber((float)g_grim->_pText->getStringHeight(&pEvent->params, pEvent->text));
		}
	}
}

void Lua_V1::ExpireText() {
	SputTextEvent *pEvent = g_grim->_pSputText->_pTextQueue;

	while (pEvent) {
		SputTextEvent *pNext = pEvent->pNext;

		if (pEvent->flags & 4) {
			SputActor *pActor = pEvent->pActor;

			g_grim->_pSputText->killEvent(pEvent);

			if (pActor && pActor->pExtra)
				pActor->pExtra->pSayText = nullptr;
		} else {
			pEvent->duration = -1;
		}

		pEvent = pNext;
	}
}

void Lua_V1::PurgeText() {
	SputTextEvent *pEvent = g_grim->_pSputText->_pTextQueue;

	while (pEvent) {
		pEvent->duration = -1;
		pEvent->flags &= ~1;
		pEvent = pEvent->pNext;
	}
}

void Lua_V1::GetTextCharPosition() {
	lua_Object textObj = lua_getparam(1);
	lua_Object posObj = lua_getparam(2);
	if (lua_isuserdata(textObj) && lua_tag(textObj) == MKTAG('T', 'X', 'E', 'T')) {
		SputTextEvent *pEvent = getTextEvent(textObj);
		int pos = luaFtol(lua_getnumber(posObj));
		int width = 0;

		if (!pEvent)
			return;

		if (pos > 0) {
			const char *pText = pEvent->text;

			do {
				char c = *pText;

				if (!c)
					break;

				pText++;
				width += g_grim->_pText->getCharKernedWidth(&pEvent->params, c);
			} while ((int)(pText - pEvent->text) < pos);
		}

		lua_pushnumber((float)width);
	}
}

void Lua_V1::BlastText() {
	SputText *pSputText = g_grim->_pSputText;
	StdRect clip;

	clip.x = 0;
	clip.y = 0;
	clip.width = g_grim->_pBlastText->getSurface()->width;
	clip.height = g_grim->_pBlastText->getSurface()->height;

	lua_Object textObj = lua_getparam(1);
	if (!lua_isstring(textObj)) {
		return;
	}

	const char *pMsgId = lua_getstring(textObj);

	g_grim->_pStdDisplay->vBufferLock(g_grim->_pSputRender->drawBuffer());

	TextParams params = pSputText->_blastTextParams;

	params.pSurface->pPixels = g_grim->_pSputRender->drawBuffer()->pPixels;
	params.pClip = &clip;

	lua_Object tableObj = lua_getparam(2);
	if (tableObj && lua_istable(tableObj))
		pSputText->getTextObjectParams(&params, tableObj, nullptr);

	char translation[32];
	const char *pText = pSputText->extractSayLineData(pMsgId, translation);
	SputTextEvent *pEvent = pSputText->allocEvent(&params, pText);

	pSputText->drawTextEvent(pEvent, 0);
	g_grim->_pStdDisplay->vBufferUnlock(g_grim->_pSputRender->drawBuffer());
	pSputText->killEvent(pEvent);
}

void Lua_V1::SetOffscreenTextPos() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	SputText *pSputText = g_grim->_pSputText;

	if (lua_isnumber(xObj))
		pSputText->_centerValue = luaFtol(lua_getnumber(xObj));
	else
		pSputText->_centerValue = 320;

	if (lua_isnumber(yObj))
		pSputText->_offscreenTextPos = luaFtol(lua_getnumber(yObj));
	else
		pSputText->_offscreenTextPos = 450;
}

void Lua_V1::TextFileGetLine() {
	char textBuf[512];
	lua_Object nameObj = lua_getparam(1);
	lua_Object posObj = lua_getparam(2);
	Common::SeekableReadStream *file;

	if (lua_isnil(nameObj) || lua_isnil(posObj)) {
		lua_pushnil();
		return;
	}

	const char *filename = lua_getstring(nameObj);
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();
	file = saveFileMan->openForLoading(filename);
	if (!file) {
		lua_pushnil();
		return;
	}

	int pos = luaFtol(lua_getnumber(posObj));

	if (pos >= 0)
		file->seek(pos, SEEK_SET);

	memset(textBuf, 0, sizeof(textBuf));
	file->readLine(textBuf, sizeof(textBuf));
	delete file;

	lua_pushstring(textBuf);
}

void Lua_V1::TextFileGetLineCount() {
	char textBuf[512];
	lua_Object nameObj = lua_getparam(1);

	if (lua_isnil(nameObj)) {
		lua_pushnil();
		return;
	}

	const char *filename = luaL_check_string(1);
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();
	Common::SeekableReadStream *file = saveFileMan->openForLoading(filename);
	if (!file) {
		lua_pushnil();
		return;
	}

	lua_Object result = lua_createtable();

	int line = 0;
	for (;;) {
		if (file->eos())
			break;
		lua_pushobject(result);
		lua_pushnumber((float)line);
		lua_pushnumber((float)file->pos());
		lua_settable();
		file->readLine(textBuf, sizeof(textBuf));
		line++;
	}
	delete file;

	lua_pushobject(result);
	lua_pushstring("count");
	lua_pushnumber((float)line);
	lua_settable();
	lua_pushobject(result);
}

// Localization function

void Lua_V1::LocalizeString() {
	char translation[32];
	char buf[640];
	lua_Object strObj = lua_getparam(1);

	if (lua_isstring(strObj)) {
		const char *pText = g_grim->_pSputText->extractSayLineData(lua_getstring(strObj), translation);

		Common::sprintf_s(buf, "/%s/%s", translation, pText);
		lua_pushstring(buf);
	}
}

void Lua_V1::SetSayLineDefaults() {
	lua_Object tableObj = lua_getparam(1);
	if (tableObj && lua_istable(tableObj))
		g_grim->_pSputText->getTextObjectParams(g_grim->_pSputText->getDefaultTextParams(), tableObj, nullptr);
}

void Lua_V1::SayLine() {
	SputActorModule *pSputActor = g_grim->_pSputActor;
	SputText *pSputText = g_grim->_pSputText;
	int vol = 127, pan = 64, paramId = 2;
	int x = -1, y = -1;
	int background = 1;
	const char *msgId = nullptr;
	lua_Object paramObj = lua_getparam(1);

	if ((lua_isuserdata(paramObj) && lua_tag(paramObj) == MKTAG('R','T','C','A'))
			|| lua_isstring(paramObj) || lua_istable(paramObj)) {
		SputActor *actor = pSputActor->getSelectedActor();
		if (lua_isuserdata(paramObj) && lua_tag(paramObj) == MKTAG('R','T','C','A')) {
			actor = pSputActor->actorDeref(lua_getuserdata(paramObj));
			paramId = 3;
			paramObj = lua_getparam(2);
		}
		if (actor) {
			while (!lua_isnil(paramObj)) {
				if (!lua_isstring(paramObj) && !lua_isnumber(paramObj) && !lua_istable(paramObj))
					return;
				if (lua_istable(paramObj)) {
					pSputText->parseSayLineTable(paramObj, &background, &vol, &pan, &x, &y);
				} else if (lua_isnumber(paramObj)) {
					background = 0;
				} else {
					char translation[32];

					msgId = lua_getstring(paramObj);
					pSputText->extractSayLineData(msgId, translation);
				}
				paramObj = lua_getparam(paramId++);
			}
			if (msgId)
				pSputActor->actorPlayVoiceDrawText(actor, msgId, background, vol, pan, x, y);
		}
	}
}

void Lua_V1::SayLineDemo() {
	SputActorModule *pSputActor = g_grim->_pSputActor;
	lua_Object paramObj = lua_getparam(1);

	if (!(lua_isuserdata(paramObj) && lua_tag(paramObj) == MKTAG('R','T','C','A')) && !lua_isstring(paramObj) && !lua_isnumber(paramObj)) {
		warning("SayLine: Invalid argument");
		return;
	}

	SputActor *actor = pSputActor->getSelectedActor();

	if (lua_isuserdata(paramObj) && lua_tag(paramObj) == MKTAG('R','T','C','A')) {
		actor = pSputActor->actorDeref(lua_getuserdata(paramObj));
		paramObj = lua_getparam(2);
	}

	if (!actor) {
		warning("SayLine: Invalid actor handle");
		return;
	}

	if (!lua_isstring(paramObj)) {
		warning("SayLine: Invalid message");
		return;
	}

	char translation[32];
	const char *msgId = lua_getstring(paramObj);

	if (!g_grim->_pSputText->extractSayLineData(msgId, translation))
		warning("SayLine: Invalid message format");

	pSputActor->sayLineDemo(actor, msgId);
}

void Lua_V1::ShutUpActorDemo() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (pActor)
		g_grim->_pSputActor->shutUpDemo(pActor);
}

void Lua_V1::ShutUpActor() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (pActor)
		g_grim->_pSputActor->shutUp(pActor);
}

void Lua_V1::PrintLine() {
	SputText *pSputText = g_grim->_pSputText;
	BlastTextSurface *pSurface = g_grim->_pBlastText->getSurface();
	int duration = -1;
	lua_Object param1Obj = lua_getparam(1);
	lua_Object param2Obj = lua_getparam(2);

	if (pSputText->_printLineClip.width >= pSurface->width)
		pSputText->_printLineClip.width = pSurface->width;

	if (pSputText->_printLineClip.height >= pSurface->height)
		pSputText->_printLineClip.height = pSurface->height;

	if ((lua_isstring(param1Obj) || lua_isnil(param1Obj)) && (lua_istable(param2Obj) || lua_isnil(param2Obj))) {
		TextParams localParams;
		TextParams *pParams;

		if (lua_isnil(param1Obj)) {
			pParams = &pSputText->_printLineParams;
		} else {
			localParams = pSputText->_printLineParams;
			pParams = &localParams;
		}

		if (lua_istable(param2Obj))
			duration = pSputText->getTextObjectParams(pParams, param2Obj, nullptr);

		if (lua_isstring(param1Obj)) {
			char translation[32];
			const char *pText = pSputText->extractSayLineData(lua_getstring(param1Obj), translation);

			if (duration < 0)
				duration = (int)((double)(40 + 3 * (int)strlen(pText)) * 16.6666679f);

			SputTextEvent *pEvent = pSputText->allocEvent(pParams, pText);

			pSputText->addToQueue(pEvent, duration, 0);
		}
	}
}

void Lua_V1::InputDialog() {
	lua_Object titleObj = lua_getparam(1);
	lua_Object messageObj = lua_getparam(2);
	lua_Object defaultObj = lua_getparam(3);

	if (!lua_isstring(titleObj) || !lua_isstring(messageObj) || (!lua_isnil(defaultObj) && !lua_isstring(defaultObj))) {
		lua_pushnil();
		return;
	}

	Common::String str = lua_getstring(titleObj);
	str += ": ";
	str += lua_getstring(messageObj);

	Common::String defaultString;

	if (lua_isstring(defaultObj))
		defaultString = lua_getstring(defaultObj);

	Grim::InputDialog d(str, defaultString);
	int res = d.runModal();
	// The KeyUp event for CTRL has been eat by the gui loop, so we
	// need to reset it manually.
	g_grim->clearEventQueue();
	if (res) {
		Common::String text = d.getString();

		if (text.size() > 100)
			text = text.substr(0, 100);

		lua_pushstring(text.c_str());
	} else {
		lua_pushnil();
	}
}

void Lua_V1::IsMessageGoing() {
	lua_Object actorObj = lua_getparam(1);

	if (!actorObj || (lua_isuserdata(actorObj) && lua_tag(actorObj) == MKTAG('R','T','C','A')) || lua_isnil(actorObj)) {
		if (lua_isuserdata(actorObj) && lua_tag(actorObj) == MKTAG('R','T','C','A')) {
			SputActor *actor = g_grim->_pSputActor->actorDeref(lua_getuserdata(actorObj));
			if (actor && actor->pExtra && actor->pExtra->talking) {
				lua_pushnumber(1.0f);
				return;
			}
		} else {
			SputTextEvent *pEvent = g_grim->_pSputText->_pTextQueue;

			if (g_grim->_pSputActor->getNumActorsTalking()) {
				lua_pushnumber(1.0f);
				return;
			}

			while (pEvent) {
				if (!(pEvent->flags & 1)) {
					lua_pushnumber(1.0f);
					return;
				}

				pEvent = pEvent->pNext;
			}
		}
		lua_pushnil();
	}
}

void Lua_V1::GetTranslationMode() {
	lua_pushnumber((float)g_grim->_pSputText->_translationMode);
}

void Lua_V1::SetTranslationMode() {
	lua_Object paramObj = lua_getparam(1);

	if (lua_isnil(paramObj)) {
		g_grim->_pSputText->_translationMode = 0;
	} else if (lua_isnumber(paramObj)) {
		int mode = luaFtol(lua_getnumber(paramObj));

		if (mode < 0)
			mode = 0;
		else if (mode > 2)
			mode = 2;

		g_grim->_pSputText->_translationMode = mode;
	}
}

} // end of namespace Grim
