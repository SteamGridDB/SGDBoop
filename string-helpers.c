#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string-helpers.h"

#if defined(_WIN32) || defined(WIN32)
#define strcasecmp _stricmp
#endif

// Overriding getline_custom
// https://stackoverflow.com/a/3501681/16642426
size_t readLine(char** lineptr, size_t* n, FILE* stream) {
	char* bufptr = NULL;
	char* p = bufptr;
	size_t size;
	int c;

	if (lineptr == NULL) {
		return -1;
	}
	if (stream == NULL) {
		return -1;
	}
	if (n == NULL) {
		return -1;
	}
	bufptr = *lineptr;
	size = *n;

	c = fgetc(stream);
	if (c == EOF) {
		return -1;
	}
	if (bufptr == NULL) {
		bufptr = malloc(128);
		if (bufptr == NULL) {
			return -1;
		}
		size = 128;
	}
	p = bufptr;
	while (c != EOF) {
		if ((p - bufptr) > (size - 1)) {
			size = size + 128;
			bufptr = realloc(bufptr, size);
			if (bufptr == NULL) {
				return -1;
			}
		}
		*p++ = c;
		if (c == '\n') {
			break;
		}
		c = fgetc(stream);
	}

	*p++ = '\0';
	*lineptr = bufptr;
	*n = size;

	return p - bufptr - 1;
}

// Check if a string starts with a given substring
// https://stackoverflow.com/a/15515276/16642426
int startsWith(const char* a, const char* b)
{
	if (strncmp(a, b, strlen(b)) == 0) return 1;
	return 0;
}

// Compare function for qsort
// https://stackoverflow.com/a/4061231/16642426
int compareStrings(const void* p1, const void* p2)
{
	const char** ia = (const char**)p1;
	const char** ib = (const char**)p2;
	return strcasecmp(*ia, *ib);
}

// Case insensitive strstr
// https://stackoverflow.com/a/56513982/16642426
char* strstr_i(const char* p1, const char* p2) {
	for (;; p1++) {
		for (size_t i = 0;; i++) {
			if (p2[i] == '\0')
				return (char*)p1;
			if (tolower((unsigned char)p1[i]) != tolower((unsigned char)p2[i]))
				break;
		}
		if (*p1 == '\0')
			return 0;
	}
}

// Replace a substring in a string with another string
char* strreplace(char* origString, const char* searchString, const char* replaceString) {
	char* searchIndex = origString;

	while (strstr(searchIndex, searchString) != NULL) {
		searchIndex = strstr(searchIndex, searchString);
		*searchIndex = '\0';

		origString = realloc(origString, strlen(origString) + strlen(replaceString) + 1);
		strcat(origString, replaceString);
		searchIndex += strlen(searchString);

		origString = realloc(origString, strlen(origString) + strlen(searchIndex) + 1);
		strcat(origString, searchIndex);
	}

	return origString;
}