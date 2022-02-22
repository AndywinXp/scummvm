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
#include "common/stream.h"

#include "engines/grim/resource.h"

#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/imuse/imuse_sndmgr.h"
#include "engines/grim/imuse/imuse_mcmp_mgr.h"

#include "engines/grim/movie/codecs/vima.h"

namespace Grim {

uint16 vimaTable[5786];

ImuseSndMgr::ImuseSndMgr(bool demo, Imuse *engine) {
	_demo = demo;
	_engine = engine;

	vimaInit(vimaTable);

	for (int l = 0; l < MAX_IMUSE_SOUNDS; l++) {
		memset(&_Oldsounds[l], 0, sizeof(SoundDesc));
		memset(&_resources[l], 0, sizeof(SndResource));
		memset(&_iMUSESounds[l], 0, sizeof(SoundInfo));
	}
}

ImuseSndMgr::~ImuseSndMgr() {
	for (int l = 0; l < MAX_IMUSE_SOUNDS; l++) {
		closeSound(&_Oldsounds[l]);
	}
}

void ImuseSndMgr::countElements(SoundDesc *sound) {
	uint32 tag;
	int32 size = 0;
	uint32 pos = sound->inStream->pos();

	do {
		tag = sound->inStream->readUint32BE();
		switch(tag) {
		case MKTAG('T','E','X','T'):
		case MKTAG('S','T','O','P'):
		case MKTAG('F','R','M','T'):
			size = sound->inStream->readUint32BE();
			sound->inStream->seek(size, SEEK_CUR);
			break;
		case MKTAG('R','E','G','N'):
			sound->numRegions++;
			size = sound->inStream->readUint32BE();
			sound->inStream->seek(size, SEEK_CUR);
			break;
		case MKTAG('J','U','M','P'):
			sound->numJumps++;
			size = sound->inStream->readUint32BE();
			sound->inStream->seek(size, SEEK_CUR);
			break;
		case MKTAG('D','A','T','A'):
			break;
		default:
			error("ImuseSndMgr::countElements() Unknown MAP tag '%s'", Common::tag2string(tag).c_str());
		}
	} while (tag != MKTAG('D','A','T','A'));

	sound->inStream->seek(pos, SEEK_SET);
}

int ImuseSndMgr::nukeOldestSound() {
	int foundId;
	SoundInfo *sound;
	int luaSoundId;
	signed int oldestTs;
	int oldestSlotId;

	foundId = -1;
	oldestTs = g_system->getMillis();

	oldestSlotId = -1;

	for (int i = 0; i < MAX_IMUSE_SOUNDS; i++) {
		if (_iMUSESounds[i].soundFilename[0] && _iMUSESounds[i].soundUncompData) {
			if (!_engine->diMUSEGetParam(i + 100, DIMUSE_P_SND_TRACK_NUM) &&
				!_engine->diMUSEGetParam(i + 100, DIMUSE_P_TRIGS_SNDS) &&
				(foundId < 0 || oldestTs > _iMUSESounds[i].tsOfAllocation)) {
				foundId = i;
				oldestTs = _iMUSESounds[i].tsOfAllocation;
			}
		} else {
			oldestSlotId = i;
		}
	}

	if (foundId >= 0) {
		sound = &_iMUSESounds[foundId];
		luaSoundId = sound->luaSoundId;
		sound->soundFilename[0] = '\0';
		resetLipsyncState(luaSoundId);
		if (sound->soundUncompData)
			free(sound->soundUncompData);
		memset(sound, 0, sizeof(SoundInfo));
		return foundId;
	} else {
		if (oldestSlotId < 0)
			return -1;
		return oldestSlotId;
	}
}

intptr ImuseSndMgr::openSound(const char *soundName) {
	int targetResId;
	uint8 *mcmpTable, *allocatedBuf, *curCompBlockPtr;
	int32 cumulativeSize;
	byte codec;

	targetResId = getExistingResource(soundName);

	if (targetResId) {
		return (intptr)targetResId;
	}

	Common::SeekableReadStream *data = g_resourceloader->openNewStreamFile(soundName);

	if (data) {
		Common::StackLock lock(_mutex);
		// Search for an empty resource slot
		int effNextResIndex = _nextResIndex;
		while (_resources[effNextResIndex].resId) {
			if ((effNextResIndex + 1) % MAX_IMUSE_SOUNDS != _nextResIndex) {
				effNextResIndex = (effNextResIndex + 1) % MAX_IMUSE_SOUNDS;
			} else {
				break;
			}
		}

		SndResource *selectedRes = &_resources[effNextResIndex];

		if (selectedRes->resId) {
			// All slots are full, abort
			delete data;
			return (intptr)nullptr;
		} else {
			_nextResIndex = effNextResIndex;
			//Common::StackLock unlock(_mutex);

			selectedRes->resId = effNextResIndex;
			selectedRes->fileCurOffset = 0;

			uint32 tag = data->readUint32BE();
			if (tag == MKTAG('M', 'C', 'M', 'P')) {
				uint16 numOfCompBlocks = data->readUint16BE();

				selectedRes->compBlocksNum = numOfCompBlocks;
				data->seek(9 * numOfCompBlocks + 6, SEEK_SET);

				// This value is used to skip a section of the header which contains comp item tags ("NULL", "VIMA")
				uint16 offsetSkipTags = data->readUint16BE();

				mcmpTable = (uint8 *)malloc(9 * numOfCompBlocks);

				// 5 as in: the number of arrays we're creating (see below); while this is sort of a messy way
				// to do it, it allows us to just write "free(res->mcmpBlockPtr)" to deallocate every array
				allocatedBuf = (uint8 *)malloc(offsetSkipTags + (sizeof(int32) * 5) * (numOfCompBlocks + 1));

				selectedRes->mcmpBlockPtr = allocatedBuf;
				selectedRes->compBlockPtr = (int32 **)&allocatedBuf[offsetSkipTags];
				selectedRes->decompBlockSizes = (int32 *)&allocatedBuf[offsetSkipTags + sizeof(int32) * (numOfCompBlocks + 1)];
				selectedRes->compBlockOffsets = (int32 *)&allocatedBuf[offsetSkipTags + sizeof(int32) * 2 * (numOfCompBlocks + 1)];
				selectedRes->compBlockSizes = (int32 *)&allocatedBuf[offsetSkipTags + sizeof(int32) * 3 * (numOfCompBlocks + 1)];
				selectedRes->vimaOffsets = (int32 *)&allocatedBuf[offsetSkipTags + sizeof(int32) * 4 * (numOfCompBlocks + 1)];

				// Remember, at this point the file pointer is at (9 * numOfCompBlocks + 6), so the next thing
				// we're reading is a block of size offsetSkipTags which contains the comp item names...
				data->read(allocatedBuf, offsetSkipTags);

				// ...then go back and read the whole compression map (where each item is 9 bytes long)
				data->seek(6, SEEK_SET);
				data->read(mcmpTable, 9 * numOfCompBlocks);

				cumulativeSize = 0;

				int32 curVIMAOffset = 9 * numOfCompBlocks + offsetSkipTags + 8;

				// Now parse the compression map and populate the arrays
				for (int i = 0; i < numOfCompBlocks; i++) {
					curCompBlockPtr = selectedRes->mcmpBlockPtr;
					codec = (byte)mcmpTable[0];

					for (int i = 0; i < codec; i++) {
						// Check the first byte (the codec id);
						// A zero codec id means this is an uncompressed block
						if (curCompBlockPtr[0]) {
							curCompBlockPtr++;

							// Run through all the bytes after the codec id
							// until we reach a zero byte
							while (*curCompBlockPtr)
								curCompBlockPtr++;

							// Count the zero byte
							curCompBlockPtr++;
						}
					}

					mcmpTable += sizeof(byte);
					selectedRes->decompBlockSizes[i] = READ_BE_INT32(mcmpTable);
					mcmpTable += sizeof(int32);
					selectedRes->compBlockSizes[i] = READ_BE_INT32(mcmpTable);
					mcmpTable += sizeof(int32);
					selectedRes->compBlockPtr[i] = (int32 *)curCompBlockPtr;
					selectedRes->compBlockOffsets[i] = cumulativeSize;
					selectedRes->vimaOffsets[i] = curVIMAOffset;

					cumulativeSize += selectedRes->decompBlockSizes[i];
					curVIMAOffset += selectedRes->compBlockSizes[i];
				}

				selectedRes->compBlockOffsets[numOfCompBlocks] = cumulativeSize;
				selectedRes->vimaOffsets[numOfCompBlocks] = curVIMAOffset;
				selectedRes->resSize = cumulativeSize;
				free(mcmpTable);
				vimaClearHeader(&selectedRes->vimaHeader);
				selectedRes->curDecompBlockSize = 0;
				selectedRes->decompDataBuffer = nullptr;
				selectedRes->curProcessedCompBlock = -1;
			} else {
				data->seek(0, SEEK_SET);
				// TODO: Remember to check if this is actually right
				selectedRes->resSize = data->size();
			}

			strncpy(selectedRes->filename, soundName, sizeof(selectedRes->filename));
			selectedRes->filename[79] = '\0';
			selectedRes->fileStream = data;

			return (intptr)selectedRes;
		}
	}

	return (intptr)nullptr;
}

void ImuseSndMgr::closeSound(intptr resPtrOrId) {
	SndResource *res;
	if (isPointerToResource(resPtrOrId)) {
		res = (SndResource *)resPtrOrId;
	} else {
		res = &_resources[(int)resPtrOrId];
	}

	int resId = res->resId;
	if (res->mcmpBlockPtr) {
		free(res->mcmpBlockPtr);
		res->mcmpBlockPtr = nullptr;
	}

	if (res->decompDataBuffer)
		free(res->decompDataBuffer);

	delete res->fileStream;

	memset(res, 0, sizeof(SndResource));
}

int ImuseSndMgr::getExistingResource(const char *soundName) {
	for (int i = 0; i < MAX_IMUSE_SOUNDS; i++) {
		if (!scumm_stricmp(soundName, _resources[i].filename)) {
			return i;
		}
	}

	return 0;
}

void ImuseSndMgr::soundSeek(intptr resPtrOrId, int offset, int mode) {
	SndResource *res;
	if (isPointerToResource(resPtrOrId)) {
		res = (SndResource *)resPtrOrId;
	} else {
		res = &_resources[(int)resPtrOrId];
	}

	switch (mode) {
	case SEEK_SET:
		res->fileCurOffset = offset;
		break;
	case SEEK_CUR:
		res->fileCurOffset += offset;
		break;
	case SEEK_END:
		res->fileCurOffset = offset + res->resSize;
		break;
	};
}

int ImuseSndMgr::soundTell(intptr resPtrOrId) {
	SndResource *res;
	if (isPointerToResource(resPtrOrId)) {
		res = (SndResource *)resPtrOrId;
	} else {
		res = &_resources[(int)resPtrOrId];
	}

	return res->fileCurOffset;
}

void ImuseSndMgr::resetLipsyncState(int soundId) {
}

void ImuseSndMgr::registerLipsync(const char *lipFilename, int soundId) {
}

void ImuseSndMgr::vimaClearHeader(VIMAHeader *header) {
	header->chanTwoStepIndexHint = 0;
	header->chanOneStepIndexHint = 0;
	header->chanTwoPCMHint = 0;
	header->chanOnePCMHint = 0;
}

bool ImuseSndMgr::isPointerToResource(intptr resPtrOrId) {
	for (int i = 0; i < MAX_IMUSE_SOUNDS; i++) {
		if (&_resources[i] == (SndResource *)resPtrOrId)
			return true;
	}

	return false;
}

void ImuseSndMgr::parseSoundHeader(SoundDesc *sound, int &headerSize) {
	Common::SeekableReadStream *data = sound->inStream;

	uint32 tag = data->readUint32BE();
	if (tag == MKTAG('R','I','F','F')) {
		sound->region = new Region[1];
		sound->jump = new Jump[1];
		sound->numJumps = 0;
		sound->numRegions = 1;
		sound->region[0].offset = 0;
		data->seek(18, SEEK_CUR);
		sound->channels = data->readByte();
		data->readByte();
		sound->freq = data->readUint32LE();
		data->seek(6, SEEK_CUR);
		sound->bits = data->readByte();
		data->seek(5, SEEK_CUR);
		sound->region[0].length = data->readUint32LE();
		headerSize = 44;
	} else if (tag == MKTAG('i','M','U','S')) {
		int32 size = 0;
		int32 headerStart = data->pos();
		data->seek(12, SEEK_CUR);

		int curIndexRegion = 0;
		int curIndexJump = 0;

		sound->numRegions = 0;
		sound->numJumps = 0;
		countElements(sound);
		sound->region = new Region [sound->numRegions];
		sound->jump = new Jump [sound->numJumps];

		do {
			tag = data->readUint32BE();
			switch(tag) {
			case MKTAG('F','R','M','T'):
				data->seek(12, SEEK_CUR);
				sound->bits = data->readUint32BE();
				sound->freq = data->readUint32BE();
				sound->channels = data->readUint32BE();
				break;
			case MKTAG('T','E','X','T'):
			case MKTAG('S','T','O','P'):
				size = data->readUint32BE();
				data->seek(size, SEEK_CUR);
				break;
			case MKTAG('R','E','G','N'):
				data->seek(4, SEEK_CUR);
				sound->region[curIndexRegion].offset = data->readUint32BE();
				sound->region[curIndexRegion].length = data->readUint32BE();
				curIndexRegion++;
				break;
			case MKTAG('J','U','M','P'):
				data->seek(4, SEEK_CUR);
				sound->jump[curIndexJump].offset = data->readUint32BE();
				sound->jump[curIndexJump].dest = data->readUint32BE();
				sound->jump[curIndexJump].hookId = data->readUint32BE();
				sound->jump[curIndexJump].fadeDelay = data->readUint32BE();
				curIndexJump++;
				break;
			case MKTAG('D','A','T','A'):
				data->seek(4, SEEK_CUR);
				break;
			default:
				error("ImuseSndMgr::prepareSound(%s) Unknown MAP tag '%s'", sound->name, Common::tag2string(tag).c_str());
			}
		} while (tag != MKTAG('D','A','T','A'));
		headerSize = data->pos() - headerStart;
		int i;
		for (i = 0; i < sound->numRegions; i++) {
			sound->region[i].offset -= headerSize;
		}
		for (i = 0; i < sound->numJumps; i++) {
			sound->jump[i].offset -= headerSize;
			sound->jump[i].dest -= headerSize;
		}
	} else {
		error("ImuseSndMgr::prepareSound() Unknown sound format");
	}
}

ImuseSndMgr::SoundDesc *ImuseSndMgr::allocSlot() {
	for (int l = 0; l < MAX_IMUSE_SOUNDS; l++) {
		if (!_Oldsounds[l].inUse) {
			_Oldsounds[l].inUse = true;
			return &_Oldsounds[l];
		}
	}

	return nullptr;
}

ImuseSndMgr::SoundDesc *ImuseSndMgr::openSound(const char *soundName, int volGroupId) {
	Common::String s = soundName;
	s.toLowercase();
	soundName = s.c_str();
	const char *extension = soundName + strlen(soundName) - 3;
	int headerSize = 0;

	SoundDesc *sound = allocSlot();
	if (!sound) {
		error("ImuseSndMgr::openSound() Can't alloc free sound slot");
	}

	strcpy(sound->name, soundName);
	sound->volGroupId = volGroupId;
	sound->inStream = nullptr;

	sound->inStream = g_resourceloader->openNewStreamFile(soundName);
	if (!sound->inStream) {
		closeSound(sound);
		return nullptr;
	}

	if (!_demo && scumm_stricmp(extension, "imu") == 0) {
		parseSoundHeader(sound, headerSize);
		sound->mcmpData = false;
		sound->headerSize = headerSize;
	} else if (scumm_stricmp(extension, "wav") == 0 || scumm_stricmp(extension, "imc") == 0 ||
			(_demo && scumm_stricmp(extension, "imu") == 0)) {
		sound->mcmpMgr = new McmpMgr();
		if (!sound->mcmpMgr->openSound(soundName, sound->inStream, headerSize)) {
			closeSound(sound);
			return nullptr;
		}
		parseSoundHeader(sound, headerSize);
		sound->mcmpData = true;
	} else {
		error("ImuseSndMgr::openSound() Unrecognized extension for sound file %s", soundName);
	}

	return sound;
}

void ImuseSndMgr::closeSound(SoundDesc *sound) {
	assert(checkForProperHandle(sound));

	if (sound->mcmpMgr) {
		delete sound->mcmpMgr;
		sound->mcmpMgr = nullptr;
	}

	if (sound->region) {
		delete[] sound->region;
		sound->region = nullptr;
	}

	if (sound->jump) {
		delete[] sound->jump;
		sound->jump = nullptr;
	}

	if (sound->inStream) {
		delete sound->inStream;
		sound->inStream = nullptr;
	}

	memset(sound, 0, sizeof(SoundDesc));
}

ImuseSndMgr::SoundDesc *ImuseSndMgr::cloneSound(SoundDesc *sound) {
	assert(checkForProperHandle(sound));

	return openSound(sound->name, sound->volGroupId);
}

bool ImuseSndMgr::checkForProperHandle(SoundDesc *sound) {
	if (!sound)
		return false;

	for (int l = 0; l < MAX_IMUSE_SOUNDS; l++) {
		if (sound == &_Oldsounds[l])
			return true;
	}

	return false;
}

int ImuseSndMgr::getFreq(SoundDesc *sound) {
	assert(checkForProperHandle(sound));
	return sound->freq;
}

int ImuseSndMgr::getBits(SoundDesc *sound) {
	assert(checkForProperHandle(sound));
	return sound->bits;
}

int ImuseSndMgr::getChannels(SoundDesc *sound) {
	assert(checkForProperHandle(sound));
	return sound->channels;
}

bool ImuseSndMgr::isEndOfRegion(SoundDesc *sound, int region) {
	assert(checkForProperHandle(sound));
	assert(region >= 0 && region < sound->numRegions);
	return sound->endFlag;
}

int ImuseSndMgr::getNumRegions(SoundDesc *sound) {
	assert(checkForProperHandle(sound));
	return sound->numRegions;
}

int ImuseSndMgr::getNumJumps(SoundDesc *sound) {
	assert(checkForProperHandle(sound));
	return sound->numJumps;
}

int ImuseSndMgr::getRegionOffset(SoundDesc *sound, int region) {
	assert(checkForProperHandle(sound));
	assert(region >= 0 && region < sound->numRegions);
	return sound->region[region].offset;
}

int ImuseSndMgr::getRegionLength(SoundDesc *sound, int region) {
	assert(checkForProperHandle(sound));
	assert(region >= 0 && region < sound->numRegions);
	return sound->region[region].length;
}

int ImuseSndMgr::getJumpIdByRegionAndHookId(SoundDesc *sound, int region, int hookId) {
	assert(checkForProperHandle(sound));
	assert(region >= 0 && region < sound->numRegions);
	int32 offset = sound->region[region].offset;
	for (int l = 0; l < sound->numJumps; l++) {
		if (offset == sound->jump[l].offset) {
			if (sound->jump[l].hookId == hookId)
				return l;
		}
	}

	return -1;
}

int ImuseSndMgr::getRegionIdByJumpId(SoundDesc *sound, int jumpId) {
	assert(checkForProperHandle(sound));
	assert(jumpId >= 0 && jumpId < sound->numJumps);
	int32 dest = sound->jump[jumpId].dest;
	for (int l = 0; l < sound->numRegions; l++) {
		if (dest == sound->region[l].offset) {
			return l;
		}
	}

	return -1;
}

int ImuseSndMgr::getJumpHookId(SoundDesc *sound, int number) {
	assert(checkForProperHandle(sound));
	assert(number >= 0 && number < sound->numJumps);
	return sound->jump[number].hookId;
}

int ImuseSndMgr::getJumpFade(SoundDesc *sound, int number) {
	assert(checkForProperHandle(sound));
	assert(number >= 0 && number < sound->numJumps);
	return sound->jump[number].fadeDelay;
}

int ImuseSndMgr::getSoundId(const char *soundName) {
	SoundInfo *sound;
	intptr resPtrOrId;
	uint8 *buffer, *newHeader;
	int soundIndex, firstAvailableSlot, foundSoundSlot;
	char *lipsyncFilename;
	int soundSize;
	int dataLen, wordsize, formatId, swapEndianFlag, nChans, sampleRate;
	char soundFilename[32];
	bool stoleSound = false;

	firstAvailableSlot = -1;

	if (!soundName)
		return 0;

	if ((int)soundName == 10000)
		return 10000;

	if ((int)soundName <= 1111)
		return 0;

	if ((int)soundName > 1367 && strlen(soundName) <= 4) {
		if (atoi(soundName) == 10000)
			return 10000;
		if (atoi(soundName) >= 1111 && atoi(soundName) < 1367)
			return 0;
	}

	sound = &_iMUSESounds[31];
	if ((int)soundName < 1367) {
		soundIndex = 31;
		while (sound->luaSoundId != (int)soundName) {
			--sound;
			if (!soundIndex--)
				return 0;
		}

		if (sound->soundUncompData) {
			return soundIndex + 100;
		}
	} else {
		soundIndex = 31;
		while (scumm_stricmp(sound->soundFilename, soundName)) {
			if (!sound->soundFilename[0])
				firstAvailableSlot = soundIndex;
			--sound;
			if (!soundIndex--) {
				if (firstAvailableSlot < 0) {
					firstAvailableSlot = nukeOldestSound();
					if (firstAvailableSlot < 0)
						return 0;
				}

				sound = &_iMUSESounds[firstAvailableSlot];
				strncpy(sound->soundFilename, soundName, sizeof(sound->soundFilename));
				sound->soundFilename[31] = 0;
				foundSoundSlot = _currentSoundSlot;

				// However strange this might be, this is what the interpreter does;
				// let's leave it for the time being in since it might have been
				// used to catch some edge-cases
				for (int i = 0; i < 32; ++i) {
					if (_iMUSESounds[0].soundFilename[0] && _iMUSESounds[0].luaSoundId == foundSoundSlot % 256 + 1111) {
						++foundSoundSlot;
						i = -1; // restart the loop
					}
				}

				_currentSoundSlot = foundSoundSlot;
				sound->luaSoundId = foundSoundSlot % 256 + 1111;
				_currentSoundSlot++;

				stoleSound = true;
			}
		}

		if (sound->soundUncompData && !stoleSound) {
			return soundIndex + 100;
		}
	}

	sound->flag = 0;
	resPtrOrId = openSound(sound->soundFilename);
	if (resPtrOrId) {
		sound->tsOfAllocation = g_system->getMillis();
		strncpy(soundFilename, sound->soundFilename, 32);
		lipsyncFilename = strrchr(soundFilename, '.');
		if (lipsyncFilename) {
			strcpy(lipsyncFilename, ".lip");
			registerLipsync(soundFilename, sound->luaSoundId);
		}
		soundSeek(resPtrOrId, 0, SEEK_END);
		sound->uncompResourceSize = soundTell(resPtrOrId);
		soundSeek(resPtrOrId, 0, SEEK_SET);
		sound->soundUncompData = (uint8 *)malloc(sound->uncompResourceSize + 80);

		if (sound->soundUncompData)
			copyDataFromResource(resPtrOrId, sound->soundUncompData, sound->uncompResourceSize);

		closeSound(resPtrOrId);
		buffer = getExtFormatBuffer(sound->soundUncompData, formatId, sampleRate, wordsize, nChans, swapEndianFlag, dataLen);

		if (formatId == 3) {
			sound->soundMap = sound->soundUncompData;
			sound->soundAddrData = sound->soundUncompData;

			return soundIndex + 100;
		} else if (formatId == 5) {
			createSoundHeader((int32 *)&sound->soundUncompData[sound->uncompResourceSize], sampleRate, 16, nChans, dataLen);
			newHeader = &sound->soundUncompData[sound->uncompResourceSize];
			sound->soundMap = newHeader;
			sound->soundAddrData = buffer - 80;
			newHeader[31] = swapEndianFlag;

			return soundIndex + 100;
		} else {
			free(sound->soundUncompData);
			resetLipsyncState(sound->luaSoundId);
			memset(sound, 0, sizeof(SoundInfo));

			return 0;
		}
	} else {
		memset(sound, 0, sizeof(SoundInfo));
	}

	return 0;
}

uint8 *ImuseSndMgr::getExtFormatBuffer(uint8 *srcBuf, int &formatId, int &sampleRate, int &wordSize, int &nChans, int &swapFlag, int32 &audioLength) {
	// This function is used to extract format metadata for sounds in WAV format (speech and sfx)

	int isMCMP;
	uint8 *bufPtr;

	if (!bufPtr)
		return nullptr;

	isMCMP = 0;
	formatId = 0;
	audioLength = 0;
	bufPtr = srcBuf;

	if (READ_BE_UINT32(srcBuf) == MKTAG('M', 'C', 'M', 'P')) {
		// Advance bufPtr to the underlying header tag ("RIFF" or "iMUS")
		uint16 numCompItems = READ_BE_UINT16(srcBuf + 4);
		uint16 offsetSkipTags = READ_BE_UINT16(srcBuf + 6 + 9 * numCompItems);

		bufPtr += 4; // Skip the "MCMP" tag
		bufPtr += 2; // Skip the numCompItems value we've read before
		bufPtr += 9 * numCompItems; // Skip the comp items
		bufPtr += 2; // Skip the offsetToHeader value we've read before
		bufPtr += offsetSkipTags;   // Skip the comp items' tags ("NULL", "VIMA", ...)

		isMCMP = 1;
	}

	// Follow the WAVE PCM header format
	if (READ_BE_UINT32(bufPtr) == MKTAG('R', 'I', 'F', 'F') &&
		READ_BE_UINT32(bufPtr + 8) == MKTAG('W', 'A', 'V', 'E') &&
		READ_BE_UINT32(bufPtr + 12) == MKTAG('f', 'm', 't', ' ')) {

		if (READ_LE_UINT32(bufPtr + 20) == 1) {
			formatId = (isMCMP != 0) + 5;
			nChans = READ_LE_UINT32(bufPtr + 22);
			sampleRate = READ_LE_UINT32(bufPtr + 24);
			wordSize = READ_LE_UINT32(bufPtr + 34);
			swapFlag = 1;

			if (READ_BE_UINT32(bufPtr + 36) == MKTAG('d', 'a', 't', 'a')) {
				audioLength = READ_BE_UINT32(bufPtr + 40);
				return bufPtr + 44;
			}
		}
		return nullptr;
	}

	// Follow the iMUSE MAP format block specification
	if (READ_BE_UINT32(bufPtr) == MKTAG('i', 'M', 'U', 'S')) {
		if (READ_BE_UINT32(bufPtr + 16) == MKTAG('F', 'R', 'M', 'T')) {
			formatId = (isMCMP != 0) + 3;
			wordSize = READ_LE_UINT32(bufPtr + 8);
			sampleRate = READ_LE_UINT32(bufPtr + 9);
			nChans = READ_LE_UINT32(bufPtr + 10);
			swapFlag = 0;
			audioLength = READ_LE_UINT32(bufPtr + 20);
			return bufPtr + 24;
		}
		return nullptr;
	}

	if (isMCMP)
		return nullptr;

	return bufPtr;
}

void ImuseSndMgr::createSoundHeader(int32 *fakeWavHeader, int sampleRate, int wordSize, int nChans, int32 audioLength) {
	// This function is used to create an iMUSE map for sounds in WAV format (speech and sfx)

	fakeWavHeader[0] = TO_LE_32(MKTAG('i', 'M', 'U', 'S'));
	fakeWavHeader[1] = TO_LE_32(audioLength + 80); // Map size
	fakeWavHeader[2] = TO_LE_32(MKTAG('M', 'A', 'P', ' '));
	fakeWavHeader[3] = 0x38000000; // Block offset
	fakeWavHeader[4] = TO_LE_32(MKTAG('F', 'R', 'M', 'T'));
	fakeWavHeader[5] = 0x14000000; // Block size minus 8, 20 bytes
	fakeWavHeader[6] = 0x50000000; // Block offset: 80 bytes
	fakeWavHeader[7] = 0;          // Endianness
	fakeWavHeader[8] = TO_LE_32(wordSize);
	fakeWavHeader[9] = TO_LE_32(sampleRate);
	fakeWavHeader[10] = TO_LE_32(nChans);
	fakeWavHeader[11] = TO_LE_32(MKTAG('R', 'E', 'G', 'N'));
	fakeWavHeader[12] = 0x08000000; // Block size minus 8: 8 bytes
	fakeWavHeader[13] = 0x50000000; // Block offset: 80 bytes
	fakeWavHeader[14] = TO_LE_32(audioLength);
	fakeWavHeader[15] = TO_LE_32(MKTAG('S', 'T', 'O', 'P'));
	fakeWavHeader[16] = 0x04000000; // Block size minus 8: 4 bytes
	fakeWavHeader[17] = TO_LE_32(audioLength + 80);
	fakeWavHeader[18] = TO_LE_32(MKTAG('D', 'A', 'T', 'A'));
	fakeWavHeader[19] = TO_LE_32(audioLength); // Data block size
}


int32 ImuseSndMgr::getDataFromRegion(SoundDesc *sound, int region, byte **buf, int32 offset, int32 size) {
	assert(checkForProperHandle(sound));
	assert(buf && offset >= 0 && size >= 0);
	assert(region >= 0 && region < sound->numRegions);

	int32 region_offset = sound->region[region].offset;
	int32 region_length = sound->region[region].length;

	if (offset + size > region_length) {
		size = region_length - offset;
		sound->endFlag = true;
	} else {
		sound->endFlag = false;
	}

	if (sound->mcmpData) {
		size = sound->mcmpMgr->decompressSample(region_offset + offset, size, buf);
	} else {
		*buf = static_cast<byte *>(malloc(size));
		sound->inStream->seek(region_offset + offset + sound->headerSize, SEEK_SET);
		sound->inStream->read(*buf, size);
	}

	return size;
}

void ImuseSndMgr::copyDataFromResource(intptr resPtrOrId, uint8 *destBuf, int32 size) {
	int32 chunkSize, subChunkSize, uncompChunkSize, blockOffset, curCompBlockSize;
	uint8 *compDataBuf, *destBufTmp;
	int firstBlockIndex, lastBlockIndex;

	compDataBuf = nullptr;
	curCompBlockSize = 0;
	destBufTmp = destBuf;

	if (!isPointerToResource(resPtrOrId)) { // If the resource has no MCMP header
		_resources[(int)resPtrOrId].fileStream->read(destBuf, size);
	} else {
		SndResource *resPtr = (SndResource *)resPtrOrId;
		chunkSize = size;

		firstBlockIndex = getCompBlockIdFromOffset(resPtr, resPtr->fileCurOffset);
		lastBlockIndex = getCompBlockIdFromOffset(resPtr, size + resPtr->fileCurOffset - 1);

		if (firstBlockIndex >= 0) {

			if (lastBlockIndex < 0) {
				chunkSize = resPtr->resSize - resPtr->fileCurOffset;
				lastBlockIndex = resPtr->compBlocksNum - 1;
			}

			if (chunkSize) {
				for (int i = firstBlockIndex; i <= lastBlockIndex; i++) {

					if (!resPtr->decompDataBuffer || i != resPtr->curProcessedCompBlock) {

						if (curCompBlockSize < resPtr->compBlockSizes[i]) {
							compDataBuf = (uint8 *)malloc(resPtr->compBlockSizes[i]);
							curCompBlockSize = resPtr->compBlockSizes[i];
						}

						resPtr->fileStream->seek(resPtr->vimaOffsets[i], SEEK_SET);
						resPtr->fileStream->read(compDataBuf, resPtr->compBlockSizes[i]);

						if (resPtr->curDecompBlockSize < resPtr->decompBlockSizes[i]) {
							resPtr->decompDataBuffer = (uint8 *)malloc(resPtr->decompBlockSizes[i]);
							resPtr->curDecompBlockSize = resPtr->decompBlockSizes[i];
						}

						if (READ_BE_UINT32(resPtr->compBlockPtr[i]) != MKTAG('V', 'I', 'M', 'A')) {
							decompressVima(compDataBuf, (int16 *)resPtr->decompDataBuffer, resPtr->decompBlockSizes[i], vimaTable);
							//VIMAGetHeaderAndDecode(
							//	&resPtr->vimaHeader,
							//	resPtr->decompDataBuffer,
							//	compDataBuf,
							//	resPtr->decompBlockSizes[i]);
						} else {
							// For uncompressed chunks (with a "NULL" tag)
							uncompChunkSize = resPtr->decompBlockSizes[i] < resPtr->compBlockSizes[i] ? resPtr->decompBlockSizes[i] : resPtr->compBlockSizes[i];
							memcpy(resPtr->decompDataBuffer, compDataBuf, uncompChunkSize);
						}

						resPtr->curProcessedCompBlock = i;
					}

					if (firstBlockIndex == lastBlockIndex) {
						subChunkSize = chunkSize;
						blockOffset = resPtr->fileCurOffset - resPtr->compBlockOffsets[i];
					} else if (i == firstBlockIndex) {
						blockOffset = resPtr->fileCurOffset - resPtr->compBlockOffsets[i];
						subChunkSize = resPtr->decompBlockSizes[i] - blockOffset;
					} else {
						blockOffset = 0;
						subChunkSize = i == lastBlockIndex ? resPtr->fileCurOffset - resPtr->compBlockOffsets[i] + chunkSize : resPtr->decompBlockSizes[i];
					}

					memcpy(destBufTmp, &resPtr->decompDataBuffer[blockOffset], subChunkSize);

					destBufTmp += subChunkSize;
				}

				if (compDataBuf) {
					free(compDataBuf);
				}

				resPtr->fileCurOffset += chunkSize;
			}
		}
	}
}

int ImuseSndMgr::getCompBlockIdFromOffset(SndResource *resPtr, int32 offset) {
	int32 *ptr;

	if (offset < resPtr->resSize) {
		if (ptr = (int32 *)bsearch(&offset, resPtr->compBlockOffsets, resPtr->compBlocksNum, sizeof(int32 *), compSortCallback)) {
			return (int)((ptr - resPtr->compBlockOffsets) >> 2);
		}
	}

	return -1;
}

static int compSortCallback(const void *a, const void *b) {
	int32 offset = (int32)a;
	int32 *offsetArray = (int32 *)b;

	if (offset >= offsetArray[0]) {
		if (offset < offsetArray[1])
			return 0;
	}

	if (offset < offsetArray[0]) {
		return -1;
	}

	return 1;
}

} // end of namespace Grim
