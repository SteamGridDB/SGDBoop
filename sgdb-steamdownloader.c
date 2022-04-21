#ifdef __unix__                             /* __unix__ is usually defined by compilers targeting Unix systems */
#define OS_Windows 0
#elif defined(_WIN32) || defined(WIN32)     /* _Win32 is usually defined by compilers targeting 32 or   64 bit Windows systems */
#define OS_Windows 1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "string-helpers.h"
#include "curl-helper.h"

// Call the BOP API
char** callAPI(char* grid_type, char* grid_id)
{
	char authHeader[] = "Authorization: Bearer <bopToken>";
	char* url = malloc(512);
	char** valuesArray = malloc(1024);

	strcpy(url, "https://www.steamgriddb.com/api/sgdbop/");
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
		headers = curl_slist_append(headers, authHeader);
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
		/*valuesArray[2] = strstr(valuesArray[1], ",");
		valuesArray[2][0] = '\0';
		valuesArray[2] += 1;*/
		valuesArray[2] = malloc(5);
		valuesArray[2][0] = 'v';
		valuesArray[2][1] = '\0';

		return valuesArray;
	}
	return NULL;
}

// Download an asset file
char* downloadAssetFile(char* app_id, char* url, char* type, char* orientation)
{
	char* tempPath;
	if (OS_Windows) {
		tempPath = getenv("TEMP");
	}
	else {
		tempPath = getenv("TMPDIR");
	}

	CURL* curl;
	FILE* fp;
	CURLcode res;

	char* outfilename = malloc(1024);
	strcpy(outfilename, tempPath);
	strcat(outfilename, "/");
	strcat(outfilename, app_id);
	if (strcmp(type, "hero") == 0) {
		strcat(outfilename, "_hero.png");
	}
	else if (strcmp(type, "logo") == 0) {
		strcat(outfilename, "_logo.png");
	}
	else if (strcmp(type, "grid") == 0 && strcmp(orientation, "v") == 0) {
		strcat(outfilename, "p.png");
	}

	curl = curl_easy_init();
	if (curl) {
		fp = fopen(outfilename, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);

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
		strcat(regeditCommand, "\\\" \\\"%1\\\"\" /f");

		int ret_val = system("REG ADD HKCR\\sgdb /t REG_SZ /d \"URL:sgdb protocol\" /f");
		if (ret_val != 0) {
			system("cls");
			printf("Please run this program as Administrator!\n");
			system("pause");
			return 1;
		}

		system("REG ADD HKCR\\sgdb /v \"URL Protocol\" /t REG_SZ /d \"\" /f");
		system(regeditCommand);

		system("cls");
		printf("Program registered correctly!\n");
		system("pause");
		return 0;
	}
}

// Get Steam's installation directory
char* getSteamDir() {
	char* steamBaseDir = malloc(MAX_PATH);
	char* steamConfigFile = malloc(MAX_PATH);
	FILE* fp;
	char* line = NULL;
	size_t len = 0;
	size_t read;
	char* steamid = malloc(64);

	if (OS_Windows) {
		FILE* terminal = _popen("reg query HKCU\\Software\\Valve\\Steam /v SteamPath", "r");
		char buf[256];
		int foundValue = 0;
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
		/* Some notes:
			* We can use ~/.local/share/Steam for linux steam dir
		*/
	}

	strcpy(steamConfigFile, steamBaseDir);
	strcat(steamConfigFile, "/config/loginusers.vdf");
	fp = fopen(steamConfigFile, "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	while ((read = getline(&line, &len, fp)) != -1) {
		if (strstr(line, "7656119")) {
			// Found line with id
			strcpy(steamid, line);
			steamid = strstr(steamid, "7656119");
			char* stringEnd = strstr(steamid, "\"");
			stringEnd[0] = '\0';
		}
		else if (strstr(line, "mostrecent") && strstr(line, "\"1\"")) {
			// Found line mostrecent
			char* ptr = malloc(256);
			unsigned long long steamidLongLong = atoll(steamid, ptr, 10);
			steamidLongLong -= 76561197960265728;
			sprintf(steamid, "%u", steamidLongLong);

			strcat(steamBaseDir, "/userdata/");
			strcat(steamBaseDir, steamid);
			strcat(steamBaseDir, "/config/grid/");
			break;
		}
	}

	fclose(fp);
	if (line)
		free(line);

	return steamBaseDir;
}

// Copy a file to another directory
// https://stackoverflow.com/a/28797171/16642426
int copyFile(char* infilepath, char* outfileDir) {

	// Try creating folder
	if (mkdir(outfileDir, 0700) == -1) {
		// Ignore error if folder exists
	}
	FILE* infile;
	FILE* outfile;
	char* infilename = infilepath + lastIndexOf(infilepath, '/') + 1;

	char outfilename[MAX_PATH];

	infile = fopen(infilepath, "rb");
	fseek(infile, 0L, SEEK_END);
	long fileSize = ftell(infile);
	fseek(infile, 0L, SEEK_SET);
	if (infile == NULL) {
		return 1;
	}
	strcpy(outfilename, outfileDir);
	strcat(outfilename, infilename);
	outfile = fopen(outfilename, "wb");

	size_t n, m;
	unsigned char buff[8192];
	do {
		n = fread(buff, 1, sizeof buff, infile);
		if (n) m = fwrite(buff, 1, n, outfile);
		else   m = 0;
	} while ((n > 0) && (n == m));
	if (m) perror("copy");

	fclose(infile);
	fclose(outfile);

	return 0;
}

int main(int argc, char** argv)
{
	// If no arguments were given, register the program
	if (argc == 0 || (argc == 1 && !startsWith(argv[0], "sgdb:%5C%5C"))) {
		// Create the sgdb URI protocol
		if (createURIprotocol() == 1) {
			return 1;
		}
	}
	else {
		// If arguments were passed, run program normally

		// If the arguments aren't of the SGDB URI, return with an error
		if (startsWith(argv[1], "sgdb:%5C%5Cbop")) {
			return 1;
		}

		// Get the params from the string
		char* type = strstr(argv[1], "sgdb://bop/") + strlen("sgdb://bop/");
		char* grid_id = strstr(type, "/");
		grid_id[0] = '\0';         // End app_id string
		grid_id += 1;              // Move 1 place

		// Get asset URL
		char** apiValues = callAPI(type, grid_id);
		char* app_id = apiValues[0];
		char* assetUrl = apiValues[1];
		char* orientation = apiValues[2];

		printf("Orientation: %s\n", orientation);

		// If no valid URL was returned, return
		if (assetUrl == NULL)
		{
			return 1;
		}

		char* outfilename = downloadAssetFile(app_id, assetUrl, type, orientation);
		char* steamDir = getSteamDir();
		copyFile(outfilename, steamDir);
	}

	return 0;
}
