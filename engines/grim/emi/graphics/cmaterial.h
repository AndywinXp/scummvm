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

#ifndef GRIM_EMI_CMATERIAL_H
#define GRIM_EMI_CMATERIAL_H

#include "common/array.h"

#include "engines/grim/emi/graphics/ctexture.h"
#include "engines/grim/emi/resources/lresource.h"

namespace Grim {

class CMaterial : public LResource {
public:
	CMaterial();
	~CMaterial() override;

	uint resSizeOfItem() override;

	virtual int getNumTextures();
	virtual CTexture *getTexture(int index);

	void resetFlags();
	void setFlags(int newFlags);
	void addFlags(int addedFlags);

	Common::Array<CTexture *> textures;
	int flags;

protected:
	bool resLoadItem(const char *itemName) override;
};

} // End of namespace Grim

#endif
