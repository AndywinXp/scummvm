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

#include "common/endian.h"
#include "common/textconsole.h"

#include "engines/grim/emi/animation.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lformatparser.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

enum {
	kKeywordAnimName = 0,
	kKeywordAnimEntries = 1,
	kKeywordAnimLength = 2,
	kKeywordAnimEntry = 3
};

static const int kNameBufferSize = 256;
static const int kMinTrackRecordSize = 21;
static const int kFloatKeySize = 8;
static const int kVectorKeySize = 16;
static const int kQuaternionKeySize = 20;

class AnimationParser : public LFormatParser {
public:
	explicit AnimationParser(EmiEngine *engine) : LFormatParser(engine) {
		addKeyword("ANIMNAME", kKeywordAnimName, 3);
		addKeyword("ANIMENTRIES", kKeywordAnimEntries, 1);
		addKeyword("ANIMLENGTH", kKeywordAnimLength, 2);
		addKeyword("ANIMENTRY", kKeywordAnimEntry, 23);
	}
};

static bool readInt(LFile *file, int *value) {
	byte bytes[4];

	if (file->read(bytes, sizeof(bytes)) != sizeof(bytes))
		return false;

	*value = READ_LE_INT32(bytes);

	return true;
}

static bool readFloat(LFile *file, float *value) {
	byte bytes[4];

	if (file->read(bytes, sizeof(bytes)) != sizeof(bytes))
		return false;

	*value = READ_LE_FLOAT32(bytes);

	return true;
}

static void writeInt(LFile *file, int value) {
	byte bytes[4];

	WRITE_LE_INT32(bytes, value);
	file->write(bytes, sizeof(bytes));
}

static void writeFloat(LFile *file, float value) {
	byte bytes[4];

	WRITE_LE_FLOAT32(bytes, value);
	file->write(bytes, sizeof(bytes));
}

static bool readTrackHeader(LFile *file, ATrack *track, int dataType, int fileSize, int keySize) {
	track->dataType = dataType;

	if (!readInt(file, &track->priority) || !readInt(file, &track->trackFlags) || !readInt(file, &track->numSamples))
		return false;

	return track->numSamples >= 0 && (fileSize <= 0 || track->numSamples <= fileSize / keySize);
}

static byte *readKeyData(LFile *file, int count, int keySize) {
	uint size = count * keySize;
	byte *raw = new byte[size];

	if (file->read(raw, size) != size) {
		delete[] raw;

		return nullptr;
	}

	return raw;
}

Animation::Animation(EmiEngine *engine) : _engine(engine) {
	length = 0.0f;
	trackCount = 0;
	entryCount = 0;
	name = "untitled";
}

Animation::~Animation() {
	for (int i = 0; i < trackCount; i++) {
		if (entries[i].track) {
			delete entries[i].track;
			entries[i].track = nullptr;
		}
	}

	entries.clear();
	name.clear();
}

bool Animation::failLoad(LFile *file, const char *reason) {
	warning("Animation::loadBinary(): %s", reason);
	_engine->getFileServices()->close(file, true);

	return false;
}

bool Animation::save(LFile *file) {
	AnimationParser parser(_engine);
	int writtenTracks = 0;

	for (int i = 0; i < entryCount; i++) {
		if (!entries[i].track) {
			warning("Animation::save(): entry %d has no track", i);
			continue;
		}

		if (entries[i].track->numSamples > 0)
			writtenTracks++;
	}

	parser.setFile(file);
	parser.writeVersion(0, 3);
	parser.writeEntry(kKeywordAnimName, name.c_str());
	parser.writeEntry(kKeywordAnimEntries, writtenTracks);
	parser.writeEntry(kKeywordAnimLength, length);
	parser.writeComment("TrackName TrackType TrackData");

	for (int i = 0; i < entryCount; i++) {
		ATrack *track = entries[i].track;

		if (track && track->numSamples) {
			parser.writeEntry(kKeywordAnimEntry, entries[i].name.c_str(), track->trackType, track->dataType);
		}
	}

	int trackIndex = 0;

	for (int i = 0; i < entryCount; i++) {
		ATrack *track = entries[i].track;

		if (track && track->numSamples) {
			LString comment = LString::format("Track %d (%s)", trackIndex++, entries[i].name.c_str());

			parser.writeComment(comment.c_str());
			track->save(file);
		}
	}

	parser.writeEndOfSection();

	// The disasm closes the file here
	_engine->getFileServices()->close(file, true);

	return true;
}

