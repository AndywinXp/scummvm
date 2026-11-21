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
#include "common/file.h"
#include "common/path.h"
#include "common/textconsole.h"

#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/utils.h"
#include "topgun/topgun.h"

namespace Topgun {

uint32 AudioLibrary::findWaveDataChunkSize(Common::SeekableReadStream *file) {
	for (;;) {
		byte header[8];

		if (file->read(header, sizeof(header)) != sizeof(header))
			return 0;

		uint32 size = READ_LE_UINT32(header + 4);

		if (READ_BE_UINT32(header) == MKTAG('d', 'a', 't', 'a'))
			return size;

		file->seek(size + (size & 1), SEEK_CUR);
	}
}

Common::File *AudioLibrary::openWaveRiffFile(const Common::String &fileName) {
	Common::File *file = new Common::File();

	if (!file->open(Common::Path(fileName, '/'))) {
		delete file;
		return nullptr;
	}

	for (;;) {
		byte header[8];

		if (file->read(header, sizeof(header)) != sizeof(header)) {
			delete file;
			return nullptr;
		}

		uint32 size = READ_LE_UINT32(header + 4);
		int64 dataStart = file->pos();

		if (READ_BE_UINT32(header) == MKTAG('R', 'I', 'F', 'F')) {
			byte formType[4];

			if (file->read(formType, sizeof(formType)) != sizeof(formType)) {
				delete file;
				return nullptr;
			}

			if (READ_BE_UINT32(formType) == MKTAG('W', 'A', 'V', 'E'))
				return file;
		}

		file->seek(dataStart + size + (size & 1), SEEK_SET);
	}
}

bool AudioLibrary::readWaveFormatChunk(Common::SeekableReadStream *file, byte *format) {
	for (;;) {
		byte header[8];

		if (file->read(header, sizeof(header)) != sizeof(header))
			return false;

		uint32 size = READ_LE_UINT32(header + 4);
		int64 dataStart = file->pos();

		if (READ_BE_UINT32(header) == MKTAG('f', 'm', 't', ' ')) {
			if (size != TG_AUDIO_FORMAT_CHUNK_SIZE)
				return false;

			if (file->read(format, TG_AUDIO_FORMAT_CHUNK_SIZE) != TG_AUDIO_FORMAT_CHUNK_SIZE)
				return false;

			if (READ_LE_UINT16(format) != 1)
				return false;

			file->seek(dataStart + size + (size & 1), SEEK_SET);
			return true;
		}

		file->seek(dataStart + size + (size & 1), SEEK_SET);
	}
}

uint16 AudioLibrary::wavePlayFile(const Common::String &fileName, uint16 flags) {
	byte header[TG_AUDIO_HEADER_CHUNK_SIZE];

	memset(header, 0, sizeof(header));

	Common::File *file = openWaveRiffFile(fileName);
	if (!file)
		return 0;

	if (!readWaveFormatChunk(file, header)) {
		delete file;
		return 0;
	}

	uint32 size = findWaveDataChunkSize(file);
	if (size == 0) {
		delete file;
		return 0;
	}

	WRITE_LE_UINT32(header + TG_AUDIO_FORMAT_CHUNK_SIZE, size);

	SoundCreateInfo info;

	info.fileStream = file;
	info.header = header;

	return (uint16)audioCommand(kCmdWaveCreate, flags, 0, &info);
}

byte *AudioLibrary::waveLoadFile(const Common::String &fileName, WaveFormat *outFormat, uint32 *outSize) {
	byte *buffer = nullptr;

	Common::File *file = openWaveRiffFile(fileName);
	if (!file)
		return nullptr;

	byte formatBytes[TG_AUDIO_FORMAT_CHUNK_SIZE];

	memset(formatBytes, 0, sizeof(formatBytes));

	bool formatRead = readWaveFormatChunk(file, formatBytes);
	readWaveFormat(formatBytes, *outFormat);

	if (formatRead) {
		uint32 size = findWaveDataChunkSize(file);

		if (size != 0) {
			UtilsLibrary *utilsLibrary = _engine->utilsLibrary();

			buffer = utilsLibrary->tgMalloc(size, 0);

			if (buffer) {
				if (file->read(buffer, size) != size)
					buffer = utilsLibrary->tgFree(buffer);
				else
					*outSize = size;
			}
		}
	}

	delete file;
	return buffer;
}

bool AudioLibrary::waveSaveFile(const Common::String &fileName, const WaveFormat *format, const byte *data, uint32 size) {
	Common::DumpFile file;

	if (!file.open(Common::Path(fileName, '/')))
		return false;

	byte formatBytes[TG_AUDIO_FORMAT_CHUNK_SIZE];

	writeWaveFormat(formatBytes, *format);

	file.writeUint32BE(MKTAG('R', 'I', 'F', 'F'));
	file.writeUint32LE(4 + 8 + TG_AUDIO_FORMAT_CHUNK_SIZE + 8 + size);
	file.writeUint32BE(MKTAG('W', 'A', 'V', 'E'));
	file.writeUint32BE(MKTAG('f', 'm', 't', ' '));
	file.writeUint32LE(TG_AUDIO_FORMAT_CHUNK_SIZE);
	file.write(formatBytes, TG_AUDIO_FORMAT_CHUNK_SIZE);
	file.writeUint32BE(MKTAG('d', 'a', 't', 'a'));
	file.writeUint32LE(size);
	file.write(data, size);

	bool failed = file.err();

	file.close();

	if (failed)
		warning("AudioLibrary::waveSaveFile(): cannot write the whole file");

	return !failed;
}

} // End of namespace Topgun
