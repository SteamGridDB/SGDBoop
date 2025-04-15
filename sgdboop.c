#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <dirent.h>
#include "string-helpers.h"
#include "curl-helper.h"
#include "gui-helper.h"
#include "crc.h"


#ifdef __unix__                             /* __unix__ is usually defined by compilers targeting Unix systems */
#define OS_Windows 0
#define MAX_PATH 600 
#define FALSE 0
#define TRUE 1
#define WCHAR char
#define LPSTR char*
int GetConsoleWindow();
void MoveWindow(int, int, int, int, int, int);
void GetModuleFileName(char*, char*, int);
FILE* _popen(char*, char*);
void _pclose(FILE*);
#include <unistd.h>
#elif defined(_WIN32) || defined(WIN32)     /* _Win32 is usually defined by compilers targeting 32 or   64 bit Windows systems */
#define OS_Windows 1
#include <windows.h>
int symlink(char* a, char* b) {
	return CreateHardLink(b, a, NULL);
}
#endif

#define API_VERSION "2"
#define API_USER_AGENT "SGDBoop/v1.2.3"

typedef struct nonSteamApp
{
	int index;
	char name[300];
	char appid[128];
	char appid_old[128];
	char type[50];
} nonSteamApp;

int _nonSteamAppsCount = 0;
int _modsCount = 0;
int _sourceModsCount = 0;
int _goldSourceModsCount = 0;
int _apiReturnedLines = 0;

// Get logfile path
char* getLogFilepath() {
	char* logFilepath = malloc(MAX_PATH);

#if OS_Windows
	WCHAR path[MAX_PATH];
	GetModuleFileName(NULL, (LPSTR)path, MAX_PATH);
	char* filename = (char*)path;
	while (strstr(filename, "\\") > 0) {
		filename = strstr(filename, "\\") + 1;
	}
	*filename = '\0';
	strcpy(logFilepath, (const char*)path);
	strcat(logFilepath, "sgdboop_error.log");
#else
	if (getenv("XDG_STATE_HOME") != NULL && strlen(getenv("XDG_STATE_HOME")) > 0) {
		strcpy(logFilepath, getenv("XDG_STATE_HOME"));
	}
	else {
		strcpy(logFilepath, getenv("HOME"));
		strcat(logFilepath, "/.local/state");
	}

	// Try creating folder
	if (access(logFilepath, 0) != 0) {
		mkdir(logFilepath, 0700);
	}

	strcat(logFilepath, "/sgdboop_error.log");
#endif

	return logFilepath;
}

// Log error messages
void logError(const char* error, const int errorCode)
{
	time_t now = time(0);
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char* logFilepath = getLogFilepath();

	FILE* logFile = fopen(logFilepath, "a");
	if (logFile == NULL) {
		logFile = fopen(logFilepath, "w");
	}
	if (logFile) {
		fprintf(logFile, "%s%s [%d]\n\n", asctime(timeinfo), error, errorCode);
		fclose(logFile);
		printf("Created logfile in %s\n", logFilepath);
	}
}

// Log an error and exit with the given error code
void exitWithError(const char* error, const int errorCode) {
	logError(error, errorCode);
	exit(errorCode);
}

void cleanupOldAssetFiles(const char* finalPath)
{
	// Copy final path and strip the extension
	char basePath[MAX_PATH];
	strcpy(basePath, finalPath);

	char* dot = strrchr(basePath, '.');
	if (dot) *dot = '\0';

	const char* extensions[] = { ".jpg", ".jpeg", ".png" }; // Steam only reads from these

	for (int i = 0; i < 3; i++) {
		char tempPath[MAX_PATH];
		strcpy(tempPath, basePath);
		strcat(tempPath, extensions[i]);

		if (strcmp(tempPath, finalPath) != 0) {
			remove(tempPath);
		}
	}
}


// Call the BOOP API
char*** callAPI(char* grid_types, char* grid_ids, char* mode)
{
	char authHeader[] = "Authorization: Bearer 62696720-6f69-6c79-2070-65656e75733f";
	char apiVersionHeader[20] = "X-BOOP-API-VER: ";
	strcat(apiVersionHeader, API_VERSION);
	char* url = malloc(512);
	char*** valuesArray = malloc((sizeof(char**)) * 500);

	strcpy(url, "https://www.steamgriddb.com/api/sgdboop/");
	strcat(url, grid_types);
	strcat(url, "/");
	strcat(url, grid_ids);

	if (strcmp(mode, "nonsteam") == 0) {
		strcat(url, "?nonsteam=1");
	}

	CURL* curl;
	CURLcode res;
	struct curl_slist* headers = NULL;
	curl = curl_easy_init();
	if (curl)
	{
		struct string s;
		init_string(&s);

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, FALSE);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, API_USER_AGENT);
		headers = curl_slist_append(headers, authHeader);
		headers = curl_slist_append(headers, apiVersionHeader);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		res = curl_easy_perform(curl);
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		curl_easy_cleanup(curl);

		if (http_code >= 400)
		{
			char* message = malloc(1000);
			strcpy(message, "API Error: ");
			strcat(message, s.ptr);

			if (startsWith(s.ptr, "error-")) {
				strreplace(message, "error-", " ");
				ShowMessageBox("SGDBoop Error", message);
			}
			exitWithError(message, (int)http_code);
		}

		char* line = s.ptr;

		while (strstr(line, "\r") > 0) {
			char* endLine = strstr(line, "\r");
			*endLine = '\0';

			valuesArray[_apiReturnedLines] = malloc((sizeof(char**)) * 1000);
			valuesArray[_apiReturnedLines][0] = line;
			valuesArray[_apiReturnedLines][1] = strstr(line, ",");
			*valuesArray[_apiReturnedLines][1] = '\0';
			valuesArray[_apiReturnedLines][1] += 1;
			valuesArray[_apiReturnedLines][2] = strstr(valuesArray[_apiReturnedLines][1], ",");
			*valuesArray[_apiReturnedLines][2] = '\0';
			valuesArray[_apiReturnedLines][2] += 1;
			valuesArray[_apiReturnedLines][3] = strstr(valuesArray[_apiReturnedLines][2], ",");
			*valuesArray[_apiReturnedLines][3] = '\0';
			valuesArray[_apiReturnedLines][3] += 1;

			line = endLine + 1;

			_apiReturnedLines++;
		}

		free(url);
		return valuesArray;
	}
	free(url);
	free(valuesArray);
	return NULL;
}

