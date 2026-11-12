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

#include "engines/grim/emi/animation/atrack.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lformatparser.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

enum {
	kKeywordTrackType = 4,
	kKeywordDataType = 5,
	kKeywordNumSamples = 6,
	kKeywordFloat = 7,
	kKeywordInt = 8,
	kKeywordVector = 9,
	kKeywordQuat = 10,
	kKeywordTrackFlags = 11,
	kKeywordPriority = 12
};

class ATrackParser : public LFormatParser {
public:
	explicit ATrackParser(EmiEngine *engine) : LFormatParser(engine) {
		addKeyword("TRACKTYPE", kKeywordTrackType, 1);
		addKeyword("PRIORITY", kKeywordPriority, 1);
		addKeyword("DATATYPE", kKeywordDataType, 1);
		addKeyword("NUMSAMPLES", kKeywordNumSamples, 1);
		addKeyword("FLOAT", kKeywordFloat, 10);
		addKeyword("INT", kKeywordInt, 6);
		addKeyword("VECTOR", kKeywordVector, 170);
		addKeyword("QUAT", kKeywordQuat, 682);
		addKeyword("TRACKFLAGS", kKeywordTrackFlags, 1);
	}
};

static int doubleToInt(double value) {
	if (!(value >= -9223372036854775808.0 && value < 9223372036854775808.0)) {
		warning("ATrack: value %f cannot be converted to an integer", value);

		return 0;
	}

	return (int)(int64)value;
}

void *AChannelHandle::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void AChannelHandle::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

ATrack::ATrack(EmiEngine *engine) : _engine(engine) {
	_trackType = 0;
	_dataType = 0;
	_priority = 0;
	_numSamples = 0;
	_capacity = 0;
	_trackFlags = 0;
}

ATrack::~ATrack() {
}

void *ATrack::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void ATrack::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

int ATrack::getSize() {
	return sizeof(ATrack);
}

bool ATrack::save(LFile *file) {
	ATrackParser parser(_engine);

	parser.setFile(file);
	parser.writeEntry(kKeywordPriority, _priority);
	parser.writeEntry(kKeywordTrackType, _trackType);
	parser.writeEntry(kKeywordDataType, _dataType);
	parser.writeEntry(kKeywordTrackFlags, _trackFlags);
	parser.writeEntry(kKeywordNumSamples, _numSamples);

	switch (_dataType) {
	case kDataFloat: {
		ATrackKeyframeData<float> *data = dynamic_cast<ATrackKeyframeData<float> *>(this);

		if (data) {
			for (int i = 0; i < _numSamples; i++)
				parser.writeEntry(kKeywordFloat, data->_keys[i].time, data->_keys[i].value);
		}

		break;
	}
	case kDataInt: {
		ATrackKeyframeData<int> *data = dynamic_cast<ATrackKeyframeData<int> *>(this);

		if (data) {
			for (int i = 0; i < _numSamples; i++)
				parser.writeEntry(kKeywordInt, data->_keys[i].time, data->_keys[i].getInt());
		}

		break;
	}
	case kDataVector: {
		ATrackKeyframeData<CVector> *data = dynamic_cast<ATrackKeyframeData<CVector> *>(this);

		if (data) {
			for (int i = 0; i < _numSamples; i++)
				parser.writeEntry(kKeywordVector, data->_keys[i].time, data->_keys[i].v._x, data->_keys[i].v._y, data->_keys[i].v._z);
		}

		break;
	}
	case kDataQuaternion: {
		ATrackKeyframeData<CQuaternion> *data = dynamic_cast<ATrackKeyframeData<CQuaternion> *>(this);

		if (data) {
			for (int i = 0; i < _numSamples; i++)
				parser.writeEntry(kKeywordQuat, data->_keys[i].time, data->_keys[i].q._x, data->_keys[i].q._y, data->_keys[i].q._z, data->_keys[i].q._w);
		}

		break;
	}
	default:
		break;
	}

	return true;
}

