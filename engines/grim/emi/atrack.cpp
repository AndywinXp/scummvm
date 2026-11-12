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

#include "engines/grim/emi/atrack.h"
#include "engines/grim/emi/lformatparser.h"
#include "engines/grim/emi/lmemoryservices.h"

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
	ATrackParser() {
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
	return gpMemoryServices->mallocImpl(size);
}

void AChannelHandle::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

ATrack::ATrack() {
	trackType = 0;
	dataType = 0;
	priority = 0;
	numSamples = 0;
	capacity = 0;
	trackFlags = 0;
}

ATrack::~ATrack() {
}

void *ATrack::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void ATrack::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

int ATrack::getSize() {
	return sizeof(ATrack);
}

bool ATrack::save(LFile *file) {
	ATrackParser parser;

	parser.setFile(file);
	parser.writeEntry(kKeywordPriority, priority);
	parser.writeEntry(kKeywordTrackType, trackType);
	parser.writeEntry(kKeywordDataType, dataType);
	parser.writeEntry(kKeywordTrackFlags, trackFlags);
	parser.writeEntry(kKeywordNumSamples, numSamples);

	switch (dataType) {
	case kDataFloat: {
		ATrackKeyframeData<float> *data = dynamic_cast<ATrackKeyframeData<float> *>(this);

		if (data) {
			for (int i = 0; i < numSamples; i++)
				parser.writeEntry(kKeywordFloat, data->keys[i].time, data->keys[i].value);
		}

		break;
	}
	case kDataInt: {
		ATrackKeyframeData<int> *data = dynamic_cast<ATrackKeyframeData<int> *>(this);

		if (data) {
			for (int i = 0; i < numSamples; i++)
				parser.writeEntry(kKeywordInt, data->keys[i].time, data->keys[i].getInt());
		}

		break;
	}
	case kDataVector: {
		ATrackKeyframeData<CVector> *data = dynamic_cast<ATrackKeyframeData<CVector> *>(this);

		if (data) {
			for (int i = 0; i < numSamples; i++)
				parser.writeEntry(kKeywordVector, data->keys[i].time, data->keys[i].v.x, data->keys[i].v.y, data->keys[i].v.z);
		}

		break;
	}
	case kDataQuaternion: {
		ATrackKeyframeData<CQuaternion> *data = dynamic_cast<ATrackKeyframeData<CQuaternion> *>(this);

		if (data) {
			for (int i = 0; i < numSamples; i++)
				parser.writeEntry(kKeywordQuat, data->keys[i].time, data->keys[i].q.x, data->keys[i].q.y, data->keys[i].q.z, data->keys[i].q.w);
		}

		break;
	}
	default:
		break;
	}

	return true;
}