// Download an asset file
char* downloadAssetFile(char* app_id, char* url, char* type, char* orientation, char* destinationDir, struct nonSteamApp* nonSteamAppData)
{
	// Try creating folder
	if (access(destinationDir, 0) != 0) {
		mkdir(destinationDir, 0700);
	}

	CURL* curl;
	FILE* fp;
	CURLcode res;

	char* outfilename = malloc(1024);
	strcpy(outfilename, destinationDir);
	strcat(outfilename, app_id);
	if (strcmp(type, "hero") == 0) {
		// Hero
		strcat(outfilename, "_hero");
	}
	else if (strcmp(type, "logo") == 0) {
		// Logo
		strcat(outfilename, "_logo");
	}
	else if (strcmp(type, "grid") == 0 && strcmp(orientation, "p") == 0) {
		// Vertical grid
		strcat(outfilename, "p");
	}
	else if (strcmp(type, "grid") == 0) {
		// Horizontal grid has no suffix
	}
	else if (strcmp(type, "icon") == 0) {
		// Icon
		strcat(outfilename, "_icon");
	}

	// Always save as jpg if icon and replacing default Steam
	if (nonSteamAppData == NULL && strcmp(type, "icon") == 0) {
		strcat(outfilename, ".jpg");
	}
	else {
		// Save as original file extension
		const char* dot = strrchr(url, '.');
		if (dot) {
			char ext[16] = { 0 };
			strncpy(ext, dot, sizeof(ext) - 1);
			char* qmark = strchr(ext, '?');
			if (qmark) *qmark = '\0'; // cut at '?'
			strcat(outfilename, ext);
		}
		else {
			strcat(outfilename, ".jpg"); // fallback
		}
	}

	curl = curl_easy_init();
	if (curl && outfilename != 0) {
		cleanupOldAssetFiles(outfilename);
		fp = fopen(outfilename, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, TRUE);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, API_USER_AGENT);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
		if (res != 0) {
			remove(outfilename);
			free(outfilename);
			return NULL;
		}

		return outfilename;
	}

	free(outfilename);
	return NULL;
}

// Create the SGDB URI protocol
int createURIprotocol() {

	char* logFilepath = getLogFilepath();
	char* popupMessage = malloc(1000);

#if OS_Windows
	char cwd[MAX_PATH];
	GetModuleFileName(NULL, cwd, MAX_PATH);

	char* regeditCommand = malloc(2028);
	strcpy(regeditCommand, "C:\\Windows\\System32\\reg.exe ADD HKCR\\sgdb\\Shell\\Open\\Command /t REG_SZ /d \"\\\"");
	strcat(regeditCommand, cwd);
	strcat(regeditCommand, "\\\" \\\"%1\\\" -new_console:z\" /f");

	int ret_val_reg = system("C:\\Windows\\System32\\reg.exe ADD HKCR\\sgdb /t REG_SZ /d \"URL:sgdb protocol\" /f");
	if (ret_val_reg != 0) {
		int ret_val_exists = system("C:\\Windows\\System32\\reg.exe query HKCR\\sgdb\\Shell\\Open\\Command /ve");
		if (ret_val_exists != 0) {
			ShowMessageBox("SGDBoop Error", "Please run this program as Administrator to register it!\n");
		} else {
			ShowMessageBox("SGDBoop Error", "SGDBoop is already registered!\nHead over to https://www.steamgriddb.com/boop to continue setup.\n\nIf you moved the program and want to register again, run SGDBoop as Administrator.\n");
		}
		free(regeditCommand);
		return 1;
	}

	system(regeditCommand);

	strcpy(regeditCommand, "C:\\Windows\\System32\\reg.exe ADD HKCR\\sgdb\\DefaultIcon /t REG_SZ /d \"");
	strcat(regeditCommand, cwd);
	strcat(regeditCommand, "\" /f");
	system(regeditCommand);

	system("C:\\Windows\\System32\\reg.exe ADD HKCR\\sgdb /v \"URL Protocol\" /t REG_SZ /d \"\" /f");

	strcpy(popupMessage, "Program registered successfully!\n\nSGDBoop is meant to be ran from a browser!\nHead over to https://www.steamgriddb.com/boop to continue setup.");
	strcat(popupMessage, "\n\nLog file path: ");
	strcat(popupMessage, logFilepath);
	ShowMessageBox("SGDBoop Information", popupMessage);
	free(regeditCommand);
	return 0;
#else
	// Do nothing on linux
	strcpy(popupMessage, "SGDBoop is meant to be ran from a browser!\nHead over to https://www.steamgriddb.com/boop to continue setup.");
	strcat(popupMessage, "\n\nLog file path: ");
	strcat(popupMessage, logFilepath);
	ShowMessageBox("SGDBoop Information", popupMessage);
	return 0;
#endif
}

