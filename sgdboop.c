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
#include "include/iup.h"
#include "crc.h"


#ifdef __unix__                             /* __unix__ is usually defined by compilers targeting Unix systems */
#define OS_Windows 0
#define MAX_PATH 600 
#define FALSE 0
#define TRUE 1
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

#define API_VERSION "1"

typedef struct nonSteamApp
{
	int index;
	char name[300];
	char appid[128];
	char appid_old[128];
} nonSteamApp;

int _nonSteamAppsCount = 0;
int _sourceModsCount = 0;
int _goldSourceModsCount = 0;

void exitWithError(char* error, int errorCode)
{
	time_t now = time(0);
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char* logFilepath = malloc(1000);
	if (OS_Windows) {
		strcpy(logFilepath, "sgdboop_error.log");
	}
	else {
		strcpy(logFilepath, "/var/log/sgdboop_error.log");
	}

	FILE* logFile = fopen(logFilepath, "a");
	if (logFile == NULL) {
		logFile = fopen(logFilepath, "w");
	}

	fprintf(logFile, "%s%s [%d]\n\n", asctime(timeinfo), error, errorCode);
	fclose(logFile);

	exit(errorCode);
}

// Call the BOOP API
char** callAPI(char* grid_type, char* grid_id, char* mode)
{
	char authHeader[] = "Authorization: Bearer 62696720-6f69-6c79-2070-65656e75733f";
	char apiVersionHeader[20] = "X-BOOP-API-VER: ";
	strcat(apiVersionHeader, API_VERSION);
	char* url = malloc(512);
	char** valuesArray = malloc(1024);

	strcpy(url, "https://www.steamgriddb.com/api/sgdboop/");
	strcat(url, grid_type);
	strcat(url, "/");
	strcat(url, grid_id);

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
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, TRUE);
		headers = curl_slist_append(headers, authHeader);
		headers = curl_slist_append(headers, apiVersionHeader);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if (res != CURLE_OK)
		{
			return NULL;
		}

		valuesArray[0] = s.ptr;
		valuesArray[1] = strstr(s.ptr, ",");
		valuesArray[1][0] = '\0';
		valuesArray[1] += 1;
		valuesArray[2] = strstr(valuesArray[1], ",");
		valuesArray[2][0] = '\0';
		valuesArray[2] += 1;

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
		strcat(outfilename, "_hero.jpg");
	}
	else if (strcmp(type, "logo") == 0) {
		// Logo
		strcat(outfilename, "_logo.jpg");
	}
	else if (strcmp(type, "grid") == 0 && strcmp(orientation, "p") == 0) {
		// Vertical grid
		strcat(outfilename, "p.jpg");
	}
	else if (strcmp(type, "grid") == 0) {
		// Horizontal grid
		strcat(outfilename, ".jpg");
	}
	else if (strcmp(type, "icon") == 0) {
		// Icon
		if (nonSteamAppData == NULL) {
			// Inject Steam's cache
			strcat(outfilename, "_icon.jpg");
		}
		else {
			// Add new icon to grid folder using its original extension
			strcat(outfilename, "_icon.");
			char* extension = strstr(url, ".com/") + 5;
			extension = strstr(extension, ".") + 1;
			strcat(outfilename, extension);
		}
	}

	curl = curl_easy_init();
	if (curl && outfilename != 0) {
		fp = fopen(outfilename, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, TRUE);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
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
	if (OS_Windows) {
		char cwd[MAX_PATH];
		GetModuleFileName(NULL, cwd, MAX_PATH);

		char* regeditCommand = malloc(2028);
		strcpy(regeditCommand, "REG ADD HKCR\\sgdb\\Shell\\Open\\Command /t REG_SZ /d \"\\\"");
		strcat(regeditCommand, cwd);
		strcat(regeditCommand, "\\\" \\\"%1\\\" -new_console:z\" /f");

		int ret_val = system("REG ADD HKCR\\sgdb /t REG_SZ /d \"URL:sgdb protocol\" /f");
		if (ret_val != 0) {
			IupMessage("SGDBoop Error", "Please run this program as Administrator to register it!\n");
			free(regeditCommand);
			return 1;
		}

		system(regeditCommand);

		strcpy(regeditCommand, "REG ADD HKCR\\sgdb\\DefaultIcon /t REG_SZ /d \"");
		strcat(regeditCommand, cwd);
		strcat(regeditCommand, "\" /f");
		system(regeditCommand);

		system("REG ADD HKCR\\sgdb /v \"URL Protocol\" /t REG_SZ /d \"\" /f");

		IupMessage("SGDBoop Information", "Program registered successfully!\n\nSGDBoop is meant to be run from a browser!\nHead over to https://www.steamgriddb.com/boop to continue setup.");
		free(regeditCommand);
		return 0;
	}
	else {
		// Do nothing on linux
		IupMessage("SGDBoop Information", "SGDBoop is meant to be run from a browser!\nHead over to https://www.steamgriddb.com/boop to continue setup.");
		return 1;
	}
}