bool ATrack::restore(LFile *file) {
	ATrackParser parser;

	parser.setFile(file);
	parser.readEntry(kKeywordPriority, &priority);
	parser.readEntry(kKeywordTrackType, &trackType);
	parser.readEntry(kKeywordDataType, &dataType);
	parser.readEntry(kKeywordTrackFlags, &trackFlags);
	parser.readEntry(kKeywordNumSamples, &numSamples);

	if (numSamples < 0) {
		warning("ATrack::restore(): invalid sample count %d", numSamples);
		numSamples = 0;
	}

	switch (dataType) {
	case kDataFloat: {
		ATrackKeyframeData<float> *data = dynamic_cast<ATrackKeyframeData<float> *>(this);

		if (!data) {
			warning("ATrack::restore(): the data type does not match the track class");
			break;
		}

		data->allocateKeys(numSamples);

		if (numSamples > 0 && !data->keys) {
			numSamples = 0;
			break;
		}

		for (int i = 0; i < numSamples; i++)
			parser.readEntry(kKeywordFloat, &data->keys[i].time, &data->keys[i].value);

		break;
	}
	case kDataInt: {
		ATrackKeyframeData<int> *data = dynamic_cast<ATrackKeyframeData<int> *>(this);

		if (!data) {
			warning("ATrack::restore(): the data type does not match the track class");
			break;
		}

		data->allocateKeys(numSamples);

		if (numSamples > 0 && !data->keys) {
			numSamples = 0;
			break;
		}

		for (int i = 0; i < numSamples; i++) {
			int intValue = 0;

			parser.readEntry(kKeywordInt, &data->keys[i].time, &intValue);
			data->keys[i].setInt(intValue);
		}

		break;
	}
	case kDataVector: {
		ATrackKeyframeData<CVector> *data = dynamic_cast<ATrackKeyframeData<CVector> *>(this);

		if (!data) {
			warning("ATrack::restore(): the data type does not match the track class");
			break;
		}

		data->allocateKeys(numSamples);

		if (numSamples > 0 && !data->keys) {
			numSamples = 0;
			break;
		}

		for (int i = 0; i < numSamples; i++)
			parser.readEntry(kKeywordVector, &data->keys[i].time, &data->keys[i].v.x, &data->keys[i].v.y, &data->keys[i].v.z);

		break;
	}
	case kDataQuaternion: {
		ATrackKeyframeData<CQuaternion> *data = dynamic_cast<ATrackKeyframeData<CQuaternion> *>(this);

		if (!data) {
			warning("ATrack::restore(): the data type does not match the track class");
			break;
		}

		data->allocateKeys(numSamples);

		if (numSamples > 0 && !data->keys) {
			numSamples = 0;
			break;
		}

		for (int i = 0; i < numSamples; i++)
			parser.readEntry(kKeywordQuat, &data->keys[i].time, &data->keys[i].q.x, &data->keys[i].q.y, &data->keys[i].q.z, &data->keys[i].q.w);

		break;
	}
	default:
		break;
	}

	return true;
}

template<class T>
ATrackData<T>::ATrackData() {
	value = T();
}

template<class T>
ATrackData<T>::~ATrackData() {
}

template<class T>
int ATrackData<T>::getSize() {
	return sizeof(ATrackData<T>);
}

template<class T>
ATrackKeyframeData<T>::ATrackKeyframeData() {
	keys = nullptr;
}

template<class T>
ATrackKeyframeData<T>::~ATrackKeyframeData() {
	if (keys) {
		gpMemoryServices->freeImpl(keys);
		keys = nullptr;
	}
}

template<class T>
int ATrackKeyframeData<T>::getSize() {
	return (int)(sizeof(ATrackKeyframeData<T>) + numSamples * (sizeof(Key) + sizeof(int)));
}

template<class T>
void ATrackKeyframeData<T>::allocateKeys(int count) {
	capacity = count;
	keys = (Key *)gpMemoryServices->mallocImpl(count * sizeof(Key));

	if (keys)
		memset(keys, 0, count * sizeof(Key));
}

template<class T>
float ATrackKeyframeData<T>::getFirstKeyTime() {
	if (numSamples <= 0 || !keys) {
		warning("ATrackKeyframeData::getFirstKeyTime(): the track has no keys");

		return 0.0f;
	}

	return keys[0].time;
}

template<class T>
float ATrackKeyframeData<T>::getLastKeyTime() {
	if (numSamples <= 0 || !keys) {
		warning("ATrackKeyframeData::getLastKeyTime(): the track has no keys");

		return 0.0f;
	}

	return keys[numSamples - 1].time;
}

template<class T>
void ATrackKeyframeData<T>::shiftTimes(float offset) {
	for (int i = 0; i < numSamples; i++)
		keys[i].time = offset + keys[i].time;
}

template<class T>
bool ATrackKeyframeData<T>::findKey(float *time, float limit, Key **key) {
	int index = 0;

	if (numSamples <= 0)
		return false;

	while (keys[index].time <= *time) {
		if (++index >= numSamples)
			return false;
	}

	if (keys[index].time > limit)
		return false;

	*key = &keys[index];
	*time = keys[index].time;

	return true;
}

template<>
void ATrackKeyframeData<float>::removeRedundantKeys() {
	int index = 0;

	while (index < numSamples - 1) {
		if (keys[index + 1].value == keys[index].value) {
			// The disasm copies one key past the end here, let's avoid that...
			for (int shift = index + 2; shift < numSamples; shift++)
				keys[shift - 1] = keys[shift];

			numSamples--;
		} else {
			index++;
		}
	}
}

