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

#include "engines/grim/emi/scene/clightnode.h"
#include "engines/grim/emi/emiengine.h"

namespace Grim {

CLightNode::CLightNode(EmiEngine *engine) : _engine(engine) {
	active = false;
	savedActive = false;
	light = nullptr;
}

CLightNode::~CLightNode() {
	setActive(false);
}

void CLightNode::setLight(CLight *newLight) {
	light = newLight;
}

void CLightNode::setActive(bool newActive) {
	if (newActive) {
		if (!active) {
			_engine->_activeLights.push_back(this);
			active = true;
		}
	} else if (active) {
		Common::List<CLightNode *>::iterator it = _engine->_activeLights.begin();

		while (it != _engine->_activeLights.end()) {
			if (*it == this)
				it = _engine->_activeLights.erase(it);
			else
				++it;
		}

		active = false;
	}
}

} // End of namespace Grim
