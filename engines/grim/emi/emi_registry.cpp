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

#include "audio/mixer.h"

#include "common/config-manager.h"
#include "common/memstream.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/emi/emi_registry.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

const EmiRegistry::Entry EmiRegistry::_entries[kNumEntries] = {
	{ "musvolume",        "music_volume",         kKindVolume,      96.0f },
	{ "sfxvolume",        "sfx_volume",           kKindVolume,     100.0f },
	{ "vocvolume",        "speech_volume",        kKindVolume,     100.0f },
	{ "speech_mode",      nullptr,                kKindSpeechMode,   2.0f },
	{ "textspeed",        "talkspeed",            kKindTextSpeed,    7.0f },
	{ "joystick_enabled", "joystick_enabled",     kKindBool,         1.0f },
	{ "camera_mode",      "camera_relative_mode", kKindBool,         0.0f },
	{ "gamma",            "gamma",                kKindGamma,        1.0f },
	{ "subtitles",        "movie_subtitles",      kKindBool,         0.0f },
	{ "musicquality",     "music_quality",        kKindBool,         0.0f },
	{ "shadowfx",         "shadow_effects",       kKindShadow,       2.0f },
	{ "moviequality",     "movie_quality",        kKindBool,         0.0f },
	{ "analog_mode",      "analog_joystick_mode", kKindBool,         0.0f },
	{ "miscfx",           "misc_video_effects",   kKindInt,          2.0f },
	{ "vocfx",            "audio_effects",        kKindBool,         1.0f }
};

Common::String EmiRegistry::_syncedSnapshot;

bool EmiRegistry::isPrefsFile(const Common::String &fileName) {
	return fileName.equalsIgnoreCase("efmi.cfg");
}

Common::SeekableReadStream *EmiRegistry::openPrefsForReading(Common::SeekableReadStream *realFile) {
	Common::String text;
	bool hadFile = realFile != nullptr;

	if (realFile) {
		int32 size = realFile->size();

		if (size > 0) {
			char *buffer = new char[size + 1];
			uint32 count = realFile->read(buffer, size);

			buffer[count] = '\0';
			text = buffer;
			delete[] buffer;
		}

		delete realFile;
	}

	bool present[kNumEntries];

	for (int i = 0; i < kNumEntries; i++)
		present[i] = false;

	Common::String result;
	Common::String line;
	uint start = 0;

	while (nextLine(text, start, line)) {
		uint colon = line.find(':');

		if (colon != Common::String::npos) {
			int index = findEntry(line.substr(0, colon));

			if (index >= 0) {
				present[index] = true;

				float value = 0.0f;

				if (readEntry(_entries[index], value)) {
					result += Common::String(_entries[index].key) + ":" + formatEntry(_entries[index], value) + "\n";

					continue;
				}
			}
		}

		result += line + "\n";
	}

	for (int i = 0; i < kNumEntries; i++) {
		float value = 0.0f;

		if (!present[i] && readEntry(_entries[i], value))
			result += Common::String(_entries[i].key) + ":" + formatEntry(_entries[i], value) + "\n";
	}

	_syncedSnapshot = buildSnapshot();

	if (!hadFile && result.empty())
		return nullptr;

	Common::MemoryWriteStreamDynamic data(DisposeAfterUse::NO);

	data.write(result.c_str(), result.size());

	return new Common::MemoryReadStream(data.getData(), data.size(), DisposeAfterUse::YES);
}

Common::WriteStream *EmiRegistry::openPrefsForWriting(Common::WriteStream *realFile) {
	if (!realFile)
		return nullptr;

	return new EmiPrefsFile(realFile);
}

void EmiRegistry::registerDefaults() {
	for (int i = 0; i < kNumEntries; i++)
		writeEntry(_entries[i], _entries[i].defaultValue, true);
}

void EmiRegistry::setFromText(const Common::String &text) {
	bool changed = false;
	Common::String line;
	uint start = 0;

	while (nextLine(text, start, line)) {
		uint colon = line.find(':');

		if (colon == Common::String::npos)
			continue;

		int index = findEntry(line.substr(0, colon));

		if (index < 0)
			continue;

		Common::String valueText = line.substr(colon + 1);
		char *end = nullptr;
		double number = strtod(valueText.c_str(), &end);

		if (end == valueText.c_str() || *end != '\0')
			continue;

		if (writeEntry(_entries[index], (float)number, false))
			changed = true;
	}

	if (changed)
		ConfMan.flushToDisk();

	_syncedSnapshot = buildSnapshot();
}

