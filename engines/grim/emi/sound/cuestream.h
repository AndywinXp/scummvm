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

#ifndef GRIM_EMI_CUESTREAM_H
#define GRIM_EMI_CUESTREAM_H

#include "common/scummsys.h"

#include "engines/grim/emi/files/lfile.h"

namespace Grim {

class MusicStream;

class CueStream {
public:
	struct Jump {
		double srcMs;
		double dstMs;
		int16 field16;
	};

	CueStream();
	~CueStream();

	void reset();

	double _ratio;
	double _invRatio;
	double _pcmBytesPerMs;
	double _invPcmBytesPerMs;
	int _startByte;
	int _srcByte;
	int _dstByte;
	int16 _loopCount;
	LString _fileName;
	MusicStream *_musicStream;
	int _paused;
	float _trim;
	int16 _field43c;
	double _start;
	double _end;
	int16 _hasJump;
	Jump _jumps[2];
};

} // End of namespace Grim

#endif