template<>
void ATrackKeyframeData<float>::blend(float time, float weight, float *out) {
	if (numSamples == 0 || weight == 0.0f)
		return;

	if (time < keys[0].time)
		return;

	if (time < keys[numSamples - 1].time) {
		int next = 1;

		if (numSamples > 1) {
			do {
				if (keys[next].time >= time)
					break;

				next++;
			} while (next < numSamples);
		}

		int previous = next - 1;
		double elapsed = (double)time - keys[previous].time;
		double span = (double)keys[next].time - keys[previous].time;
		double fraction = 0.0;

		// The original could divide by zero here, let's avoid that...
		if (span != 0.0)
			fraction = elapsed / span;

		if (weight >= 1.0f) {
			*out = (float)(((double)keys[next].value - keys[previous].value) * fraction + keys[previous].value);
		} else {
			*out = (float)((((double)keys[next].value - keys[previous].value) * fraction + keys[previous].value - *out) * weight + *out);
		}
	} else {
		*out = (float)(((double)*out - keys[numSamples - 1].value) * (1.0 - weight) + keys[numSamples - 1].value);
	}
}

template<>
void ATrackKeyframeData<float>::sample(float previousTime, float time, float *out, AChannelHandle *channel) {
	if (numSamples == 0)
		return;

	if (time < keys[0].time)
		return;

	if (time < keys[numSamples - 1].time) {
		int next = 1;

		if (numSamples > 1) {
			while (true) {
				if (keys[next].time >= time)
					break;

				if (channel && keys[next].time > previousTime)
					channel->onKey(this, keys[next].time, &keys[next]);

				if (++next >= numSamples)
					return;
			}

			double span = (double)keys[next].time - keys[next - 1].time;
			double fraction = 0.0;

			// The original could divide by zero here, let's avoid that...
			if (span != 0.0)
				fraction = ((double)time - keys[next - 1].time) / span;

			*out = (float)(fraction * ((double)keys[next].value - keys[next - 1].value) + keys[next - 1].value);
		}
	} else {
		if (channel) {
			float lastTime = keys[numSamples - 1].time;

			if (previousTime < lastTime)
				channel->onKey(this, lastTime, &keys[numSamples - 1]);
		}

		*out = keys[numSamples - 1].value;
	}
}

template<>
void ATrackKeyframeData<int>::removeRedundantKeys() {
	int index = 0;

	while (index < numSamples - 1) {
		if (keys[index + 1].getInt() == keys[index].getInt()) {
			// The disasm copies one key past the end here, let's avoid that...
			for (int shift = index + 2; shift < numSamples; shift++)
				keys[shift - 1] = keys[shift];

			numSamples--;
		} else {
			index++;
		}
	}
}

template<>
void ATrackKeyframeData<int>::blend(float time, float weight, int *out) {
	if (numSamples == 0 || weight == 0.0f)
		return;

	if (time < keys[0].time)
		return;

	if (time < keys[numSamples - 1].time) {
		int next = 1;

		if (numSamples > 1) {
			do {
				if (keys[next].time >= time)
					break;

				next++;
			} while (next < numSamples);
		}

		int previous = next - 1;
		double elapsed = (double)time - keys[previous].time;
		double span = (double)keys[next].time - keys[previous].time;
		double fraction = 0.0;

		// The original could divide by zero here, let's avoid that...
		if (span != 0.0)
			fraction = elapsed / span;

		int difference = (int)((uint)keys[next].getInt() - (uint)keys[previous].getInt());

		if (weight >= 1.0f) {
			*out = doubleToInt((double)difference * fraction + (double)keys[previous].getInt());
		} else {
			int interpolated = doubleToInt((double)difference * fraction + (double)keys[previous].getInt());
			int delta = (int)((uint)interpolated - (uint)*out);

			*out = doubleToInt((double)delta * weight + (double)*out);
		}
	} else {
		int delta = (int)((uint)*out - (uint)keys[numSamples - 1].getInt());

		*out = doubleToInt((double)delta * (1.0 - weight) + (double)keys[numSamples - 1].getInt());
	}
}