bool Animation::restore(LFile *file) {
	AnimationParser parser(_engine);
	char nameBuffer[kNameBufferSize];
	char entryName[kNameBufferSize];
	int count = 0;

	parser.setFile(file);

	if (!parser.readVersion(0, 3))
		return false;

	nameBuffer[0] = 0;
	parser.readEntry(kKeywordAnimName, nameBuffer);
	name = nameBuffer;
	parser.readEntry(kKeywordAnimEntries, &count);
	parser.readEntry(kKeywordAnimLength, &length);

	if (count < 0) {
		warning("Animation::restore(): invalid track count %d", count);

		return false;
	}

	entries.resize(count);
	trackCount = count;
	entryCount = count;

	for (int i = 0; i < entryCount; i++) {
		int trackType = 0;
		int dataType = 0;

		entryName[0] = 0;
		parser.readEntry(kKeywordAnimEntry, entryName, &trackType, &dataType);

		switch (dataType) {
		case ATrack::kDataFloat:
			entries[i].name = entryName;
			entries[i].track = new ATrackKeyframeData<float>(_engine);
			break;
		case ATrack::kDataInt:
			entries[i].name = entryName;
			entries[i].track = new ATrackKeyframeData<int>(_engine);
			break;
		case ATrack::kDataVector:
			entries[i].name = entryName;
			entries[i].track = new ATrackKeyframeData<CVector>(_engine);
			break;
		case ATrack::kDataQuaternion:
			entries[i].name = entryName;
			entries[i].track = new ATrackKeyframeData<CQuaternion>(_engine);
			break;
		default:
			break;
		}
	}

	for (int i = 0; i < entryCount; i++) {
		if (entries[i].track)
			entries[i].track->restore(file);
		else
			warning("Animation::restore(): entry %d has an unknown track type", i);
	}

	return true;
}

bool Animation::loadBinary(const char *path) {
	LFile *file = _engine->getFileServices()->open(path, "rb");

	if (!file)
		return false;

	int fileSize = _engine->getFileServices()->getSize(path);
	char buffer[kNameBufferSize];
	int nameLength;
	int count;

	if (!readInt(file, &nameLength) || nameLength < 1 || nameLength > kNameBufferSize)
		return failLoad(file, "invalid animation name");

	memset(buffer, 0, sizeof(buffer));

	if (file->read(buffer, nameLength) != (uint)nameLength)
		return failLoad(file, "unexpected end of file");

	buffer[kNameBufferSize - 1] = 0;
	name = buffer;

	if (!readFloat(file, &length) || !readInt(file, &count))
		return failLoad(file, "unexpected end of file");

	if (count < 0 || (fileSize > 0 && count > fileSize / kMinTrackRecordSize))
		return failLoad(file, "invalid track count");

	entries.resize(count);
	trackCount = count;
	entryCount = count;

	for (int i = 0; i < entryCount; i++) {
		int trackNameLength;
		int dataType;

		if (!readInt(file, &trackNameLength) || trackNameLength < 1 || trackNameLength > kNameBufferSize)
			return failLoad(file, "invalid track name");

		memset(buffer, 0, sizeof(buffer));

		if (file->read(buffer, trackNameLength) != (uint)trackNameLength)
			return failLoad(file, "unexpected end of file");

		buffer[kNameBufferSize - 1] = 0;
		entries[i].name = buffer;

		if (!readInt(file, &dataType))
			return failLoad(file, "unexpected end of file");

		switch (dataType) {
		case ATrack::kDataFloat: {
			ATrackKeyframeData<float> *track = new ATrackKeyframeData<float>(_engine);

			entries[i].track = track;

			if (!readTrackHeader(file, track, dataType, fileSize, kFloatKeySize))
				return failLoad(file, "invalid track header");

			track->allocateKeys(track->numSamples);

			if (track->numSamples > 0) {
				byte *raw = track->keys ? readKeyData(file, track->numSamples, kFloatKeySize) : nullptr;

				if (!raw)
					return failLoad(file, "unexpected end of file");

				for (int j = 0; j < track->numSamples; j++) {
					track->keys[j].value = READ_LE_FLOAT32(raw + j * kFloatKeySize);
					track->keys[j].time = READ_LE_FLOAT32(raw + j * kFloatKeySize + 4);
				}

				delete[] raw;
			}

			break;
		}
		case ATrack::kDataInt: {
			ATrackKeyframeData<int> *track = new ATrackKeyframeData<int>(_engine);

			entries[i].track = track;

			if (!readTrackHeader(file, track, dataType, fileSize, kFloatKeySize))
				return failLoad(file, "invalid track header");

			track->allocateKeys(track->numSamples);

			if (track->numSamples > 0) {
				byte *raw = track->keys ? readKeyData(file, track->numSamples, kFloatKeySize) : nullptr;

				if (!raw)
					return failLoad(file, "unexpected end of file");

				for (int j = 0; j < track->numSamples; j++) {
					track->keys[j].setInt(READ_LE_INT32(raw + j * kFloatKeySize));
					track->keys[j].time = READ_LE_FLOAT32(raw + j * kFloatKeySize + 4);
				}

				delete[] raw;
			}

			break;
		}
		case ATrack::kDataVector: {
			ATrackKeyframeData<CVector> *track = new ATrackKeyframeData<CVector>(_engine);

			entries[i].track = track;

			if (!readTrackHeader(file, track, dataType, fileSize, kVectorKeySize))
				return failLoad(file, "invalid track header");

			track->allocateKeys(track->numSamples);

			if (track->numSamples > 0) {
				byte *raw = track->keys ? readKeyData(file, track->numSamples, kVectorKeySize) : nullptr;

				if (!raw)
					return failLoad(file, "unexpected end of file");

				for (int j = 0; j < track->numSamples; j++) {
					track->keys[j].v.x = READ_LE_FLOAT32(raw + j * kVectorKeySize);
					track->keys[j].v.y = READ_LE_FLOAT32(raw + j * kVectorKeySize + 4);
					track->keys[j].v.z = READ_LE_FLOAT32(raw + j * kVectorKeySize + 8);
					track->keys[j].time = READ_LE_FLOAT32(raw + j * kVectorKeySize + 12);
				}

				delete[] raw;
			}

			break;
		}
		case ATrack::kDataQuaternion: {
			ATrackKeyframeData<CQuaternion> *track = new ATrackKeyframeData<CQuaternion>(_engine);

			entries[i].track = track;

			if (!readTrackHeader(file, track, dataType, fileSize, kQuaternionKeySize))
				return failLoad(file, "invalid track header");

			track->allocateKeys(track->numSamples);

			if (track->numSamples > 0) {
				byte *raw = track->keys ? readKeyData(file, track->numSamples, kQuaternionKeySize) : nullptr;

				if (!raw)
					return failLoad(file, "unexpected end of file");

				for (int j = 0; j < track->numSamples; j++) {
					track->keys[j].q.x = READ_LE_FLOAT32(raw + j * kQuaternionKeySize);
					track->keys[j].q.y = READ_LE_FLOAT32(raw + j * kQuaternionKeySize + 4);
					track->keys[j].q.z = READ_LE_FLOAT32(raw + j * kQuaternionKeySize + 8);
					track->keys[j].q.w = READ_LE_FLOAT32(raw + j * kQuaternionKeySize + 12);
					track->keys[j].time = READ_LE_FLOAT32(raw + j * kQuaternionKeySize + 16);
				}

				delete[] raw;
			}

			break;
		}
		default:
			break;
		}
	}

	_engine->getFileServices()->close(file, true);

	return true;
}

