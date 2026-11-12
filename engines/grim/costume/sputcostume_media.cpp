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
#include "common/util.h"

#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/lua/lua.h"
#include "engines/grim/actor/sputactor.h"
#include "engines/grim/costume/sputcostume.h"
#include "engines/grim/render/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/std/std_bitmap.h"

namespace Grim {

static const int soundKeyValues[3] = { 0, 1, 2 };
static const char *const soundKeyNames[3] = { "Play", "Stop", "Stop Looping" };
static const CosComponentNames soundKeyDesc = { 3, soundKeyValues, soundKeyNames, nullptr, nullptr };

static const int demoSoundKeyValues[2] = { 0, 1 };
static const char *const demoSoundKeyNames[2] = { "Play", "Stop" };
static const CosComponentNames demoSoundKeyDesc = { 2, demoSoundKeyValues, demoSoundKeyNames, nullptr, nullptr };

static const int smushKeyValues[2] = { 0, 1 };
static const char *const smushKeyNames[2] = { "Play", "Stop" };
static const CosComponentNames smushKeyDesc = { 2, smushKeyValues, smushKeyNames, nullptr, nullptr };

void BitmapHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	ResRef *pRef = static_cast<ResRef *>(pInstData);

	if (!pRef->valid) {
		warning("BitmapHandler::change(): Bad Bitmap Reference");
		return;
	}

	StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(pRef->handle);

	pBitmap->curFrame = keyValue - 1;
}

int BitmapHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	char name[32];

	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = pComp->name;

	Common::strlcpy(name, args, 32);

	char *pBitmapName = strtok(name, ",");

	if (!pBitmapName) {
		warning("BitmapHandler::request(): empty bitmap name");
		return 0;
	}

	StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(_pSputCostume->_pSputResource->alloc(kSputTagBknd, pBitmapName));

	if (!pBitmap)
		return 0;

	Common::strlcpy(pBitmap->aName, pBitmapName, 32);

	ResRef *pRes = new ResRef();

	_pSputCostume->_pRes->setReference(pRes, pBitmap);

	int numFrames = pBitmap->numFrames;
	int *aNameIds = new int[numFrames + 1];

	aNameIds[0] = 0;

	if (_pSputCostume->_aLECBitmaps)
		delete[] _pSputCostume->_aLECBitmaps;

	_pSputCostume->_aLECBitmaps = new CosStateBitmapDesc[numFrames + 1]();

	for (int i = 0; i < numFrames; i++) {
		StdVBuffer *pFrame = pBitmap->apFrames[i];

		aNameIds[i + 1] = i + 1;

		_pSputCostume->_aLECBitmaps[i].pPixels = pFrame->pPixels;
		_pSputCostume->_aLECBitmaps[i].width = pFrame->tex.width;
		_pSputCostume->_aLECBitmaps[i].height = pFrame->tex.height;
		_pSputCostume->_aLECBitmaps[i].reserved12 = 1;
	}

	const int *pBitmapExtra = nullptr;

	if (pBitmap->flags & 2)
		pBitmapExtra = reinterpret_cast<const int *>(&pBitmap->pPalette);

	CosComponentNames keyDesc = { numFrames + 1, aNameIds, nullptr, pBitmapExtra ? _pSputCostume->_aLECBitmaps : nullptr, pBitmapExtra };
	int result = _pSputCostume->_cos.registerComponent(hCostume, args, pRes, fourcc, pParentId, parentFourcc, &keyDesc);

	delete[] aNameIds;

	if (result) {
		_pSputCostume->_pLastBitmap = pBitmap;
		return 1;
	}

	_pSputCostume->_pLastBitmap = nullptr;

	return 0;
}

void BitmapHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	ResRef *pRes = static_cast<ResRef *>(pCompRes);

	_pSputCostume->_pLastBitmap = pCompRes;
	_pSputCostume->_pRes->removeReference(pRes);
	delete pRes;
}

// The component name is "bitmap,layer", it is kept whole in the bitmap structure
static int getAnimationLayer(const StdBitmapStruct *pBitmap) {
	char name[32];

	Common::strlcpy(name, pBitmap->aName, 32);
	strtok(name, ",");

	const char *pLayer = strtok(nullptr, ",");

	return pLayer && atoi(pLayer) > 0 ? 1 : 0;
}

void AnimationHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	ResRef *pRef = static_cast<ResRef *>(pInstData);

	if (!pRef->valid) {
		warning("AnimationHandler::change(): Bad Bitmap Reference");
		return;
	}

	StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(pRef->handle);

	_pSputCostume->_pSputRender->addAnimationBitmap(pBitmap, getAnimationLayer(pBitmap));
	pBitmap->curFrame = keyValue - 1;
}

int AnimationHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	if (!BitmapHandler::request(hCostume, fourcc, pParentId, parentFourcc, pComp))
		return 0;

	StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(_pSputCostume->_pLastBitmap);

	Common::strlcpy(pBitmap->aName, pComp->name, 32);
	pBitmap->curFrame = -1;
	_pSputCostume->_pSputRender->addAnimationBitmap(pBitmap, getAnimationLayer(pBitmap));

	return 1;
}

void AnimationHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	// The EXE looks the reference up again after freeing it and never finds it
	BitmapHandler::freeComponent(hCostume, fourcc, pCompRes);
}

void AnimationHandler::stop(void *pInstData) {
	ResRef *pRef = static_cast<ResRef *>(pInstData);

	if (!pRef->valid) {
		warning("AnimationHandler::stop(): Bad Bitmap Reference");
		return;
	}

	_pSputCostume->_pSputRender->removeAnimationBitmap(pRef->handle);
}

int AnimationHandler::adjust(void **ppCompRes, char *args, int argsSize) {
	// TODO: grimPlat_InputDialog("Enter a 1 to make this an overlay...")
	return 0;
}

void SoundHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	SoundCompRes *pRes = static_cast<SoundCompRes *>(pInstData);

	if (keyValue < 0 || keyValue >= 3) {
		warning("SoundHandler::change(): invalid key %d", keyValue);
		return;
	}

	int action = soundKeyValues[keyValue];

	if (action || g_grim->getImuse()->soundsIsSoundPlaying(pRes->name)) {
		if (action == 1) {
			if (fourcc == kSputCostumeTagResidentSound)
				g_grim->getImuse()->soundsStopSound(pRes->name);
			else if (fourcc == kSputCostumeTagStreamedSound)
				g_grim->getImuse()->soundStopVoiceStream();
		} else if (action == 2) {
			g_grim->getImuse()->iMUSESetHook(pRes->name, 0x80);
		}
	} else if (fourcc == kSputCostumeTagResidentSound) {
		SputRoom *pSputRoom = _pSputCostume->_pSputRoom;
		Room *pRoom = pSputRoom->getCurrentRoom();
		int soundId = g_grim->getImuse()->soundsLoadStartSound(pRes->name, pRes->priority, 1);

		if (pRoom) {
			float maxDist;
			float panFactor;
			float maxRadius;
			byte roomMinVolume;
			byte roomMaxVolume;

			pSputRoom->getSoundParameters(pRoom, &maxDist, &roomMinVolume, &roomMaxVolume, &panFactor, &maxRadius);

			int userData = _pSputCostume->_cos.instGetUserData(hInst);
			SputActor *pActor = &_pSputCostume->_aActors[userData];
			byte volume = pRes->volume;

			if (roomMaxVolume < volume)
				volume = roomMaxVolume;

			int minVolume = roomMinVolume;

			if (minVolume <= pRes->minVolume)
				minVolume = pRes->minVolume;

			pSputRoom->setSoundPosition((const char *)(intptr)soundId, pActor->actorPos, minVolume, volume, maxDist, maxRadius);
		}
	} else if (fourcc == kSputCostumeTagStreamedSound) {
		g_grim->getImuse()->soundStartVoiceStream(pRes->name, pRes->volume, pRes->pan);
		g_grim->getImuse()->soundsSetParam(pRes->name, IMUSE_P_GROUP, 1);
		g_grim->getImuse()->soundsSetParam(pRes->name, IMUSE_P_PRIORITY, pRes->priority);
	}
}

int SoundHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	char name[32];
	byte volume = 127;
	int8 pan = 64;
	int priority = 100;
	int minVolume = 1;

	memset(name, 0, sizeof(name));

	if (pParentId)
		return 0;

	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = pComp->name;

	Common::strlcpy(name, args, 32);
	strtok(name, ",");

	char *pVolume = strtok(nullptr, ",");

	if (pVolume) {
		volume = (byte)atoi(pVolume);

		char *aTokens[3];

		for (int i = 0; i < 3; i++) {
			aTokens[i] = strtok(nullptr, ",");

			if (!aTokens[i])
				warning("SoundHandler::request(): missing sound parameter in %s", args);
		}

		if (aTokens[0])
			pan = (int8)atoi(aTokens[0]);

		if (aTokens[1])
			priority = atoi(aTokens[1]);

		if (aTokens[2])
			minVolume = atoi(aTokens[2]);
	}

	SoundCompRes *pRes = new SoundCompRes();
	const char *pSoundName = name;

	if (!name[0])
		pSoundName = args;

	Common::strlcpy(pRes->name, pSoundName, 32);
	pRes->pan = pan;
	pRes->minVolume = minVolume;
	pRes->volume = volume;
	pRes->priority = priority;

	return _pSputCostume->_cos.registerComponent(hCostume, args, pRes, fourcc, nullptr, parentFourcc, &soundKeyDesc) != 0;
}

void SoundHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	SoundCompRes *pRes = static_cast<SoundCompRes *>(pCompRes);

	delete pRes;
}

void SoundHandler::stop(void *pInstData) {
	SoundCompRes *pRes = static_cast<SoundCompRes *>(pInstData);

	g_grim->getImuse()->soundsStopSound(pRes->name);
}

int SoundHandler::adjust(void **ppCompRes, char *args, int argsSize) {
	SoundCompRes *pRes = static_cast<SoundCompRes *>(*ppCompRes);
	int aValues[4];

	aValues[0] = pRes->volume;
	aValues[1] = pRes->pan;
	aValues[2] = pRes->priority;
	aValues[3] = pRes->minVolume;

	int accepted = 0; // TODO: grimPlat_ControlPanel("Maximum Volume", "Pan", "Priority", "Minimum Volume")

	if (!accepted || aValues[0] < aValues[3])
		return 0;

	char *pToken = strtok(args, ",");

	if (!pToken) {
		warning("SoundHandler::adjust(): empty component name");
		return 0;
	}

	pRes->volume = (byte)CLIP(aValues[0], 0, 127);
	pRes->pan = (int8)CLIP(aValues[1], 0, 127);
	pRes->priority = CLIP(aValues[2], 0, 127);
	pRes->minVolume = CLIP(aValues[3], 0, 127);

	char text[32];

	snprintf(text, sizeof(text), "%s,%d,%d,%d,%d", pToken, pRes->volume, pRes->pan, pRes->priority, pRes->minVolume);
	Common::strlcpy(pToken, text, 32);

	return 1;
}

void DemoSoundHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	DemoSoundCompRes *pRes = static_cast<DemoSoundCompRes *>(pInstData);

	if (keyValue < 0 || keyValue >= 2) {
		warning("DemoSoundHandler::change(): invalid key %d", keyValue);
		return;
	}

	if (demoSoundKeyValues[keyValue]) {
		if (fourcc == kSputCostumeTagResidentSound)
			g_grim->getImuse()->soundsStopSound(pRes->name);
		else if (fourcc == kSputCostumeTagStreamedSound)
			g_grim->getImuse()->soundStopVoiceStream();
	} else if (fourcc == kSputCostumeTagResidentSound) {
		g_grim->getImuse()->soundsLoadStartSound(pRes->name, pRes->priority, pRes->group);
		g_grim->getImuse()->soundsSetParam(pRes->name, IMUSE_P_VOLUME, pRes->volume);
		g_grim->getImuse()->soundsSetParam(pRes->name, IMUSE_P_PAN, pRes->pan);
	} else if (fourcc == kSputCostumeTagStreamedSound) {
		g_grim->getImuse()->soundStartVoiceStream(pRes->name, pRes->volume, pRes->pan);
		g_grim->getImuse()->soundsSetParam(pRes->name, IMUSE_P_GROUP, pRes->group);
		g_grim->getImuse()->soundsSetParam(pRes->name, IMUSE_P_PRIORITY, pRes->priority);
	}
}

int DemoSoundHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	char name[32];
	byte volume = 127;
	int8 pan = 64;
	int priority = 100;
	int group = 1;

	memset(name, 0, sizeof(name));

	if (pParentId) {
		warning("Sounds must be children of the costume");
		return 0;
	}

	// TODO: grimPlat_OpenFileDialog branch (args == NULL)
	const char *args = pComp->name;

	Common::strlcpy(name, args, 32);
	strtok(name, ",");

	char *pVolume = strtok(nullptr, ",");

	if (pVolume) {
		volume = (byte)atoi(pVolume);

		char *aTokens[3];

		for (int i = 0; i < 3; i++) {
			aTokens[i] = strtok(nullptr, ",");

			if (!aTokens[i])
				warning("DemoSoundHandler::request(): missing sound parameter in %s", args);
		}

		if (aTokens[0])
			pan = (int8)atoi(aTokens[0]);

		if (aTokens[1])
			priority = atoi(aTokens[1]);

		if (aTokens[2])
			group = atoi(aTokens[2]);
	}

	DemoSoundCompRes *pRes = new DemoSoundCompRes();
	const char *pSoundName = name;

	if (!name[0])
		pSoundName = args;

	memset(pRes, 0, sizeof(DemoSoundCompRes));
	Common::strlcpy(pRes->name, pSoundName, 32);
	pRes->pan = pan;
	pRes->group = group;
	pRes->volume = volume;
	pRes->priority = priority;

	return _pSputCostume->_cos.registerComponent(hCostume, args, pRes, fourcc, nullptr, parentFourcc, &demoSoundKeyDesc) != 0;
}

void DemoSoundHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	delete static_cast<DemoSoundCompRes *>(pCompRes);
}

int DemoSoundHandler::adjust(void **ppCompRes, char *args, int argsSize) {
	// TODO: grimPlat_ControlPanel dialog
	return 0;
}

void SmushHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	if (!keyValue)
		_pSputCostume->_pGrim->playMovie(static_cast<const char *>(pInstData), 0, 0, -1, -1);
}

void DemoSmushHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	char fileName[64];

	snprintf(fileName, sizeof(fileName), "MOVIES\\%s", static_cast<const char *>(pInstData));

	if (!keyValue)
		_pSputCostume->_pGrim->playMovie(fileName, 0, 0, -1, -1);
}

int SmushHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = pComp->name;
	size_t size = strlen(args) + 1;
	char *pName = new char[size];

	Common::strlcpy(pName, args, size);

	return _pSputCostume->_cos.registerComponent(hCostume, args, pName, fourcc, pParentId, parentFourcc, &smushKeyDesc) != 0;
}

void SmushHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	char *pName = static_cast<char *>(pCompRes);

	delete[] pName;
}

void SmushHandler::saveLoad(int hInst, int compIndex, int bSave, void *pInstData, Common::WriteStream *pWrite, Common::ReadStream *pRead) {
	_pSputCostume->stub();
}

void IMuseHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	_pSputCostume->nullSub1();
}

int IMuseHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	if (pParentId)
		return 0;

	const char *name = "iMuse Sequence";

	if (fourcc != kSputCostumeTagSequence)
		name = "iMuse State";

	return _pSputCostume->_cos.registerComponent(hCostume, name, (void *)(intptr)1, fourcc, nullptr, parentFourcc, nullptr) != 0;
}

void IMuseHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	_pSputCostume->stub();
}

void LuaVarHandler::change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {
	float number = (float)keyValue;

	lua_pushnumber(number);
	lua_setglobal(static_cast<const char *>(pInstData));
}

int LuaVarHandler::request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) {
	char name[32];

	memset(name, 0, sizeof(name));

	if (pParentId)
		return 0;

	// TODO: grimPlat_InputDialog branch (args == NULL)
	Common::strlcpy(name, pComp->name, 32);

	char *pVarName = new char[32];

	Common::strlcpy(pVarName, name, 32);

	return _pSputCostume->_cos.registerComponent(hCostume, pVarName, pVarName, fourcc, nullptr, parentFourcc, nullptr) != 0;
}

void LuaVarHandler::freeComponent(int hCostume, int fourcc, void *pCompRes) {
	char *pVarName = static_cast<char *>(pCompRes);

	delete[] pVarName;
}

void LuaVarHandler::stop(void *pInstData) {
	lua_pushnil();
	lua_setglobal(static_cast<const char *>(pInstData));
}

int LuaVarHandler::adjust(void **ppCompRes, char *args, int argsSize) {
	char *pVarName = static_cast<char *>(*ppCompRes);
	char text[32];

	memset(text, 0, sizeof(text));

	int accepted = 0; // TODO: grimPlat_InputDialog("Change variable properties", "Variable name")

	if (!accepted)
		return 0;

	Common::strlcpy(pVarName, text, 32);
	Common::strlcpy(args, text, argsSize);

	return 1;
}

} // End of namespace Grim
