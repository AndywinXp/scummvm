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

#ifndef GRIM_EMI_EMI_REGISTRY_H
#define GRIM_EMI_EMI_REGISTRY_H

#include "common/str.h"
#include "common/stream.h"

namespace Grim {

class EmiEngine;

class EmiRegistry {
public:
	static bool isPrefsFile(const Common::String &fileName);
	static Common::SeekableReadStream *openPrefsForReading(EmiEngine *engine, Common::SeekableReadStream *realFile);
	static Common::WriteStream *openPrefsForWriting(EmiEngine *engine, Common::WriteStream *realFile);
	static void registerDefaults();
	static void setFromText(EmiEngine *engine, const Common::String &text);
	static bool changedSinceSync(EmiEngine *engine);

private:
	enum {
		kNumEntries = 15,
		kMinVolume = 25,
		kMaxVolume = 100,
		kMinTextSpeed = 1,
		kMaxTextSpeed = 10
	};

	enum {
		kKindVolume,
		kKindSpeechMode,
		kKindTextSpeed,
		kKindGamma,
		kKindBool,
		kKindShadow,
		kKindInt
	};

	struct Entry {
		const char *key;
		const char *confKey;
		int kind;
		float defaultValue;
	};

	static const Entry _entries[kNumEntries];

	static Common::String buildSnapshot();
	static int findEntry(const Common::String &key);
	static bool nextLine(const Common::String &text, uint &start, Common::String &line);
	static bool readEntry(const Entry &entry, float &value);
	static bool writeEntry(const Entry &entry, float value, bool asDefault);
	static Common::String formatEntry(const Entry &entry, float value);
	static bool readConfBool(const char *confKey, bool &value);
	static bool readConfInt(const char *confKey, int &value);
	static bool readConfFloat(const char *confKey, float &value);
	static void storeConf(const char *confKey, const Common::String &value, bool asDefault);
	static void storeConfBool(const char *confKey, bool value, bool asDefault);
	static int convertVolumeFromConf(int volume);
	static int convertVolumeToConf(int volume);
	static int convertTalkSpeedFromConf(int talkSpeed);
	static int convertTalkSpeedToConf(int talkSpeed);
	static int convertSpeechModeFromConf(bool subtitles, bool speechMute);
};

class EmiPrefsFile : public Common::WriteStream {
public:
	EmiPrefsFile(EmiEngine *engine, Common::WriteStream *realFile);
	~EmiPrefsFile() override;

	void *operator new(size_t size);
	void operator delete(void *block);

	uint32 write(const void *dataPtr, uint32 dataSize) override;
	bool flush() override;
	void finalize() override;
	int64 pos() const override;
	bool err() const override;
	void clearErr() override;

private:
	EmiEngine *_engine;
	Common::WriteStream *_realFile;
	Common::String _text;
};

} // End of namespace Grim

#endif