// Delete the SGDB URI protocol
int deleteURIprotocol() {
#if OS_Windows
	int ret_val = system("C:\\Windows\\System32\\reg.exe DELETE HKCR\\sgdb /f");
	if (ret_val != 0) {
		system("cls");
		printf("Please run this program as Administrator!\n");
		system("pause");
		return 1;
	}

	system("cls");
	printf("Program unregistered successfully!\n");
	system("pause");
	return 0;
#else
	// Do nothing on linux
	printf("A SGDB URL argument is required.\nExample: SGDBoop sgdb://boop/[ASSET_TYPE]/[ASSET_ID]\n");
	return 1;
#endif
}

// Get Steam's base directory
char* getSteamBaseDir() {

	char* steamBaseDir = malloc(MAX_PATH);
	int foundValue = 0;

#if OS_Windows
	FILE* terminal = _popen("C:\\Windows\\System32\\reg.exe query HKCU\\Software\\Valve\\Steam /v SteamPath", "r");
	char buf[256];
	while (fgets(buf, sizeof(buf), terminal) != 0) {
		if (strstr(buf, "SteamPath")) {
			char* extractedValue = strstr(buf, "REG_SZ") + 10;
			strcpy(steamBaseDir, extractedValue);
			foundValue = 1;
		}
	}
	_pclose(terminal);
	if (!foundValue) {
		free(steamBaseDir);
		return NULL;
	}

	int steamDirLength = strlen(steamBaseDir);
	for (int i = 0; i < steamDirLength; i++) {
		if (steamBaseDir[i] == '\n') {
			steamBaseDir[i] = '\0';
		}
	}
#else
	foundValue = 1;
	strcpy(steamBaseDir, getenv("HOME"));

	char steamFlatpakDir[MAX_PATH];
	strcpy(steamFlatpakDir, "/var/lib/flatpak/app/com.valvesoftware.Steam");

	// If flatpaked Steam is installed
	if (access(steamFlatpakDir, 0) == 0) {
		strcat(steamBaseDir, "/.var/app/com.valvesoftware.Steam/data/Steam");
	}
	else {
		// Steam installed on host
		strcat(steamBaseDir, "/.steam/steam");
	}
#endif

	if (foundValue < 1) {
		free(steamBaseDir);
		return NULL;
	}

	return steamBaseDir;
}

// Find the most recently logged-in user
char* getMostRecentUser(char* steamBaseDir) {

	char* steamid = malloc(512);
	char* steamConfigFile = malloc(MAX_PATH);
	strcpy(steamConfigFile, steamBaseDir);
	strcat(steamConfigFile, "/config/loginusers.vdf");

	FILE* fp;
	char* line = NULL;
	size_t len = 0;
	size_t read;
	fp = fopen(steamConfigFile, "r");
	free(steamConfigFile);
	if (fp == NULL) {
		free(steamid);
		exitWithError("Couldn't find logged in user", 95);
	}

	while ((read = readLine(&line, &len, fp)) != -1) {
		if (strstr(line, "7656119") && !strstr(line, "PersonaName")) {
			// Found line with id
			strcpy(steamid, strstr(line, "7656119"));
			char* stringEnd = strstr(steamid, "\"");
			stringEnd[0] = '\0';
		}
		else if ((strstr(line, "mostrecent") || strstr(line, "MostRecent")) && strstr(line, "\"1\"")) {
			// Found line mostrecent
			unsigned long long steamidLongLong = atoll(steamid);
			steamidLongLong -= 76561197960265728;
			sprintf(steamid, "%llu", steamidLongLong);
			break;
		}
	}

	fclose(fp);
	if (line)
		free(line);

	return steamid;
}

// Get Steam's destination directory based on artwork type
char* getSteamDestinationDir(char* type, struct nonSteamApp* nonSteamAppData) {

	char* steamBaseDir = getSteamBaseDir();
	if (steamBaseDir == NULL) {
		return NULL;
	}

	if (strcmp(type, "icon") == 0 && nonSteamAppData == NULL) {
		// If it's a Steam app icon
		strcat(steamBaseDir, "/appcache/librarycache/");
	}
	else {
		// If it's not a Steam app icon, read the loginusers.vdf to find the most recent user
		char* steamid = getMostRecentUser(steamBaseDir);
		strcat(steamBaseDir, "/userdata/");
		strcat(steamBaseDir, steamid);
		strcat(steamBaseDir, "/config/grid/");
		free(steamid);
	}

	return steamBaseDir;
}

// Get source mods appids
struct nonSteamApp* getSourceMods(const char* type)
{
	int goldsource = 0;
	if (strcmp(type, "goldsource") == 0) {
		goldsource = 1;
	}

	// Get source mod install path
	int foundValue = 0;
	char* sourceModPath = malloc(MAX_PATH);
	char regValue[50];

	if (goldsource) {
		// Goldsource mods
		strcpy(regValue, "ModInstallPath");
	}
	else {
		// Source mods
		strcpy(regValue, "SourceModInstallPath");
	}

#if OS_Windows
	// Windows: Query registry
	char regeditCommand[200];

