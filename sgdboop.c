#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <inttypes.h>
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

// Multiple Assets!
#define MAX_ASSETS 5
struct assetData
{
	char assetUrl[400];
	char type[100];
	char orientation[2];
};
struct assetData assets[MAX_ASSETS];
int n_assets = 0;

int addAsset(char* assetUrl, char* type, char* orientation) {
	if (n_assets>=MAX_ASSETS) {
		return -1;
	}
	strcpy(assets[n_assets].orientation, orientation);
	strcpy(assets[n_assets].assetUrl, assetUrl);
	strcpy(assets[n_assets].type, type);
	n_assets++;
	return 1;
}

int _nonSteamAppsCount = 0;

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
		exit(95);
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
struct nonSteamApp* getNonSteamApps(char* type, char* orientation) {

	char* shortcutsVdfPath = getSteamBaseDir();
	char* steamid = getMostRecentUser(shortcutsVdfPath);

	// Get the shortcuts.vdf file
	strcat(shortcutsVdfPath, "/userdata/");
	strcat(shortcutsVdfPath, steamid);
	strcat(shortcutsVdfPath, "/config/shortcuts.vdf");

	// Parse the file
	FILE* fp;
	unsigned char buf[2] = { 0 };
	size_t bytes = 0;
	size_t read = sizeof buf;
	fp = fopen(shortcutsVdfPath, "rb");
	if (fp == NULL) {
		free(shortcutsVdfPath);
		IupMessage("SGDBoop Error", "Could not find any non-Steam apps.");
		exit(90);
	}
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

	struct nonSteamApp* apps = malloc(sizeof(nonSteamApp) * 500000);
	unsigned char* parsingChar = fileContent;
	unsigned char parsingAppid[512];
	uint64_t intBytes[4];
	crcInit();

	int old_id_required = 0;
	if (strcmp(type, "grid") == 0 && strcmp(orientation, "l") == 0) {
		old_id_required = 1;
	}

