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

int DiMUSE_v2::addTrackToList(DiMUSETrack **listPtr, DiMUSETrack *listPtr_Item) {
	// [0] is ->prev, [1] is ->next
	if (!listPtr_Item || listPtr_Item->prev || listPtr_Item->next) {
		debug(5, "DiMUSE_v2::addTrackToList(): ERROR: arguments might be null");
		return -5;
	} else {
		// Set item's next element to the list
		listPtr_Item->next = *listPtr;

		if (*listPtr) {
			// If the list is empty, use this item as the list
			(*listPtr)->prev = listPtr_Item;
		}

		// Set the previous element of the item as NULL, 
		// effectively making the item the first element of the list
		listPtr_Item->prev = NULL;

		// Update the list with the new data
		*listPtr = listPtr_Item;

	}
	return 0;
}

int DiMUSE_v2::removeTrackFromList(DiMUSETrack **listPtr, DiMUSETrack *listPtr_Item) {
	DiMUSETrack *currentTrack = *listPtr;
	DiMUSETrack *nextTrack;
	if (listPtr_Item && currentTrack) {
		do {
			if (currentTrack == listPtr_Item)
				break;
			currentTrack = currentTrack->next;
		} while (currentTrack);

		if (currentTrack) {
			nextTrack = listPtr_Item->next;

			if (nextTrack)
				nextTrack->prev = listPtr_Item->prev;

			if (listPtr_Item->prev) {
				listPtr_Item->prev->next = listPtr_Item->next;
			} else {
				*listPtr = listPtr_Item->next;
			}

			listPtr_Item->prev = NULL;
			listPtr_Item->next = NULL;
			return 0;
		} else {
			debug(5, "DiMUSE_v2::removeTrackFromList(): ERROR: item not on list");
			return -3;
		}
	} else {
		debug(5, "DiMUSE_v2::removeTrackFromList(): ERROR: arguments might be null");
		return -5;
	}
}

int DiMUSE_v2::addStreamZoneToList(DiMUSEStreamZone **listPtr, DiMUSEStreamZone *listPtr_Item) {
	if (!listPtr_Item || listPtr_Item->prev || listPtr_Item->next) {
		debug(5, "DiMUSE_v2::addStreamZoneToList(): ERROR: arguments might be null");
		return -5;
	} else {
		// Set item's next element to the list
		listPtr_Item->next = *listPtr;

		if (*listPtr) {
			// If the list is empty, use this item as the list
			(*listPtr)->prev = listPtr_Item;
		}

		// Set the previous element of the item as NULL, 
		// effectively making the item the first element of the list
		listPtr_Item->prev = NULL;

		// Update the list with the new data
		*listPtr = listPtr_Item;

	}
	return 0;
}

int DiMUSE_v2::removeStreamZoneFromList(DiMUSEStreamZone **listPtr, DiMUSEStreamZone *listPtr_Item) {
	DiMUSEStreamZone *currentStrZone = *listPtr;
	DiMUSEStreamZone *nextStrZone;
	if (listPtr_Item && currentStrZone) {
		do {
			if (currentStrZone == listPtr_Item)
				break;
			currentStrZone = currentStrZone->next;
		} while (currentStrZone);

		if (currentStrZone) {
			nextStrZone = listPtr_Item->next;

			if (nextStrZone)
				nextStrZone->prev = listPtr_Item->prev;

			if (listPtr_Item->prev) {
				listPtr_Item->prev->next = listPtr_Item->next;
			} else {
				*listPtr = listPtr_Item->next;
			}

			listPtr_Item->prev = NULL;
			listPtr_Item->next = NULL;
			return 0;
		} else {
			debug(5, "DiMUSE_v2::removeStreamZoneFromList(): ERROR: item not on list");
			return -3;
		}
	} else {
		debug(5, "DiMUSE_v2::removeStreamZoneFromList(): ERROR: arguments might be null");
		return -5;
	}
}

int DiMUSE_v2::clampNumber(int value, int minValue, int maxValue) {
	if (value < minValue)
		return minValue;

	if (value > maxValue)
		return maxValue;

	return value;
}

int DiMUSE_v2::clampTuning(int value, int minValue, int maxValue) {
	if (minValue > value) {
		value += (12 * ((minValue - value) + 11) / 12);
	}

	if (maxValue < value) {
		value -= (12 * ((value - maxValue) + 11) / 12);
	}

	return value;
}

int DiMUSE_v2::checkHookId(int *trackHookId, int sampleHookId) {
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

void DiMUSE_v2::internalStrcpy(char *dst, char *marker) {
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

int DiMUSE_v2::internalStrcmp(char *marker1, char *marker2) {
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

int DiMUSE_v2::internalStrlen(char *marker) {
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
