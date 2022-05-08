#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// Overriding getline_custom
// https://stackoverflow.com/a/3501681/16642426
size_t readLine(char**, size_t*, FILE*);

// Check if a string starts with a given substring
// https://stackoverflow.com/a/15515276/16642426
int startsWith(const char*, const char*);

// Compare function for sorting
int compareStrings(const void*, const void*);

// Case insensitive strstr
// https://stackoverflow.com/a/56513982/16642426
char* strstr_i(const char*, const char*);
