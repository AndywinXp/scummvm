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

#ifndef GRIM_EMI_AINST_H
#define GRIM_EMI_AINST_H

#include "common/list.h"

#include "engines/grim/emi/atrack.h"

namespace Grim {

class EmiEngine;

class AInst {
public:
	enum {
		kPlaying = 0x01,
		kPaused = 0x02,
		kFadingIn = 0x04,
		kFadingOut = 0x08,
		kAttached = 0x10,
		kLooping = 0x20
	};

	explicit AInst(EmiEngine *engine);
	virtual ~AInst();

	virtual void onFinished(int code);

	void play();
	void setLoop(bool loop);
	void pause();
	void stop();
	void fadeIn(float seconds);
	void fadeOut(float seconds);
	void update(float deltaTime);

	Common::List<AChannelHandle *> channels;
	uint flags;
	float length;
	float time;
	float weight;
	float prevTime;
	float prevWeight;
	float speed;
	float fadeRate;

private:
	void finish();

	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
