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

#include "common/file.h"
#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/chore/cos.h"
#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/std/std_sys.h"

namespace Grim {

void SputActorModule::sayLineDemo(SputActor *pActor, const char *msgId) {
	int speechText = _speechMode & 1;
	int speechVoice = _speechMode & 2;
	char translation[32];
	char voiceFile[36];

	if (!pActor->pExtra)
		allocateActorExtra(pActor);

	Extra *pExtra = pActor->pExtra;

	translation[0] = 0;
	voiceFile[0] = 0;

	const char *pText = _pSputText->extractSayLineData(msgId, translation);

	if (translation[0]) {
		static const char *const aExtensions[4] = { "wvc", "wav", "imc", "imu" };

		for (int i = 0; i < 4 && !voiceFile[0]; i++) {
			Common::String fileName = Common::String::format("%s.%s", translation, aExtensions[i]);
			Common::SeekableReadStream *pFile = _pHS->openFile(fileName.c_str());

			if (pFile) {
				delete pFile;
				Common::strlcpy(voiceFile, fileName.c_str(), sizeof(voiceFile));
			}
		}
	}

	if (pExtra->talking) {
		if (translation[0]) {
			char currentId[13];

			memcpy(currentId, pExtra->sayLineId, 12);
			currentId[12] = 0;

			if (!strcmp(currentId, translation))
				return;
		}

		shutUpDemo(pActor);
	}

	pExtra->talkTimerMs = 0;
	pExtra->talking = 1;
	strncpy(pExtra->sayLineId, translation, 12);
	_numActorsTalking++;
	pExtra->phoneme = 0;

	if (speechVoice) {
		if (voiceFile[0]) {
			g_imuse->soundStartVoiceStream(voiceFile, 127, 64);
			Common::strlcpy(pExtra->voiceName, voiceFile, sizeof(pExtra->voiceName));
		} else {
			pExtra->voiceName[0] = 0;
		}

		pExtra->talkTimerMs = -1;
	}

	if (speechText || !pExtra->voiceName[0]) {
		SputTextEvent *pEvent = _pSputText->createActorTextDemo(pActor, pText);

		pExtra->pSayText = pEvent;

		if (!pExtra->voiceName[0])
			pExtra->talkTimerMs = 48 * (int)strlen(pText) + 640;

		if (pEvent)
			_pSputText->addToQueue(pEvent, -1, 5);
	}

	// There is no lip sync data in the demo, so the mumble chore always plays
	CostumeNode *pNode = pActor->pCostumeStack;

	if (pNode) {
		RdThing *pHeadThing = pNode->pHeadThing;

		do {
			if (pNode->pHeadThing != pHeadThing)
				break;

			if (pNode->mumbleChore >= 0)
				break;

			pNode = pNode->pNext;
		} while (pNode);

		if (pNode && pNode->pHeadThing == pHeadThing && pNode->mumbleChore >= 0)
			_pCos->instPlayChore(pNode->hCostumeInstance, pNode->mumbleChore, 1, 0);
	}
}

void SputActorModule::shutUpDemo(SputActor *pActor) {
	if (!pActor) {
		warning("SputActorModule::shutUpDemo(): no actor");
		return;
	}

	CostumeNode *pNode = pActor->pCostumeStack;
	Extra *pExtra = pActor->pExtra;

	if (!pNode || !pExtra)
		return;

	if (pExtra->voiceName[0])
		g_imuse->soundStopVoiceStream();

	if (pExtra->pSayText)
		_pSputText->killEvent(pExtra->pSayText);

	RdThing *pHeadThing = pNode->pHeadThing;

	do {
		if (pNode->pHeadThing != pHeadThing)
			break;

		if (pNode->mumbleChore >= 0)
			break;

		pNode = pNode->pNext;
	} while (pNode);

	if (pNode && pNode->pHeadThing == pHeadThing && pNode->mumbleChore >= 0 && _pCos->instChoreStatus(pNode->hCostumeInstance, pNode->mumbleChore))
		_pCos->instStopChore(pNode->hCostumeInstance, pNode->mumbleChore);

	pNode = pActor->pCostumeStack;
	pHeadThing = pNode->pHeadThing;

	do {
		if (pNode->pHeadThing != pHeadThing)
			break;

		if (pNode->aChore[0] >= 0)
			break;

		pNode = pNode->pNext;
	} while (pNode);

	if (pNode && pNode->pHeadThing == pHeadThing && pNode->aChore[0] >= 0)
		_pCos->instCompleteChore(pNode->hCostumeInstance, pNode->aChore[0]);

	pExtra->phoneme = 0;
	pExtra->voiceName[0] = 0;
	pExtra->pSayText = nullptr;
	pExtra->sayLineId[0] = 0;
	pExtra->talking = 0;

	if (_numActorsTalking > 0)
		_numActorsTalking--;
}

int SputActorModule::isVoicePlayingDemo(SputActor *pActor) {
	return pActor->pExtra->voiceName[0] && g_imuse->imGetParam(kTalkSoundID, IMUSE_P_SND_TRACK_NUM) > 0;
}

void SputActorModule::stopActorDemo(SputActor *pActor) {
	CostumeNode *pNode = pActor->pCostumeStack;

	if (pNode) {
		if (pNode->rCostume.valid)
			_pCos->instRescue(pNode->hCostumeInstance, pNode->rCostume.serial);

		_pSputCostume->resetCostume(pNode->hCostumeInstance);
	}

	if (pActor->pExtra && pActor->pExtra->talking)
		shutUpDemo(pActor);
}

void SputActorModule::updateSpeechOnly(float delta) {
	if (_pGrim->getGameFlags() & ADGF_DEMO)
		return;

	for (int i = 0; i < 4; i++) {
		if (_apTalkingActors[i])
			updateActorSpeech(_apTalkingActors[i], delta);
	}
}

void SputActorModule::updateActorSpeech(SputActor *pActor, float delta) {
	Extra *pExtra = pActor->pExtra;

	if (!pExtra->talking)
		return;

	if (pExtra->voiceName[0] && pExtra->phoneme && pActor->pCostumeStack) {
		int phoneme = g_imuse->iMUSEGetPhoneme(pExtra->soundId, 0);

		if (phoneme) {
			int mouthChore = matchPhonemeWithAnim(pActor, phoneme);
			CostumeNode *pNode = pActor->pCostumeStack;
			RdThing *pHeadThing = pNode->pHeadThing;

			do {
				if (pNode->pHeadThing != pHeadThing)
					break;

				if (pNode->aChore[mouthChore] >= 0)
					break;

				pNode = pNode->pNext;
			} while (pNode);

			if (pNode && pNode->pHeadThing == pHeadThing)
				_pCos->instCompleteChore(pNode->hCostumeInstance, pNode->aChore[mouthChore]);
		} else {
			shutUp(pActor);
		}
	}

	double elapsed = (double)delta * -1000.0;

	if (elapsed > -9.0e18 && elapsed < 9.0e18)
		pActor->pExtra->talkTimerMs += (int)(int64)elapsed;
	else
		warning("SputActorModule::updateActorSpeech(): the time delta is out of range");

	if ((pActor->pExtra->voiceName[0] && !isVoicePlaying(pActor)) || (!pActor->pExtra->voiceName[0] && pActor->pExtra->talkTimerMs < 0))
		shutUp(pActor);
}

int SputActorModule::matchPhonemeWithAnim(SputActor *pActor, int phoneme) {
	if (phoneme == 95)
		return 0;

	LipSyncEntry key;

	key.phoneme = phoneme;
	key.mouthAnim = 0;

	LipSyncEntry *pEntry = (LipSyncEntry *)bsearch(&key, _aLipSyncAnims, _numLipSyncAnims, sizeof(LipSyncEntry), compareAnimEntry);

	if (pEntry)
		return pEntry->mouthAnim;

	return 1;
}

int SputActorModule::compareAnimEntry(const void *pKey, const void *pEntry) {
	return *(const int *)pKey - *(const int *)pEntry;
}

void SputActorModule::actorPlayVoiceDrawText(SputActor *pActor, const char *msgId, int bVoiceStream, int volume, int pan, int x, int y) {
	if (!pActor->pExtra)
		allocateActorExtra(pActor);

	int speechText = _speechMode & 1;
	int speechVoice = _speechMode & 2;
	char translation[32];

	translation[0] = 0;

	const char *pText = _pSputText->extractSayLineData(msgId, translation);
	char voiceFile[36];

	voiceFile[0] = 0;

	if (translation[0]) {
		static const char *const aExtensions[4] = { "wvc", "wav", "imc", "imu" };
		bool bFound = false;

		for (int i = 0; i < 4 && !bFound; i++) {
			Common::String fileName = Common::String::format("%s.%s", translation, aExtensions[i]);
			Common::SeekableReadStream *pFile = _pHS->openFile(fileName.c_str());

			if (pFile) {
				delete pFile;
				Common::strlcpy(voiceFile, fileName.c_str(), sizeof(voiceFile));
				bFound = true;
			}
		}

		if (!bFound)
			voiceFile[0] = 0;
	}

	if (pActor->pExtra->talking) {
		if (translation[0]) {
			char currentId[13];

			memcpy(currentId, pActor->pExtra->sayLineId, 12);
			currentId[12] = 0;

			if (!strcmp(currentId, translation))
				return;
		}

		shutUp(pActor);
	}

	pActor->pExtra->talkTimerMs = 0;
	pActor->pExtra->talking = 1;
	strncpy(pActor->pExtra->sayLineId, translation, 12);
	_numActorsTalking++;

	for (int i = 0; i < 4; i++) {
		if (!_apTalkingActors[i]) {
			_apTalkingActors[i] = pActor;
			break;
		}
	}

	if (speechVoice) {
		if (voiceFile[0]) {
			if (!bVoiceStream) {
				pActor->pExtra->soundId = g_imuse->soundsLoadProcessStartSound(voiceFile, 127, 2);

				if (!pActor->pExtra->soundId) {
					pActor->pExtra->voiceName[0] = 0;
				} else {
					g_imuse->soundsSetParam((const char *)(intptr)pActor->pExtra->soundId, IMUSE_P_VOLUME, volume);

					if (pActor->flags & 8)
						_pSputRoom->setSoundPosition((const char *)(intptr)pActor->pExtra->soundId, pActor->actorPos, 20, 127, 8.0f, 0.80000001f);

					Common::strlcpy(pActor->pExtra->voiceName, voiceFile, sizeof(pActor->pExtra->voiceName));
					pActor->pExtra->phoneme = g_imuse->iMUSEGetPhoneme(pActor->pExtra->soundId, 0);
				}
			} else if (!g_imuse->soundStartVoiceStream(voiceFile, volume, pan)) {
				pActor->pExtra->soundId = kTalkSoundID;

				if (pActor->flags & 8)
					_pSputRoom->setSoundPosition((const char *)(intptr)kTalkSoundID, pActor->actorPos, 20, 127, 8.0f, 0.80000001f);

				Common::strlcpy(pActor->pExtra->voiceName, voiceFile, sizeof(pActor->pExtra->voiceName));
				pActor->pExtra->phoneme = g_imuse->iMUSEGetPhoneme(pActor->pExtra->soundId, 0);
			} else {
				pActor->pExtra->voiceName[0] = 0;
			}
		} else {
			pActor->pExtra->voiceName[0] = 0;
		}

		pActor->pExtra->talkTimerMs = -1;
	}

	if (speechText || !pActor->pExtra->voiceName[0]) {
		SputTextEvent *pEvent = _pSputText->createActorText(pActor, pText, x, y);

		pActor->pExtra->pSayText = pEvent;

		if (!pActor->pExtra->voiceName[0])
			pActor->pExtra->talkTimerMs = 15 * (11 - _textRate) * (int)strlen(pText) + 640;

		if (pEvent)
			_pSputText->addToQueue(pEvent, -1, 13);

		if (bVoiceStream) {
			for (int i = 0; i < 4; i++) {
				SputActor *pOther = _apTalkingActors[i];

				if (pOther && pOther != pActor && pOther->pExtra->pSayText)
					pOther->pExtra->pSayText->flags |= 2;
			}
		} else {
			for (int i = 0; i < 4; i++) {
				SputActor *pOther = _apTalkingActors[i];

				if (pOther && pOther != pActor && pOther->pExtra->pSayText && pOther->pExtra->soundId == kTalkSoundID) {
					if (pActor->pExtra->pSayText)
						pActor->pExtra->pSayText->flags |= 2;

					break;
				}
			}
		}
	}

	Extra *pExtra = pActor->pExtra;
	CostumeNode *pNode = pActor->pCostumeStack;

	if (pNode && pExtra->phoneme) {
		RdThing *pHeadThing = pNode->pHeadThing;

		do {
			if (pNode->pHeadThing != pHeadThing)
				break;

			int i = 1;

			while (i <= 9 && pNode->aChore[i] < 0)
				i++;

			if (i <= 9)
				break;

			pNode = pNode->pNext;
		} while (pNode);

		if (!pNode || pNode->pHeadThing != pHeadThing)
			pExtra->phoneme = 0;
	}

	if (!pActor->pExtra->voiceName[0] || !pActor->pExtra->phoneme) {
		pNode = pActor->pCostumeStack;

		if (pNode) {
			RdThing *pHeadThing = pNode->pHeadThing;

			do {
				if (pNode->pHeadThing != pHeadThing)
					break;

				if (pNode->mumbleChore >= 0)
					break;

				pNode = pNode->pNext;
			} while (pNode);

			if (pNode && pNode->pHeadThing == pHeadThing) {
				if (pNode->mumbleChore >= 0)
					_pCos->instPlayChore(pNode->hCostumeInstance, pNode->mumbleChore, 1, 0);
			}
		}
	}
}

void SputActorModule::shutUp(SputActor *pActor) {
	if (!pActor)
		return;

	Extra *pExtra = pActor->pExtra;

	if (!pExtra)
		return;

	if (pExtra->voiceName[0]) {
		if (pExtra->soundId == kTalkSoundID)
			g_imuse->soundStopVoiceStream();
		else
			g_imuse->soundsStopSound((const char *)(intptr)pExtra->soundId);
	}

	if (pActor->pExtra->pSayText)
		_pSputText->killEvent(pActor->pExtra->pSayText);

	CostumeNode *pNode = pActor->pCostumeStack;

	if (pNode) {
		RdThing *pHeadThing = pNode->pHeadThing;

		do {
			if (pNode->pHeadThing != pHeadThing)
				break;

			if (pNode->mumbleChore >= 0)
				break;

			pNode = pNode->pNext;
		} while (pNode);

		if (pNode && pNode->pHeadThing == pHeadThing && pNode->mumbleChore >= 0 && _pCos->instChoreStatus(pNode->hCostumeInstance, pNode->mumbleChore))
			_pCos->instStopChore(pNode->hCostumeInstance, pNode->mumbleChore);

		pNode = pActor->pCostumeStack;
		pHeadThing = pNode->pHeadThing;

		do {
			if (pNode->pHeadThing != pHeadThing)
				break;

			if (pNode->aChore[0] >= 0)
				break;

			pNode = pNode->pNext;
		} while (pNode);

		if (pNode && pNode->pHeadThing == pHeadThing && pNode->aChore[0] >= 0)
			_pCos->instCompleteChore(pNode->hCostumeInstance, pNode->aChore[0]);
	}

	for (int i = 0; i < 4; i++) {
		if (_apTalkingActors[i] == pActor)
			_apTalkingActors[i] = nullptr;
	}

	pActor->pExtra->phoneme = 0;
	pActor->pExtra->voiceName[0] = 0;
	pActor->pExtra->pSayText = nullptr;
	pActor->pExtra->sayLineId[0] = 0;
	pActor->pExtra->talking = 0;

	if (_numActorsTalking > 0)
		_numActorsTalking--;
}

int SputActorModule::isVoicePlaying(SputActor *pActor) {
	Extra *pExtra = pActor->pExtra;

	if (!pExtra->voiceName[0])
		return 0;

	if (pExtra->soundId == kTalkSoundID)
		return g_imuse->imGetParam(kTalkSoundID, IMUSE_P_SND_TRACK_NUM) > 0;

	return g_imuse->soundsIsSoundPlaying(pExtra->voiceName);
}

} // End of namespace Grim
