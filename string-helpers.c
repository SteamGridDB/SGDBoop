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
char* strstr_i(char* p1, const char* p2) {
	while (*p1) {
		for (size_t i = 0;; i++) {
			if (p2[i] == '\0')
				return (char*)p1;
			if (tolower(p1[i]) != tolower(p2[i]))
				break;
		}
		++p1;
	}
	return 0;
}

// Replace a substring in a string with another string
char* strreplace(char* origString, const char* searchString, const char* replaceString) {
	char* searchIndex = origString;
	char* tmpString = malloc(strlen(origString) + 1);
	strcpy(tmpString, origString);

	while (strstr(searchIndex, searchString) != NULL) {
		
		searchIndex = strstr(searchIndex, searchString);
		*searchIndex = '\0';
		*(tmpString + (searchIndex - origString)) = '\0';

		tmpString = realloc(tmpString, strlen(tmpString) + strlen(replaceString) + 1);
		strcat(tmpString, replaceString);
		searchIndex += strlen(searchString);

		tmpString = realloc(tmpString, strlen(tmpString) + strlen(searchIndex) + 1);
		strcat(tmpString, searchIndex);
	}

	origString = realloc(origString, strlen(tmpString) + 1);
	strcpy(origString, tmpString);
	free(tmpString);
	return origString;
}

// Case-insensitive string comparison
// https://stackoverflow.com/a/5820991/16642426
int strcmp_i(const char * a, const char * b)
{
	for (;; a++, b++)
	{
		int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
		if (d != 0 || !*a)
			return d;
	}
}