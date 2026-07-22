#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string-helpers.h"

#if defined(_WIN32) || defined(WIN32)
#define strcasecmp _stricmp

/*
getline() shim https://stackoverflow.com/a/47229318

The original code is public domain -- Will Hartung 4/9/2009
Modifications, public domain as well, by Antti Haapala, 11/10/2017
   - Switched to getc on 5/23/2019
   - Proper error handling on IO error and wraparound 
     check on buffer extension 3/31/2025 - 4/2/2025
*/
#include <errno.h>
#include <stdint.h>
#define MINIMUM_BUFFER_SIZE 128

// if typedef doesn't exist (msvc, blah)
typedef intptr_t ssize_t;

ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    size_t pos;
    int c;

    if (lineptr == NULL || stream == NULL || n == NULL) {
        errno = EINVAL;
        return -1;
    }

    c = getc(stream);
    if (c == EOF) {
        return -1;
    }

    if (*lineptr == NULL) {
        *lineptr = malloc(MINIMUM_BUFFER_SIZE);
        if (*lineptr == NULL) {
            return -1;
        }
        *n = MINIMUM_BUFFER_SIZE;
    }

    pos = 0;
    while(c != EOF) {
        if (pos + 1 >= *n) {
            size_t new_size = *n + (*n >> 2);

            // have some reasonable minimum
            if (new_size < MINIMUM_BUFFER_SIZE) {
                new_size = MINIMUM_BUFFER_SIZE;
            }
            
            // size_t wraparound
            if (new_size <= *n) {
                errno = ENOMEM;
                return -1;
            }

            // Note you might also want to check that PTRDIFF_MAX
            // is not exceeded!

            char *new_ptr = realloc(*lineptr, new_size);
            if (new_ptr == NULL) {
                return -1;
            }
            *n = new_size;
            *lineptr = new_ptr;
        }

        ((unsigned char *)(*lineptr))[pos ++] = c;
        if (c == '\n') {
            break;
        }
        c = getc(stream);
    }

    (*lineptr)[pos] = '\0';
    
    // if an IO error occurred, return -1
    if (c == EOF && !feof(stream)) {
        return -1;
    }

    // otherwise we successfully read until the end-of-file
    // or the delimiter
    return pos;
}
#endif

size_t readLine(char** lineptr, size_t* n, FILE* stream) {
    ssize_t result = getline(lineptr, n, stream);
    return (result < 0) ? (size_t)-1 : (size_t)result;
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

// Custom memmem
// https://stackoverflow.com/a/52989329
const unsigned char* sgdb_memmem(const void* haystack, size_t haystack_len,
	const void* const needle, const size_t needle_len)
{
	if (haystack == NULL) return NULL;
	if (haystack_len == 0) return NULL;
	if (needle == NULL) return NULL;
	if (needle_len == 0) return NULL;

	for (const unsigned char* h = haystack;
		haystack_len >= needle_len;
		++h, --haystack_len) {
		if (!memcmp(h, needle, needle_len)) {
			return h;
		}
	}
	return NULL;
}
