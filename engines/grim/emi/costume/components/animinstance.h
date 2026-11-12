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

#ifndef GRIM_EMI_ANIMINSTANCE_H
#define GRIM_EMI_ANIMINSTANCE_H

#include "engines/grim/emi/animation/ainst.h"
#include "engines/grim/emi/costume/componentinstance.h"

namespace Grim {

class Actor;
class EmiEngine;

class AnimInstance : public ComponentInstance, public AInst {
public:
	AnimInstance(EmiEngine *engine, const LString &componentName, Actor *actor);
	~AnimInstance() override;

	void update(float deltaTime) override;
	void setKey(float value, float elapsed, float nextValue, float remaining, bool partial) override;
	void fadeIn(float seconds) override;
	void fadeOut(float seconds) override;
	void reset() override;

	void bindAnimation();
	void unbindAnimation();

	Actor *_owner;
	bool _bound;

private:
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
