#ifndef GUI_HELPER_H
#define GUI_HELPER_H

int ShowMessageBox(const char* title, const char* message);
int SelectionDialog(const char* title, int count, const char** list, int selection);

#if defined(_WIN32) || defined(WIN32) 
// This function is only used in Windows. It shouldn't exist, but we have to deal with Microsoft.
wchar_t * ConvertStringToUnicode(char * string);
#endif

#endif