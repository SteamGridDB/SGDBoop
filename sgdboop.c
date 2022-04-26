#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include "string-helpers.h"
#include "curl-helper.h"

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
#include <libloaderapi.h>
#endif

#define API_VERSION "1"

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
		int ret_val = system("xdg-mime default com.steamgriddb.SGDBoop.desktop x-scheme-handler/sgdb");
		if (ret_val != 0) {
			system("clear");
			printf("Something went wrong. Please make sure xdg-utils is installed.\n");
			return 1;
		}
		return 0;
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
		return 0;
	}
}

// Get Steam's destination directory based on artwork type
char* getSteamDestinationDir(char* type) {
	char* steamBaseDir = malloc(MAX_PATH);
	char steamConfigFile[MAX_PATH];
	FILE* fp;
	char* line = NULL;
	size_t len = 0;
	size_t read;
	char* steamid = malloc(512);
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
			exit(1);
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
		strcat(steamBaseDir, "/.steam/steam");
	}

	if (strcmp(type, "icon") == 0) {
		// If it's an icon
		strcat(steamBaseDir, "/appcache/librarycache/");
	}
	else {
		// If it's not an icon, read the loginusers.vdf to find the most recent user
		strcpy(steamConfigFile, steamBaseDir);
		strcat(steamConfigFile, "/config/loginusers.vdf");
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

				strcat(steamBaseDir, "/userdata/");
				strcat(steamBaseDir, steamid);
				strcat(steamBaseDir, "/config/grid/");
				break;
			}
		}

		fclose(fp);
		if (steamid)
			free(steamid);
		if (line)
			free(line);
	}

	if (!foundValue) {
		steamBaseDir = NULL;
	}

	return steamBaseDir;
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

		// Download asset file
		char* outfilename = downloadAssetFile(app_id, assetUrl, type, orientation, steamDestDir);
		if (outfilename == NULL) {
			return 84;
		}
	}

	return 0;
}
