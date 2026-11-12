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

#ifndef GRIM_EMI_OVERWORLD_H
#define GRIM_EMI_OVERWORLD_H

#include "common/list.h"

namespace Grim {

class Actor;
class CDrawContext;
class CLightNode;
class EmiEngine;

class Overworld {
public:
	explicit Overworld(EmiEngine *engine);
	~Overworld();

	int draw(CDrawContext *drawContext);
	void putActor(Actor *actor);
	int removeActor(Actor *actor);
	int clearActors();
	void addLight(CLightNode *light);
	void removeLight(CLightNode *light);
	void setLightsActive(bool active);

	Common::List<CLightNode *> lights;
	bool enabled;
	Common::List<Actor *> actors;

private:
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
