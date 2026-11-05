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

#ifndef GRIM_SPUTPRIMITIVE_H
#define GRIM_SPUTPRIMITIVE_H

#include "common/scummsys.h"

#include "engines/grim/primitivequeue.h"
#include "engines/grim/std/std_sys.h"

namespace Common {
class ReadStream;
class WriteStream;
}

namespace Grim {

class RenderDroid;

class SputPrimitive {
public:
	explicit SputPrimitive(RenderDroid *pRd);
	~SputPrimitive();

	int initialize(SysStruct *pHS);
	void pause();
	void resume();
	int terminate();
	int startup();
	int shutdown();
	int save(Common::WriteStream *pStream);
	int restore(Common::ReadStream *pStream);

	PrimitiveQueue *getCurrent() { return &_current; }

private:
	SysStruct *_pHS;
	int _bInitialized;
	int _bStarted;
	PrimitiveQueue _current;
};

} // End of namespace Grim

#endif
