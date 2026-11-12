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
#include "common/ustr.h"

#include "engines/grim/emi/actor/actor.h"
#include "engines/grim/emi/costume/choreinstance.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/sound/lipdata.h"
#include "engines/grim/emi/sound/sounds.h"
#include "engines/grim/emi/text/textevent.h"

namespace Grim {

static const int kVoiceIdSize = TextEvent::kTagBufferSize;
static const int kVoicePathSize = 36;
static const int kVoiceSoundMode = 2;
static const int kMouthSlot = 4;
static const int kFirstVisemeSlot = 6;
static const int kLastVisemeSlot = 14;
static const float kVisemeFadeTime = 0.0625f;
static const int kSpeechTextFlags = 13;
static const int kMinTextRate = 1;
static const int kMaxTextRate = 10;
static const int kTextMarginX = 40;
static const int kTextMarginY = 25;

static int truncateSpeechTime(double value) {
	if (!(value > -2147483649.0 && value < 2147483648.0)) {
		warning("Actor::updateActorSpeech(): the time is out of range");

		return -2147483647 - 1;
	}

	return (int)value;
}

void Actor::updateActorSpeech(float deltaTime) {
	if (!extra) {
		warning("Actor::updateActorSpeech(): missing extra data");

		return;
	}

	if (!extra->talking)
		return;

	if (extra->soundHandle && extra->lipsyncActive) {
		int mouthShape;

		if (_engine->isDumpingFrames()) {
			mouthShape = LipData::getLipMouthShape(_engine, (uint)extra->soundHandle, extra->visemeMs);
			extra->visemeMs = truncateSpeechTime((double)deltaTime * 1000.0 + (double)extra->visemeMs);
		} else {
			mouthShape = LipData::getLipMouthShape(_engine, (uint)extra->soundHandle, -1);
		}

		if (mouthShape == 0) {
			shutUp(false);
		} else if (mouthShape < 0 || mouthShape >= kNumChoreSlots) {
			warning("Actor::updateActorSpeech(): invalid mouth shape %d", mouthShape);
		} else if (mouthShape != extra->visemeCurrent) {
			int currentShape = extra->visemeCurrent;

			if (currentShape != 0) {
				if (currentShape < 0 || currentShape >= kNumChoreSlots) {
					warning("Actor::updateActorSpeech(): invalid current mouth shape %d", currentShape);
				} else {
					if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[currentShape]))
						ChoreInstance::getInstance(_engine, choreSlotIds[currentShape])->fadeOut(kVisemeFadeTime);

					choreSlotIds[currentShape] = 0;
				}
			}

			uint startedId;

			startChore(&startedId, choreSlotNames[mouthShape], choreSlotSetNames[mouthShape]);
			choreSlotIds[mouthShape] = startedId;

			if (ChoreInstance::isInstanceValid(_engine, startedId))
				ChoreInstance::getInstance(_engine, choreSlotIds[mouthShape])->fadeIn(kVisemeFadeTime);

			extra->visemeCurrent = mouthShape;
		}
	}

	int elapsedMs = truncateSpeechTime((double)deltaTime * -1000.0);

	extra->talkTimerMs = (int)((uint)extra->talkTimerMs + (uint)elapsedMs);

	if ((extra->soundHandle && !isVoicePlaying()) || (!extra->soundHandle && extra->talkTimerMs < 0))
		shutUp(false);
}

