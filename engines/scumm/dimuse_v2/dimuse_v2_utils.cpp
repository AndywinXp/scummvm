/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "scumm/dimuse_v2/dimuse_v2.h"

namespace Scumm {

int DiMUSE_v2::iMUSE_addItemToList(int *listPtr, int *listPtr_Item) {
	// [0] is ->prev, [1] is ->next
	if (!listPtr_Item || listPtr_Item[0] || listPtr_Item[1]) {
		debug(5, "ERR: list arg err when adding...\n");
		return -5;
	} else {
		// Set item's next element to the list
		listPtr_Item[1] = *listPtr;

		if (*listPtr) {
			// If the list is empty, use this item as the list
			listPtr = listPtr_Item; // TODO: Is this right?
		}
		// Set the previous element of the item as NULL, 
		// effectively making the item the first element of the list
		*listPtr_Item = NULL;

		// Update the list with the new data
		listPtr = listPtr_Item;

	}
	return 0;
}

int DiMUSE_v2::iMUSE_removeItemFromList(int *listPtr, int *listPtr_Item) {
	int *element = (int *)*listPtr;
	if (listPtr_Item && element) {
		do {
			if (element == listPtr_Item)
				break;
			element = (int *)element[1];
		} while (element);

		if (element) {
			int *next_element = (int *)listPtr_Item[1];

			if (next_element)
				next_element[0] = listPtr_Item[0];

			if (*listPtr_Item) {
				*(int *)(*listPtr_Item + 4) = listPtr_Item[1];
			} else {
				*listPtr = listPtr_Item[1];
			}

			listPtr_Item[0] = NULL;
			listPtr_Item[1] = NULL;
			return 0;
		} else {
			debug(5, "ERR: item not on list...\n");
			return -3;
		}
	} else {
		debug(5, "ERR: list arg err when removing...\n");
		return -5;
	}
}

int DiMUSE_v2::iMUSE_clampNumber(int value, int minValue, int maxValue) {
	if (value < minValue)
		return minValue;

	if (value > maxValue)
		return maxValue;

	return value;
}

int DiMUSE_v2::iMUSE_clampTuning(int value, int minValue, int maxValue) {
	if (minValue > value) {
		value += (12 * ((minValue - value) + 11) / 12);
	}

	if (maxValue < value) {
		value -= (12 * ((value - maxValue) + 11) / 12);
	}

	return value;
}

int DiMUSE_v2::iMUSE_SWAP32(uint8 *value) {
	return value[3] | ((value[2] | ((value[1] | (*value << 8)) << 8)) << 8);
}

int DiMUSE_v2::iMUSE_checkHookId(int *trackHookId, int sampleHookId) {
	if (sampleHookId) {
		if (*trackHookId == sampleHookId) {
			*trackHookId = 0;
			return 0;
		} else {
			return -1;
		}
	} else if (*trackHookId == 128) {
		*trackHookId = 0;
		return -1;
	} else {
		return 0;
	}
}

// Probably just a strcpy, but if there's undefined behavior it's best to match it
void DiMUSE_v2::iMUSE_strcpy(char *dst, char *marker) {
	char currentChar;

	if ((dst != NULL) && (marker != NULL)) {
		do {
			currentChar = *marker;
			marker = marker + 1;
			*dst = currentChar;
			dst = dst + 1;
		} while (currentChar != '\0');
	}
	return;
}

// Probably just a stricmp, but if there's undefined behavior it's best to match it
int DiMUSE_v2::iMUSE_compareTEXT(char *marker1, char *marker2) {
	if (*marker1 != 0) {
		while ((*marker2 != 0 && (*marker1 == *marker2))) {
			marker1 = marker1 + 1;
			marker2 = marker2 + 1;
			if (*marker1 == 0) {
				return (int)(char)(*marker2 | *marker1);
			}
		}
	}
	return (int)(char)(*marker2 | *marker1);
}

// Probably just a strlen, but if there's undefined behavior it's best to match it
int DiMUSE_v2::iMUSE_getMarkerLength(char *marker) {
	int resultingLength;
	char curChar;

	resultingLength = 0;
	curChar = *marker;
	while (curChar != '\0') {
		marker = marker + 1;
		resultingLength += 1;
		curChar = *marker;
	}
	return resultingLength;
}

} // End of namespace Scumm