// Delete the SGDB URI protocol
int deleteURIprotocol() {
	if (OS_Windows) {

		int ret_val = system("REG DELETE HKCR\\sgdb /f");
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
	}
	else {
		// Do nothing on linux
		printf("A SGDB URL argument is required.\nExample: SGDBoop sgdb://boop/[ASSET_TYPE]/[ASSET_ID]\n");
		return 1;
	}
}

// Get Steam's base directory
char* getSteamBaseDir() {

	char* steamBaseDir = malloc(MAX_PATH);
	int foundValue = 0;

	if (OS_Windows) {
		FILE* terminal = _popen("reg query HKCU\\Software\\Valve\\Steam /v SteamPath", "r");
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
	}
	else {
		foundValue = 1;
		strcpy(steamBaseDir, getenv("HOME"));

		char steamFlatpakDir[MAX_PATH];
		strcpy(steamFlatpakDir, getenv("HOME"));
		strcat(steamFlatpakDir, "/.var/app/com.valvesoftware.Steam/data/Steam");

		// If flatpaked Steam is installed
		if (access(steamFlatpakDir, 0) == 0) {
			strcat(steamBaseDir, "/.var/app/com.valvesoftware.Steam/data/Steam");
		}
		else {
			// Steam installed on host
			strcat(steamBaseDir, "/.steam/steam");
		}
	}

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
	if (fp == NULL) {
		free(steamid);
		free(steamConfigFile);
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

	if (OS_Windows) {
		// Windows: Query registry
		char regeditCommand[200];

		strcpy(regeditCommand, "reg query HKCU\\Software\\Valve\\Steam /v ");
		strcat(regeditCommand, regValue);

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
	}
	else {
		// Linux: Read registry.vdf
		FILE* fp_reg;
		char* line_reg = NULL;
		size_t len_reg = 0;
		size_t read_reg;

		fp_reg = fopen("~/.steam/steam/registry.vdf", "r");
		
		// If the file doesn't exist, skip this function
		if (fp_reg == NULL) {
			return NULL;
		}

		while ((read_reg = readLine(&line_reg, &len_reg, fp_reg)) != -1) {

			// If line contains the regvalue's key, capture the value
			unsigned char* extractedValue = strstr(line_reg, regValue);

			if (extractedValue > 0) {
				extractedValue += strlen(regValue) + 1;
				extractedValue = strstr(extractedValue, "\"");
				unsigned char* extractedValueEnd = strstr(extractedValue, "\"");
				*extractedValueEnd = '\0';

				strcpy(sourceModPath, extractedValue);
				foundValue = 1;
				break;
			}
		}
	}
	
	if (!foundValue) {
		free(sourceModPath);
		return NULL;
	}

	struct dirent* dir;
	DIR* dr = opendir(sourceModPath);

	if (dr == NULL)
	{
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

		while ((read = readLine(&line, &len, fp)) != -1) {


			// If line contains the "game" key, get the mod's name and create the struct entry
			unsigned char* commentChar = strstr(line, "//");
			unsigned char* nameStartChar = strstr(line, "game");
			unsigned char* steamAppIdStartChar = strstr(line, "SteamAppId");

			if (nameStartChar > 0 && commentChar == 0) {

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

					if (goldsource) {
						modsCount++;
						break;
					}
				}
			}
			else if (steamAppIdStartChar > 0) {

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

	if (goldsource) {
		_goldSourceModsCount = modsCount;
	}
	else {
		_sourceModsCount = modsCount;
	}

	struct nonSteamApp* sourceMods = malloc(sizeof(nonSteamApp) * modsCount);

	for (int i = 0; i < modsCount; i++) {
		sourceMods[i].index = _nonSteamAppsCount;
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
	}

	return sourceMods;
}

// Parse shortcuts file and return a pointer to a list of structs containing the app data
struct nonSteamApp* getNonSteamApps(char* type, char* orientation) {

	char* shortcutsVdfPath = getSteamBaseDir();
	char* steamid = getMostRecentUser(shortcutsVdfPath);
	struct nonSteamApp* apps = malloc(sizeof(nonSteamApp) * 1500000);
	crcInit();

	// Get the shortcuts.vdf file
	strcat(shortcutsVdfPath, "/userdata/");
	strcat(shortcutsVdfPath, steamid);
	strcat(shortcutsVdfPath, "/config/shortcuts.vdf");

	int old_id_required = 0;
	if (strcmp(type, "grid") == 0 && strcmp(orientation, "l") == 0) {
		old_id_required = 1;
	}

	// Parse the file
	FILE* fp;
	unsigned char buf[2] = { 0 };
	size_t bytes = 0;
	size_t read = sizeof buf;
	fp = fopen(shortcutsVdfPath, "rb");
	if (fp != NULL) {
		fseek(fp, 0L, SEEK_END);
		size_t filesize = ftell(fp) + 2;
		fseek(fp, 0, SEEK_SET);

		unsigned char* fileContent = malloc(filesize + 1);
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
		while (strstr_i(parsingChar, "appname") > 0) {

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
				unsigned char* hexBytes = appidPtr + 7;
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

			// If an old appid is required, calculate it
			if (appid == 0 || old_id_required) {

				*nameEndChar = '\0';
				*exeEndChar = '\0';

				strcpy(parsingAppid, exeStartChar);
				strcat(parsingAppid, nameStartChar);
				appid_old = crcFast(parsingAppid, strlen(parsingAppid));
				if (appid == 0) {
					appid = appid_old;
				}

				*exeEndChar = '\x03';
			}

			*nameEndChar = '\0'; // Close name string

			// Do math magic. Valve pls fix
			appid = (((appid | 0x80000000) << 32) | 0x02000000) >> 32;

			if (old_id_required) {
				appid_old = (((appid_old | 0x80000000) << 32) | 0x02000000);
				sprintf(apps[_nonSteamAppsCount].appid_old, "%" PRIu64, appid_old);
			}
			else {
				strcpy(apps[_nonSteamAppsCount].appid_old, "none");
			}

			// Add the values to struct
			apps[_nonSteamAppsCount].index = _nonSteamAppsCount;
			strcpy(apps[_nonSteamAppsCount].name, nameStartChar);
			sprintf(apps[_nonSteamAppsCount].appid, "%lu", (unsigned long)appid);
			_nonSteamAppsCount++;

			// Move parser to end of app data
			*nameEndChar = 0x03; // Revert name string to prevent string-related problems
			parsingChar = appBlockEndPtr + 2;
		}
	}
	
	// Add source (and goldsource) mods
	struct nonSteamApp* sourceMods = getSourceMods("source");
	struct nonSteamApp* goldSourceMods = getSourceMods("goldsource");
	for (int i = 0; i < _sourceModsCount; i++) {
		apps[_nonSteamAppsCount].index = _nonSteamAppsCount;
		strcpy(apps[_nonSteamAppsCount].name, sourceMods[i].name);
		strcpy(apps[_nonSteamAppsCount].appid_old, sourceMods[i].appid_old);
		strcpy(apps[_nonSteamAppsCount].appid, sourceMods[i].appid);

		_nonSteamAppsCount++;
	}
	for (int i = 0; i < _goldSourceModsCount; i++) {
		apps[_nonSteamAppsCount].index = _nonSteamAppsCount;
		strcpy(apps[_nonSteamAppsCount].name, goldSourceMods[i].name);
		strcpy(apps[_nonSteamAppsCount].appid_old, goldSourceMods[i].appid_old);
		strcpy(apps[_nonSteamAppsCount].appid, goldSourceMods[i].appid);

		_nonSteamAppsCount++;
	}
	

	// Exit with an error if no non-steam apps were found
	if (_nonSteamAppsCount < 1) {
		IupMessage("SGDBoop Error", "Could not find any non-Steam apps.");
		free(apps);
		exitWithError("Could not find any non-Steam apps in the according file.", 91);
	}

	return apps;
}

// Select a non-steam app from a dropdown list and return its ID
struct nonSteamApp* selectNonSteamApp(char* sgdbName, struct nonSteamApp* apps) {

	char* appid = malloc(128);

	char** values = malloc(sizeof(char*) * _nonSteamAppsCount);
	for (int i = 0; i < _nonSteamAppsCount; i++) {
		values[i] = malloc(strlen(apps[i].name) + 1);
		strcpy(values[i], apps[i].name);
	}

	struct nonSteamApp* appData = malloc(sizeof(nonSteamApp));

	char* title = malloc(40 + strlen(sgdbName));
	strcpy(title, "SGDBoop: Pick a game for '");
	strcat(title, sgdbName);
	strcat(title, "'");

	qsort(values, _nonSteamAppsCount, sizeof(const char*), compareStrings);

	int selection = 0;
	for (int i = 0; i < _nonSteamAppsCount; i++) {
		if (strcmp(values[i], sgdbName) == 0) {
			selection = i + 1;
			break;
		}
	}

	int retval = IupListDialog(1, title, _nonSteamAppsCount, (const char**)values, selection, strlen(title) - 12, 14, NULL);

	// Exit when user clicks cancel
	if (retval < 0) {
		free(apps);
		free(values);
		free(title);
		exit(0);
	}

	// Find match
	for (int i = 0; i < _nonSteamAppsCount; i++) {
		if (strcmp(apps[i].name, values[retval]) == 0) {
			strcpy(appData->appid, apps[i].appid);
			strcpy(appData->name, apps[i].name);
			appData->index = apps[i].index;
			if (strcmp(apps[i].appid_old, "none") != 0) {
				strcpy(appData->appid_old, apps[i].appid_old);
			}
			break;
		}
	}

	free(apps);
	free(values);
	free(title);

	return appData;
}

// Create a symlink for a file that has the old nonsteam appid format
void createOldIdSymlink(struct nonSteamApp* appData, char* steamDestDir) {
	char linkPath[MAX_PATH];
	char targetPath[MAX_PATH];

	strcpy(linkPath, steamDestDir);
	strcat(linkPath, appData->appid_old);
	strcat(linkPath, ".jpg");

	strcpy(targetPath, steamDestDir);
	strcat(targetPath, appData->appid);
	strcat(targetPath, ".jpg");

	int result = symlink(targetPath, linkPath);
}

// Update shortcuts.vdf with the new icon value
void updateVdf(struct nonSteamApp* appData, char* filePath) {

	char* shortcutsVdfPath = getSteamBaseDir();
	char* steamid = getMostRecentUser(shortcutsVdfPath);

	// Get the shortcuts.vdf file
	strcat(shortcutsVdfPath, "/userdata/");
	strcat(shortcutsVdfPath, steamid);
	strcat(shortcutsVdfPath, "/config/shortcuts.vdf");

	// Parse the file
	FILE* fp;
	unsigned char buf[1] = { 0 };
	size_t bytes = 0;
	size_t read = sizeof buf;
	fp = fopen(shortcutsVdfPath, "rb");
	if (fp == NULL) {
		free(shortcutsVdfPath);
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
	strcat(iconContent, "\"");
	strcat(iconContent, filePath);
	strcat(iconContent, "\"\003");

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

// Add an icon to IUP windows
void loadIupIcon() {
	unsigned char image_data[256] = { 0 };

	Ihandle* sgdboop_image = IupImage(16, 16, image_data);
	IupSetAttribute(sgdboop_image, "0", "BGCOLOR");
	IupSetHandle("SGDBOOPIMAGE", sgdboop_image);
	IupSetGlobal("ICON", "SGDBOOPIMAGE");
}

int main(int argc, char** argv)
{
	// If no arguments were given, register the program
	if (OS_Windows) {
		MoveWindow(GetConsoleWindow(), -3000, -3000, 0, 0, FALSE);
	}

	if (argc == 0 || (argc == 1 && !startsWith(argv[0], "sgdb://"))) {
		// Enable IUP GUI
		IupOpen(&argc, &argv);
		loadIupIcon();

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

		// Get the params from the string
		char* type = strstr(argv[1], "sgdb://boop/") + strlen("sgdb://boop/");
		char* grid_id = strstr(type, "/");
		grid_id[0] = '\0';         // End app_id string
		grid_id += 1;              // Move 1 place

		char* mode = strstr(grid_id, "/"); // If there's a method string, use it
		if (mode > 0) {
			*mode = '\0';
			mode++;
		}
		else {
			mode = malloc(sizeof("default") + 1);
			strcpy(mode, "default");
		}

		// Get asset URL
		char** apiValues = callAPI(type, grid_id, mode);
		if (apiValues == NULL) {
			exitWithError("API didn't return an appropriate response.", 82);
		}
		char* app_id = apiValues[0];
		char* orientation = apiValues[1];
		char* assetUrl = apiValues[2];

		struct nonSteamApp* nonSteamAppData = NULL;

		// If the game is a non-steam app, select an imported app
		if (startsWith(app_id, "nonsteam-")) {

			// Enable IUP GUI
			IupOpen(&argc, &argv);
			loadIupIcon();

			// Get non-steam apps
			struct nonSteamApp* apps = getNonSteamApps(type, orientation);
			// Show selection screen and return the appid
			nonSteamAppData = selectNonSteamApp(strstr(app_id, "-") + 1, apps);

			app_id = nonSteamAppData->appid;
		}

		// Get Steam base dir
		char* steamDestDir = getSteamDestinationDir(type, nonSteamAppData);
		if (steamDestDir == NULL) {
			exitWithError("Could not locate Steam destination directory.", 83);
		}

		// Download asset file
		char* outfilename = downloadAssetFile(app_id, assetUrl, type, orientation, steamDestDir, nonSteamAppData);
		if (outfilename == NULL) {
			exitWithError("Could not download asset file.", 84);
		}

		// Non-Steam specific actions
		if (nonSteamAppData) {

			// If the asset is a non-Steam horizontal grid, create a symlink (for back. compat.)
			if (strcmp(type, "grid") == 0 && strcmp(orientation, "l") == 0) {
				createOldIdSymlink(nonSteamAppData, steamDestDir);
			}

			// If the asset is a non-Steam icon, add the 
			else if (strcmp(type, "icon") == 0) {
				updateVdf(nonSteamAppData, outfilename);
			}

			free(nonSteamAppData);
		}
	}

	return 0;
}
