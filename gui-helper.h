#ifndef GUI_HELPER_H
#define GUI_HELPER_H
#endif

int ShowMessageBox(const char* title, const char* message);
void PopulateListBoxWithSelection(int tabIndex, int selection);
void PopulateListBox(int tabIndex);

int SelectionDialog(const char* title, int nonSteamCount, const char** nonSteamList, int modsCount, const char** modsList, int steamCount, const char** steamList, int selection);
#if defined(_WIN32) || defined(WIN32)
int ShowMessageBoxW(const wchar_t* title, const wchar_t* message);
// This function is only used in Windows. It shouldn't exist, but we have to deal with Microsoft.
wchar_t * ConvertStringToUnicode(const char* string);
#endif