void Actor::sayLine(const char *text, int volume, int pan, int x, int y) {
	if (!text) {
		warning("Actor::sayLine(): the text is null");

		return;
	}

	if (!extra)
		allocateActorExtra();

	bool textEnabled = (_engine->_speechMode & 1) != 0;
	bool voiceEnabled = (_engine->_speechMode & 2) != 0;
	char voiceId[kVoiceIdSize];
	char voicePath[kVoicePathSize];
	const char *lineText = TextEvent::translate(_engine, text, voiceId);

	voicePath[0] = 0;

	if (voiceId[0] && _engine->getGamePlatform() == Common::kPlatformPS2) {
		Common::sprintf_s(voicePath, "%s.SCX", voiceId);

		if (!_engine->getFileServices()->exists(voicePath))
			voicePath[0] = 0;
	} else if (voiceId[0]) {
		Common::sprintf_s(voicePath, "%s.wvc", voiceId);

		LFile *voiceFile = _engine->getFileServices()->open(voicePath, "rb");

		if (!voiceFile) {
			Common::sprintf_s(voicePath, "%s.wav", voiceId);
			voiceFile = _engine->getFileServices()->open(voicePath, "rb");
		}

		if (voiceFile)
			_engine->getFileServices()->close(voiceFile, true);
		else
			voicePath[0] = 0;
	}

	if (extra->talking) {
		if (voiceId[0]) {
			if (!memchr(extra->sayLineId, 0, Extra_v2::kSayLineIdSize))
				warning("Actor::sayLine(): the current line id is not terminated");
			else if (strcmp(extra->sayLineId, voiceId) == 0)
				return;
		}

		shutUp(true);
	}

	extra->talkTimerMs = 0;
	extra->talking = true;

	int idLength = 0;

	while (idLength < Extra_v2::kSayLineIdSize && voiceId[idLength]) {
		extra->sayLineId[idLength] = voiceId[idLength];
		idLength++;
	}

	while (idLength < Extra_v2::kSayLineIdSize) {
		extra->sayLineId[idLength] = 0;
		idLength++;
	}

	_engine->_numSpeakers++;

	int speakerSlot = 0;

	while (speakerSlot < kNumSpeakers && _engine->_speakers[speakerSlot])
		speakerSlot++;

	if (speakerSlot < kNumSpeakers)
		_engine->_speakers[speakerSlot] = this;

	bool createText = textEnabled;

	if (voiceEnabled) {
		if (voicePath[0]) {
			extra->soundHandle = (int)_engine->getSounds()->loadSound(voicePath, kVoiceSoundMode);

			if (extra->soundHandle)
				_engine->getSounds()->startSound((uint)extra->soundHandle, 0, false, volume);

			extra->lipsyncActive = _engine->getSounds()->getSoundLipData((uint)extra->soundHandle) != nullptr;

			if (extra->lipsyncActive)
				extra->visemeMs = 0;
		}

		extra->talkTimerMs = -1;

		if (!extra->soundHandle)
			createText = true;
	}

	if (createText) {
		LString lineString(lineText);
		TextEvent *textEvent = createActorText(lineString, x, y);

		extra->textEventId = textEvent->id;

		if (!extra->soundHandle)
			extra->talkTimerMs = (int)(10u * ((uint)(11 - _engine->_actorTextRate) * (uint)strlen(lineText) + 64u));

		textEvent->activate(-1.0f, kSpeechTextFlags);
	}

	if (extra->lipsyncActive) {
		int visemeSlot = kFirstVisemeSlot;

		while (visemeSlot <= kLastVisemeSlot && choreSlotNames[visemeSlot].empty())
			visemeSlot++;

		if (visemeSlot > kLastVisemeSlot)
			extra->lipsyncActive = false;
	}

	if (!extra->lipsyncActive) {
		uint startedId;

		startChore(&startedId, choreSlotNames[kMouthSlot], choreSlotSetNames[kMouthSlot]);
		choreSlotIds[kMouthSlot] = startedId;

		if (ChoreInstance::isInstanceValid(_engine, startedId))
			ChoreInstance::getInstance(_engine, choreSlotIds[kMouthSlot])->setPlaying();
	}
}

TextEvent *Actor::createActorText(const LString &text, int x, int y) {
	TextEvent *textEvent = new TextEvent(_engine, text);

	textEvent->geometryReady = false;
	textEvent->clipRect[0] = _engine->_textScreenRect[0];
	textEvent->clipRect[1] = _engine->_textScreenRect[1];
	textEvent->clipRect[2] = _engine->_textScreenRect[2];
	textEvent->clipRect[3] = _engine->_textScreenRect[3];

	if (extra)
		textEvent->color = extra->talkColor;
	else
		warning("Actor::createActorText(): missing extra data");

	textEvent->justify = 6;
	textEvent->geometryReady = false;

	int rectX;
	int rectY;
	int rectWidth;

	if (!onScreen || _engine->getMode() == EmiEngine::kModeMovie) {
		rectX = _engine->_offscreenTextRect.x;
		rectY = _engine->_offscreenTextRect.y;
		rectWidth = _engine->_offscreenTextRect.w;
	} else {
		rectX = drawnRect[0];
		rectY = drawnRect[1];
		rectWidth = drawnRect[2];
	}

	int halfWidth = textEvent->getWidth() / 2;
	int textX = x;

	if (x == -1)
		textX = rectX + rectWidth / 2;

	if (textX - halfWidth < kTextMarginX)
		textX = halfWidth + kTextMarginX;

	if (textX + halfWidth > _engine->_textScreenRect[2] - kTextMarginX)
		textX = _engine->_textScreenRect[2] - halfWidth - kTextMarginX;

	textEvent->x = textX;
	textEvent->y = 0;
	textEvent->geometryReady = false;

	int height = textEvent->getHeight();
	int textY = y;

	if (y == -1)
		textY = rectY - height;

	if (textY < kTextMarginY)
		textY = kTextMarginY;

	if (textY + height > _engine->_textScreenRect[3] - kTextMarginY)
		textY = _engine->_textScreenRect[3] - height - kTextMarginY;

	textEvent->x = textX;
	textEvent->y = textY;
	textEvent->geometryReady = false;
	textEvent->actorId = id;
	actorFlags |= 4;

	return textEvent;
}

