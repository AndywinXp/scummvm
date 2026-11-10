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

#include "common/compression/deflate.h"
#include "common/textconsole.h"

#include "engines/grim/emi/zfile.h"

namespace Grim {

ZFileServices *gpZFileServices = nullptr;

namespace {

class LFileReadStream : public Common::SeekableReadStream {
public:
	LFileReadStream(LFile *file) : _file(file), _eos(false) {}

	uint32 read(void *dataPtr, uint32 dataSize) override {
		uint32 count = _file->read(dataPtr, dataSize);

		if (count < dataSize)
			_eos = true;

		return count;
	}

	bool eos() const override {
		return _eos;
	}

	int64 pos() const override {
		return _file->tell();
	}

	int64 size() const override {
		int position = _file->tell();

		_file->seek(0, 2);
		int total = _file->tell();
		_file->seek(position, 0);

		return total;
	}

	bool seek(int64 offset, int whence = SEEK_SET) override {
		_eos = false;
		_file->seek((int)offset, whence);

		return true;
	}

private:
	LFile *_file;
	bool _eos;
};

class LFileWriteStream : public Common::WriteStream {
public:
	LFileWriteStream(LFile *file) : _file(file), _pos(0) {}

	uint32 write(const void *dataPtr, uint32 dataSize) override {
		uint32 count = _file->write(dataPtr, dataSize);

		_pos += count;

		return count;
	}

	int64 pos() const override {
		return _pos;
	}