	strcpy(regeditCommand, "C:\\Windows\\System32\\reg.exe query HKCU\\Software\\Valve\\Steam /v ");
	strcat(regeditCommand, regValue);
	strcat(regeditCommand, " 2>nul"); // Suppress error output

	FILE* terminal = _popen(regeditCommand, "r");
	char buf[256];
	while (fgets(buf, sizeof(buf), terminal) != 0) {
		if (strstr(buf, regValue)) {
			char* extractedValue = strstr(buf, "REG_SZ") + 10;
			strcpy(sourceModPath, extractedValue);
			foundValue = 1;
		}
	}
	_pclose(terminal);


	int sourceModDirLength = strlen(sourceModPath);
	for (int i = 0; i < sourceModDirLength; i++) {
		if (sourceModPath[i] == '\n') {
			sourceModPath[i] = '\0';
		}
	}
#else
	// Linux: Read registry.vdf
	FILE* fp_reg;
	char* line_reg = NULL;
	size_t len_reg = 0;
	size_t read_reg;

	// Fix reg value for registry.vdf
	char regValueTemp[50];
	strcpy(regValueTemp, "\"");
	strcat(regValueTemp, regValue);
	strcpy(regValue, regValueTemp);

	char* regFileLocation = getSteamBaseDir();
	regFileLocation = strreplace(regFileLocation, "/.steam/steam", "/.steam/registry.vdf");
	fp_reg = fopen(regFileLocation, "r");

	// If the file doesn't exist, skip this function
	if (fp_reg == NULL) {
		char errorMessage[500];
		sprintf(errorMessage, "File registry.vdf could not be found in %s", regFileLocation);
		free(regFileLocation);
		logError(errorMessage, 96);
		return NULL;
	}

	free(regFileLocation);

	while ((read_reg = readLine(&line_reg, &len_reg, fp_reg)) != -1) {

		// If line contains the regvalue's key, capture the value
		unsigned char* extractedValue = strstr(line_reg, regValue);

		if (extractedValue > 0) {
			extractedValue += strlen(regValue) + 1;
			extractedValue = strstr(extractedValue, "\"") + 1;

			unsigned char* extractedValueEnd = strstr(extractedValue, "\"");
			*extractedValueEnd = '\0';

			strcpy(sourceModPath, extractedValue);
			foundValue = 1;
			break;
		}
	}
	free(line_reg);

	// Replace "//" with "\"
	if (foundValue) {
		sourceModPath = strreplace(sourceModPath, "\\\\", "/");
	}
#endif 

	if (!foundValue) {
		// Allow this to fail silently, the registry key doesn't always need to be there
		return NULL;
	}

	struct dirent* dir;
	DIR* dr = opendir(sourceModPath);

	if (dr == NULL)
	{
		char errorMessage[500];
		sprintf(errorMessage, "Could not read directory %s", sourceModPath);
		free(sourceModPath);
		logError(errorMessage, 98);
		return NULL;
	}

	// Iterate through each file
	char* filepath = malloc(500);
	char** sourceModsNames = malloc(sizeof(char*) * 500000);
	int* sourceModsSteamAppIds = malloc(sizeof(char*) * 500000);
	char** sourceModsDirs = malloc(sizeof(char*) * 500000);
	unsigned long modsCount = 0;

	while ((dir = readdir(dr)) != NULL)
	{

		// If file exists, parse it
		strcpy(filepath, sourceModPath);
		strcat(filepath, "/");
		strcat(filepath, dir->d_name);

		if (goldsource) {
			// Goldsource mods
			strcat(filepath, "/liblist.gam");
		}
		else {
			// Source mods
			strcat(filepath, "/gameinfo.txt");
		}


		FILE* fp;
		char* line = NULL;
		size_t len = 0;
		size_t read;

		fp = fopen(filepath, "r");

		// If the file doesn't exist, move on to the next file
		if (fp == NULL) {
			continue;
		}

		int foundGameKey = 0;

		while ((read = readLine(&line, &len, fp)) != -1) {

			// If line contains the "game" key, get the mod's name and create the struct entry
			unsigned char* commentChar = strstr(line, "//");
			unsigned char* nameStartChar = strstr(line, "game");
			unsigned char* steamAppIdStartChar = strstr(line, "SteamAppId");

			// Make sure to first capture the "game" key, properly
			if (nameStartChar > 0 && commentChar == 0 && !foundGameKey && ((char *) nameStartChar == line || isspace(*(nameStartChar - 1))) && isspace(*(nameStartChar + 4))) {

				nameStartChar = strstr(line, "\"") + 1;
				unsigned char* nameEndChar = strstr(nameStartChar, "\"");
				*nameEndChar = '\0';

				if (nameStartChar > 0) {
					if (goldsource) {
						// Skip folders that are supposed to be skipped
						if (strcmp(dir->d_name, "bshift") == 0 ||
							strcmp(dir->d_name, "cstrike") == 0 ||
							strcmp(dir->d_name, "czero") == 0 ||
							strcmp(dir->d_name, "czeror") == 0 ||
							strcmp(dir->d_name, "dmc") == 0 ||
							strcmp(dir->d_name, "dod") == 0 ||
							strcmp(dir->d_name, "gearbox") == 0 ||
							strcmp(dir->d_name, "ricochet") == 0 ||
							strcmp(dir->d_name, "tfc") == 0 ||
							strcmp(dir->d_name, "valve") == 0)
						{
							continue;
						}
					}

					sourceModsNames[modsCount] = malloc(strlen(nameStartChar) + 1);
					sourceModsDirs[modsCount] = malloc(strlen(dir->d_name) + 1);
					strcpy(sourceModsNames[modsCount], nameStartChar);
					strcpy(sourceModsDirs[modsCount], dir->d_name);

					foundGameKey = 1;

					if (goldsource) {
						modsCount++;
						break;
					}
				}
			}
			else if (steamAppIdStartChar > 0 && foundGameKey) {

				// If line contains the "SteamAppId" key, get the mod's name and create the struct entry
				unsigned char* steamAppIdStartChar = strstr(line, "SteamAppId");

				// Extract steamAppId value
				steamAppIdStartChar = strstr(line, "SteamAppId") + 10;
				while (isspace(*steamAppIdStartChar)) {
					steamAppIdStartChar++;
				}
				unsigned char* steamAppIdEndChar = steamAppIdStartChar;
				while (!isspace(*steamAppIdEndChar)) {
					steamAppIdEndChar++;
				}
				*steamAppIdEndChar = '\0';

				if (steamAppIdStartChar > 0) {
					sourceModsSteamAppIds[modsCount] = atoi(steamAppIdStartChar);

					modsCount++;

					break;
				}
			}
		}

		fclose(fp);
		if (line)
			free(line);
	}
	closedir(dr);
	free(filepath);