void Actor::shutUp(bool stopNow) {
	if (!extra)
		return;

	if (extra->soundHandle) {
		if (stopNow)
			_engine->getSounds()->stopSound((uint)extra->soundHandle);
		else
			_engine->getSounds()->releaseSoundWhenDone((uint)extra->soundHandle);

		extra->soundHandle = 0;
	}

	if (extra->textEventId) {
		TextEvent *textEvent = TextEvent::getEvent(_engine, extra->textEventId);

		if (textEvent)
			delete textEvent;
	}

	if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[kMouthSlot]))
		ChoreInstance::getInstance(_engine, choreSlotIds[kMouthSlot])->stop();

	choreSlotIds[kMouthSlot] = 0;

	int currentShape = extra->visemeCurrent;

	if (currentShape != 0) {
		if (currentShape < 0 || currentShape >= kNumChoreSlots) {
			warning("Actor::shutUp(): invalid current mouth shape %d", currentShape);
		} else {
			if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[currentShape]))
				ChoreInstance::getInstance(_engine, choreSlotIds[currentShape])->fadeOut(kVisemeFadeTime);

			choreSlotIds[currentShape] = 0;
		}

		extra->visemeCurrent = 0;
	}

	for (int slot = kFirstVisemeSlot; slot <= kLastVisemeSlot; slot++) {
		if (ChoreInstance::isInstanceValid(_engine, choreSlotIds[slot])) {
			// The disasm passes the string object itself as the %s argument, let's use the name...
			Common::String message = Common::String::format("%s still talk choring", actorName.c_str());

			warning("%s", message.c_str());
			_engine->showMessage(message.c_str(), "Warning:");
		}
	}

	for (int i = 0; i < kNumSpeakers; i++) {
		if (_engine->_speakers[i] == this)
			_engine->_speakers[i] = nullptr;
	}

	extra->lipsyncActive = false;
	extra->textEventId = 0;
	extra->sayLineId[0] = 0;
	extra->talking = false;

	if (_engine->_numSpeakers > 0)
		_engine->_numSpeakers--;
}

bool Actor::isVoicePlaying() {
	if (!extra) {
		warning("Actor::isVoicePlaying(): missing extra data");

		return false;
	}

	if (!extra->soundHandle)
		return false;

	return _engine->getSounds()->isSoundPlayingNoFiniteLoop((uint)extra->soundHandle);
}

int Actor::getNumActorsTalking(EmiEngine *engine) {
	return engine->_numSpeakers;
}

int Actor::setSpeechMode(EmiEngine *engine, int newSpeechMode) {
	if (newSpeechMode != engine->_speechMode && (newSpeechMode & 3) != 0)
		engine->_speechMode = newSpeechMode;

	return newSpeechMode;
}

int Actor::getSpeechMode(EmiEngine *engine) {
	return engine->_speechMode;
}

int Actor::setActorTextRate(EmiEngine *engine, int newTextRate) {
	if (newTextRate < kMinTextRate)
		newTextRate = kMinTextRate;
	else if (newTextRate > kMaxTextRate)
		newTextRate = kMaxTextRate;

	engine->_actorTextRate = newTextRate;

	return newTextRate;
}

int Actor::getActorTextRate(EmiEngine *engine) {
	return engine->_actorTextRate;
}

} // End of namespace Grim
