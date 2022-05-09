#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string-helpers.h"

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
// https://codereview.stackexchange.com/a/256749
int ci_strcmp(const unsigned char* s1, const unsigned char* s2)
{
	for (;;) {
		int c1 = tolower(*s1++);
		int c2 = tolower(*s2++);
		int result = (c1 > c2) - (c1 < c2);
		if (result || !c1) return result;
	}
}
int compareStrings(const void* p1, const void* p2)
{
	const unsigned char* const* sp1 = p1;
	const unsigned char* const* sp2 = p2;
	return ci_strcmp(*sp1, *sp2);
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
