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

#include "common/str.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/lua/lua.h"
#include "engines/grim/normaltext.h"
#include "engines/grim/res.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/text.h"

namespace Grim {

static uint16 colorToRgb565(int color) {
	uint value = (uint)color;

	return (uint16)(((value >> 19) & 0x1F) | (32 * (((uint16)(value >> 3) << 6) | ((value >> 10) & 0x3F))));
}

SputText::SputText(Res *pRes, RenderDroid *pRd, NormalText *pNormalText, Text *pText, BlastText *pBlastText) : _pRes(pRes), _pRd(pRd),
		_pNormalText(pNormalText), _pText(pText), _pBlastText(pBlastText), _pSputRender(nullptr), _pSputRoom(nullptr),
		_pHS(nullptr), _bInitialized(0), _bStarted(0), _bLocked(0), _pTextQueue(nullptr), _pFont(nullptr), _pEmergencyFont(nullptr),
		_translationMode(0), _centerValue(320), _offscreenTextPos(450), _textPosX(0), _textPosY(0),
		_luaTextObjXRef(0), _luaTextObjYRef(0), _luaTextObjFontRef(0), _luaTextObjWidthRef(0), _luaTextObjHeightRef(0),
		_luaTextObjFGColorRef(0), _luaTextObjBGColorRef(0), _luaTextObjFXColorRef(0), _luaTextObjHIColorRef(0),
		_luaTextObjDurationRef(0), _luaTextObjCenterRef(0), _luaTextObjLJustifyRef(0), _luaTextObjRJustifyRef(0),
		_luaTextObjVolumeRef(0), _luaTextObjBufferRef(0), _luaTextObjBackgroundRef(0) {
	_printLineClip.x = 0;
	_printLineClip.y = 0;
	_printLineClip.width = 640;
	_printLineClip.height = 480;

	_printLineParams.pSurface = _pBlastText->getSurface();
	_printLineParams.pClip = &_printLineClip;
	_printLineParams.pFont = nullptr;
	_printLineParams.x = 0;
	_printLineParams.y = 100;
	_printLineParams.fgColor = 0;
	_printLineParams.bgColor = 0;
	_printLineParams.fxColor = 0;
	_printLineParams.scaleX = 0x10000;
	_printLineParams.scaleY = 0x10000;
	_printLineParams.flags = 4;
	_printLineParams.pColorMap = nullptr;

	_defaultTextParams.pSurface = _pBlastText->getSurface();
	_defaultTextParams.pClip = nullptr;
	_defaultTextParams.pFont = nullptr;
	_defaultTextParams.x = 0;
	_defaultTextParams.y = 100;
	_defaultTextParams.fgColor = 0;
	_defaultTextParams.bgColor = 0;
	_defaultTextParams.fxColor = 0;
	_defaultTextParams.scaleX = 0x10000;
	_defaultTextParams.scaleY = 0x10000;
	_defaultTextParams.flags = 5;
	_defaultTextParams.pColorMap = nullptr;

	_blastTextParams.pSurface = _pBlastText->getSurface();
	_blastTextParams.pClip = nullptr;
	_blastTextParams.pFont = nullptr;
	_blastTextParams.x = 0;
	_blastTextParams.y = 200;
	_blastTextParams.fgColor = 10;
	_blastTextParams.bgColor = 0;
	_blastTextParams.fxColor = 0;
	_blastTextParams.scaleX = 0x10000;
	_blastTextParams.scaleY = 0x10000;
	_blastTextParams.flags = 4;
	_blastTextParams.pColorMap = nullptr;
}

void SputText::killEvent(SputTextEvent *pEvent) {
	if (pEvent) {
		if (pEvent->pNext)
			pEvent->pNext->pPrev = pEvent->pPrev;

		if (pEvent->pPrev)
			pEvent->pPrev->pNext = pEvent->pNext;

		if (pEvent == _pTextQueue)
			_pTextQueue = pEvent->pNext;

		delete pEvent;
	}
}

void SputText::update(int bForce) {
	if (!_bLocked) {
		int frameTime = 0; // TODO: sputTime_GetFrameTime()
		SputTextEvent *pEvent = _pTextQueue;

		while (pEvent) {
			if (pEvent->duration > 0 || (pEvent->flags & 1)) {
				int flags = pEvent->flags;

				if (((flags & 8) || bForce) && !(flags & 2))
					drawTextEvent(pEvent, 1);

				pEvent->duration -= frameTime;
				pEvent = pEvent->pNext;
			} else {
				SputTextEvent *pNext = pEvent->pNext;

				killEvent(pEvent);
				pEvent = pNext;
			}
		}
	}
}

int SputText::initialize(SysStruct *pHS) {
	_pHS = pHS;

	if (_pNormalText->initialize(pHS))
		return 1;

	int result = _pText->sysInitialize(pHS);

	if (result)
		return 1;

	_bInitialized = 1;

	return result;
}

void SputText::pause() {
	_bLocked = 1;
}

void SputText::resume() {
	_bLocked = 0;
}

SputTextEvent *SputText::createActorText(SputActor *pActor, const char *pText, int x, int y) {
	BlastTextSurface *pSurface = _pBlastText->getSurface();
	StdRect clip;
	StdRect offscreenRect;
	TextParams params = _defaultTextParams;

	if (!pActor || !pActor->pExtra || !params.pFont) {
		warning("SputText::createActorText(): no actor, actor extra data or font");
		return nullptr;
	}

	clip.x = 0;
	clip.y = 0;
	clip.width = pSurface->width;
	clip.height = pSurface->height;
	offscreenRect.x = _centerValue;
	offscreenRect.y = _offscreenTextPos;
	offscreenRect.width = _textPosX;
	offscreenRect.height = _textPosY;

	params.pClip = &clip;
	params.fgColor = pActor->pExtra->talkColor;
	params.bgColor = 0;
	params.fxColor = 0;

	int mode = 2; // TODO: grimlib_mode
	StdRect *pRect;

	if (!pActor->onScreen || mode == 3) {
		pRect = &offscreenRect;

		if (!pActor->pCostumeStack || (pActor->flags & 2)) {
			if (!scumm_stricmp(pActor->curSetName, _pSputRoom->getCurrentRoomName())) {
				int screen[3];

				if (_pRd->_primit3.getScreenCoord(pActor->actorPos, screen)) {
					offscreenRect.x = screen[0];
					offscreenRect.y = screen[1];
				}
			}
		}
	} else {
		pRect = &pActor->drawnRect;
	}

	int halfWidth = _pText->getStringWidth(&params, pText) / 2;
	int penX = x;

	if (x == -1)
		penX = pRect->x + pRect->width / 2;

	if (penX - halfWidth < 40)
		penX = halfWidth + 40;

	if (halfWidth + penX > pSurface->width - 40)
		penX = pSurface->width - halfWidth - 40;

	params.x = penX;
	params.y = params.pFont->baseLine;

	int height = _pText->getStringHeight(&params, pText);
	int penY = y;

	if (y == -1)
		penY = pRect->y - height;

	if (penY < 10)
		penY = 10;

	if (penY + height > pSurface->height - 10)
		penY = pSurface->height - height - 10;

	params.x = penX;
	params.y = params.pFont->baseLine + penY;

	SputTextEvent *pEvent = allocEvent(&params, pText);

	pEvent->pActor = pActor;
	pActor->flags |= 4;

	return pEvent;
}

SputTextEvent *SputText::addToQueue(SputTextEvent *pEvent, int duration, int flags) {
	if (pEvent) {
		if (!pEvent->params.pFont) {
			warning("SputText::addToQueue(): the event has no font");
			return pEvent;
		}

		pEvent->duration = duration;
		pEvent->flags = flags;
		pEvent->x = pEvent->params.x;

		int y = pEvent->params.y - pEvent->params.pFont->baseLine;

		pEvent->y = y;
		pEvent->params.y = y;
		pEvent->pNext = _pTextQueue;

		if (_pTextQueue)
			_pTextQueue->pPrev = pEvent;

		_pTextQueue = pEvent;
	}

	return pEvent;
}

int SputText::release() {
	_pHS = nullptr;
	_pNormalText->terminate();
	_pText->sysTerminate();

	int result = _bInitialized;

	if (_bInitialized)
		_bInitialized = 0;

	return result;
}

int SputText::startup() {
	if (_bStarted || _pNormalText->startup() || _pText->sysStartup())
		return 1;

	_pFont = _pText->getSystemFont();
	_printLineParams.pFont = _pFont;
	_defaultTextParams.pFont = _pFont;
	_blastTextParams.pFont = _pFont;

	// TODO: sputLua_RegisterFunctions(textFunc_table)

	lua_pushstring("x");
	_luaTextObjXRef = lua_ref(1);
	lua_pushstring("y");
	_luaTextObjYRef = lua_ref(1);
	lua_pushstring("font");
	_luaTextObjFontRef = lua_ref(1);
	lua_pushstring("width");
	_luaTextObjWidthRef = lua_ref(1);
	lua_pushstring("height");
	_luaTextObjHeightRef = lua_ref(1);
	lua_pushstring("fgcolor");
	_luaTextObjFGColorRef = lua_ref(1);
	lua_pushstring("bgcolor");
	_luaTextObjBGColorRef = lua_ref(1);
	lua_pushstring("fxcolor");
	_luaTextObjFXColorRef = lua_ref(1);
	lua_pushstring("hicolor");
	_luaTextObjHIColorRef = lua_ref(1);
	lua_pushstring("duration");
	_luaTextObjDurationRef = lua_ref(1);
	lua_pushstring("center");
	_luaTextObjCenterRef = lua_ref(1);
	lua_pushstring("ljustify");
	_luaTextObjLJustifyRef = lua_ref(1);
	lua_pushstring("rjustify");
	_luaTextObjRJustifyRef = lua_ref(1);
	lua_pushstring("volume");
	_luaTextObjVolumeRef = lua_ref(1);
	lua_pushstring("pan");
	_luaTextObjBufferRef = lua_ref(1);
	lua_pushstring("background");
	_luaTextObjBackgroundRef = lua_ref(1);

	_bStarted = 1;

	return 0;
}

int SputText::shutdown() {
	_pNormalText->shutdown();
	_pText->sysShutdown();

	lua_unref(_luaTextObjXRef);
	lua_unref(_luaTextObjYRef);
	lua_unref(_luaTextObjFontRef);
	lua_unref(_luaTextObjWidthRef);
	lua_unref(_luaTextObjHeightRef);
	lua_unref(_luaTextObjFGColorRef);
	lua_unref(_luaTextObjBGColorRef);
	lua_unref(_luaTextObjFXColorRef);
	lua_unref(_luaTextObjHIColorRef);
	lua_unref(_luaTextObjDurationRef);
	lua_unref(_luaTextObjCenterRef);
	lua_unref(_luaTextObjLJustifyRef);
	lua_unref(_luaTextObjRJustifyRef);
	lua_unref(_luaTextObjVolumeRef);
	lua_unref(_luaTextObjBufferRef);
	lua_unref(_luaTextObjBackgroundRef);

	while (_pTextQueue)
		killEvent(_pTextQueue);

	_bStarted = 0;

	return 0;
}

SputTextEvent *SputText::allocEvent(const TextParams *pParams, const char *pText) {
	SputTextEvent *pEvent = new SputTextEvent();
	BlastTextSurface *pSurface = _pBlastText->getSurface();

	if (!pText) {
		warning("SputText::allocEvent(): no text");
		pText = "";
	}

	Common::strlcpy(pEvent->text, pText, sizeof(pEvent->text));
	pEvent->x = pParams->x;
	pEvent->y = pParams->y;
	pEvent->params = *pParams;

	if (_pEmergencyFont)
		pEvent->params.pFont = _pEmergencyFont;

	pEvent->params.pColorMap = (byte *)_pSputRender->getColors();
	pEvent->params.pClip = &pEvent->clip;

	if (pParams->pClip) {
		pEvent->clip = *pParams->pClip;
	} else {
		pEvent->clip.x = 0;
		pEvent->clip.y = 0;
		pEvent->clip.width = pSurface->width;
		pEvent->clip.height = pSurface->height;
	}

	pEvent->pActor = nullptr;
	pEvent->serial = (int)(intptr)pEvent;

	return pEvent;
}

const char *SputText::extractSayLineData(const char *msgId, char *translation) {
	if (translation)
		*translation = 0;

	const char *pTranslated = msgId; // TODO: polyglot_getTranslation(msgId)
	const char *pResult;

	if (translation && msgId && *msgId == '/') {
		const char *pSlash = strchr(msgId + 1, '/');

		pResult = pSlash;

		if (pSlash) {
			int length = pSlash - msgId - 1;

			if (length > 31) {
				warning("SputText::extractSayLineData(): the voice name is too long, truncated");
				length = 31;
			}

			strncpy(translation, msgId + 1, length);
			translation[length] = 0;
		}
	} else {
		pResult = pTranslated;
	}

	if (_translationMode == 1) {
		if (!pResult) {
			warning("SputText::extractSayLineData(): the message has no closing slash");
			return pTranslated;
		}

		return pResult + 1;
	}

	if (_translationMode == 2)
		return msgId;

	return pTranslated;
}

void SputText::drawTextEvent(SputTextEvent *pEvent, int bMarkDirty) {
	if (!pEvent)
		return;

	if (!pEvent->params.pFont) {
		warning("SputText::drawTextEvent(): the event has no font");
		return;
	}

	StdRect rect;
	uint16 *pColors = _pSputRender->getColors();
	int baseLine = pEvent->params.pFont->baseLine;

	pEvent->params.x = pEvent->x;
	rect.x = pEvent->x;
	pEvent->params.y = pEvent->y + baseLine;
	rect.y = pEvent->params.y - baseLine;
	rect.width = _pText->getStringWidth(&pEvent->params, pEvent->text);
	rect.height = _pText->getStringHeight(&pEvent->params, pEvent->text);

	if (pEvent->params.flags & 1)
		rect.x += rect.width / -2;

	int foreground = pEvent->params.fgColor;
	int background = pEvent->params.bgColor;
	int effect = pEvent->params.fxColor;

	pColors[0] = colorToRgb565(foreground);
	pColors[1] = colorToRgb565(background);
	pColors[2] = colorToRgb565(effect);
	_pSputRender->colorCorrect(pColors, 3);

	pEvent->params.fgColor = 0;
	pEvent->params.bgColor = 1;
	pEvent->params.fxColor = 2;

	_pText->printText(&pEvent->params, "%s", pEvent->text);

	pEvent->params.x = pEvent->x;
	pEvent->params.y = pEvent->y + pEvent->params.pFont->baseLine;

	if (bMarkDirty)
		_pSputRender->markDirtyRect(&rect, 0);

	pEvent->params.fxColor = effect;
	pEvent->params.bgColor = background;
	pEvent->params.fgColor = foreground;
}

SputTextEvent *SputText::findSerialNumber(int serial) {
	SputTextEvent *pEvent = _pTextQueue;

	while (pEvent) {
		if (pEvent->serial == serial)
			return pEvent;

		pEvent = pEvent->pNext;
	}

	return nullptr;
}

int SputText::save(Common::WriteStream *pStream) {
	int numEvents = 0;

	for (SputTextEvent *pEvent = _pTextQueue; pEvent; pEvent = pEvent->pNext)
		numEvents++;

	pStream->writeSint32LE(numEvents);

	if (pStream->err())
		return 1;

	for (SputTextEvent *pEvent = _pTextQueue; pEvent; pEvent = pEvent->pNext) {
		pEvent->serial = (int)(intptr)pEvent;

		pStream->write(pEvent->text, 512);
		pStream->writeSint32LE(0);
		pStream->writeSint32LE(0);
		pStream->writeSint32LE((int)(intptr)pEvent->params.pFont);
		pStream->writeSint32LE(pEvent->params.x);
		pStream->writeSint32LE(pEvent->params.y);
		pStream->writeSint32LE(pEvent->params.fgColor);
		pStream->writeSint32LE(pEvent->params.bgColor);
		pStream->writeSint32LE(pEvent->params.fxColor);
		pStream->writeSint32LE(pEvent->params.scaleX);
		pStream->writeSint32LE(pEvent->params.scaleY);
		pStream->writeSint32LE(pEvent->params.flags);
		pStream->writeSint32LE(pEvent->x);
		pStream->writeSint32LE(pEvent->y);
		pStream->writeSint32LE(pEvent->clip.x);
		pStream->writeSint32LE(pEvent->clip.y);
		pStream->writeSint32LE(pEvent->clip.width);
		pStream->writeSint32LE(pEvent->clip.height);
		pStream->writeSint32LE(pEvent->duration);
		pStream->writeSint32LE(0);
		pStream->writeSint32LE(pEvent->flags);
		pStream->writeSint32LE(pEvent->serial);
	}

	pStream->writeSint32LE((int)(intptr)_printLineParams.pFont);
	pStream->writeSint32LE((int)(intptr)_defaultTextParams.pFont);
	pStream->writeSint32LE((int)(intptr)_blastTextParams.pFont);

	if (pStream->err()) {
		warning("SputText::save(): write error");
		return 1;
	}

	return 0;
}

int SputText::restore(Common::ReadStream *pStream) {
	while (_pTextQueue)
		killEvent(_pTextQueue);

	int numEvents = pStream->readSint32LE();

	if (pStream->err() || pStream->eos() || numEvents < 0) {
		warning("SputText::restore(): invalid event count");
		return 1;
	}

	for (int i = 0; i < numEvents; i++) {
		SputTextEvent *pEvent = new SputTextEvent();

		pStream->read(pEvent->text, 512);

		pStream->readSint32LE();
		pStream->readSint32LE();
		int fontSerial = pStream->readSint32LE();

		pEvent->params.x = pStream->readSint32LE();
		pEvent->params.y = pStream->readSint32LE();
		pEvent->params.fgColor = pStream->readSint32LE();
		pEvent->params.bgColor = pStream->readSint32LE();
		pEvent->params.fxColor = pStream->readSint32LE();
		pEvent->params.scaleX = pStream->readSint32LE();
		pEvent->params.scaleY = pStream->readSint32LE();
		pEvent->params.flags = pStream->readSint32LE();
		pEvent->x = pStream->readSint32LE();
		pEvent->y = pStream->readSint32LE();
		pEvent->clip.x = pStream->readSint32LE();
		pEvent->clip.y = pStream->readSint32LE();
		pEvent->clip.width = pStream->readSint32LE();
		pEvent->clip.height = pStream->readSint32LE();
		pEvent->duration = pStream->readSint32LE();

		pStream->readSint32LE();

		pEvent->flags = pStream->readSint32LE();
		pEvent->serial = pStream->readSint32LE();

		if (pStream->err() || pStream->eos()) {
			warning("SputText::restore(): read error");
			delete pEvent;
			pEvent = nullptr;

			return 1;
		}

		if (pEvent->text[511] != 0) {
			warning("SputText::restore(): unterminated event text");
			pEvent->text[511] = 0;
		}

		pEvent->params.pFont = static_cast<BlastTextFont *>(_pRes->handleFromSerialNumber(fontSerial));

		pEvent->params.pSurface = _pBlastText->getSurface();
		pEvent->params.pClip = &pEvent->clip;
		pEvent->params.pColorMap = (byte *)_pSputRender->getColors();
		pEvent->pActor = nullptr; // TODO: sputActor_Restore must set the actor of the event found by sputText_FindSerialNumber

		pEvent->pPrev = nullptr;
		pEvent->pNext = _pTextQueue;

		if (_pTextQueue)
			_pTextQueue->pPrev = pEvent;

		_pTextQueue = pEvent;
	}

	int printLineFontSerial = pStream->readSint32LE();
	int defaultFontSerial = pStream->readSint32LE();
	int blastTextFontSerial = pStream->readSint32LE();

	if (pStream->err() || pStream->eos()) {
		warning("SputText::restore(): read error");
		return 1;
	}

	_pFont = _pText->getSystemFont();
	_printLineParams.pFont = static_cast<BlastTextFont *>(_pRes->handleFromSerialNumber(printLineFontSerial));
	_defaultTextParams.pFont = static_cast<BlastTextFont *>(_pRes->handleFromSerialNumber(defaultFontSerial));
	_blastTextParams.pFont = static_cast<BlastTextFont *>(_pRes->handleFromSerialNumber(blastTextFontSerial));

	if (!_printLineParams.pFont)
		_printLineParams.pFont = _pFont;

	if (!_defaultTextParams.pFont)
		_defaultTextParams.pFont = _pFont;

	if (!_blastTextParams.pFont)
		_blastTextParams.pFont = _pFont;

	return 0;
}

} // End of namespace Grim