	// Parse the vdf content
	while (strstr_i(parsingChar, "appname") > 0) {

		uint64_t appid_old = 0;
		uint64_t appid = 0;

		// Find app name
		unsigned char* nameStartChar = strstr_i(parsingChar, "appname") + 8;
		unsigned char* nameEndChar = strstr(nameStartChar, "\x03");

		// Find exe path
		unsigned char* exeStartChar = strstr_i(parsingChar, "exe") + 4;
		unsigned char* exeEndChar = strstr(exeStartChar, "\x03");

		unsigned char* appidPtr = strstr_i(parsingChar, "appid");
		unsigned char* appBlockEndPtr = strstr(parsingChar, "\x08") + 1; // gcc fucks with optimization on strstr for 2 consecutive hex values. DON'T EDIT THIS.
		while (*appBlockEndPtr != 0x03 && *appBlockEndPtr != 0x00) {
			appBlockEndPtr = strstr(appBlockEndPtr, "\x08") + 1;
		}

		// If appid was found in this app block
		if (appidPtr > 0 && appidPtr < appBlockEndPtr) {
			unsigned char* hexBytes = appidPtr + 6;
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

	// Exit with an error if no non-steam apps were found
	if (_nonSteamAppsCount < 1) {
		IupMessage("SGDBoop Error", "Could not find any non-Steam apps.");
		free(fileContent);
		free(apps);
		exit(91);
	}

	free(fileContent);

	return apps;
}

// Select a non-steam app from a dropdown list and return its ID
struct nonSteamApp* selectNonSteamApp(char* sgdbName, struct nonSteamApp* apps) {

	char temp[512];
	sprintf(temp, "%d", _nonSteamAppsCount);
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

	// Exit when use clicks cancel
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
		if (!startsWith(argv[1], "sgdb://")) {
			return 81;
		}

		// Get the params from the string
		char* source = strstr(argv[1], "sgdb://") + strlen("sgdb://");
		char* type = strstr(source, "/");
		type[0] = '\0';         // End source string
		type += 1;              // Move 1 place
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

		// Create app_id var
		char* app_id;

		// Get asset URL from SteamGridDB
		if (strcmp(source, "boop")==0) {
			printf("Source is SteamGridDB!\n");

    	    char** apiValues = callAPI(type, grid_id, mode);
			if (apiValues == NULL) {
				return 82;
			}
			app_id = apiValues[0];
			addAsset(apiValues[2], type, apiValues[1]);
		}
		else if(strcmp(source, "steam")==0) {		
			printf("Source is Steam!\n");

			char buffer[400];
			char furl[] = "https://cdn.cloudflare.steamstatic.com/steam/apps/%s/%s?t=%ld";
			time_t epoch_time = time(NULL);

			char id_buffer[100];
			snprintf (id_buffer, 100, "nonsteam-%s", "NO NAME"); // TODO: Get name from steam (maybe by URI?)
			app_id = id_buffer;

			if (strcmp(type,"all")==0) {
				// vertical grid
				snprintf (buffer, 400, furl, grid_id, "library_600x900_2x.jpg", epoch_time );
				addAsset(buffer, "grid", "p"); // vertical

				// horizontal grid
				snprintf (buffer, 400, furl, grid_id, "header.jpg", epoch_time );
				addAsset(buffer, "grid", "l"); // horizontal

				// hero
				snprintf (buffer, 400, furl, grid_id, "library_hero.jpg", epoch_time );
				addAsset(buffer, "grid", "l"); // horizontal

				// logo
				snprintf (buffer, 400, furl, grid_id, "logo.png", epoch_time );
				addAsset(buffer, "grid", "l"); // horizontal
			} else {
				char* orientation = "l";
				if(strcmp(type,"grid")==0) {
					snprintf (buffer, 400, furl, grid_id, "library_600x900_2x.jpg", epoch_time );
					orientation = "p";
				} else if (strcmp(type, "hero")==0) {
					snprintf (buffer, 400, furl, grid_id, "library_hero.jpg", epoch_time );
				} else if (strcmp(type, "logo")==0) {
					snprintf (buffer, 400, furl, grid_id, "logo.png", epoch_time );
				} else {
					return 92; // non supported type
				}

				addAsset(buffer, type, orientation);
			}
		}
		else {
			printf("No compatible source detected in URI!\n");
			return -1;
		}

		if (n_assets <= 0 || n_assets > MAX_ASSETS) {
			printf("No assets detected to download!\n");
			return -1;
		}

		char compat_orientation[] = "p";
		for (int i = 0; i < n_assets; i++) {
			if (strcmp(assets[i].orientation,"l")==0) {
				strcpy(compat_orientation,"l");
				break;
			}
		}

		struct nonSteamApp* nonSteamAppData = NULL;

		// If the game is a non-steam app, select an imported app
		if (startsWith(app_id, "nonsteam-")) {
			if (strcmp(type, "icon") == 0) {
				return 92;
			}

			// Enable IUP GUI
			IupOpen(&argc, &argv);
			loadIupIcon();
			printf("GUI enabled.\n");
			
			// Get non-steam apps
			struct nonSteamApp* apps = getNonSteamApps(type, compat_orientation);			

			// Show selection screen and return the appid
			nonSteamAppData = selectNonSteamApp(strstr(app_id, "-") + 1, apps);

			app_id = nonSteamAppData->appid;
		}

		// Get Steam base dir
		char* steamDestDir = getSteamDestinationDir(type);
		if (steamDestDir == NULL) {
			return 83;
		}

		for (int i = 0; i < n_assets; i++) {
			// Download asset file
			char* outfilename = downloadAssetFile(app_id, assets[i].assetUrl, assets[i].type, assets[i].orientation, steamDestDir);
			if (outfilename == NULL) {
				return 84;
			}

			// If the asset is a horizontal grid, create a symlink (for back. compat.)
			if (nonSteamAppData && strcmp(assets[i].type, "grid") == 0 && strcmp(assets[i].orientation, "l") == 0) {
				createOldIdSymlink(nonSteamAppData, steamDestDir);
			}
		}
		
		if (nonSteamAppData) {
			free(nonSteamAppData);
		}
	}

	return 0;
}