template<>
void ATrackKeyframeData<int>::sample(float previousTime, float time, int *out, AChannelHandle *channel) {
	if (numSamples == 0)
		return;

	if (time < keys[0].time)
		return;

	if (time < keys[numSamples - 1].time) {
		int next = 1;

		if (numSamples > 1) {
			while (true) {
				if (keys[next].time >= time)
					break;

				if (channel && keys[next].time > previousTime)
					channel->onKey(this, keys[next].time, &keys[next]);

				if (++next >= numSamples)
					return;
			}

			double span = (double)keys[next].time - keys[next - 1].time;
			double fraction = 0.0;

			// The original could divide by zero here, let's avoid that...
			if (span != 0.0)
				fraction = ((double)time - keys[next - 1].time) / span;

			int difference = (int)((uint)keys[next].getInt() - (uint)keys[next - 1].getInt());

			*out = doubleToInt(fraction * (double)difference + (double)keys[next - 1].getInt());
		}
	} else {
		if (channel) {
			float lastTime = keys[numSamples - 1].time;

			if (previousTime < lastTime)
				channel->onKey(this, lastTime, &keys[numSamples - 1]);
		}

		*out = keys[numSamples - 1].getInt();
	}
}

template<>
void ATrackKeyframeData<CVector>::removeRedundantKeys() {
	int index = 0;

	while (index < numSamples - 1) {
		if (keys[index + 1].v.x == keys[index].v.x && keys[index + 1].v.y == keys[index].v.y && keys[index + 1].v.z == keys[index].v.z) {
			// The disasm copies one key past the end here, let's avoid that...
			for (int shift = index + 2; shift < numSamples; shift++)
				keys[shift - 1] = keys[shift];

			numSamples--;
		} else {
			index++;
		}
	}
}

template<>
void ATrackKeyframeData<CVector>::blend(float time, float weight, CVector *out) {
	if (numSamples == 0 || weight == 0.0f)
		return;

	if (time < keys[0].time)
		return;

	if (time < keys[numSamples - 1].time) {
		int next = 1;

		if (numSamples > 1) {
			do {
				if (keys[next].time >= time)
					break;

				next++;
			} while (next < numSamples);
		}

		int previous = next - 1;
		double span = (double)keys[next].time - keys[previous].time;
		float fraction = 0.0f;

		// The original could divide by zero here, let's avoid that...
		if (span != 0.0)
			fraction = (float)(((double)time - keys[previous].time) / span);

		const CVector &from = keys[previous].v;
		const CVector &to = keys[next].v;

		if (weight >= 1.0f) {
			out->x = (float)(((double)to.x - from.x) * fraction + from.x);
			out->y = (float)(((double)to.y - from.y) * fraction + from.y);
			out->z = (float)(((double)to.z - from.z) * fraction + from.z);
		} else {
			float oldY = out->y;
			float oldZ = out->z;
			float interpolatedY = (float)(((double)to.y - from.y) * fraction + from.y);
			float interpolatedZ = (float)(((double)to.z - from.z) * fraction + from.z);

			out->x = (float)((((double)to.x - from.x) * fraction + from.x - out->x) * weight + out->x);
			out->y = (float)(((double)interpolatedY - oldY) * weight + oldY);
			out->z = (float)(((double)interpolatedZ - oldZ) * weight + oldZ);
		}
	} else {
		const CVector &last = keys[numSamples - 1].v;
		double remaining = 1.0 - weight;

		out->x = (float)(((double)out->x - last.x) * remaining + last.x);
		out->y = (float)(((double)out->y - last.y) * remaining + last.y);
		out->z = (float)(((double)out->z - last.z) * remaining + last.z);
	}
}

