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

#include "engines/grim/std/std_sort.h"

namespace Grim {

static void swap(char *a, char *b, size_t width) {
	if (a == b)
		return;

	for (size_t i = 0; i < width; i++) {
		char tmp = a[i];
		a[i] = b[i];
		b[i] = tmp;
	}
}

static void shortsort(char *lo, char *hi, size_t width, SortCompare compare) {
	while (hi > lo) {
		char *maxPointer = lo;

		for (char *cur = lo + width; cur <= hi; cur += width) {
			if (compare(cur, maxPointer) > 0)
				maxPointer = cur;
		}

		swap(maxPointer, hi, width);
		hi -= width;
	}
}

// qsort is redefined because the order of equal elements depends on the algorithm used by the executable
void sort(void *base, size_t num, size_t width, SortCompare compare) {
	if (num < 2 || !width)
		return;

	char *lo = (char *)base;
	char *hi = lo + width * (num - 1);
	char *loStack[30];
	char *hiStack[30];
	int stackPtr = 0;

	while (true) {
		size_t count = (hi - lo) / width + 1;

		if (count <= 8) {
			shortsort(lo, hi, width, compare);
		} else {
			swap(lo + width * (count >> 1), lo, width);

			char *left = lo;
			char *right = hi + width;

			while (true) {
				do {
					left += width;
				} while (left <= hi && compare(left, lo) <= 0);

				do {
					right -= width;
				} while (right > lo && compare(right, lo) >= 0);

				if (right < left)
					break;

				swap(left, right, width);
			}

			swap(lo, right, width);

			if (right - lo - 1 >= hi - left) {
				if (lo + width < right) {
					loStack[stackPtr] = lo;
					hiStack[stackPtr] = right - width;
					stackPtr++;
				}

				if (left < hi) {
					lo = left;
					continue;
				}
			} else {
				if (left < hi) {
					loStack[stackPtr] = left;
					hiStack[stackPtr] = hi;
					stackPtr++;
				}

				if (lo + width < right) {
					hi = right - width;
					continue;
				}
			}
		}

		if (stackPtr == 0)
			return;

		stackPtr--;
		lo = loStack[stackPtr];
		hi = hiStack[stackPtr];
	}
}

// bsearch is redefined because the element found among equal ones depends on the algorithm used by the executable
void *binarySearch(const void *key, const void *base, size_t num, size_t width, SortCompare compare) {
	const char *lo = (const char *)base;

	while (num) {
		size_t half = num >> 1;

		if (!half)
			return compare(key, lo) == 0 ? (void *)lo : nullptr;

		size_t mid = (num & 1) ? half : half - 1;
		const char *midPointer = lo + mid * width;
		int result = compare(key, midPointer);

		if (result == 0)
			return (void *)midPointer;

		if (result < 0) {
			num = mid;
		} else {
			lo = midPointer + width;
			num = half;
		}
	}

	return nullptr;
}

} // End of namespace Grim