bool Animation::saveBinary(const char *path) {
	LFile *file = _engine->getFileServices()->open(path, "wb");

	if (!file)
		return false;

	int writtenTracks = 0;

	for (int i = 0; i < entryCount; i++) {
		if (entries[i].track && entries[i].track->numSamples > 0)
			writtenTracks++;
	}

	writeInt(file, (int)name.size() + 1);
	file->write(name.c_str(), name.size() + 1);
	writeFloat(file, length);
	writeInt(file, writtenTracks);

	for (int i = 0; i < entryCount; i++) {
		ATrack *track = entries[i].track;

		if (!track || track->numSamples <= 0)
			continue;

		writeInt(file, (int)entries[i].name.size() + 1);
		file->write(entries[i].name.c_str(), entries[i].name.size() + 1);
		writeInt(file, track->dataType);
		writeInt(file, track->priority);
		writeInt(file, track->trackFlags);
		writeInt(file, track->numSamples);

		switch (track->dataType) {
		case ATrack::kDataFloat: {
			ATrackKeyframeData<float> *data = dynamic_cast<ATrackKeyframeData<float> *>(track);

			if (data) {
				for (int j = 0; j < track->numSamples; j++) {
					writeFloat(file, data->keys[j].value);
					writeFloat(file, data->keys[j].time);
				}
			}

			break;
		}
		case ATrack::kDataInt: {
			ATrackKeyframeData<int> *data = dynamic_cast<ATrackKeyframeData<int> *>(track);

			if (data) {
				for (int j = 0; j < track->numSamples; j++) {
					writeInt(file, data->keys[j].getInt());
					writeFloat(file, data->keys[j].time);
				}
			}

			break;
		}
		case ATrack::kDataVector: {
			ATrackKeyframeData<CVector> *data = dynamic_cast<ATrackKeyframeData<CVector> *>(track);

			if (data) {
				for (int j = 0; j < track->numSamples; j++) {
					writeFloat(file, data->keys[j].v.x);
					writeFloat(file, data->keys[j].v.y);
					writeFloat(file, data->keys[j].v.z);
					writeFloat(file, data->keys[j].time);
				}
			}

			break;
		}
		case ATrack::kDataQuaternion: {
			ATrackKeyframeData<CQuaternion> *data = dynamic_cast<ATrackKeyframeData<CQuaternion> *>(track);

			if (data) {
				for (int j = 0; j < track->numSamples; j++) {
					writeFloat(file, data->keys[j].q.x);
					writeFloat(file, data->keys[j].q.y);
					writeFloat(file, data->keys[j].q.z);
					writeFloat(file, data->keys[j].q.w);
					writeFloat(file, data->keys[j].time);
				}
			}

			break;
		}
		default:
			break;
		}
	}

	_engine->getFileServices()->close(file, true);

	return true;
}

} // End of namespace Grim
