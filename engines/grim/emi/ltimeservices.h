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

#ifndef GRIM_EMI_LTIMESERVICES_H
#define GRIM_EMI_LTIMESERVICES_H

#include "common/scummsys.h"

namespace Grim {

class LTimeStamp {
public:
	LTimeStamp();

	operator double() const;
	LTimeStamp &operator+=(const LTimeStamp &other);
	LTimeStamp &operator-=(const LTimeStamp &other);
	LTimeStamp &operator=(const LTimeStamp &other);

	void stamp();

	double seconds;
};

class LTimeServices {
public:
	LTimeServices();

	virtual double getCounter();
	virtual double getElapsedMillis(const double *from);
	virtual double getMillisBetween(const double *from, const double *to);
	virtual double getElapsedSeconds(const double *from);
	virtual double getSecondsBetween(const double *from, const double *to);
};

extern LTimeServices *gpTimeServices;

} // End of namespace Grim

#endif
