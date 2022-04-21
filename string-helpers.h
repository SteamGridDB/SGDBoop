#include <stdio.h>
#include <stdlib.h>

// Overriding getline_custom
// https://stackoverflow.com/a/3501681/16642426
size_t getline(char**, size_t*, FILE*);

// Get last index of char in string
// https://stackoverflow.com/a/19342633/16642426
int lastIndexOf(const char*, char);

// Check if a string starts with a given substring
// https://stackoverflow.com/a/15515276/16642426
int startsWith(const char*, const char*);