	free(sourceModPath);

	if (goldsource) {
		_goldSourceModsCount = modsCount;
	}
	else {
		_sourceModsCount = modsCount;
	}

	struct nonSteamApp* sourceMods = malloc(sizeof(nonSteamApp) * modsCount);

	for (int i = 0; i < modsCount; i++) {
		sourceMods[i].index = _modsCount;
		char int_string[50];
		unsigned int hex_index = i;
		if (goldsource) {
			hex_index += _sourceModsCount; // Gold source mods mode must be called after normal source mods
		}
		sprintf(int_string, "%x", hex_index);
		hex_index = (unsigned int)strtol(int_string, NULL, 16);

		uint64_t appid_old = crcFast(sourceModsDirs[i], strlen(sourceModsDirs[i]));

		if (goldsource) {
			appid_old = (((appid_old | 0x80000000) << 32) | 0x02000000) - 0x1000000 + 70;
		}
		else {
			appid_old = (((appid_old | 0x80000000) << 32) | 0x02000000) + sourceModsSteamAppIds[i] - 0x1000000;
		}

		sprintf(sourceMods[i].appid, "%lu", 2147483649 + hex_index);
		strcpy(sourceMods[i].name, sourceModsNames[i]);
		sprintf(sourceMods[i].appid_old, "%" PRIu64, appid_old);

		free(sourceModsNames[i]);
		free(sourceModsDirs[i]);
	}

	free(sourceModsNames);
	free(sourceModsSteamAppIds);
	free(sourceModsDirs);

	return sourceMods;
}

// Parse shortcuts file and return a pointer to a list of structs containing the app data
struct nonSteamApp* getNonSteamApps() {

	char* shortcutsVdfPath = getSteamBaseDir();
	char* steamid = getMostRecentUser(shortcutsVdfPath);
	struct nonSteamApp* apps = malloc(sizeof(nonSteamApp) * 1500000);
	crcInit();

	// Get the shortcuts.vdf file
	strcat(shortcutsVdfPath, "/userdata/");
	strcat(shortcutsVdfPath, steamid);
	strcat(shortcutsVdfPath, "/config/shortcuts.vdf");

	free(steamid);

