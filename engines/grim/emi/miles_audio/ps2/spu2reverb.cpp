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

#include "common/util.h"

#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/miles_audio/ps2/spu2reverb.h"

namespace Grim {

// Effect area sizes in units of 8 bytes, as stored in libsd
static const uint32 kAreaSizes[EMISpu2Reverb::kModeCount] = {
	0x0002, 0x04D8, 0x03E8, 0x0908, 0x0DFC, 0x15BC, 0x1ED8, 0x3008, 0x3008, 0x0780
};

// Register presets of libsd, in the order of the SPU reverb registers
static const uint16 kPresets[EMISpu2Reverb::kModeCount][EMISpu2Reverb::kRegisterCount] = {
	{
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0001, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0001, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
	},
	{
		0x007D, 0x005B, 0x6D80, 0x54B8, 0xBED0, 0x0000, 0x0000, 0xBA80,
		0x5800, 0x5300, 0x04D6, 0x0333, 0x03F0, 0x0227, 0x0374, 0x01EF,
		0x0336, 0x01B7, 0x0335, 0x01B6, 0x0334, 0x01B5, 0x0334, 0x01B5,
		0x0334, 0x01B5, 0x01B4, 0x0136, 0x00B8, 0x005C, 0x8000, 0x8000
	},
	{
		0x0033, 0x0025, 0x70F0, 0x4FA8, 0xBCE0, 0x4410, 0xC0F0, 0x9C00,
		0x5280, 0x4EC0, 0x03E4, 0x031B, 0x03A4, 0x02AF, 0x0372, 0x0266,
		0x031C, 0x025D, 0x025C, 0x018E, 0x022F, 0x0135, 0x01D2, 0x00B7,
		0x018F, 0x00B5, 0x00B4, 0x0080, 0x004C, 0x0026, 0x8000, 0x8000
	},
	{
		0x00B1, 0x007F, 0x70F0, 0x4FA8, 0xBCE0, 0x4510, 0xBEF0, 0xB4C0,
		0x5280, 0x4EC0, 0x0904, 0x076B, 0x0824, 0x065F, 0x07A2, 0x0616,
		0x076C, 0x05ED, 0x05EC, 0x042E, 0x050F, 0x0305, 0x0462, 0x02B7,
		0x042F, 0x0265, 0x0264, 0x01B2, 0x0100, 0x0080, 0x8000, 0x8000
	},
	{
		0x00E3, 0x00A9, 0x6F60, 0x4FA8, 0xBCE0, 0x4510, 0xBEF0, 0xA680,
		0x5680, 0x52C0, 0x0DFB, 0x0B58, 0x0D09, 0x0A3C, 0x0BD9, 0x0973,
		0x0B59, 0x08DA, 0x08D9, 0x05E9, 0x07EC, 0x04B0, 0x06EF, 0x03D2,
		0x05EA, 0x031D, 0x031C, 0x0238, 0x0154, 0x00AA, 0x8000, 0x8000
	},
	{
		0x01A5, 0x0139, 0x6000, 0x5000, 0x4C00, 0xB800, 0xBC00, 0xC000,
		0x6000, 0x5C00, 0x15BA, 0x11BB, 0x14C2, 0x10BD, 0x11BC, 0x0DC1,
		0x11C0, 0x0DC3, 0x0DC0, 0x09C1, 0x0BC4, 0x07C1, 0x0A00, 0x06CD,
		0x09C2, 0x05C1, 0x05C0, 0x041A, 0x0274, 0x013A, 0x8000, 0x8000
	},
	{
		0x033D, 0x0231, 0x7E00, 0x5000, 0xB400, 0xB000, 0x4C00, 0xB000,
		0x6000, 0x5400, 0x1ED6, 0x1A31, 0x1D14, 0x183B, 0x1BC2, 0x16B2,
		0x1A32, 0x15EF, 0x15EE, 0x1055, 0x1334, 0x0F2D, 0x11F6, 0x0C5D,
		0x1056, 0x0AE1, 0x0AE0, 0x07A2, 0x0464, 0x0232, 0x8000, 0x8000
	},
	{
		0x0003, 0x0003, 0x7FFF, 0x7FFF, 0x0000, 0x0000, 0x0000, 0x8100,
		0x0000, 0x0000, 0x1FFD, 0x0FFD, 0x1009, 0x0009, 0x0000, 0x0000,
		0x1009, 0x0009, 0x1FFF, 0x1FFF, 0x1FFE, 0x1FFE, 0x1FFE, 0x1FFE,
		0x1FFE, 0x1FFE, 0x1008, 0x1004, 0x0008, 0x0004, 0x8000, 0x8000
	},
	{
		0x0003, 0x0003, 0x7FFF, 0x7FFF, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x1FFD, 0x0FFD, 0x1009, 0x0009, 0x0000, 0x0000,
		0x1009, 0x0009, 0x1FFF, 0x1FFF, 0x1FFE, 0x1FFE, 0x1FFE, 0x1FFE,
		0x1FFE, 0x1FFE, 0x1008, 0x1004, 0x0008, 0x0004, 0x8000, 0x8000
	},
	{
		0x0017, 0x0013, 0x70F0, 0x4FA8, 0xBCE0, 0x4510, 0xBEF0, 0x8500,
		0x5F80, 0x54C0, 0x0371, 0x02AF, 0x02E5, 0x01DF, 0x02B0, 0x01D7,
		0x0358, 0x026A, 0x01D6, 0x011E, 0x012D, 0x00B1, 0x011F, 0x0059,
		0x01A0, 0x00E3, 0x0058, 0x0040, 0x0028, 0x0014, 0x8000, 0x8000
	}
};

// Half-band FIR used by the SPU2 to resample the reverb between 48 and 24 kHz
static const int16 kResampleCoefficients[EMISpu2Reverb::kResampleTaps] = {
	-1, 0, 2, 0, -10, 0, 35, 0, -103, 0, 266, 0, -616, 0, 1332, 0, -2960, 0, 10246, 16384,
	10246, 0, -2960, 0, 1332, 0, -616, 0, 266, 0, -103, 0, 35, 0, -10, 0, 2, 0, -1
};

static const uint32 kInitialEffectEnd = 0xEFFFF;
static const uint32 kEffectEndShift = 17;
static const int kWallVolumeStep = 258;

EMISpu2Reverb::EMISpu2Reverb() {
	_ram = new int16[kRamSize];
	memset(_ram, 0, kRamSize * sizeof(int16));

	_mode = 0;
	_effectEnabled = false;
	_effectVolumeLeft = 0;
	_effectVolumeRight = 0;
	// The effect produces nothing until the first call to setEffectAttr() moves the start address
	_effectEnd = kInitialEffectEnd;
	_effectStart = _effectEnd;

	memset(_addresses, 0, sizeof(_addresses));
	memset(_volumes, 0, sizeof(_volumes));

	_cycles = 0;
	_bufferPosition = 0;
	memset(_downBuffer, 0, sizeof(_downBuffer));
	memset(_upBuffer, 0, sizeof(_upBuffer));
}

EMISpu2Reverb::~EMISpu2Reverb() {
	delete[] _ram;
	_ram = nullptr;
}

void *EMISpu2Reverb::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void EMISpu2Reverb::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

int EMISpu2Reverb::setEffectAttr(int mode, int depthLeft, int depthRight, int delay, int feedback) {
	bool clear = false;
	int oldMode = 0;

	if (mode & kModeClear) {
		mode &= ~kModeClear;
		clear = true;
		oldMode = _mode;
	}

	if ((uint)mode >= kModeCount)
		return -1;

	_mode = mode;

	uint32 start = getEffectEndAddress() - (kAreaSizes[mode] * 8 - 2);
	uint16 registers[kRegisterCount];

	memcpy(registers, kPresets[mode], sizeof(registers));

	if (mode == kModeEcho || mode == kModeDelay) {
		int step = delay + 1;

		registers[kRegSameLeftDst] = (uint16)(step * 64 - registers[kRegApf1Size]);
		registers[kRegSameRightDst] = (uint16)(step * 32 - registers[kRegApf2Size]);
		registers[kRegSameLeftSrc] = (uint16)(registers[kRegSameRightSrc] + step * 32);
		registers[kRegComb1LeftSrc] = (uint16)(registers[kRegComb1RightSrc] + step * 32);
		registers[kRegApf1RightDst] = (uint16)(registers[kRegApf2RightDst] + step * 32);
		registers[kRegApf1LeftDst] = (uint16)(registers[kRegApf2LeftDst] + step * 32);
		registers[kRegWallVolume] = (uint16)(feedback * kWallVolumeStep);
	}

	bool wasEnabled = _effectEnabled;

	if (wasEnabled)
		_effectEnabled = false;

	if (wasEnabled && clear)
		clearEffectWorkArea(oldMode);

	writeRegisters(registers);

	_effectVolumeLeft = (int16)depthLeft;
	_effectVolumeRight = (int16)depthRight;

	_effectStart = start >> 1;

	if (clear)
		clearEffectWorkArea(mode);

	if (wasEnabled)
		_effectEnabled = true;

	return 0;
}

void EMISpu2Reverb::setEffectEnable(bool enabled) {
	_effectEnabled = enabled;
}

void EMISpu2Reverb::setEffectVolume(int left, int right) {
	_effectVolumeLeft = (int16)left;
	_effectVolumeRight = (int16)right;
}

void EMISpu2Reverb::setEffectEndAddress(uint32 address) {
	_effectEnd = ((address >> kEffectEndShift) << 16) | (_effectEnd & 0xFFFF);
}

uint32 EMISpu2Reverb::getEffectEndAddress() const {
	return ((_effectEnd >> 16) << kEffectEndShift) | 0x1FFFF;
}

void EMISpu2Reverb::clearEffectWorkArea(int mode) {
	if (mode == 0)
		return;

	uint32 end = getEffectEndAddress() >> 1;
	uint32 start = (getEffectEndAddress() - (kAreaSizes[mode] * 8 - 2)) >> 1;

	for (uint32 address = start; address <= end; address++)
		_ram[address & (kRamSize - 1)] = 0;
}

void EMISpu2Reverb::writeRegisters(const uint16 *registers) {
	for (int i = 0; i < kRegisterCount; i++) {
		_volumes[i] = (int16)registers[i];
		_addresses[i] = (uint32)registers[i] << 2;
	}
}

void EMISpu2Reverb::process(int32 inLeft, int32 inRight, int32 *outLeft, int32 *outRight) {
	_cycles++;

	*outLeft = 0;
	*outRight = 0;

	if (_effectStart >= _effectEnd)
		return;

	inLeft = clampMix(inLeft);
	inRight = clampMix(inRight);

	_downBuffer[0][_bufferPosition] = (int16)inLeft;
	_downBuffer[1][_bufferPosition] = (int16)inRight;
	_downBuffer[0][_bufferPosition | kResampleBufferSize] = (int16)inLeft;
	_downBuffer[1][_bufferPosition | kResampleBufferSize] = (int16)inRight;

	// The left side runs on even cycles and the right side on odd cycles
	int right = _cycles & 1;

	uint32 sameSrc = getIndex(right ? _addresses[kRegSameRightSrc] : _addresses[kRegSameLeftSrc]);
	uint32 sameDst = getIndex(right ? _addresses[kRegSameRightDst] : _addresses[kRegSameLeftDst]);
	uint32 samePrevious = getIndex(right ? _addresses[kRegSameRightDst] - 1 : _addresses[kRegSameLeftDst] - 1);
	uint32 diffSrc = getIndex(right ? _addresses[kRegDiffLeftSrc] : _addresses[kRegDiffRightSrc]);
	uint32 diffDst = getIndex(right ? _addresses[kRegDiffRightDst] : _addresses[kRegDiffLeftDst]);
	uint32 diffPrevious = getIndex(right ? _addresses[kRegDiffRightDst] - 1 : _addresses[kRegDiffLeftDst] - 1);
	uint32 comb1Src = getIndex(right ? _addresses[kRegComb1RightSrc] : _addresses[kRegComb1LeftSrc]);
	uint32 comb2Src = getIndex(right ? _addresses[kRegComb2RightSrc] : _addresses[kRegComb2LeftSrc]);
	uint32 comb3Src = getIndex(right ? _addresses[kRegComb3RightSrc] : _addresses[kRegComb3LeftSrc]);
	uint32 comb4Src = getIndex(right ? _addresses[kRegComb4RightSrc] : _addresses[kRegComb4LeftSrc]);
	uint32 apf1Src = getIndex(right ? _addresses[kRegApf1RightDst] - _addresses[kRegApf1Size] : _addresses[kRegApf1LeftDst] - _addresses[kRegApf1Size]);
	uint32 apf1Dst = getIndex(right ? _addresses[kRegApf1RightDst] : _addresses[kRegApf1LeftDst]);
	uint32 apf2Src = getIndex(right ? _addresses[kRegApf2RightDst] - _addresses[kRegApf2Size] : _addresses[kRegApf2LeftDst] - _addresses[kRegApf2Size]);
	uint32 apf2Dst = getIndex(right ? _addresses[kRegApf2RightDst] : _addresses[kRegApf2LeftDst]);

	int32 input = multiply(right ? _volumes[kRegInputRight] : _volumes[kRegInputLeft], downsample(right));
	int32 iirVolume = _volumes[kRegIirVolume];
	int32 wallVolume = _volumes[kRegWallVolume];

	int32 same = multiply(iirVolume, input + multiply(wallVolume, _ram[sameSrc]) - _ram[samePrevious]) + _ram[samePrevious];
	int32 diff = multiply(iirVolume, input + multiply(wallVolume, _ram[diffSrc]) - _ram[diffPrevious]) + _ram[diffPrevious];

	int32 out = multiply(_volumes[kRegComb1Volume], _ram[comb1Src]) + multiply(_volumes[kRegComb2Volume], _ram[comb2Src]) +
		multiply(_volumes[kRegComb3Volume], _ram[comb3Src]) + multiply(_volumes[kRegComb4Volume], _ram[comb4Src]);

	int32 apf1 = out - multiply(_volumes[kRegApf1Volume], _ram[apf1Src]);

	out = _ram[apf1Src] + multiply(_volumes[kRegApf1Volume], apf1);

	int32 apf2 = out - multiply(_volumes[kRegApf2Volume], _ram[apf2Src]);

	out = _ram[apf2Src] + multiply(_volumes[kRegApf2Volume], apf2);

	// Only the writes depend on the enable bit
	if (_effectEnabled) {
		_ram[sameDst] = (int16)clampMix(same);
		_ram[diffDst] = (int16)clampMix(diff);
		_ram[apf1Dst] = (int16)clampMix(apf1);
		_ram[apf2Dst] = (int16)clampMix(apf2);
	}

	out = clampMix(out);

	_upBuffer[right][_bufferPosition] = (int16)out;
	_upBuffer[right ^ 1][_bufferPosition] = 0;
	_upBuffer[right][_bufferPosition | kResampleBufferSize] = (int16)out;
	_upBuffer[right ^ 1][_bufferPosition | kResampleBufferSize] = 0;

	_bufferPosition = (_bufferPosition + 1) & (kResampleBufferSize - 1);

	int32 left = 0;
	int32 rightOut = 0;

	upsample(&left, &rightOut);

	*outLeft = (_effectVolumeLeft * left) >> 15;
	*outRight = (_effectVolumeRight * rightOut) >> 15;
}

uint32 EMISpu2Reverb::getIndex(uint32 offset) const {
	uint32 start = _effectStart & 0x3FFFFF;
	uint32 end = (_effectEnd & 0x3FFFFF) | 0xFFFF;
	uint32 index = ((_cycles >> 1) + offset) % ((end - start) + 1);

	index += start;

	return index & (kRamSize - 1);
}

int32 EMISpu2Reverb::downsample(int channel) const {
	int index = (int)((_bufferPosition - kResampleTaps) & (kResampleBufferSize - 1));
	int32 out = 0;

	for (int i = 0; i < kResampleTaps; i++)
		out += _downBuffer[channel][index + i] * kResampleCoefficients[i];

	return clampMix(out >> 15);
}

void EMISpu2Reverb::upsample(int32 *left, int32 *right) const {
	int index = (int)((_bufferPosition - kResampleTaps) & (kResampleBufferSize - 1));
	int32 sumLeft = 0;
	int32 sumRight = 0;

	for (int i = 0; i < kResampleTaps; i++) {
		// The upsampling taps are the downsampling taps doubled and clamped
		int32 coefficient = CLIP<int32>(kResampleCoefficients[i] * 2, -32768, 32767);

		sumLeft += _upBuffer[0][index + i] * coefficient;
		sumRight += _upBuffer[1][index + i] * coefficient;
	}

	*left = clampMix(sumLeft >> 15);
	*right = clampMix(sumRight >> 15);
}

int32 EMISpu2Reverb::multiply(int32 a, int32 b) {
	// PCSX2 multiplies in 32 bits, which can overflow on loud input
	return (int32)(((int64)a * b) >> 15);
}

int32 EMISpu2Reverb::clampMix(int32 value) {
	return CLIP<int32>(value, -0x8000, 0x7FFF);
}

} // End of namespace Grim
