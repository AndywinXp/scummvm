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

#if !defined(GRIM_IMUSE_FADES_H)
#define GRIM_IMUSE_FADES_H

#include "common/scummsys.h"
#include "common/textconsole.h"
#include "common/util.h"

namespace Grim {

class IMuseDigiFadesHandler {

private:
	Imuse *_engine;
	IMuseDigiFade _fades[DIMUSE_MAX_FADES];
	int _fadesOn;

public:
	IMuseDigiFadesHandler(Imuse *engine);
	~IMuseDigiFadesHandler();

	int init();
	void deinit();
	void saveLoad(Common::Serializer &ser);
	int fadeParam(int soundId, int transitionType, int destinationValue, int fadeLength);
	void clearFadeStatus(int soundId, int transitionType);
	void loop();

};

} // End of namespace Grim
#endif
