#ifndef GUI_HELPER_H
#define GUI_HELPER_H

int ShowMessageBox(const char* title, const char* message);
void PopulateListBox(int tabIndex);

#if defined(_WIN32) || defined(WIN32)
int SelectionDialog(const wchar_t* title, int count, const char** list, int modsCount, const char** modsList, int selection);

// This function is only used in Windows. It shouldn't exist, but we have to deal with Microsoft.
wchar_t * ConvertStringToUnicode(const char* string);
#else
int SelectionDialog(const char* title, int count, const char** list, int modsCount, const char** modsList, int selection);
#endif

#endif