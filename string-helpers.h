#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

size_t readLine(char**, size_t*, FILE*);

// Check if a string starts with a given substring
// https://stackoverflow.com/a/15515276/16642426
int startsWith(const char*, const char*);

// Compare strings for qsort
// https://stackoverflow.com/a/4061231/16642426
int compareStrings(const void*, const void*);

// Case insensitive strstr
char* strstr_i(char*, const char*);

// Replace a substring in a string with another string
char* strreplace(char*, const char*, const char*);

// Case-insensitive string comparison
// https://stackoverflow.com/a/5820991/16642426
int strcmp_i(const char*, const char*);

// Custom memmem
// https://stackoverflow.com/a/52989329
const unsigned char* sgdb_memmem(const void*, size_t, const void* const, const size_t);