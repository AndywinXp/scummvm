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

#ifndef GRIM_EMI_CSPRITE_H
#define GRIM_EMI_CSPRITE_H

#include "engines/grim/emi/cmaterial.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/cvector.h"
#include "engines/grim/emi/lresourcehandle.h"
#include "engines/grim/emi/rprimitive.h"
#include "engines/grim/emi/rprimitiveinstance.h"

namespace Grim {

class Camera;
class EmiEngine;
class RNode;

class CSprite : public RPrimitive {
public:
	enum {
		kNumCorners = 4
	};

	explicit CSprite(EmiEngine *engine);
	virtual ~CSprite();

	virtual RPrimitiveInstance *createInstance();
	virtual bool restore(LFile *file);

	bool loadBinary(const char *path);
	bool saveBinary(const char *path);

	EmiEngine *getEngine() const;

	LResourceHandle<CMaterial> material;
	float width;
	float height;
	CVector offset;
	Color corners[kNumCorners];
	float uv[kNumCorners * 2];
	int drawFlags;

protected:
	EmiEngine *_engine;
};

class CSpriteInstance : public RPrimitiveInstance {
public:
	CSpriteInstance(CSprite *spritePrimitive);
	virtual ~CSpriteInstance();

	virtual void output(RNode *node, Camera *camera);
	virtual int getSaveSize();
	virtual bool saveState(tSaveRestoreAccess *access);
	virtual bool restoreState(tSaveRestoreAccess *access);

	int textureIndex;

private:
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