	// Parse the file
	FILE* fp;
	unsigned char buf[2] = { 0 };
	size_t bytes = 0;
	size_t read = sizeof buf;
	fp = fopen(shortcutsVdfPath, "rb");
	free(shortcutsVdfPath);
	if (fp != NULL) {
		fseek(fp, 0L, SEEK_END);
		size_t filesize = ftell(fp) + 2;
		fseek(fp, 0, SEEK_SET);

		unsigned char* fileContent = malloc(filesize + 1);
		memset(fileContent, '\xAB', filesize);
		unsigned char* realFileContent = malloc(filesize + 1);
		unsigned int currentFileByte = 0;

		// Load the vdf in memory and fix string-related issues
		while ((bytes = fread(buf, sizeof * buf, read, fp)) == read) {
			for (int i = 0; i < sizeof buf; i++) {
				if (buf[i] == 0x00) {
					fileContent[currentFileByte] = 0x03;
				}
				else {
					fileContent[currentFileByte] = buf[i];
				}
				realFileContent[currentFileByte] = buf[i];
				currentFileByte++;
			}
		}
		fileContent[filesize - 2] = '\x08';
		fileContent[filesize - 1] = '\x03';
		fileContent[filesize] = '\0';

		fclose(fp);
		unsigned char* parsingChar = fileContent;
		unsigned char parsingAppid[512];
		uint64_t intBytes[4];

		// Parse the vdf content
		while ((parsingChar - fileContent) < filesize && strstr_i(parsingChar, "appname") > 0) {

			uint64_t appid_old = 0;
			uint64_t appid = 0;

			// Find app name
			unsigned char* nameStartChar = strstr_i(parsingChar, "\001appname") + 9;
			unsigned char* nameEndChar = strstr(nameStartChar, "\x03");

			// Find exe path
			unsigned char* exeStartChar = strstr_i(parsingChar, "\001exe") + 5;
			unsigned char* exeEndChar = strstr(exeStartChar, "\x03");

			unsigned char* appidPtr = strstr_i(parsingChar, "\002appid");
			unsigned char* appBlockEndPtr = strstr(parsingChar, "\x08") + 1; // gcc fucks with optimization on strstr for 2 consecutive hex values. DON'T EDIT THIS.
			while (*appBlockEndPtr != 0x03 && *appBlockEndPtr != 0x00) {
				appBlockEndPtr = strstr(appBlockEndPtr, "\x08") + 1;
			}

			// If appid was found in this app block
			if (appidPtr > 0 && appidPtr < appBlockEndPtr) {
				unsigned char* hexBytes = realFileContent + (appidPtr - fileContent) + 7;
				intBytes[0] = *(hexBytes + 3);
				intBytes[1] = *(hexBytes + 2);
				intBytes[2] = *(hexBytes + 1);
				intBytes[3] = *(hexBytes + 0);

				appid =
					((uint64_t)intBytes[0] << 24) |
					((uint64_t)intBytes[1] << 16) |
					((uint64_t)intBytes[2] << 8) |
					((uint64_t)intBytes[3] << 0);
			}

			// Calculate old app id
			*nameEndChar = '\0';
			*exeEndChar = '\0';

			strcpy(parsingAppid, exeStartChar);
			strcat(parsingAppid, nameStartChar);
			appid_old = crcFast(parsingAppid, strlen(parsingAppid));
			if (appid == 0) {
				appid = appid_old;
			}

			*exeEndChar = '\x03';

			// Do math magic. Valve pls fix
			appid = (((appid | 0x80000000) << 32) | 0x02000000) >> 32;
			appid_old = (((appid_old | 0x80000000) << 32) | 0x02000000);
			sprintf(apps[_nonSteamAppsCount].appid_old, "%" PRIu64, appid_old);

			// Add the values to struct
			apps[_nonSteamAppsCount].index = _nonSteamAppsCount;
			strcpy(apps[_nonSteamAppsCount].name, nameStartChar);
			sprintf(apps[_nonSteamAppsCount].appid, "%lu", (unsigned long)appid);
			strcpy(apps[_nonSteamAppsCount].type, "nonsteam-app");
			_nonSteamAppsCount++;

			// Move parser to end of app data
			*nameEndChar = 0x03; // Revert name string to prevent string-related problems
			parsingChar = appBlockEndPtr + 2;
		}

		free(fileContent);
		free(realFileContent);
	}

	return apps;
}

struct nonSteamApp* getMods() {
	struct nonSteamApp* apps = malloc(sizeof(nonSteamApp) * 1500000);
	struct nonSteamApp* sourceMods = getSourceMods("source");
	struct nonSteamApp* goldSourceMods = getSourceMods("goldsource");
	for (int i = 0; i < _sourceModsCount; i++) {
		apps[_modsCount].index = _modsCount;
		strcpy(apps[_modsCount].name, sourceMods[i].name);
		strcpy(apps[_modsCount].appid_old, sourceMods[i].appid_old);
		strcpy(apps[_modsCount].appid, sourceMods[i].appid);
		strcpy(apps[_modsCount].type, "source-mod");

		_modsCount++;
	}
	for (int i = 0; i < _goldSourceModsCount; i++) {
		apps[_modsCount].index = _modsCount;
		strcpy(apps[_modsCount].name, goldSourceMods[i].name);
		strcpy(apps[_modsCount].appid_old, goldSourceMods[i].appid_old);
		strcpy(apps[_modsCount].appid, goldSourceMods[i].appid);
		strcpy(apps[_modsCount].type, "goldsource-mod");

		_modsCount++;
	}
	free(sourceMods);
	free(goldSourceMods);

	return apps;
}

// Select a non-steam app from a dropdown list and return its ID
struct nonSteamApp* selectNonSteamApp(char* sgdbName, struct nonSteamApp* apps, struct nonSteamApp* appsMods) {

	char* appid = malloc(128);

	char** values = malloc(sizeof(char*) * _nonSteamAppsCount);
	for (int i = 0; i < _nonSteamAppsCount; i++) {
		values[i] = malloc(strlen(apps[i].name) + 1);
		strcpy(values[i], apps[i].name);
	}

	char** modsValues = malloc(sizeof(char*) * _modsCount);
	for (int i = 0; i < _modsCount; i++) {
		modsValues[i] = malloc(strlen(appsMods[i].name) + 1);
		strcpy(modsValues[i], appsMods[i].name);
	}

	struct nonSteamApp* appData = malloc(sizeof(nonSteamApp));

	// Create title string
	char* title = malloc(40 + strlen(sgdbName));
	strcpy(title, "SGDBoop: Pick a game for '");
	strcat(title, sgdbName);
	strcat(title, "'");

	// Sort values
	qsort(values, _nonSteamAppsCount, sizeof(const char*), compareStrings);
	qsort(modsValues, _modsCount, sizeof(const char*), compareStrings);

	// Find matching value
	int selection = -1;
	for (int i = 0; i < _nonSteamAppsCount; i++) {
		if (strcmp_i(values[i], sgdbName) == 0) {
			selection = i;
			break;
		}
	}

	if (selection < 0) {
		for (int i = 0; i < _modsCount; i++) {
			if (strcmp_i(modsValues[i], sgdbName) == 0) {
				selection = i + _nonSteamAppsCount;
				break;
			}
		}
	}