bool EmiRegistry::changedSinceSync() {
	return buildSnapshot() != _syncedSnapshot;
}

Common::String EmiRegistry::buildSnapshot() {
	Common::String snapshot;

	for (int i = 0; i < kNumEntries; i++) {
		float value = 0.0f;

		if (readEntry(_entries[i], value))
			snapshot += Common::String(_entries[i].key) + ":" + formatEntry(_entries[i], value) + "\n";
	}

	return snapshot;
}

int EmiRegistry::findEntry(const Common::String &key) {
	for (int i = 0; i < kNumEntries; i++) {
		if (key == _entries[i].key)
			return i;
	}

	return -1;
}

bool EmiRegistry::nextLine(const Common::String &text, uint &start, Common::String &line) {
	if (start >= text.size())
		return false;

	uint end = start;

	while (end < text.size() && text[end] != '\n')
		end++;

	line = Common::String(text.c_str() + start, end - start);
	start = end + 1;

	while (!line.empty() && line.lastChar() == '\r')
		line.deleteLastChar();

	return true;
}

bool EmiRegistry::readEntry(const Entry &entry, float &value) {
	if (entry.kind == kKindSpeechMode) {
		if (!ConfMan.hasKey("subtitles") && !ConfMan.hasKey("speech_mute"))
			return false;

		bool subtitles = false;
		bool speechMute = false;

		readConfBool("subtitles", subtitles);
		readConfBool("speech_mute", speechMute);

		value = (float)convertSpeechModeFromConf(subtitles, speechMute);

		return true;
	}

	if (!ConfMan.hasKey(entry.confKey))
		return false;

	if (entry.kind == kKindGamma) {
		float gamma = 0.0f;

		if (!readConfFloat(entry.confKey, gamma))
			return false;

		value = CLIP<float>(gamma, 0.5f, 1.5f);

		return true;
	}

	if (entry.kind == kKindBool || entry.kind == kKindShadow) {
		bool flag = false;

		if (!readConfBool(entry.confKey, flag))
			return false;

		if (entry.kind == kKindShadow)
			value = flag ? 2.0f : 1.0f;
		else
			value = flag ? 1.0f : 0.0f;

		return true;
	}

	int number = 0;

	if (!readConfInt(entry.confKey, number))
		return false;

	if (entry.kind == kKindVolume)
		value = (float)convertVolumeFromConf(number);
	else if (entry.kind == kKindTextSpeed)
		value = (float)convertTalkSpeedFromConf(number);
	else
		value = (float)number;

	return true;
}

bool EmiRegistry::writeEntry(const Entry &entry, float value, bool asDefault) {
	if (!asDefault) {
		float current = 0.0f;

		if (readEntry(entry, current) && ABS(current - value) < 0.005f)
			return false;
	}

	int rounded = (int)(value + 0.5f);

	if (entry.kind == kKindSpeechMode) {
		if (rounded < 1 || rounded > 3) {
			warning("EmiRegistry::writeEntry(): invalid speech mode %d", rounded);

			return false;
		}

		storeConfBool("subtitles", rounded == 1 || rounded == 3, asDefault);
		storeConfBool("speech_mute", rounded == 1, asDefault);
	} else if (entry.kind == kKindVolume) {
		storeConf(entry.confKey, Common::String::format("%d", convertVolumeToConf(rounded)), asDefault);
	} else if (entry.kind == kKindTextSpeed) {
		storeConf(entry.confKey, Common::String::format("%d", convertTalkSpeedToConf(rounded)), asDefault);
	} else if (entry.kind == kKindGamma) {
		storeConf(entry.confKey, Common::String::format("%.2f", value), asDefault);
	} else if (entry.kind == kKindShadow) {
		storeConfBool(entry.confKey, rounded == 2, asDefault);
	} else if (entry.kind == kKindBool) {
		storeConfBool(entry.confKey, rounded == 1, asDefault);
	} else {
		storeConf(entry.confKey, Common::String::format("%d", rounded), asDefault);
	}

	return true;
}

