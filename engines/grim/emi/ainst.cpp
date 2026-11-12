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

#include "common/textconsole.h"

#include "engines/grim/emi/ainst.h"
#include "engines/grim/emi/emiengine.h"

namespace Grim {

AInst::AInst(EmiEngine *engine) : _engine(engine) {
	length = 0.0f;
	fadeRate = 0.0f;
	time = 0.0f;
	prevTime = 0.0f;
	prevWeight = 0.0f;
	speed = 1.0f;
	weight = 1.0f;
	flags = 0;
	_engine->_animInstances.push_back(this);
}

AInst::~AInst() {
	Common::List<AInst *>::iterator it = _engine->_animInstances.begin();

	while (it != _engine->_animInstances.end()) {
		if (*it == this)
			it = _engine->_animInstances.erase(it);
		else
			++it;
	}

	while (!channels.empty()) {
		uint previousCount = channels.size();

		channels.front()->detach(this);

		if (channels.size() == previousCount) {
			warning("AInst::~AInst(): a channel did not release the instance");
			break;
		}
	}

	channels.clear();
}

void AInst::onFinished(int code) {
}

void AInst::play() {
	if ((flags & kPlaying) == 0) {
		bool notAttached = (flags & kAttached) == 0;

		if ((flags & kPaused) != 0)
			flags &= ~kPaused;
		else
			time = 0.0f;

		if ((flags & (kFadingIn | kFadingOut)) == 0)
			weight = 1.0f;

		flags |= kPlaying;

		if (notAttached) {
			flags |= kAttached;

			Common::List<AChannelHandle *>::iterator it;

			for (it = channels.begin(); it != channels.end(); ++it)
				(*it)->activate(this);
		}
	}
}

void AInst::setLoop(bool loop) {
	if (loop)
		flags |= kLooping;
	else
		flags &= ~kLooping;
}

void AInst::pause() {
	if ((flags & kPaused) == 0) {
		bool notAttached = (flags & kAttached) == 0;

		flags = (flags & ~(kPlaying | kPaused)) | kPaused;

		if (notAttached) {
			flags |= kAttached;

			Common::List<AChannelHandle *>::iterator it;

			for (it = channels.begin(); it != channels.end(); ++it)
				(*it)->activate(this);
		}
	}
}

void AInst::stop() {
	uint oldFlags = flags;

	flags = 0;

	if ((oldFlags & kAttached) != 0) {
		Common::List<AChannelHandle *>::iterator it;

		for (it = channels.begin(); it != channels.end(); ++it)
			(*it)->deactivate(this);
	}
}

void AInst::fadeIn(float seconds) {
	if ((flags & kFadingIn) == 0) {
		weight = 0.0f;
		flags = (flags & ~(kFadingIn | kFadingOut)) | kFadingIn;

		// The original could divide by zero here, let's avoid that...
		fadeRate = seconds != 0.0f ? (float)(1.0 / seconds) : 0.0f;
	}
}

void AInst::fadeOut(float seconds) {
	if ((flags & kFadingOut) == 0) {
		// The original could divide by zero here, let's avoid that...
		double rate = seconds != 0.0f ? (double)weight / seconds : 0.0;

		flags = (flags & ~(kFadingIn | kFadingOut)) | kFadingOut;
		fadeRate = (float)rate;
	}
}

void AInst::finish() {
	onFinished(1);

	uint oldFlags = flags;

	flags = 0;

	if ((oldFlags & kAttached) != 0) {
		Common::List<AChannelHandle *>::iterator it;

		for (it = channels.begin(); it != channels.end(); ++it)
			(*it)->deactivate(this);
	}
}

void AInst::update(float deltaTime) {
	prevWeight = weight;
	prevTime = time;

	uint oldFlags = flags;

	if ((oldFlags & kFadingIn) != 0) {
		double newWeight = (double)deltaTime * fadeRate + weight;

		weight = (float)newWeight;

		if (newWeight > 1.0) {
			weight = 1.0f;
			flags = oldFlags & ~kFadingIn;
		}
	} else if ((oldFlags & kFadingOut) != 0) {
		double newWeight = (double)weight - (double)deltaTime * fadeRate;

		weight = (float)newWeight;

		if (newWeight < 0.0) {
			weight = 0.0f;
			flags = oldFlags & ~kFadingOut;
		}
	}

	uint currentFlags = flags;

	if ((currentFlags & kPlaying) != 0) {
		double newTime = (double)deltaTime * speed + time;

		time = (float)newTime;

		if (newTime < 0.0) {
			if ((currentFlags & kLooping) != 0)
				time = (float)(newTime + length);
			else
				finish();
		} else if (newTime > length) {
			if ((currentFlags & kLooping) != 0) {
				double current = time;
				double reduced;

				if (length <= 0.0f) {
					warning("AInst::update(): looping instance with no length");

					return;
				}

				do {
					reduced = current - length;
					current = reduced;
				} while (reduced > length);

				time = (float)reduced;
			} else {
				finish();
			}
		}
	}
}

} // End of namespace Grim
