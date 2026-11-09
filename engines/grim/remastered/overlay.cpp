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

#include "image/png.h"

#include "graphics/surface.h"

#include "engines/grim/remastered/overlay.h"
// LEGACY: #include "engines/grim/resource.h"
// LEGACY: #include "engines/grim/material.h"
// LEGACY: #include "engines/grim/gfx_base.h"

namespace Grim {

Overlay::Overlay(const Common::String &filename, Common::SeekableReadStream *data) :
		_x(0), _y(0) {
	// LEGACY: _material = g_resourceloader->loadMaterial(filename, nullptr, true);
}

Overlay::~Overlay() {
	// LEGACY: delete _material;
}

void Overlay::draw() {
	// LEGACY: _material->select();
	// LEGACY: g_driver->drawOverlay(this);
}

int Overlay::getWidth() const {
	// LEGACY: return _material->getData()->_textures[0]->_width;
	return 0;
}

int Overlay::getHeight() const {
	// LEGACY: return _material->getData()->_textures[0]->_height;
	return 0;
}

}
