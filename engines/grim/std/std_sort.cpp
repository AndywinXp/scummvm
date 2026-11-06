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

static void swap(char *pA, char *pB, size_t width) {
	if (pA == pB)
		return;

	for (size_t i = 0; i < width; i++) {
		char tmp = pA[i];
		pA[i] = pB[i];
		pB[i] = tmp;
	}
}

static void shortsort(char *pLo, char *pHi, size_t width, SortCompare compare) {
	while (pHi > pLo) {
		char *pMax = pLo;

		for (char *pCur = pLo + width; pCur <= pHi; pCur += width) {
			if (compare(pCur, pMax) > 0)
				pMax = pCur;
		}

		swap(pMax, pHi, width);
		pHi -= width;
	}
}

// qsort is redefined because the order of equal elements depends on the algorithm used by the executable
void sort(void *pBase, size_t num, size_t width, SortCompare compare) {
	if (num < 2 || !width)
		return;

	char *pLo = (char *)pBase;
	char *pHi = pLo + width * (num - 1);
	char *aLoStack[30];
	char *aHiStack[30];
	int stackPtr = 0;

	while (true) {
		size_t count = (pHi - pLo) / width + 1;

		if (count <= 8) {
			shortsort(pLo, pHi, width, compare);
		} else {
			swap(pLo + width * (count >> 1), pLo, width);

			char *pLeft = pLo;
			char *pRight = pHi + width;

			while (true) {
				do {
					pLeft += width;
				} while (pLeft <= pHi && compare(pLeft, pLo) <= 0);

				do {
					pRight -= width;
				} while (pRight > pLo && compare(pRight, pLo) >= 0);

				if (pRight < pLeft)
					break;

				swap(pLeft, pRight, width);
			}

			swap(pLo, pRight, width);

			if (pRight - pLo - 1 >= pHi - pLeft) {
				if (pLo + width < pRight) {
					aLoStack[stackPtr] = pLo;
					aHiStack[stackPtr] = pRight - width;
					stackPtr++;
				}

				if (pLeft < pHi) {
					pLo = pLeft;
					continue;
				}
			} else {
				if (pLeft < pHi) {
					aLoStack[stackPtr] = pLeft;
					aHiStack[stackPtr] = pHi;
					stackPtr++;
				}

				if (pLo + width < pRight) {
					pHi = pRight - width;
					continue;
				}
			}
		}

		if (stackPtr == 0)
			return;

		stackPtr--;
		pLo = aLoStack[stackPtr];
		pHi = aHiStack[stackPtr];
	}
}

} // End of namespace Grim
