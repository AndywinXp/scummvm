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

#include <math.h>

#include "common/textconsole.h"

#include "engines/grim/emi/audiosystem.h"
#include "engines/grim/emi/caudiochannel.h"
#include "engines/grim/emi/cnode.h"
#include "engines/grim/emi/csnd.h"
#include "engines/grim/emi/csoundcue.h"
#include "engines/grim/emi/cwavesound.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/lstring.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/sounds.h"

namespace Grim {

static const uint kSndFlagsMask = 0x003FFFFF;
static const int kSavedPositionSize = 16;
static const int kSavedPositionOffset = 4;

CSoundCue *CSoundCue::firstCue = nullptr;
CSoundCue *CSoundCue::lastCue = nullptr;
int CSoundCue::cueCount = 0;
uint CSoundCue::createdCount = 0;
uint CSoundCue::destroyedCount = 0;
uint CSoundCue::updatedCount = 0;

CSoundCue::CSoundCue(LResourceHandle<CSnd> *sndHandle, CNode *node) {
	prev = nullptr;
	next = nullptr;
	sound = nullptr;
	priority = 0.0f;
	volume = 0.0f;
	mvol = 0.0f;
	pan = 0.0f;
	reserved48 = 0;
	freq = 0.0f;
	state = kStateNone;
	cutoffNear = 0.0f;
	cutoffFar = 0.0f;
	flags = 0;

	for (int i = 0; i < 5; i++)
		reserved72[i] = 0;

	nodeRef.cue = nullptr;
	channel = nullptr;
	group = 0;
	lipData = nullptr;
	lipSize = 0;
	handle = 0;
	category = 0;

	if (!sndHandle->lock() || !sndHandle->typed) {
		initDefaults(node);
	} else {
		CSnd *snd = sndHandle->typed;

		initFromSnd(snd, node);
		soundHandle.setResource(snd->getSoundHandle()->resource);

		if (soundHandle.lock())
			sound = dynamic_cast<CWaveSound *>(soundHandle.typed);
		else
			sound = nullptr;
	}

	sndHandle->unlock();

	nodeRef.cue = this;

	if (firstCue)
		firstCue->prev = this;

	prev = nullptr;
	next = firstCue;
	firstCue = this;

	if (!lastCue)
		lastCue = this;

	cueCount++;
}

CSoundCue::CSoundCue(const LString &name, CNode *node) {
	prev = nullptr;
	next = nullptr;
	sound = nullptr;
	priority = 0.0f;
	volume = 0.0f;
	mvol = 0.0f;
	pan = 0.0f;
	reserved48 = 0;
	freq = 0.0f;
	state = kStateNone;
	cutoffNear = 0.0f;
	cutoffFar = 0.0f;
	flags = 0;

	for (int i = 0; i < 5; i++)
		reserved72[i] = 0;

	nodeRef.cue = nullptr;
	channel = nullptr;
	group = 0;
	lipData = nullptr;
	lipSize = 0;
	handle = 0;
	category = 0;

	LString upperName(name);

	upperName.toUppercase();

	if (upperName.find(".SND") == LString::npos) {
		initDefaults(node);

		LResourceHandle<LSound> rawHandle(name);

		soundHandle.setResource(rawHandle.resource);

		if (soundHandle.lock())
			sound = dynamic_cast<CWaveSound *>(soundHandle.typed);
		else
			sound = nullptr;
	} else {
		LResourceHandle<CSnd> sndHandle(name);

		if (sndHandle.lock() && sndHandle.typed) {
			CSnd *snd = sndHandle.typed;

			initFromSnd(snd, node);
			soundHandle.setResource(snd->getSoundHandle()->resource);

			if (soundHandle.lock())
				sound = dynamic_cast<CWaveSound *>(soundHandle.typed);
			else
				sound = nullptr;
		} else {
			initDefaults(node);
		}

		sndHandle.unlock();
	}

	nodeRef.cue = this;

	if (firstCue)
		firstCue->prev = this;

	prev = nullptr;
	next = firstCue;
	firstCue = this;

	if (!lastCue)
		lastCue = this;

	cueCount++;
}

CSoundCue::~CSoundCue() {
	releaseChannel();
	soundHandle.unlock();
	sound = nullptr;

	if (lipData)
		delete[] reinterpret_cast<byte *>(lipData);

	lipData = nullptr;

	if (this == firstCue) {
		CSoundCue *oldFirst = firstCue;

		firstCue = firstCue->next;

		if (firstCue)
			firstCue->prev = nullptr;
		else
			lastCue = nullptr;

		oldFirst->prev = nullptr;
		oldFirst->next = nullptr;
		cueCount--;
	} else if (this == lastCue) {
		CSoundCue *oldLast = lastCue;

		lastCue = lastCue->prev;

		if (lastCue)
			lastCue->next = nullptr;
		else
			firstCue = nullptr;

		oldLast->next = nullptr;
		oldLast->prev = nullptr;
		cueCount--;
	} else if (next && prev) {
		next->prev = prev;
		prev->next = next;
		cueCount--;
		prev = nullptr;
		next = nullptr;
	} else if (next || prev) {
		warning("CSoundCue::~CSoundCue(): inconsistent cue list");
	}

	destroyedCount++;
}

void *CSoundCue::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void CSoundCue::operator delete(void *block) {
	gpMemoryServices->free(block);
}

bool CSoundCue::play(bool releaseWhenDone) {
	stop();

	if (releaseWhenDone)
		flags |= kFlagReleaseWhenDone;
	else
		flags &= ~kFlagReleaseWhenDone;

	startPlay(false);

	return channel != nullptr;
}

bool CSoundCue::startPlay(bool startPaused) {
	releaseChannel();

	if (!AudioSystem::isCueInRange(this))
		return channel != nullptr;

	channel = AudioSystem::allocateAudioChannel(this);

	if (!channel) {
		state = kStateWaiting;

		return channel != nullptr;
	}

	if (channel->assignToSample(this)) {
		channel->setVolume(getCategoryGain() * volume);
		channel->setPan(pan);

		if (startPaused) {
			channel->pause();

			return channel != nullptr;
		}

		channel->start();
		state = kStateStarted;

		return channel != nullptr;
	}

	releaseChannel();
	state = kStateWaiting;

	if (AudioSystem::debugLevel > 1) {
		const char *failedName = "<file_error>";

		if (soundHandle.resource)
			failedName = soundHandle.resource->name.c_str();

		warning("CSoundCue::startPlay(): failed to initialize channel for sound <%s>", failedName);
	}

	return channel != nullptr;
}

bool CSoundCue::startPlay() {
	releaseChannel();

	if (!AudioSystem::isCueInRange(this))
		return channel != nullptr;

	channel = AudioSystem::allocateAudioChannel(this);

	if (!channel) {
		state = kStateWaiting;

		return channel != nullptr;
	}

	if (channel->assignToSample(this)) {
		state = kStateStopped;

		return channel != nullptr;
	}

	releaseChannel();
	state = kStateWaiting;

	if (AudioSystem::debugLevel > 1) {
		const char *failedName = "<file_error>";

		if (soundHandle.resource)
			failedName = soundHandle.resource->name.c_str();

		warning("CSoundCue::startPlay(): failed to initialize channel for sound <%s>", failedName);
	}

	return channel != nullptr;
}

bool CSoundCue::update() {
	if ((flags & kFlagStartInRange) != 0) {
		bool inRange = AudioSystem::isCueInRange(this);

		if (inRange) {
			flags |= kFlagInRange;

			if (getState() != kStatePlaying)
				play(true);
		} else {
			flags &= ~kFlagInRange;

			if (getState() == kStatePlaying)
				stop();
		}
	}

	if (state == kStateWaiting || state == kStateStopped || state == kStateFinished) {
		if ((flags & kFlagStartInRange) == 0 && (flags & kFlagReleaseWhenDone) != 0)
			return false;
	}

	return true;
}

CSoundCue *CSoundCue::newSoundCue(LResourceHandle<CSnd> *sndHandle, int category, CNode *node) {
	CSoundCue *cue = nullptr;

	if (category == 0)
		warning("CSoundCue::newSoundCue(): the default category is not supported");
	else
		cue = new CSoundCue(sndHandle, node);

	if (cue)
		cue->category = category;

	if (!cue || cue->sound)
		return cue;

	deleteSoundCue(cue);

	return nullptr;
}

CSoundCue *CSoundCue::newSoundCue(const LString &name, int category, CNode *node) {
	CSoundCue *cue = nullptr;

	if (name.empty())
		return nullptr;

	LString fullName(name);
	LString extension;

	LStringUtil::getExtension(fullName, extension);
	extension.toUppercase();

	if (extension.empty()) {
		extension = "SND";
		fullName += ".";
		fullName += extension;
	}

	if (extension == "SND") {
		LResourceHandle<CSnd> sndHandle(fullName);

		cue = newSoundCue(&sndHandle, category, node);
	} else {
		switch (category) {
		case 0:
			warning("CSoundCue::newSoundCue(): the default category is not supported");

			break;

		case 2:
			cue = new CSoundCue(name, node);
			cue->category = 2;

			break;

		case 3:
			warning("CSoundCue::newSoundCue(): the music category is not supported");

			break;

		case 4:
			break;

		default:
			cue = new CSoundCue(name, node);
			cue->category = category;

			break;
		}
	}

	if (cue && !cue->sound) {
		deleteSoundCue(cue);
		cue = nullptr;
	}

	return cue;
}

void CSoundCue::deleteSoundCue(CSoundCue *cue) {
	if (cue) {
		cue->stop();
		delete cue;
	} else {
		warning("CSoundCue::deleteSoundCue(): passed a NULL pointer");
	}
}

void CSoundCue::releaseAllCues() {
	while (cueCount) {
		if (!firstCue) {
			warning("CSoundCue::releaseAllCues(): inconsistent cue list");

			break;
		}

		delete firstCue;
	}
}

void CSoundCue::releaseUnlockedCues() {
	CSoundCue *cue = firstCue;

	while (cue) {
		CSoundCue *current = cue;

		cue = cue->next;

		if (!Lua_V2::isSoundLocked((int16)current->handle)) {
			CAudioChannel *currentChannel = current->channel;

			if (!currentChannel || !currentChannel->isPaused()) {
				Sounds::unregisterSound(current->handle);
				deleteSoundCue(current);
			}
		}
	}
}

void CSoundCue::updateSoundCues() {
	CSoundCue *cue = firstCue;

	updatedCount = 0;

	while (cue) {
		CSoundCue *current = cue;

		updatedCount++;
		cue = cue->next;

		if (!current->update() && !Sounds::isCueRegistered(current))
			deleteSoundCue(current);
	}
}

bool CSoundCue::stopCueToFreeChannel(bool finiteLoopsFirst, bool anyNonLooping) {
	CSoundCue *found = nullptr;

	if (finiteLoopsFirst) {
		CSoundCue *cue = firstCue;

		while (cue) {
			CSoundCue *current = cue;

			cue = cue->next;

			if (current->category != 2 && current->hasFiniteLoopCount()) {
				found = current;

				break;
			}
		}
	}

	if (!found) {
		if (!anyNonLooping)
			return false;

		CSoundCue *cue = firstCue;

		while (cue) {
			CSoundCue *current = cue;

			cue = cue->next;

			if (current->category != 2 && (current->flags & kFlagLoop) == 0) {
				found = current;

				break;
			}
		}

		if (!found)
			return false;
	}

	found->stop();

	return true;
}

bool CSoundCue::saveCues(tSaveRestoreAccess *access) {
	int16 savedCount = (int16)cueCount;
	CSoundCue *cue = firstCue;

	access(&savedCount, sizeof(savedCount));

	while (cue) {
		CSoundCue *current = cue;

		cue = cue->next;
		current->save(access);
	}

	return true;
}

bool CSoundCue::restoreCues(tSaveRestoreAccess *access, SoundCueMap *cueMap) {
	LString cueName;
	int16 restoredCount = 0;
	int16 nameLength = 0;
	int category = 0;
	byte loopFlag = 0;
	byte pausedFlag = 0;
	float restoredVolume = 0.0f;
	byte positionBytes[kSavedPositionSize];
	int restoredPositionMs = 0;
	float restoredPan = 0.0f;
	uint16 restoredHandle = 0;

	access(&restoredCount, sizeof(restoredCount));
	memset(positionBytes, 0, sizeof(positionBytes));

	for (int16 i = 0; i < restoredCount; i++) {
		access(&nameLength, sizeof(nameLength));

		if (nameLength < 0) {
			warning("CSoundCue::restoreCues(): invalid name length %d", (int)nameLength);

			return false;
		}

		char *nameBuffer = new char[nameLength + 1];

		access(nameBuffer, nameLength);
		nameBuffer[nameLength] = 0;
		cueName = nameBuffer;
		delete[] nameBuffer;

		access(&category, sizeof(category));
		access(&loopFlag, sizeof(loopFlag));
		access(&pausedFlag, sizeof(pausedFlag));
		access(&restoredVolume, sizeof(restoredVolume));
		access(positionBytes, kSavedPositionSize);
		access(&restoredPositionMs, sizeof(restoredPositionMs));
		access(&restoredPan, sizeof(restoredPan));
		access(&restoredHandle, sizeof(restoredHandle));

		CSoundCue *cue = newSoundCue(cueName, category, nullptr);

		if (!cue)
			continue;

		if (loopFlag)
			cue->flags |= kFlagLoop;
		else
			cue->flags &= ~kFlagLoop;

		if (pausedFlag) {
			cue->startPlay(true);

			if (cue->channel)
				cue->channel->setPaused(true);
		} else if (cue->channel) {
			cue->channel->setPaused(false);
		}

		if (AudioSystem::lipCallback && cue->category == 2)
			AudioSystem::lipCallback(cueName.c_str(), cue);

		SoundPosition restoredPosition;

		restoredPosition.valid = positionBytes[0] != 0;
		memcpy(&restoredPosition.pos, positionBytes + kSavedPositionOffset, sizeof(restoredPosition.pos));

		cue->setVolume(restoredVolume);
		cue->setPosition(restoredPosition);
		cue->setPositionMs(restoredPositionMs);
		cue->setPan(restoredPan);

		if (restoredHandle) {
			(*cueMap)[restoredHandle] = cue;
			cue->handle = restoredHandle;
		}
	}

	return true;
}

int CSoundCue::getSoundType() {
	if (!soundHandle.resource)
		return CWaveSound::kTypeWave;

	LSound *lockedSound = nullptr;

	if (soundHandle.lock())
		lockedSound = soundHandle.typed;

	if (!lockedSound)
		return CWaveSound::kTypeWave;

	int type = lockedSound->getType();

	soundHandle.unlock();

	return type;
}

bool CSoundCue::start(bool releaseWhenDone) {
	if ((flags & kFlagStartInRange) != 0)
		return false;

	return play(releaseWhenDone);
}

void CSoundCue::stop() {
	if (channel)
		channel->stop();
}

void CSoundCue::pause() {
	if (channel)
		channel->pause();
}

void CSoundCue::resume() {
	if (channel)
		channel->resume();
}

float CSoundCue::getPriority() {
	return priority;
}

float CSoundCue::getVolume() {
	return volume;
}

float CSoundCue::getPan() {
	return pan;
}

float CSoundCue::getCutoffNear() {
	return cutoffNear;
}

float CSoundCue::getCutoffFar() {
	return cutoffFar;
}

float CSoundCue::getCategoryGain() {
	switch (category) {
	case 1:
		return AudioSystem::getCategoryGain(1);

	case 2:
		return AudioSystem::getCategoryGain(2);

	case 3:
		return AudioSystem::getCategoryGain(3);

	default:
		break;
	}

	return 1.0f;
}

void CSoundCue::setVolume(float newVolume) {
	volume = newVolume;

	float gain = getCategoryGain();

	if (channel)
		channel->setVolume(gain * newVolume);
}

void CSoundCue::setPan(float newPan) {
	pan = newPan;

	if (channel)
		channel->setPan(newPan);
}

bool CSoundCue::setPosition(float x, float y, float z) {
	position.set(x, y, z);
	updateSpatialization();

	return true;
}

bool CSoundCue::setPosition(const SoundPosition &newPosition) {
	position = newPosition;
	updateSpatialization();

	return true;
}

bool CSoundCue::updateSpatialization() {
	if (!position.valid)
		return false;

	float worldX = position.pos.x;
	float worldY = position.pos.y;
	float worldZ = position.pos.z;
	float localX = worldX;
	float localY = worldY;
	float localZ = worldZ;
	float panValue;
	float volumeFactor = 1.0f;

	if (AudioSystem::listener.node)
		AudioSystem::listener.node->worldToLocal(&worldX, &worldY, &worldZ, &localX, &localY, &localZ);

	if (localZ < 0.0f || localZ > 0.0f) {
		double ratio = (double)localX / (double)localZ;
		double twiceRatio = ratio + ratio;

		panValue = (float)twiceRatio;

		if (!(twiceRatio >= 0.0))
			panValue = panValue * -1.0f;

		if (panValue > 1.0f)
			panValue = 1.0f;

		if (!(localX >= 0.0f))
			panValue = panValue * -1.0f;

		if (!(localZ >= 0.0f)) {
			volumeFactor = 0.125f;
		} else {
			double ratioSquared = ratio * ratio;

			if (ratioSquared > (double)0.2f) {
				double reduced = 1.0 - (ratioSquared - (double)0.2f);

				volumeFactor = (float)reduced;

				if (reduced < 0.125)
					volumeFactor = 0.125f;
			}
		}
	} else {
		volumeFactor = 0.125f;

		if (!(localX >= 0.0f)) {
			panValue = -1.0f;
		} else {
			panValue = 0.0f;

			if (localX > 0.0f)
				panValue = 1.0f;
		}
	}

	setPan(panValue);

	double distanceSquared = (double)localZ * localZ + (double)localY * localY + (double)localX * localX;

	volumeFactor = volumeFactor * volume;

	if (distanceSquared > 9.0) {
		double scaled = (sqrt(distanceSquared) - (double)3.0f) * (double)0.12f;

		volumeFactor = (float)(9.0 / (scaled * scaled + 9.0) * volumeFactor);
	}

	float gain = getCategoryGain();

	if (channel)
		channel->setVolume(gain * volumeFactor);

	return true;
}

bool CSoundCue::hasFiniteLoopCount() {
	return channel && channel->hasFiniteLoopCount();
}

bool CSoundCue::getPosition(CVector *outPosition) {
	if ((flags & kFlagPositional) == 0)
		return false;

	CNode *followedNode = nodeRef.node;

	if (followedNode) {
		if ((followedNode->nodeFlags & CNode::kWorldValid) == 0)
			followedNode->updateWorldTransform();

		*outPosition = followedNode->world.pos;

		return ((flags >> 29) & 1) != 0;
	}

	*outPosition = fixedPosition;

	return ((flags >> 29) & 1) != 0;
}

int CSoundCue::initDefaults(CNode *node) {
	priority = 0.5f;
	volume = 1.0f;
	mvol = 1.0f;
	pan = 0.0f;
	freq = 1.0f;
	cutoffNear = 1.0f;
	cutoffFar = 50.0f;
	flags = 0;
	channel = nullptr;
	fixedPosition = CVector();
	state = kStateNone;
	reserved48 = 0;

	for (int i = 0; i < 5; i++)
		reserved72[i] = 0;

	soundName.clear();
	lipData = nullptr;

	if (node) {
		nodeRef.setNode(node);
		flags |= kFlagPositional;
	}

	createdCount++;

	return (int)createdCount;
}

void CSoundCue::initFromSnd(CSnd *snd, CNode *node) {
	initDefaults(node);

	if (!snd)
		return;

	volume = snd->getVolume();
	mvol = snd->getMvol();
	priority = snd->getPrio();
	pan = snd->getPan();
	cutoffNear = snd->getCutoff0();
	cutoffFar = snd->getCutoff1();
	freq = snd->getFreq();
	flags |= (uint)snd->getFlags() & kSndFlagsMask;
	group = snd->getGroup();
	soundName = snd->name;
}

void CSoundCue::setLipData(const void *data, uint size) {
	if (lipData && lipSize < size) {
		warning("CSoundCue::setLipData(): the lip data does not fit in the old buffer");

		delete[] reinterpret_cast<byte *>(lipData);
		lipData = nullptr;
	}

	if (!lipData)
		lipData = reinterpret_cast<LipData *>(new byte[size]);

	memcpy(lipData, data, size);
	lipSize = size;
}

void CSoundCue::releaseChannel() {
	if (channel)
		channel->release();

	channel = nullptr;
}

void CSoundCue::unlinkFromCueList() {
	if (firstCue != this && !prev)
		return;

	if (prev)
		prev->next = next;
	else
		firstCue = next;

	if (next)
		next->prev = prev;
	else
		lastCue = prev;

	prev = nullptr;
	next = nullptr;
	cueCount--;
}

int CSoundCue::getState() {
	int channelStatus = 0;

	if (channel)
		channelStatus = channel->getStatus();

	switch (channelStatus) {
	case 2:
		return 5;

	case 3:
	case 4:
		return 2;

	case 5:
		return 3;

	default:
		break;
	}

	return 0;
}

int CSoundCue::getPositionMs(int *totalMs) {
	if (channel)
		return channel->getPositionMs(totalMs);

	if (totalMs)
		*totalMs = 0;

	return 0;
}

void CSoundCue::setPositionMs(int milliseconds) {
	if (channel)
		channel->setPositionMs(milliseconds);
}

bool CSoundCue::save(tSaveRestoreAccess *access) {
	const char *savedName = "<file_error>";

	if (soundHandle.resource)
		savedName = soundHandle.resource->name.c_str();

	int16 savedLength = (int16)strlen(savedName);
	byte positionBytes[kSavedPositionSize];
	byte loopFlag = 0;
	byte pausedFlag = 0;

	memset(positionBytes, 0, sizeof(positionBytes));
	positionBytes[0] = position.valid ? 1 : 0;
	memcpy(positionBytes + kSavedPositionOffset, &position.pos, sizeof(position.pos));

	if (channel)
		pausedFlag = channel->isPaused() ? 1 : 0;

	loopFlag = (flags & kFlagLoop) != 0 ? 1 : 0;

	int savedPositionMs = getPositionMs(nullptr);

	access(&savedLength, sizeof(savedLength));
	access(const_cast<char *>(savedName), savedLength);
	access(&category, sizeof(category));
	access(&loopFlag, sizeof(loopFlag));
	access(&pausedFlag, sizeof(pausedFlag));
	access(&volume, sizeof(volume));
	access(positionBytes, kSavedPositionSize);
	access(&savedPositionMs, sizeof(savedPositionMs));
	access(&pan, sizeof(pan));
	access(&handle, sizeof(handle));

	return true;
}

} // End of namespace Grim
