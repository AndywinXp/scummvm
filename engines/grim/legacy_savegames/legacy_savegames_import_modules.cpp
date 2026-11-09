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
#include "common/textconsole.h"

#include "engines/grim/chore/cos.h"
#include "engines/grim/grim.h"
#include "engines/grim/legacy_savegames/legacy_savegames_import.h"
#include "engines/grim/lua_v1.h"
#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/res.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcostume_components.h"
#include "engines/grim/sputobjectstate.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputsmush.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/sputtime.h"
#include "engines/grim/std/std_bitmap.h"
#include "engines/grim/text.h"
#include "engines/grim/walkplane.h"

namespace Grim {

static Common::String getFontName(Res *pRes, BlastTextFont *pFont) {
	ResNode *pNode = pRes->checkAllocatedRes(pFont);

	if (pNode && pNode->tag == kSputTagFont)
		return pNode->pName;

	return Common::String();
}

static BlastTextFont *loadFontByName(Res *pRes, const char *name, BlastTextFont *pSystemFont) {
	if (name && name[0]) {
		BlastTextFont *pFont = static_cast<BlastTextFont *>(pRes->getResource(kSputTagFont, name));

		if (pFont)
			return pFont;

		warning("LegacyImporter::setDefaultFonts(): font %s not found, using the system font", name);
	}

	return pSystemFont;
}

int LegacyImporter::setChoreInstance(Cos *pCos, int hInst, int choreIndex, uint32 state, int time) {
	int index = pCos->instHandleToIndex(hInst);

	if (index < 0 || index >= pCos->_maxInstances || pCos->_aInstances[index].bFree) {
		warning("LegacyImporter::setChoreInstance(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &pCos->_aInstances[index];

	if (choreIndex < 0 || choreIndex >= pCos->_aCostumes[pInstance->costume].numChores) {
		warning("LegacyImporter::setChoreInstance(): invalid chore %d", choreIndex);
		return 0;
	}

	pInstance->aChoreInstances[choreIndex].state = state;
	pInstance->aChoreInstances[choreIndex].time = time;

	return 1;
}

void LegacyImporter::resetActors(SputActorModule *pActors) {
	for (int i = 0; i < 4; i++)
		pActors->_apTalkingActors[i] = nullptr;

	pActors->_numActorsTalking = 0;
	pActors->_pSelectedActor = nullptr;

	for (int i = 0; i < pActors->_maxActors; i++) {
		if (pActors->_aActors[i].bInUse)
			pActors->freeActor(&pActors->_aActors[i]);
	}

	pActors->_numActors = 0;
}

int LegacyImporter::addPuppetTrack(SputActorModule *pActors, ModelInst *pModelInst, RdKeyframe *pKeyframe, int priority, int priorityMasked, uint status, float frame, float fade, float fadeSpeed) {
	RdPuppet *pPuppet = pModelInst->pThing->pPuppet;

	if (!pPuppet) {
		warning("LegacyImporter::addPuppetTrack(): the model has no puppet for the track %s", pKeyframe->aName);
		return -1;
	}

	int trackNum = pActors->_pRd->_puppet.addTrack(pPuppet, pKeyframe, priority, priorityMasked);

	if (trackNum < 0 || trackNum >= 6) {
		warning("LegacyImporter::addPuppetTrack(): the keyframe %s could not be added to the puppet", pKeyframe->aName);
		return -1;
	}

	pPuppet->aTracks[trackNum].status = status;
	pPuppet->aTracks[trackNum].curFrame = frame;
	pPuppet->aTracks[trackNum].prevFrame = frame;
	pPuppet->aTracks[trackNum].fade = fade;
	pPuppet->aTracks[trackNum].fadeSpeed = fadeSpeed;

	return trackNum;
}

void LegacyImporter::beginRoom(SputRoom *pSputRoom, Room *pRoom, int curSetup, int bRoomRef) {
	pRoom->curSetup = curSetup;

	if (pSputRoom->_currentRoomRef.valid)
		pSputRoom->_pRes->removeReference(&pSputRoom->_currentRoomRef);

	pSputRoom->_pRes->setReference(&pSputRoom->_currentRoomRef, pRoom);

	if (!pSputRoom->_currentRoomRef.valid)
		error("LegacyImporter::beginRoom(): Bad room Reference");

	pSputRoom->loadResources(static_cast<Room *>(pSputRoom->_currentRoomRef.handle));
	Common::strlcpy(pSputRoom->_currentRoomName, pRoom->name, 32);

	if (bRoomRef)
		pSputRoom->_pRes->setReference(&pRoom->roomRef, pRoom);
}

SputObjectState *LegacyImporter::addRoomObjectState(SputRoom *pSputRoom, int setupIndex, int objectType, const char *bitmapName, const char *zbufferName, int colorFrame, int zFrame) {
	SputObjectState *pState = pSputRoom->addObjectState(setupIndex, objectType, bitmapName, zbufferName, -1);

	if (!pState)
		return nullptr;

	if (pState->colorImage.bitmapRef.valid) {
		StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(pState->colorImage.bitmapRef.handle);

		if (colorFrame >= (int)pBitmap->numFrames) {
			warning("LegacyImporter::addRoomObjectState(): frame %d of %s is out of range", colorFrame, bitmapName);
			colorFrame = -1;
		}

		pBitmap->curFrame = colorFrame;
	}

	if (pState->zImage.bitmapRef.valid) {
		StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(pState->zImage.bitmapRef.handle);

		if (zFrame >= (int)pBitmap->numFrames) {
			warning("LegacyImporter::addRoomObjectState(): frame %d of %s is out of range", zFrame, zbufferName);
			zFrame = -1;
		}

		pBitmap->curFrame = zFrame;
	}

	return pState;
}

void LegacyImporter::setCurrentRoom(SputRoom *pSputRoom, Room *pRoom, byte minVol, byte maxVol, float maxDist, float panFactor, float maxRadius) {
	pSputRoom->setCurrentRoom(pRoom);

	pSputRoom->_pWalkSystem->setCollection(pRoom->name);

	pRoom->minVol = minVol;
	pRoom->maxVol = maxVol;
	pRoom->maxDist = maxDist;
	pRoom->panFactor = panFactor;
	pRoom->maxRadius = maxRadius;
}

void LegacyImporter::getDefaultFontNames(SputText *pSputText, Common::String *pPrintLine, Common::String *pDefault, Common::String *pBlastText) {
	*pPrintLine = getFontName(pSputText->_pRes, pSputText->_printLineParams.pFont);
	*pDefault = getFontName(pSputText->_pRes, pSputText->_defaultTextParams.pFont);
	*pBlastText = getFontName(pSputText->_pRes, pSputText->_blastTextParams.pFont);
}

void LegacyImporter::setDefaultFonts(SputText *pSputText, const char *printLineFont, const char *defaultFont, const char *blastTextFont) {
	while (pSputText->_pTextQueue)
		pSputText->killEvent(pSputText->_pTextQueue);

	pSputText->_pFont = pSputText->_pText->getSystemFont();
	pSputText->_printLineParams.pFont = loadFontByName(pSputText->_pRes, printLineFont, pSputText->_pFont);
	pSputText->_defaultTextParams.pFont = loadFontByName(pSputText->_pRes, defaultFont, pSputText->_pFont);
	pSputText->_blastTextParams.pFont = loadFontByName(pSputText->_pRes, blastTextFont, pSputText->_pFont);
}

void LegacyImporter::setSayLineDefaults(SputText *pSputText, int x, int y, int fgColor, int justify) {
	int baseLine = 0;

	if (pSputText->_defaultTextParams.pFont)
		baseLine = pSputText->_defaultTextParams.pFont->baseLine;

	pSputText->_defaultTextParams.x = x;
	pSputText->_defaultTextParams.y = y + baseLine;
	pSputText->_defaultTextParams.fgColor = fgColor;

	if (justify == 1)
		pSputText->_defaultTextParams.flags = 5;
	else if (justify == 2)
		pSputText->_defaultTextParams.flags = 4;
	else if (justify == 3)
		pSputText->_defaultTextParams.flags = 6;
}

void LegacyImporter::setRenderState(SputRender *pSputRender, int irisMode, int irisCenterX, int irisCenterY, int irisDuration, int irisElapsed, int camActorIndex) {
	pSputRender->_irisStart = 0;
	pSputRender->_irisDuration = 0.0f;
	pSputRender->_irisCenterX = 320;
	pSputRender->_irisCenterY = 240;
	pSputRender->_renderFlags = 0;
	pSputRender->_pageFlipEnable = 1;
	pSputRender->_movieJustFinished = 0;
	pSputRender->_gCamActorIndex = camActorIndex;

	if (irisMode == 1) {
		pSputRender->irisDown(320, 240, 1);
		pSputRender->_irisStart = pSputRender->_pSputTime->getGameTime() - 2;
	} else if (irisMode == 2 || irisMode == 3) {
		if (irisMode == 2)
			pSputRender->irisDown(irisCenterX, irisCenterY, irisDuration);
		else
			pSputRender->irisUp(irisCenterX, irisCenterY, irisDuration);

		pSputRender->_irisStart = pSputRender->_pSputTime->getGameTime() - irisElapsed;
	}
}

void LegacyImporter::startMovie(SputSmush *pSputSmush, const char *name, int bLooping, int x, int y) {
	pSputSmush->setTarget(pSputSmush->_pMainGfxBuffer);

	pSputSmush->_playing = 0;
	pSputSmush->_paused = 0;
	pSputSmush->_abort = 0;
	pSputSmush->_looping = 0;
	pSputSmush->_x = 0;
	pSputSmush->_y = 0;
	memset(pSputSmush->_curMovieFile, 0, sizeof(pSputSmush->_curMovieFile));

	if (name)
		pSputSmush->startMovie(name, bLooping, x, y);
}

void LegacyImporter::getEngineRefs(LegacyLuaRefs *pRefs) {
	Lua_V1 *pLua = _pGrim->_pLua;
	SputText *pSputText = _pGrim->_pSputText;

	pRefs->systemTable = pLua->refSystemTable;
	pRefs->typeOverride = pLua->refTypeOverride;
	pRefs->oldConcatFallback = pLua->refOldConcatFallback;
	pRefs->textObject[0] = pSputText->_luaTextObjXRef;
	pRefs->textObject[1] = pSputText->_luaTextObjYRef;
	pRefs->textObject[2] = pSputText->_luaTextObjFontRef;
	pRefs->textObject[3] = pSputText->_luaTextObjWidthRef;
	pRefs->textObject[4] = pSputText->_luaTextObjHeightRef;
	pRefs->textObject[5] = pSputText->_luaTextObjFGColorRef;
	pRefs->textObject[6] = pSputText->_luaTextObjBGColorRef;
	pRefs->textObject[7] = pSputText->_luaTextObjFXColorRef;
	pRefs->textObject[8] = pSputText->_luaTextObjHIColorRef;
	pRefs->textObject[9] = pSputText->_luaTextObjDurationRef;
	pRefs->textObject[10] = pSputText->_luaTextObjCenterRef;
	pRefs->textObject[11] = pSputText->_luaTextObjLJustifyRef;
	pRefs->textObject[12] = pSputText->_luaTextObjRJustifyRef;
	pRefs->textObject[13] = pSputText->_luaTextObjVolumeRef;
	pRefs->textObject[14] = pSputText->_luaTextObjBufferRef;
	pRefs->textObject[15] = pSputText->_luaTextObjBackgroundRef;
}

} // End of namespace Grim
