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

#ifndef TOPGUN_LIBRARIES_AUDIO_MIDI_H
#define TOPGUN_LIBRARIES_AUDIO_MIDI_H

#include "audio/midiplayer.h"
#include "common/scummsys.h"
#include "common/str.h"

namespace Topgun {

class MidiSequencer : public Audio::MidiPlayer {
public:
	MidiSequencer();

	bool playFile(const Common::String &fileName);
	bool playData(const byte *data, uint32 size);

protected:
	void sendToChannel(byte channel, uint32 b) override;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_AUDIO_MIDI_H
