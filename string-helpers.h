#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// Overriding getline_custom
// https://stackoverflow.com/a/3501681/16642426
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