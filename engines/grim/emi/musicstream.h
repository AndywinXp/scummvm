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

#ifndef GRIM_EMI_MUSICSTREAM_H
#define GRIM_EMI_MUSICSTREAM_H

#include "common/scummsys.h"

#include "engines/grim/emi/lfile.h"

namespace Grim {

class EMIMilesDriver;
struct EMIMilesStream;
class StreamScript;

class MusicStream {
public:
	enum {
		kStatusDone = 2,
		kStatusPlaying = 4
	};

	static MusicStream *open(StreamScript *script, const LString &fileName);
	~MusicStream();

	void start();
	void pause(bool paused);
	int getStatus();
	void setVolume(int volume);
	void setPan(int pan);
	void setLoopCount(int loopCount);
	void setLoopBlock(int startByte, int endByte);
	void setPosition(int bytePosition);
	void setPositionSynced(int bytePosition, uint stamp);
	int getPosition();
	int getPositionStamped(uint *stamp);
	void setMsPosition(int milliseconds);
	void getMsPosition(int *total, int *current);

private:
	MusicStream(StreamScript *script, EMIMilesDriver *driver, EMIMilesStream *stream);

	StreamScript *_script;
	EMIMilesDriver *_driver;
	EMIMilesStream *_stream;
};

} // End of namespace Grim

#endif