bool ATrack::restore(LFile *file) {
	ATrackParser parser(_engine);

	parser.setFile(file);
	parser.readEntry(kKeywordPriority, &_priority);
	parser.readEntry(kKeywordTrackType, &_trackType);
	parser.readEntry(kKeywordDataType, &_dataType);
	parser.readEntry(kKeywordTrackFlags, &_trackFlags);
	parser.readEntry(kKeywordNumSamples, &_numSamples);

	if (_numSamples < 0) {
		warning("ATrack::restore(): invalid sample count %d", _numSamples);
		_numSamples = 0;
	}

	switch (_dataType) {
	case kDataFloat: {
		ATrackKeyframeData<float> *data = dynamic_cast<ATrackKeyframeData<float> *>(this);

		if (!data) {
			warning("ATrack::restore(): the data type does not match the track class");
			break;
		}

		data->allocateKeys(_numSamples);

		if (_numSamples > 0 && !data->_keys) {
			_numSamples = 0;
			break;
		}

		for (int i = 0; i < _numSamples; i++)
			parser.readEntry(kKeywordFloat, &data->_keys[i].time, &data->_keys[i].value);

		break;
	}
	case kDataInt: {
		ATrackKeyframeData<int> *data = dynamic_cast<ATrackKeyframeData<int> *>(this);

		if (!data) {
			warning("ATrack::restore(): the data type does not match the track class");
			break;
		}

		data->allocateKeys(_numSamples);

		if (_numSamples > 0 && !data->_keys) {
			_numSamples = 0;
			break;
		}

		for (int i = 0; i < _numSamples; i++) {
			int intValue = 0;

			parser.readEntry(kKeywordInt, &data->_keys[i].time, &intValue);
			data->_keys[i].setInt(intValue);
		}

		break;
	}
	case kDataVector: {
		ATrackKeyframeData<CVector> *data = dynamic_cast<ATrackKeyframeData<CVector> *>(this);

		if (!data) {
			warning("ATrack::restore(): the data type does not match the track class");
			break;
		}

		data->allocateKeys(_numSamples);

		if (_numSamples > 0 && !data->_keys) {
			_numSamples = 0;
			break;
		}

		for (int i = 0; i < _numSamples; i++)
			parser.readEntry(kKeywordVector, &data->_keys[i].time, &data->_keys[i].v._x, &data->_keys[i].v._y, &data->_keys[i].v._z);

		break;
	}
	case kDataQuaternion: {
		ATrackKeyframeData<CQuaternion> *data = dynamic_cast<ATrackKeyframeData<CQuaternion> *>(this);

		if (!data) {
			warning("ATrack::restore(): the data type does not match the track class");
			break;
		}

		data->allocateKeys(_numSamples);

		if (_numSamples > 0 && !data->_keys) {
			_numSamples = 0;
			break;
		}

		for (int i = 0; i < _numSamples; i++)
			parser.readEntry(kKeywordQuat, &data->_keys[i].time, &data->_keys[i].q._x, &data->_keys[i].q._y, &data->_keys[i].q._z, &data->_keys[i].q._w);

		break;
	}
	default:
		break;
	}

	return true;
}

template<class T>
ATrackData<T>::ATrackData(EmiEngine *engine) : ATrack(engine) {
	_value = T();
}

template<class T>
ATrackData<T>::~ATrackData() {
}

template<class T>
int ATrackData<T>::getSize() {
	return sizeof(ATrackData<T>);
}

template<class T>
ATrackKeyframeData<T>::ATrackKeyframeData(EmiEngine *engine) : ATrackData<T>(engine) {
	_keys = nullptr;
}

template<class T>
ATrackKeyframeData<T>::~ATrackKeyframeData() {
	if (_keys) {
		_engine->getMemoryServices()->freeImpl(_keys);
		_keys = nullptr;
	}
}

template<class T>
int ATrackKeyframeData<T>::getSize() {
	return (int)(sizeof(ATrackKeyframeData<T>) + _numSamples * (sizeof(Key) + sizeof(int)));
}

template<class T>
void ATrackKeyframeData<T>::allocateKeys(int count) {
	_capacity = count;
	_keys = (Key *)_engine->getMemoryServices()->mallocImpl(count * sizeof(Key));

	if (_keys)
		memset(_keys, 0, count * sizeof(Key));
}

template<class T>
float ATrackKeyframeData<T>::getFirstKeyTime() {
	if (_numSamples <= 0 || !_keys) {
		warning("ATrackKeyframeData::getFirstKeyTime(): the track has no keys");

		return 0.0f;
	}

	return _keys[0].time;
}

template<class T>
float ATrackKeyframeData<T>::getLastKeyTime() {
	if (_numSamples <= 0 || !_keys) {
		warning("ATrackKeyframeData::getLastKeyTime(): the track has no keys");

		return 0.0f;
	}

	return _keys[_numSamples - 1].time;
}

template<class T>
void ATrackKeyframeData<T>::shiftTimes(float offset) {
	for (int i = 0; i < _numSamples; i++)
		_keys[i].time = offset + _keys[i].time;
}