	// Create selection dialog
	int retval = SelectionDialog(title, _nonSteamAppsCount, (const char**)values, _modsCount, (const char**)modsValues, selection);

	// Exit when user clicks cancel
	if (retval < 0) {
		free(apps);
		free(values);
		free(appsMods);
		free(modsValues);
		free(title);
		exit(0);
	}


	// Find match
	struct nonSteamApp * matchedStruct;
	char ** matchedValues;
	if (retval >= _nonSteamAppsCount) {
		retval -= _nonSteamAppsCount;
		matchedStruct = appsMods;
		matchedValues = modsValues;
	} else {
		matchedStruct = apps;
		matchedValues = values;
	}

	for (int i = 0; i < _nonSteamAppsCount + _modsCount; i++) {
		if (strcmp(matchedStruct[i].name, matchedValues[retval]) == 0) {
			strcpy(appData->appid, matchedStruct[i].appid);
			strcpy(appData->name, matchedStruct[i].name);
			appData->index = matchedStruct[i].index;
			strcpy(appData->appid_old, matchedStruct[i].appid_old);
			strcpy(appData->type, matchedStruct[i].type);
			break;
		}
	}

	free(apps);
	free(values);
	free(appsMods);
	free(modsValues);
	free(title);

	return appData;
}

// Create a symlink for a file that has the old nonsteam appid format
int createOldIdSymlink(struct nonSteamApp* appData, char* steamDestDir) {
	char linkPath[MAX_PATH];
	char targetPath[MAX_PATH];

	strcpy(linkPath, steamDestDir);
	strcat(linkPath, appData->appid_old);
	strcat(linkPath, ".jpg");

	strcpy(targetPath, steamDestDir);
	strcat(targetPath, appData->appid);
	strcat(targetPath, ".jpg");

	return symlink(targetPath, linkPath);
}

// Update shortcuts.vdf with the new icon value
void updateVdf(struct nonSteamApp* appData, char* filePath) {
	char* shortcutsVdfPath = getSteamBaseDir();
	char* steamid = getMostRecentUser(shortcutsVdfPath);

	// Get the shortcuts.vdf file
	strcat(shortcutsVdfPath, "/userdata/");
	strcat(shortcutsVdfPath, steamid);
	strcat(shortcutsVdfPath, "/config/shortcuts.vdf");

	free(steamid);

	// Parse the file
	FILE* fp;
	unsigned char buf[1] = { 0 };
	size_t bytes = 0;
	size_t read = sizeof buf;
	fp = fopen(shortcutsVdfPath, "rb");
	free(shortcutsVdfPath);
	if (fp == NULL) {
		exitWithError("Shortcuts vdf could not be found.", 93);
	}
	fseek(fp, 0L, SEEK_END);
	size_t filesize = ftell(fp) + 2;
	fseek(fp, 0, SEEK_SET);

	unsigned char* fileContent = malloc(filesize + 1);
	unsigned char* new_fileContent = malloc(filesize + 513);
	unsigned int currentFileByte = 0;

	// Load the vdf in memory and fix string-related issues
	while ((bytes = fread(buf, sizeof * buf, read, fp)) == read) {
		if (buf[0] == 0x00) {
			fileContent[currentFileByte] = 0x03;
		}
		else {
			fileContent[currentFileByte] = buf[0];
		}
		currentFileByte++;
	}
	fileContent[filesize - 2] = '\x08';
	fileContent[filesize - 1] = '\x03';
	fileContent[filesize] = '\0';
	fclose(fp);

	unsigned char* appBlockStart = fileContent;
	unsigned char* appBlockEndPtr = fileContent;
	unsigned char* iconStartChar = 0;
	unsigned char* iconEndChar = 0;
	char iconContent[512];
	strcpy(iconContent, "\001icon\003");
	strcat(iconContent, filePath);
	strcat(iconContent, "\003");

	// Find the app's block
	for (int i = 0; i < appData->index + 1; i++) {
		appBlockStart = appBlockEndPtr;
		appBlockEndPtr = strstr(appBlockStart, "\x08") + 1; // gcc fucks with optimization on strstr for 2 consecutive hex values. DON'T EDIT THIS.
		while (*appBlockEndPtr != 0x03 && *appBlockEndPtr != 0x00) {
			appBlockEndPtr = strstr(appBlockEndPtr, "\x08") + 1;
		}
	}

	if (appBlockStart > 0) {

		// Find icon key
		iconStartChar = strstr_i(appBlockStart, "\001icon");
		iconEndChar = strstr(iconStartChar, "\x03") + 1;
		iconEndChar = strstr(iconEndChar, "\x03");

		// Find exe key
		unsigned char* exeStartChar = strstr_i(appBlockStart, "\001exe") + 5;
		unsigned char* exeEndChar = strstr(exeStartChar, "\x03");

		if (iconStartChar == 0 || iconStartChar > appBlockEndPtr) {
			// Didn't find icon block
			iconStartChar = exeEndChar + 1;
			iconEndChar = iconStartChar;
		}

		*iconStartChar = '\0';

		// Set the new file contents
		strcpy(new_fileContent, fileContent);
		strcat(new_fileContent, iconContent);
		strcat(new_fileContent, iconEndChar + 1);

		// Write the file back
		FILE* fp_w = fopen(shortcutsVdfPath, "wb");
		if (fp_w == NULL) {
			free(fileContent);
			free(new_fileContent);
			free(shortcutsVdfPath);
			exitWithError("Could not write to shortcuts vdf file.", 94);
		}

		int newFileSize = strlen(new_fileContent) - 2;

		for (int i = 0; i < newFileSize; i++) {
			// Revert 0x03 to 0x00
			if (new_fileContent[i] == 0x03) {
				new_fileContent[i] = 0x00;
			}

			// Write byte to file
			fwrite(&new_fileContent[i], sizeof(char), 1, fp_w);
		}

		fclose(fp_w);

		free(fileContent);
		free(new_fileContent);
	}
}