Common::String EmiRegistry::formatEntry(const Entry &entry, float value) {
	if (entry.kind == kKindGamma)
		return Common::String::format("%g", (double)value);

	return Common::String::format("%d", (int)(value + 0.5f));
}

bool EmiRegistry::readConfBool(const char *confKey, bool &value) {
	Common::String text = ConfMan.get(confKey);

	if (Common::parseBool(text, value))
		return true;

	warning("EmiRegistry::readConfBool(): invalid value for %s", confKey);

	return false;
}

bool EmiRegistry::readConfInt(const char *confKey, int &value) {
	Common::String text = ConfMan.get(confKey);
	char *end = nullptr;
	long number = strtol(text.c_str(), &end, 0);

	if (end == text.c_str()) {
		warning("EmiRegistry::readConfInt(): invalid value for %s", confKey);

		return false;
	}

	value = (int)number;

	return true;
}

bool EmiRegistry::readConfFloat(const char *confKey, float &value) {
	Common::String text = ConfMan.get(confKey);
	char *end = nullptr;
	double number = strtod(text.c_str(), &end);

	if (end == text.c_str()) {
		warning("EmiRegistry::readConfFloat(): invalid value for %s", confKey);

		return false;
	}

	value = (float)number;

	return true;
}

void EmiRegistry::storeConf(const char *confKey, const Common::String &value, bool asDefault) {
	if (asDefault)
		ConfMan.registerDefault(confKey, value);
	else
		ConfMan.set(confKey, value);
}

void EmiRegistry::storeConfBool(const char *confKey, bool value, bool asDefault) {
	storeConf(confKey, value ? "true" : "false", asDefault);
}

int EmiRegistry::convertVolumeFromConf(int volume) {
	int range = kMaxVolume - kMinVolume;
	int result = (volume * range + Audio::Mixer::kMaxMixerVolume / 2) / Audio::Mixer::kMaxMixerVolume + kMinVolume;

	return CLIP<int>(result, kMinVolume, kMaxVolume);
}

int EmiRegistry::convertVolumeToConf(int volume) {
	int range = kMaxVolume - kMinVolume;
	int result = ((volume - kMinVolume) * Audio::Mixer::kMaxMixerVolume + range / 2) / range;

	return CLIP<int>(result, 0, Audio::Mixer::kMaxMixerVolume);
}

int EmiRegistry::convertTalkSpeedFromConf(int talkSpeed) {
	int result = (talkSpeed * kMaxTextSpeed + 127) / 255;

	return CLIP<int>(result, kMinTextSpeed, kMaxTextSpeed);
}

int EmiRegistry::convertTalkSpeedToConf(int talkSpeed) {
	int result = (talkSpeed * 255 + kMaxTextSpeed / 2) / kMaxTextSpeed;

	return CLIP<int>(result, 0, 255);
}

int EmiRegistry::convertSpeechModeFromConf(bool subtitles, bool speechMute) {
	if (!subtitles && !speechMute)
		return 2;

	if (subtitles && !speechMute)
		return 3;

	if (!subtitles)
		warning("EmiRegistry::convertSpeechModeFromConf(): both subtitles and speech are off, using subtitles only");

	return 1;
}

void *EmiPrefsFile::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void EmiPrefsFile::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

EmiPrefsFile::EmiPrefsFile(Common::WriteStream *realFile) : _realFile(realFile) {
}

EmiPrefsFile::~EmiPrefsFile() {
	_realFile->finalize();
	delete _realFile;
	_realFile = nullptr;

	EmiRegistry::setFromText(_text);
}

uint32 EmiPrefsFile::write(const void *dataPtr, uint32 dataSize) {
	uint32 written = _realFile->write(dataPtr, dataSize);

	_text += Common::String((const char *)dataPtr, written);

	return written;
}

bool EmiPrefsFile::flush() {
	return _realFile->flush();
}

void EmiPrefsFile::finalize() {
	_realFile->finalize();
}

int64 EmiPrefsFile::pos() const {
	return _realFile->pos();
}

bool EmiPrefsFile::err() const {
	return _realFile->err();
}

void EmiPrefsFile::clearErr() {
	_realFile->clearErr();
}

} // End of namespace Grim
