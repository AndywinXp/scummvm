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

#ifndef GRIM_EMI_DEMOMUSIC_H
#define GRIM_EMI_DEMOMUSIC_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"

namespace Common {
class FSDirectory;
class SeekableReadStream;
}

namespace Grim {

class EMIMilesDriver;
class Sounds;
struct EMIMilesStream;

// Music module of the Windows demo: a cue map with up to four streams per cue
class DemoMusicScript {
public:
	explicit DemoMusicScript(Sounds *sounds);
	~DemoMusicScript();

	bool loadMusicMap();
	void closeMusicMap();
	bool isLoaded() const;
	void setMasterVolume(float volume);
	void startCue(int id);
	void updateStreams();
	void pauseStreams();
	void resumeStreams();
	void closeAllStreams();

private:
	static const int kMaxSlots = 4;

	struct Slot {
		Common::String fileName;
		EMIMilesStream *stream;
		int state;
		float fadeVolume;
		bool paused;
	};

	struct Cue {
		int id;
		Common::String name;
		int x;
		int y;
		int sync;
		float fadeRate;
		int slotCount;
		Slot slots[kMaxSlots];
	};

	EMIMilesDriver *getMilesDriver();
	void parseMusicMap(Common::SeekableReadStream *script);
	void addCue(int id, const char *name, int x, int y, int sync);
	void addSlot(const char *fileName);
	void openSlot(Slot &slot, int carry);
	void closeSlot(Slot &slot);
	void applySlotVolume(Slot &slot);
	void pushSearchDir(const char *directory);
	void clearSearchDirs();
	Common::SeekableReadStream *openStream(const char *path);
	Common::SeekableReadStream *openAudioFile(const char *fileName);

	Sounds *_sounds;
	Common::Array<Cue> _cues;
	Common::Array<Common::String> _searchDirs;
	Common::FSDirectory *_gameDirectory;
	bool _loaded;
	float _masterVolume;
	uint _lastUpdateMs;
	bool _clockStarted;
};

} // End of namespace Grim

#endif