// Build as Windows app on windows instead of console
#if OS_Windows
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	int argc = __argc;
	char** argv = __argv;
#else
int main(int argc, char** argv)
{
#endif

	// If no arguments were given, register the program
	if (argc == 0 || (argc == 1 && !startsWith(argv[0], "sgdb://"))) {
		// Create the sgdb URI protocol
		if (createURIprotocol() == 1) {
			exitWithError("Could not create URI protocol.", 80);
		}
	}
	else {

		// If argument is unregister, unregister and exit
		if (strcmp(argv[1], "unregister") == 0) {
			if (deleteURIprotocol() == 1) {
				exitWithError("Could not unregister the URI protocol.", 85);
			}
			return 0;
		}

		// If sgdb:// arguments were passed, run program normally

		// If the arguments aren't of the SGDB URI, return with an error
		if (!startsWith(argv[1], "sgdb://boop")) {
			exitWithError("Invalid URI schema.", 81);
		}

		// Test mode
		if (strcmp(argv[1], "sgdb://boop/test") == 0) {
			// Show a message
			#if OS_Windows
			ShowMessageBoxW(L"SGDBoop Test", L"(ノ◕ヮ◕)ノ*:・゚✧     SGDBoop is working!    ★・゚:*ヽ(◕ヮ◕ヽ)");
			#else
			ShowMessageBox("SGDBoop Test", "(ノ◕ヮ◕)ノ*:・゚✧   SGDBoop is working!  ★・゚:*ヽ(◕ヮ◕ヽ)");
			#endif
			return 0;
		}

		// Get the params from the string
		char* types = strstr(argv[1], "sgdb://boop/") + strlen("sgdb://boop/");
		char* grid_ids = strstr(types, "/");
		grid_ids[0] = '\0';         // End app_id string
		grid_ids += 1;              // Move 1 place

		char* mode = strstr(grid_ids, "/"); // If there's a method string, use it
		if (mode > 0) {
			*mode = '\0';
			mode++;
		}
		else {
			mode = malloc(sizeof("default") + 1);
			strcpy(mode, "default");
		}

		// Get asset URL
		char*** apiValues = callAPI(types, grid_ids, mode);
		if (apiValues == NULL) {
			exitWithError("API didn't return an appropriate response.", 82);
		}

		// Use the same nonSteamApp object for all calls
		struct nonSteamApp* nonSteamAppData = NULL;

		for (int line = 0; line < _apiReturnedLines; line++) {

			char* app_id = apiValues[line][0];
			char* orientation = apiValues[line][1];
			char* assetUrl = apiValues[line][2];
			char* asset_type = apiValues[line][3];


			// If the game is a non-steam app, select an imported app
			if (startsWith(app_id, "nonsteam-")) {
				// Select app once
				if (line < 1) {
					// Do not include mods in the dropdown list if the only asset selected was an icon
					int includeMods = TRUE;
					if (strcmp(types, "icon") == 0 || (strcmp(types, "steam") == 0 && strcmp(asset_type, "icon") == 0)) {
						includeMods = FALSE;
					}

					// Get non-steam apps
					struct nonSteamApp* apps = getNonSteamApps();
					struct nonSteamApp* appsMods = NULL;
					if (includeMods) {
						appsMods = getMods(includeMods);
					}

					// Exit with an error if nothing found
					if (_nonSteamAppsCount < 1 && _modsCount < 1) {
						ShowMessageBox("SGDBoop Error", "Could not find any non-Steam apps or mods.");
						free(apps);
						free(appsMods);
						exitWithError("Could not find any non-Steam apps or mods.", 91);
					}

					// Show selection screen and return the struct
					nonSteamAppData = selectNonSteamApp(strstr(app_id, "-") + 1, apps, appsMods);
				}

				// Skip icons for source/goldsource mods
				if (strcmp(asset_type, "icon") == 0 &&
					(strcmp(nonSteamAppData->type, "source-mod") == 0 || strcmp(nonSteamAppData->type, "goldsource-mod") == 0)) {
					continue;
				}

				app_id = nonSteamAppData->appid;
			}

			// Get Steam base dir
			char* steamDestDir = getSteamDestinationDir(asset_type, nonSteamAppData);
			if (steamDestDir == NULL) {
				exitWithError("Could not locate Steam destination directory.", 83);
			}

			// Download asset file
			char* outfilename = downloadAssetFile(app_id, assetUrl, asset_type, orientation, steamDestDir, nonSteamAppData);
			if (outfilename == NULL) {
				exitWithError("Could not download asset file.", 84);
			}

			// Non-Steam specific actions
			if (nonSteamAppData) {
				// If the asset is a non-Steam icon, add the path to the vdf
				if (strcmp(asset_type, "icon") == 0) {
					updateVdf(nonSteamAppData, outfilename);
				}

			}

			free(steamDestDir);
		}

		free(nonSteamAppData);
	}

	return 0;
}