template<class T>
bool ATrackKeyframeData<T>::findKey(float *time, float limit, Key **key) {
	int index = 0;

	if (_numSamples <= 0)
		return false;

	while (_keys[index].time <= *time) {
		if (++index >= _numSamples)
			return false;
	}

	if (_keys[index].time > limit)
		return false;

	*key = &_keys[index];
	*time = _keys[index].time;

	return true;
}

template<>
void ATrackKeyframeData<float>::removeRedundantKeys() {
	int index = 0;

	while (index < _numSamples - 1) {
		if (_keys[index + 1].value == _keys[index].value) {
			// The disasm copies one key past the end here, let's avoid that...
			for (int shift = index + 2; shift < _numSamples; shift++)
				_keys[shift - 1] = _keys[shift];

			_numSamples--;
		} else {
			index++;
		}
	}
}

template<>
void ATrackKeyframeData<float>::blend(float time, float weight, float *out) {
	if (_numSamples == 0 || weight == 0.0f)
		return;

	if (time < _keys[0].time)
		return;

	if (time < _keys[_numSamples - 1].time) {
		int next = 1;

		if (_numSamples > 1) {
			do {
				if (_keys[next].time >= time)
					break;

				next++;
			} while (next < _numSamples);
		}

		int previous = next - 1;
		double elapsed = (double)time - _keys[previous].time;
		double span = (double)_keys[next].time - _keys[previous].time;
		double fraction = 0.0;

		// The original could divide by zero here, let's avoid that...
		if (span != 0.0)
			fraction = elapsed / span;

		if (weight >= 1.0f) {
			*out = (float)(((double)_keys[next].value - _keys[previous].value) * fraction + _keys[previous].value);
		} else {
			*out = (float)((((double)_keys[next].value - _keys[previous].value) * fraction + _keys[previous].value - *out) * weight + *out);
		}
	} else {
		*out = (float)(((double)*out - _keys[_numSamples - 1].value) * (1.0 - weight) + _keys[_numSamples - 1].value);
	}
}

template<>
void ATrackKeyframeData<float>::sample(float previousTime, float time, float *out, AChannelHandle *channel) {
	if (_numSamples == 0)
		return;

	if (time < _keys[0].time)
		return;

	if (time < _keys[_numSamples - 1].time) {
		int next = 1;

		if (_numSamples > 1) {
			while (true) {
				if (_keys[next].time >= time)
					break;

				if (channel && _keys[next].time > previousTime)
					channel->onKey(this, _keys[next].time, &_keys[next]);

				if (++next >= _numSamples)
					return;
			}

			double span = (double)_keys[next].time - _keys[next - 1].time;
			double fraction = 0.0;

			// The original could divide by zero here, let's avoid that...
			if (span != 0.0)
				fraction = ((double)time - _keys[next - 1].time) / span;

			*out = (float)(fraction * ((double)_keys[next].value - _keys[next - 1].value) + _keys[next - 1].value);
		}
	} else {
		if (channel) {
			float lastTime = _keys[_numSamples - 1].time;

			if (previousTime < lastTime)
				channel->onKey(this, lastTime, &_keys[_numSamples - 1]);
		}

		*out = _keys[_numSamples - 1].value;
	}
}

template<>
void ATrackKeyframeData<int>::removeRedundantKeys() {
	int index = 0;

	while (index < _numSamples - 1) {
		if (_keys[index + 1].getInt() == _keys[index].getInt()) {
			// The disasm copies one key past the end here, let's avoid that...
			for (int shift = index + 2; shift < _numSamples; shift++)
				_keys[shift - 1] = _keys[shift];

			_numSamples--;
		} else {
			index++;
		}
	}
}

template<>
void ATrackKeyframeData<int>::blend(float time, float weight, int *out) {
	if (_numSamples == 0 || weight == 0.0f)
		return;

	if (time < _keys[0].time)
		return;

	if (time < _keys[_numSamples - 1].time) {
		int next = 1;

		if (_numSamples > 1) {
			do {
				if (_keys[next].time >= time)
					break;

				next++;
			} while (next < _numSamples);
		}

		int previous = next - 1;
		double elapsed = (double)time - _keys[previous].time;
		double span = (double)_keys[next].time - _keys[previous].time;
		double fraction = 0.0;

		// The original could divide by zero here, let's avoid that...
		if (span != 0.0)
			fraction = elapsed / span;

		int difference = (int)((uint)_keys[next].getInt() - (uint)_keys[previous].getInt());

		if (weight >= 1.0f) {
			*out = doubleToInt((double)difference * fraction + (double)_keys[previous].getInt());
		} else {
			int interpolated = doubleToInt((double)difference * fraction + (double)_keys[previous].getInt());
			int delta = (int)((uint)interpolated - (uint)*out);

			*out = doubleToInt((double)delta * weight + (double)*out);
		}
	} else {
		int delta = (int)((uint)*out - (uint)_keys[_numSamples - 1].getInt());

		*out = doubleToInt((double)delta * (1.0 - weight) + (double)_keys[_numSamples - 1].getInt());
	}
}

