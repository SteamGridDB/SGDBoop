#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include "string-helpers.h"
#include "curl-helper.h"
#include "include/iup.h"

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
#endif

#define API_VERSION "1"

typedef struct nonSteamApp
{
	int index;
	char name[300];
	unsigned int appid;
} nonSteamApp;

int _nonSteamGamesCount = 0;

// Call the BOOP API
char** callAPI(char* grid_type, char* grid_id)
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
char* downloadAssetFile(char* app_id, char* url, char* type, char* orientation, char* destinationDir)
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
		strcat(outfilename, "_hero.png");
	}
	else if (strcmp(type, "logo") == 0) {
		// Logo
		strcat(outfilename, "_logo.png");
	}
	else if (strcmp(type, "grid") == 0 && strcmp(orientation, "p") == 0) {
		// Vertical grid
		strcat(outfilename, "p.png");
	}
	else if (strcmp(type, "grid") == 0) {
		// Horizontal grid
		strcat(outfilename, ".png");
	}
	else if (strcmp(type, "icon") == 0) {
		// Icon
		strcat(outfilename, "_icon.jpg");
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
			return NULL;
		}

		return outfilename;
	}

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
			system("cls");
			printf("Please run this program as Administrator!\n");
			system("pause");
			free(regeditCommand);
			return 1;
		}

		system(regeditCommand);

		strcpy(regeditCommand, "REG ADD HKCR\\sgdb\\DefaultIcon /t REG_SZ /d \"");
		strcat(regeditCommand, cwd);
		strcat(regeditCommand, "\" /f");
		system(regeditCommand);

		system("REG ADD HKCR\\sgdb /v \"URL Protocol\" /t REG_SZ /d \"\" /f");

		system("cls");
		printf("Program registered successfully!\n");
		system("pause");
		free(regeditCommand);
		return 0;
	}
	else {
		// Do nothing on linux
		printf("A SGDB URL argument is required.\nExample: SGDBoop sgdb://boop/[ASSET_TYPE]/[ASSET_ID]\n");
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
		free(steamBaseDir);
		exit(EXIT_FAILURE);
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
char* getSteamDestinationDir(char* type) {

	char* steamBaseDir = getSteamBaseDir();
	if (steamBaseDir == NULL) {
		return NULL;
	}

	if (strcmp(type, "icon") == 0) {
		// If it's an icon
		strcat(steamBaseDir, "/appcache/librarycache/");
	}
	else {
		// If it's not an icon, read the loginusers.vdf to find the most recent user
		char* steamid = getMostRecentUser(steamBaseDir);
		strcat(steamBaseDir, "/userdata/");
		strcat(steamBaseDir, steamid);
		strcat(steamBaseDir, "/config/grid/");
	}

	return steamBaseDir;
}

// Parse shortcuts file and return a pointer to a list of structs containing the app data
struct nonSteamApp* getNonSteamApps(steamDestDir) {

	char* shortcutsVdfPath = getSteamBaseDir();
	char* steamid = getMostRecentUser(shortcutsVdfPath);

	// Get the shortcuts.vdf file
	strcat(shortcutsVdfPath, "/userdata/");
	strcat(shortcutsVdfPath, steamid);
	strcat(shortcutsVdfPath, "/config/shortcuts.vdf");

	// Parse the file
	FILE* fp;
	char* line = NULL;
	size_t len = 0;
	size_t read;
	fp = fopen(shortcutsVdfPath, "r");
	if (fp == NULL) {
		free(shortcutsVdfPath);
		exit(90);
	}

	while ((read = readLine(&line, &len, fp)) != -1) {
		// Add code for parsing here
	}

	fclose(fp);
	if (line)
		free(line);

	// Exit with an error if no non-steam apps were found
	if (_nonSteamGamesCount < 1) {
		IupMessage("SGDBoop Error", "No non-steam apps were found!");
		exit(91);
	}

	// After all games are found, create a struct array for them
	struct nonSteamApp* apps = malloc(sizeof(nonSteamApp) * _nonSteamGamesCount);

	return apps;
}

// Select a non-steam app from a dropdown list and return its ID
char* selectNonSteamApp(char* sgdbName, struct nonSteamApp* apps) {

	char temp[512];
	sprintf(temp, "%d", _nonSteamGamesCount);
	IupMessage("Found games", temp);
	char* appid = malloc(128);

	char** values = malloc(sizeof(char*) * _nonSteamGamesCount);
	for (int i = 0; i < nonSteamAppsCount; i++) {
		values[i] = &apps[i].name;
	}

	char* title = malloc(40 + strlen(sgdbName));
	strcpy(title, "SGDBoop: Pick a game for '");
	strcat(title, sgdbName);
	strcat(title, "'");

	qsort(values, _nonSteamGamesCount, sizeof(const char*), compareStrings);

	int retval = IupListDialog(1, title, _nonSteamGamesCount, (const char**)values, 0, strlen(title) - 15, 10, NULL);
	IupMessage("Your selection", values[retval]);

	strcpy(appid, apps[retval].appid);
	free(apps);

	exit(1);
	return appid;
}

int main(int argc, char** argv)
{

	// If no arguments were given, register the program
	if (argc == 0 || (argc == 1 && !startsWith(argv[0], "sgdb://"))) {
		// Create the sgdb URI protocol
		if (createURIprotocol() == 1) {
			return 80;
		}
	}
	else {

		if (OS_Windows) {
			MoveWindow(GetConsoleWindow(), -3000, -3000, 0, 0, FALSE);
		}

		// If argument is unregister, unregister and exit
		if (strcmp(argv[1], "unregister") == 0) {
			if (deleteURIprotocol() == 1) {
				return 85;
			}
			return 0;
		}

		// If sgdb:// arguments were passed, run program normally

		// If the arguments aren't of the SGDB URI, return with an error
		if (!startsWith(argv[1], "sgdb://boop")) {
			return 81;
		}

		// Get the params from the string
		char* type = strstr(argv[1], "sgdb://boop/") + strlen("sgdb://boop/");
		char* grid_id = strstr(type, "/");
		grid_id[0] = '\0';         // End app_id string
		grid_id += 1;              // Move 1 place

		// Get asset URL
		char** apiValues = callAPI(type, grid_id);
		if (apiValues == NULL) {
			return 82;
		}
		char* app_id = apiValues[0];
		char* orientation = apiValues[1];
		char* assetUrl = apiValues[2];

		// Get Steam base dir
		char* steamDestDir = getSteamDestinationDir(type);
		if (steamDestDir == NULL) {
			return 83;
		}

		// If the game is a non-steam app, select an imported app
		if (startsWith(app_id, "nonsteam-")) {

			// Enable IUP GUI
			IupOpen(&argc, &argv);

			// Get non-steam apps
			struct nonSteamApp* apps = getNonSteamApps();

			// Show selection screen and return the appid
			app_id = selectNonSteamApp(strstr(app_id, "-") + 1, apps);
		}

		// Download asset file
		char* outfilename = downloadAssetFile(app_id, assetUrl, type, orientation, steamDestDir);
		if (outfilename == NULL) {
			return 84;
		}
	}

	return 0;
}