	void finalize() override {
		if (_file->writeStream)
			_file->writeStream->finalize();
	}

private:
	LFile *_file;
	int64 _pos;
};

} // End of anonymous namespace

GzStream::GzStream() {
	file = nullptr;
	rawStream = nullptr;
	readStream = nullptr;
	writeStream = nullptr;
	mode = 0;
	transparent = false;
	zErr = 0;
	zEof = false;
	totalIn = 0;
	totalOut = 0;
}

static void checkHeader(GzStream *s) {
	LFileReadStream *raw = new LFileReadStream(s->file);

	s->rawStream = raw;

	int64 total = raw->size();

	if (total < 2) {
		s->readStream = raw;
		s->transparent = true;
		s->zErr = total == 0 ? 1 : 0;

		return;
	}

	Common::SeekableReadStream *wrapped = Common::wrapCompressedReadStream(raw, DisposeAfterUse::NO);
	if (!wrapped)
		wrapped = raw;

	s->readStream = wrapped;
	s->transparent = wrapped == raw;
	s->zErr = 0;
}

static int destroy(GzStream *s) {
	int result = 0;

	if (!s)
		return -2;

	delete s->writeStream;

	if (s->readStream != s->rawStream)
		delete s->readStream;

	delete s->rawStream;

	if (s->file)
		gpFileServices->close(s->file, true);

	if (s->zErr < 0)
		result = s->zErr;

	delete s;

	return result;
}

static char parseMode(const char *mode, Common::String &openMode) {
	char result = 0;

	for (const char *p = mode; *p; p++) {
		if (*p == 'r')
			result = 'r';

		if (*p == 'w' || *p == 'a')
			result = 'w';

		// Compression level and strategy are not used
		if ((*p < '0' || *p > '9') && *p != 'f' && *p != 'h')
			openMode += *p;
	}

	return result;
}

static GzStream *gzOpenFile(LFile *file, const char *mode) {
	Common::String openMode;
	char gzMode = parseMode(mode, openMode);

	if (!gzMode)
		return nullptr;

	GzStream *s = new GzStream();

	s->path = file->fileName;
	s->mode = gzMode;
	s->file = file;

	if (s->mode == 'w') {
		s->writeStream = Common::wrapCompressedWriteStream(new LFileWriteStream(s->file));

		return s;
	}

	checkHeader(s);

	return s;
}

static GzStream *gzOpen(const char *path, const char *mode) {
	Common::String openMode;

	if (!path || !mode)
		return nullptr;

	if (!parseMode(mode, openMode) || !gpFileServices)
		return nullptr;

	LFile *file = gpFileServices->open(path, openMode.c_str());
	if (!file)
		return nullptr;

	return gzOpenFile(file, mode);
}

static GzStream *gzopen(const char *path, const char *mode) {
	return gzOpen(path, mode);
}

static int gzread(GzStream *file, void *buffer, uint size) {
	if (!file || file->mode != 'r')
		return -2;

	if (file->zErr == -3 || file->zErr == -1)
		return -1;

	if (file->zErr == 1)
		return 0;

	if (size == 0)
		return 0;

	uint count = file->readStream->read(buffer, size);

	if (!file->transparent) {
		if (file->readStream->err())
			file->zErr = -3;
		else if (count < size)
			file->zErr = 1;
	} else if (count == 0) {
		file->zEof = true;
	}

	file->totalOut += count;

	return count;
}

static int gzwrite(GzStream *file, const void *buffer, uint size) {
	if (!file || file->mode != 'w')
		return -2;

	if (size == 0)
		return 0;

	uint count = file->writeStream->write(buffer, size);

	if (count < size)
		file->zErr = -1;

	file->totalIn += count;

	return count;
}

static int gzrewind(GzStream *file) {
	if (!file || file->mode != 'r')
		return -1;

	file->zErr = 0;
	file->zEof = false;
	file->totalIn = 0;
	file->totalOut = 0;

	return file->readStream->seek(0, SEEK_SET) ? 0 : -1;
}

static int gzseek(GzStream *file, int offset, int whence) {
	byte skipBuffer[0x4000];

	if (!file)
		return -1;

	if (whence == 2)
		return -1;

	if (file->zErr == -1 || file->zErr == -3)
		return -1;

	if (file->mode == 'w') {
		int toWrite = offset;

		if (whence == 0)
			toWrite = offset - file->totalIn;

		if (toWrite < 0)
			return -1;

		memset(skipBuffer, 0, sizeof(skipBuffer));

		while (toWrite > 0) {
			uint chunk = toWrite < 0x4000 ? toWrite : 0x4000;
			int written = gzwrite(file, skipBuffer, chunk);

			if (!written)
				return -1;

			toWrite -= written;
		}

		return file->totalIn;
	}

	int target = offset;

	if (whence == 1)
		target = file->totalOut + offset;

	if (target < 0)
		return -1;

	if (file->transparent) {
		if (!file->readStream->seek(target, SEEK_SET))
			return -1;

		file->totalOut = target;
		file->totalIn = target;

		return target;
	}

	if ((uint)target < file->totalOut) {
		if (gzrewind(file) < 0)
			return -1;
	} else {
		target -= file->totalOut;
	}

	if (target == 0)
		return file->totalOut;

	while (target > 0) {
		uint chunk = target < 0x4000 ? target : 0x4000;
		int count = gzread(file, skipBuffer, chunk);

		if (count <= 0)
			return -1;

		target -= count;
	}

	return file->totalOut;
}

static int gzclose(GzStream *file) {
	if (!file)
		return -2;

	return destroy(file);
}

ZFile::ZFile() {
	gzFile = nullptr;
}

ZFile::~ZFile() {
	if (gzFile) {
		warning("ZFile::~ZFile(): file not closed");
		gzclose(gzFile);
		gzFile = nullptr;
	}
}

uint ZFile::read(void *buffer, uint size) {
	return gzread(gzFile, buffer, size);
}

int ZFile::seek(int offset, int whence) {
	return gzseek(gzFile, offset, whence);
}

uint ZFile::writeCompressed(const void *buffer, uint size) {
	return gzwrite(gzFile, buffer, size);
}

ZFile *ZFileServices::open(const char *path, const char *mode) {
	GzStream *stream = gzopen(path, mode);

	if (!stream) {
		LString resolved;

		if (gpFileServices->resolvePath(resolved, path))
			stream = gzopen(resolved.c_str(), mode);

		if (!stream)
			return nullptr;
	}

	ZFile *file = new ZFile();
	file->gzFile = stream;

	return file;
}

ZFile *ZFileServices::createZFile(LFile *file, const char *mode) {
	GzStream *stream = gzOpenFile(file, mode);
	if (!stream)
		return nullptr;

	ZFile *zFile = new ZFile();
	zFile->gzFile = stream;

	return zFile;
}

LFile *ZFileServices::openExtended(const char *path, const char *mode, int unused1, int unused2) {
	return open(path, mode);
}

void ZFileServices::closeZFile(ZFile *file) {
	if (!file) {
		warning("ZFileServices::closeZFile(): null file");

		return;
	}

	gzclose(file->gzFile);
	file->gzFile = nullptr;
	delete file;
}

ZFileServices *getZFileServices() {
	if (!gpZFileServices)
		gpZFileServices = new ZFileServices();

	return gpZFileServices;
}

} // End of namespace Grim