template<>
void ATrackKeyframeData<int>::sample(float previousTime, float time, int *out, AChannelHandle *channel) {
	if (_numSamples == 0)
		return;

	if (time < _keys[0].time)
		return;

	if (time < _keys[_numSamples - 1].time) {
		int next = 1;

		if (_numSamples > 1) {
			while (true) {
				if (_keys[next].time >= time)
					break;

				if (channel && _keys[next].time > previousTime)
					channel->onKey(this, _keys[next].time, &_keys[next]);

				if (++next >= _numSamples)
					return;
			}

			double span = (double)_keys[next].time - _keys[next - 1].time;
			double fraction = 0.0;

			// The original could divide by zero here, let's avoid that...
			if (span != 0.0)
				fraction = ((double)time - _keys[next - 1].time) / span;

			int difference = (int)((uint)_keys[next].getInt() - (uint)_keys[next - 1].getInt());

			*out = doubleToInt(fraction * (double)difference + (double)_keys[next - 1].getInt());
		}
	} else {
		if (channel) {
			float lastTime = _keys[_numSamples - 1].time;

			if (previousTime < lastTime)
				channel->onKey(this, lastTime, &_keys[_numSamples - 1]);
		}

		*out = _keys[_numSamples - 1].getInt();
	}
}

template<>
void ATrackKeyframeData<CVector>::removeRedundantKeys() {
	int index = 0;

	while (index < _numSamples - 1) {
		if (_keys[index + 1].v._x == _keys[index].v._x && _keys[index + 1].v._y == _keys[index].v._y && _keys[index + 1].v._z == _keys[index].v._z) {
			// The disasm copies one key past the end here, let's avoid that...
			for (int shift = index + 2; shift < _numSamples; shift++)
				_keys[shift - 1] = _keys[shift];

			_numSamples--;
		} else {
			index++;
		}
	}
}

template<>
void ATrackKeyframeData<CVector>::blend(float time, float weight, CVector *out) {
	if (_numSamples == 0 || weight == 0.0f)
		return;

	if (time < _keys[0].time)
		return;

	if (time < _keys[_numSamples - 1].time) {
		int next = 1;

		if (_numSamples > 1) {
			do {
				if (_keys[next].time >= time)
					break;

				next++;
			} while (next < _numSamples);
		}

		int previous = next - 1;
		double span = (double)_keys[next].time - _keys[previous].time;
		float fraction = 0.0f;

		// The original could divide by zero here, let's avoid that...
		if (span != 0.0)
			fraction = (float)(((double)time - _keys[previous].time) / span);

		const CVector &from = _keys[previous].v;
		const CVector &to = _keys[next].v;

		if (weight >= 1.0f) {
			out->_x = (float)(((double)to._x - from._x) * fraction + from._x);
			out->_y = (float)(((double)to._y - from._y) * fraction + from._y);
			out->_z = (float)(((double)to._z - from._z) * fraction + from._z);
		} else {
			float oldY = out->_y;
			float oldZ = out->_z;
			float interpolatedY = (float)(((double)to._y - from._y) * fraction + from._y);
			float interpolatedZ = (float)(((double)to._z - from._z) * fraction + from._z);

			out->_x = (float)((((double)to._x - from._x) * fraction + from._x - out->_x) * weight + out->_x);
			out->_y = (float)(((double)interpolatedY - oldY) * weight + oldY);
			out->_z = (float)(((double)interpolatedZ - oldZ) * weight + oldZ);
		}
	} else {
		const CVector &last = _keys[_numSamples - 1].v;
		double remaining = 1.0 - weight;

		out->_x = (float)(((double)out->_x - last._x) * remaining + last._x);
		out->_y = (float)(((double)out->_y - last._y) * remaining + last._y);
		out->_z = (float)(((double)out->_z - last._z) * remaining + last._z);
	}
}

