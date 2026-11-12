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

#include "engines/grim/emi/animation/ainst.h"
#include "engines/grim/emi/emiengine.h"

namespace Grim {

AInst::AInst(EmiEngine *engine) : _engine(engine) {
	_length = 0.0f;
	_fadeRate = 0.0f;
	_time = 0.0f;
	_prevTime = 0.0f;
	_prevWeight = 0.0f;
	_speed = 1.0f;
	_weight = 1.0f;
	_flags = 0;
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

	while (!_channels.empty()) {
		uint previousCount = _channels.size();

		_channels.front()->detach(this);

		if (_channels.size() == previousCount) {
			warning("AInst::~AInst(): a channel did not release the instance");
			break;
		}
	}

	_channels.clear();
}

void AInst::onFinished(int code) {
}

void AInst::play() {
	if ((_flags & kPlaying) == 0) {
		bool notAttached = (_flags & kAttached) == 0;

		if ((_flags & kPaused) != 0)
			_flags &= ~kPaused;
		else
			_time = 0.0f;

		if ((_flags & (kFadingIn | kFadingOut)) == 0)
			_weight = 1.0f;

		_flags |= kPlaying;

		if (notAttached) {
			_flags |= kAttached;

			Common::List<AChannelHandle *>::iterator it;

			for (it = _channels.begin(); it != _channels.end(); ++it)
				(*it)->activate(this);
		}
	}
}

void AInst::setLoop(bool loop) {
	if (loop)
		_flags |= kLooping;
	else
		_flags &= ~kLooping;
}

void AInst::pause() {
	if ((_flags & kPaused) == 0) {
		bool notAttached = (_flags & kAttached) == 0;

		_flags = (_flags & ~(kPlaying | kPaused)) | kPaused;

		if (notAttached) {
			_flags |= kAttached;

			Common::List<AChannelHandle *>::iterator it;

			for (it = _channels.begin(); it != _channels.end(); ++it)
				(*it)->activate(this);
		}
	}
}

void AInst::stop() {
	uint oldFlags = _flags;

	_flags = 0;

	if ((oldFlags & kAttached) != 0) {
		Common::List<AChannelHandle *>::iterator it;

		for (it = _channels.begin(); it != _channels.end(); ++it)
			(*it)->deactivate(this);
	}
}

void AInst::fadeIn(float seconds) {
	if ((_flags & kFadingIn) == 0) {
		_weight = 0.0f;
		_flags = (_flags & ~(kFadingIn | kFadingOut)) | kFadingIn;

		// The original could divide by zero here, let's avoid that...
		_fadeRate = seconds != 0.0f ? (float)(1.0 / seconds) : 0.0f;
	}
}

void AInst::fadeOut(float seconds) {
	if ((_flags & kFadingOut) == 0) {
		// The original could divide by zero here, let's avoid that...
		double rate = seconds != 0.0f ? (double)_weight / seconds : 0.0;

		_flags = (_flags & ~(kFadingIn | kFadingOut)) | kFadingOut;
		_fadeRate = (float)rate;
	}
}

void AInst::finish() {
	onFinished(1);

	uint oldFlags = _flags;

	_flags = 0;

	if ((oldFlags & kAttached) != 0) {
		Common::List<AChannelHandle *>::iterator it;

		for (it = _channels.begin(); it != _channels.end(); ++it)
			(*it)->deactivate(this);
	}
}

void AInst::update(float deltaTime) {
	_prevWeight = _weight;
	_prevTime = _time;

	uint oldFlags = _flags;

	if ((oldFlags & kFadingIn) != 0) {
		double newWeight = (double)deltaTime * _fadeRate + _weight;

		_weight = (float)newWeight;

		if (newWeight > 1.0) {
			_weight = 1.0f;
			_flags = oldFlags & ~kFadingIn;
		}
	} else if ((oldFlags & kFadingOut) != 0) {
		double newWeight = (double)_weight - (double)deltaTime * _fadeRate;

		_weight = (float)newWeight;

		if (newWeight < 0.0) {
			_weight = 0.0f;
			_flags = oldFlags & ~kFadingOut;
		}
	}

	uint currentFlags = _flags;

	if ((currentFlags & kPlaying) != 0) {
		double newTime = (double)deltaTime * _speed + _time;

		_time = (float)newTime;

		if (newTime < 0.0) {
			if ((currentFlags & kLooping) != 0)
				_time = (float)(newTime + _length);
			else
				finish();
		} else if (newTime > _length) {
			if ((currentFlags & kLooping) != 0) {
				double current = _time;
				double reduced;

				if (_length <= 0.0f) {
					warning("AInst::update(): looping instance with no length");

					return;
				}

				do {
					reduced = current - _length;
					current = reduced;
				} while (reduced > _length);

				_time = (float)reduced;
			} else {
				finish();
			}
		}
	}
}

} // End of namespace Grim
