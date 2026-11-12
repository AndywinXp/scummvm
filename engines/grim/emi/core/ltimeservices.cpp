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

#include "common/system.h"

#include "engines/grim/emi/core/ltimeservices.h"

namespace Grim {

static const double kFrequency = 1000.0;

LTimeStamp::LTimeStamp() {
	_seconds = 0.0;
}

LTimeStamp::operator double() const {
	return _seconds;
}

LTimeStamp &LTimeStamp::operator+=(const LTimeStamp &other) {
	_seconds = other._seconds + _seconds;

	return *this;
}

LTimeStamp &LTimeStamp::operator-=(const LTimeStamp &other) {
	_seconds = _seconds - other._seconds;

	return *this;
}

LTimeStamp &LTimeStamp::operator=(const LTimeStamp &other) {
	_seconds = other._seconds;

	return *this;
}

void LTimeStamp::stamp() {
	_seconds = (double)g_system->getMillis() * (1.0 / kFrequency);
}

double LTimeServices::getCounter() {
	return (double)g_system->getMillis();
}

double LTimeServices::getElapsedMillis(const double *from) {
	double now = getCounter();

	return getMillisBetween(from, &now);
}

double LTimeServices::getMillisBetween(const double *from, const double *to) {
	return (*to - *from) / kFrequency * 1000.0;
}

double LTimeServices::getElapsedSeconds(const double *from) {
	double now = getCounter();

	return getSecondsBetween(from, &now);
}

double LTimeServices::getSecondsBetween(const double *from, const double *to) {
	return (*to - *from) / kFrequency;
}

} // End of namespace Grim
