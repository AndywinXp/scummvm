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

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/files/zfile.h"

namespace Grim {

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
		if (_file->_writeStream)
			_file->_writeStream->finalize();
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

ZFileServices::ZFileServices(EmiEngine *engine) : LFileServices(engine) {
}

void ZFileServices::checkHeader(GzStream *s) {
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

int ZFileServices::gzDestroy(GzStream *s) {
	int result = 0;

	if (!s)
		return -2;

	delete s->writeStream;

	if (s->readStream != s->rawStream)
		delete s->readStream;

	delete s->rawStream;

	if (s->file)
		_engine->getFileServices()->close(s->file, true);

	if (s->zErr < 0)
		result = s->zErr;

	delete s;

	return result;
}

char ZFileServices::parseMode(const char *mode, Common::String &openMode) {
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

GzStream *ZFileServices::gzOpenFile(LFile *file, const char *mode) {
	Common::String openMode;
	char gzMode = parseMode(mode, openMode);

	if (!gzMode)
		return nullptr;

	GzStream *s = new GzStream();

	s->path = file->_fileName;
	s->mode = gzMode;
	s->file = file;

	if (s->mode == 'w') {
		s->writeStream = Common::wrapCompressedWriteStream(new LFileWriteStream(s->file));

		return s;
	}

	checkHeader(s);

	return s;
}

GzStream *ZFileServices::gzOpen(const char *path, const char *mode) {
	Common::String openMode;

	if (!path || !mode)
		return nullptr;

	LFileServices *fileServices = _engine->getFileServices();

	if (!parseMode(mode, openMode) || !fileServices)
		return nullptr;

	LFile *file = fileServices->open(path, openMode.c_str());
	if (!file)
		return nullptr;

	return gzOpenFile(file, mode);
}

GzStream *ZFileServices::gzopen(const char *path, const char *mode) {
	return gzOpen(path, mode);
}

int ZFileServices::gzRead(GzStream *file, void *buffer, uint size) {
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

int ZFileServices::gzWrite(GzStream *file, const void *buffer, uint size) {
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

int ZFileServices::gzRewind(GzStream *file) {
	if (!file || file->mode != 'r')
		return -1;

	file->zErr = 0;
	file->zEof = false;
	file->totalIn = 0;
	file->totalOut = 0;

	return file->readStream->seek(0, SEEK_SET) ? 0 : -1;
}

int ZFileServices::gzSeek(GzStream *file, int offset, int whence) {
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
			int written = gzWrite(file, skipBuffer, chunk);

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
		if (gzRewind(file) < 0)
			return -1;
	} else {
		target -= file->totalOut;
	}

	if (target == 0)
		return file->totalOut;

	while (target > 0) {
		uint chunk = target < 0x4000 ? target : 0x4000;
		int count = gzRead(file, skipBuffer, chunk);

		if (count <= 0)
			return -1;

		target -= count;
	}

	return file->totalOut;
}

int ZFileServices::gzClose(GzStream *file) {
	if (!file)
		return -2;

	return gzDestroy(file);
}

ZFile::ZFile(ZFileServices *services) : LFile(services->_engine), _services(services) {
	_gzFile = nullptr;
}

ZFile::~ZFile() {
	if (_gzFile) {
		warning("ZFile::~ZFile(): file not closed");
		_services->gzClose(_gzFile);
		_gzFile = nullptr;
	}
}

uint ZFile::read(void *buffer, uint size) {
	return ZFileServices::gzRead(_gzFile, buffer, size);
}

int ZFile::seek(int offset, int whence) {
	return ZFileServices::gzSeek(_gzFile, offset, whence);
}

uint ZFile::writeCompressed(const void *buffer, uint size) {
	return ZFileServices::gzWrite(_gzFile, buffer, size);
}

ZFile *ZFileServices::open(const char *path, const char *mode) {
	GzStream *stream = gzopen(path, mode);

	if (!stream) {
		LString resolved;

		if (_engine->getFileServices()->resolvePath(resolved, path))
			stream = gzopen(resolved.c_str(), mode);

		if (!stream)
			return nullptr;
	}

	ZFile *file = new ZFile(this);
	file->_gzFile = stream;

	return file;
}

ZFile *ZFileServices::createZFile(LFile *file, const char *mode) {
	GzStream *stream = gzOpenFile(file, mode);
	if (!stream)
		return nullptr;

	ZFile *zFile = new ZFile(this);
	zFile->_gzFile = stream;

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

	gzClose(file->_gzFile);
	file->_gzFile = nullptr;
	delete file;
}

} // End of namespace Grim