template<>
void ATrackKeyframeData<CVector>::sample(float previousTime, float time, CVector *out, AChannelHandle *channel) {
	if (numSamples == 0)
		return;

	if (time < keys[0].time)
		return;

	if (time < keys[numSamples - 1].time) {
		int next = 1;

		if (numSamples > 1) {
			while (true) {
				if (keys[next].time >= time)
					break;

				if (channel && keys[next].time > previousTime)
					channel->onKey(this, keys[next].time, &keys[next]);

				if (++next >= numSamples)
					return;
			}

			double span = (double)keys[next].time - keys[next - 1].time;
			double fraction = 0.0;

			// The original could divide by zero here, let's avoid that...
			if (span != 0.0)
				fraction = ((double)time - keys[next - 1].time) / span;

			const CVector &from = keys[next - 1].v;
			const CVector &to = keys[next].v;

			out->x = (float)(((double)to.x - from.x) * fraction + from.x);
			out->y = (float)(((double)to.y - from.y) * fraction + from.y);
			out->z = (float)(((double)to.z - from.z) * fraction + from.z);
		}
	} else {
		if (channel) {
			float lastTime = keys[numSamples - 1].time;

			if (previousTime < lastTime)
				channel->onKey(this, lastTime, &keys[numSamples - 1]);
		}

		out->x = keys[numSamples - 1].v.x;
		out->y = keys[numSamples - 1].v.y;
		out->z = keys[numSamples - 1].v.z;
	}
}

template<>
void ATrackKeyframeData<CQuaternion>::removeRedundantKeys() {
	int index = 0;

	while (index < numSamples - 1) {
		if (keys[index + 1].q.x == keys[index].q.x && keys[index + 1].q.y == keys[index].q.y && keys[index + 1].q.z == keys[index].q.z && keys[index + 1].q.w == keys[index].q.w) {
			// The disasm copies one key past the end here, let's avoid that...
			for (int shift = index + 2; shift < numSamples; shift++)
				keys[shift - 1] = keys[shift];

			numSamples--;
		} else {
			index++;
		}
	}
}

template<>
void ATrackKeyframeData<CQuaternion>::blend(float time, float weight, CQuaternion *out) {
	if (numSamples == 0 || weight == 0.0f)
		return;

	if (time < keys[0].time)
		return;

	if (time < keys[numSamples - 1].time) {
		int next = 1;

		if (numSamples > 1) {
			do {
				if (keys[next].time >= time)
					break;

				next++;
			} while (next < numSamples);
		}

		int previous = next - 1;
		double span = (double)keys[next].time - keys[previous].time;
		float fraction = 0.0f;

		// The original could divide by zero here, let's avoid that...
		if (span != 0.0)
			fraction = (float)(((double)time - keys[previous].time) / span);

		if (weight >= 1.0f) {
			keys[previous].q.slerp(keys[next].q, fraction, out);
		} else {
			CQuaternion oldValue;
			CQuaternion interpolated;

			oldValue = *out;
			keys[previous].q.slerp(keys[next].q, fraction, &interpolated);
			oldValue.slerp(interpolated, weight, out);
		}
	} else {
		float remaining = (float)(1.0 - weight);

		keys[numSamples - 1].q.slerp(*out, remaining, out);
	}
}

template<>
void ATrackKeyframeData<CQuaternion>::sample(float previousTime, float time, CQuaternion *out, AChannelHandle *channel) {
	if (numSamples == 0)
		return;

	if (time < keys[0].time)
		return;

	if (time < keys[numSamples - 1].time) {
		int next = 1;

		if (numSamples > 1) {
			while (true) {
				if (keys[next].time >= time)
					break;

				if (channel && keys[next].time > previousTime)
					channel->onKey(this, keys[next].time, &keys[next]);

				if (++next >= numSamples)
					return;
			}

			double span = (double)keys[next].time - keys[next - 1].time;
			float fraction = 0.0f;

			// The original could divide by zero here, let's avoid that...
			if (span != 0.0)
				fraction = (float)(((double)time - keys[next - 1].time) / span);

			keys[next - 1].q.slerp(keys[next].q, fraction, out);
		}
	} else {
		if (channel) {
			float lastTime = keys[numSamples - 1].time;

			if (previousTime < lastTime)
				channel->onKey(this, lastTime, &keys[numSamples - 1]);
		}

		*out = keys[numSamples - 1].q;
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
