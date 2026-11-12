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

#ifndef GRIM_EMI_ATRACK_H
#define GRIM_EMI_ATRACK_H

#include "engines/grim/emi/math/cquaternion.h"
#include "engines/grim/emi/math/cvector.h"
#include "engines/grim/emi/files/lfile.h"
#include "engines/grim/emi/core/lsaverestore.h"

namespace Grim {

class AInst;
class ATrack;
class EmiEngine;

struct ScalarKey {
	int getInt() const {
		int result;

		memcpy(&result, &value, sizeof(result));

		return result;
	}

	void setInt(int newValue) {
		memcpy(&value, &newValue, sizeof(value));
	}

	float value;
	float time;
};

struct VectorKey {
	CVector v;
	float time;
};

struct QuatKey {
	CQuaternion q;
	float time;
};

template<class T>
struct ATrackKey {
};

template<>
struct ATrackKey<float> {
	typedef ScalarKey Type;
};

template<>
struct ATrackKey<int> {
	typedef ScalarKey Type;
};

template<>
struct ATrackKey<CVector> {
	typedef VectorKey Type;
};

template<>
struct ATrackKey<CQuaternion> {
	typedef QuatKey Type;
};

class AChannelHandle {
public:
	void *operator new(size_t size);
	void operator delete(void *block);

	virtual bool detach(AInst *inst) = 0;
	virtual void activate(AInst *inst) = 0;
	virtual void deactivate(AInst *inst) = 0;
	virtual void onKey(ATrack *track, float time, void *key) = 0;

protected:
	~AChannelHandle() {}
};

class ATrack : public LSaveRestore {
public:
	enum {
		kDataFloat = 1,
		kDataInt = 2,
		kDataVector = 3,
		kDataQuaternion = 4
	};

	explicit ATrack(EmiEngine *engine);
	virtual ~ATrack();

	void *operator new(size_t size);
	void operator delete(void *block);

	virtual bool save(LFile *file);
	virtual bool restore(LFile *file);
	virtual int getSize();
	virtual void shiftTimes(float offset) = 0;
	virtual void removeRedundantKeys() = 0;
	virtual float getFirstKeyTime() = 0;
	virtual float getLastKeyTime() = 0;

	int priority;
	int trackFlags;
	int trackType;
	int dataType;
	int capacity;
	int numSamples;

protected:
	EmiEngine *_engine;
};

template<class T>
class ATrackData : public ATrack {
public:
	typedef typename ATrackKey<T>::Type Key;

	explicit ATrackData(EmiEngine *engine);
	virtual ~ATrackData();

	virtual int getSize();
	virtual void blend(float time, float weight, T *out) = 0;
	virtual void sample(float previousTime, float time, T *out, AChannelHandle *channel) = 0;
	virtual bool findKey(float *time, float limit, Key **key) = 0;

	T value;
};

template<class T>
class ATrackKeyframeData : public ATrackData<T> {
public:
	typedef typename ATrackKey<T>::Type Key;

	explicit ATrackKeyframeData(EmiEngine *engine);
	virtual ~ATrackKeyframeData();

	virtual int getSize();
	virtual void shiftTimes(float offset);
	virtual void removeRedundantKeys();
	virtual float getFirstKeyTime();
	virtual float getLastKeyTime();
	virtual void blend(float time, float weight, T *out);
	virtual void sample(float previousTime, float time, T *out, AChannelHandle *channel);
	virtual bool findKey(float *time, float limit, Key **key);

	void allocateKeys(int count);

	using ATrackData<T>::capacity;
	using ATrackData<T>::numSamples;

	Key *keys;

protected:
	using ATrackData<T>::_engine;
};

template<> void ATrackKeyframeData<float>::removeRedundantKeys();
template<> void ATrackKeyframeData<float>::blend(float time, float weight, float *out);
template<> void ATrackKeyframeData<float>::sample(float previousTime, float time, float *out, AChannelHandle *channel);
template<> void ATrackKeyframeData<int>::removeRedundantKeys();
template<> void ATrackKeyframeData<int>::blend(float time, float weight, int *out);
template<> void ATrackKeyframeData<int>::sample(float previousTime, float time, int *out, AChannelHandle *channel);
template<> void ATrackKeyframeData<CVector>::removeRedundantKeys();
template<> void ATrackKeyframeData<CVector>::blend(float time, float weight, CVector *out);
template<> void ATrackKeyframeData<CVector>::sample(float previousTime, float time, CVector *out, AChannelHandle *channel);
template<> void ATrackKeyframeData<CQuaternion>::removeRedundantKeys();
template<> void ATrackKeyframeData<CQuaternion>::blend(float time, float weight, CQuaternion *out);
template<> void ATrackKeyframeData<CQuaternion>::sample(float previousTime, float time, CQuaternion *out, AChannelHandle *channel);

} // End of namespace Grim

#endif