template<>
void ATrackKeyframeData<CVector>::sample(float previousTime, float time, CVector *out, AChannelHandle *channel) {
	if (_numSamples == 0)
		return;

	if (time < _keys[0].time)
		return;

	if (time < _keys[_numSamples - 1].time) {
		int next = 1;

		if (_numSamples > 1) {
			while (true) {
				if (_keys[next].time >= time)
					break;

				if (channel && _keys[next].time > previousTime)
					channel->onKey(this, _keys[next].time, &_keys[next]);

				if (++next >= _numSamples)
					return;
			}

			double span = (double)_keys[next].time - _keys[next - 1].time;
			double fraction = 0.0;

			// The original could divide by zero here, let's avoid that...
			if (span != 0.0)
				fraction = ((double)time - _keys[next - 1].time) / span;

			const CVector &from = _keys[next - 1].v;
			const CVector &to = _keys[next].v;

			out->_x = (float)(((double)to._x - from._x) * fraction + from._x);
			out->_y = (float)(((double)to._y - from._y) * fraction + from._y);
			out->_z = (float)(((double)to._z - from._z) * fraction + from._z);
		}
	} else {
		if (channel) {
			float lastTime = _keys[_numSamples - 1].time;

			if (previousTime < lastTime)
				channel->onKey(this, lastTime, &_keys[_numSamples - 1]);
		}

		out->_x = _keys[_numSamples - 1].v._x;
		out->_y = _keys[_numSamples - 1].v._y;
		out->_z = _keys[_numSamples - 1].v._z;
	}
}

template<>
void ATrackKeyframeData<CQuaternion>::removeRedundantKeys() {
	int index = 0;

	while (index < _numSamples - 1) {
		if (_keys[index + 1].q._x == _keys[index].q._x && _keys[index + 1].q._y == _keys[index].q._y && _keys[index + 1].q._z == _keys[index].q._z && _keys[index + 1].q._w == _keys[index].q._w) {
			// The disasm copies one key past the end here, let's avoid that...
			for (int shift = index + 2; shift < _numSamples; shift++)
				_keys[shift - 1] = _keys[shift];

			_numSamples--;
		} else {
			index++;
		}
	}
}

template<>
void ATrackKeyframeData<CQuaternion>::blend(float time, float weight, CQuaternion *out) {
	if (_numSamples == 0 || weight == 0.0f)
		return;

	if (time < _keys[0].time)
		return;

	if (time < _keys[_numSamples - 1].time) {
		int next = 1;

		if (_numSamples > 1) {
			do {
				if (_keys[next].time >= time)
					break;

				next++;
			} while (next < _numSamples);
		}

		int previous = next - 1;
		double span = (double)_keys[next].time - _keys[previous].time;
		float fraction = 0.0f;

		// The original could divide by zero here, let's avoid that...
		if (span != 0.0)
			fraction = (float)(((double)time - _keys[previous].time) / span);

		if (weight >= 1.0f) {
			_keys[previous].q.slerp(_keys[next].q, fraction, out);
		} else {
			CQuaternion oldValue;
			CQuaternion interpolated;

			oldValue = *out;
			_keys[previous].q.slerp(_keys[next].q, fraction, &interpolated);
			oldValue.slerp(interpolated, weight, out);
		}
	} else {
		float remaining = (float)(1.0 - weight);

		_keys[_numSamples - 1].q.slerp(*out, remaining, out);
	}
}

template<>
void ATrackKeyframeData<CQuaternion>::sample(float previousTime, float time, CQuaternion *out, AChannelHandle *channel) {
	if (_numSamples == 0)
		return;

	if (time < _keys[0].time)
		return;

	if (time < _keys[_numSamples - 1].time) {
		int next = 1;

		if (_numSamples > 1) {
			while (true) {
				if (_keys[next].time >= time)
					break;

				if (channel && _keys[next].time > previousTime)
					channel->onKey(this, _keys[next].time, &_keys[next]);

				if (++next >= _numSamples)
					return;
			}

			double span = (double)_keys[next].time - _keys[next - 1].time;
			float fraction = 0.0f;

			// The original could divide by zero here, let's avoid that...
			if (span != 0.0)
				fraction = (float)(((double)time - _keys[next - 1].time) / span);

			_keys[next - 1].q.slerp(_keys[next].q, fraction, out);
		}
	} else {
		if (channel) {
			float lastTime = _keys[_numSamples - 1].time;

			if (previousTime < lastTime)
				channel->onKey(this, lastTime, &_keys[_numSamples - 1]);
		}

		*out = _keys[_numSamples - 1].q;
	}
}

template class ATrackData<float>;
template class ATrackData<int>;
template class ATrackData<CVector>;
template class ATrackData<CQuaternion>;
template class ATrackKeyframeData<float>;
template class ATrackKeyframeData<int>;
template class ATrackKeyframeData<CVector>;
template class ATrackKeyframeData<CQuaternion>;

} // End of namespace Grim
