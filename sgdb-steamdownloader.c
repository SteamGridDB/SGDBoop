#ifdef __unix__                             /* __unix__ is usually defined by compilers targeting Unix systems */
    #define OS_Windows 0
#elif defined(_WIN32) || defined(WIN32)     /* _Win32 is usually defined by compilers targeting 32 or   64 bit Windows systems */
    #define OS_Windows 1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "curl-helper.h"

char** callAPI(char *grid_type, char *grid_id)
{
    char authHeader[] = "Authorization: Bearer <bopToken>";
    char *url = malloc(512);
    char** valuesArray = malloc(1024);

    strcpy(url, "https://www.steamgriddb.com/api/sgdbop/");
    strcat(url, grid_type);
    strcat(url, "/");
    strcat(url, grid_id);

    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    curl = curl_easy_init();
    if (curl)
    {
        struct string s;
        init_string(&s);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
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
        return valuesArray;
    }
    return NULL;
}

char* downloadAssetFile(char* app_id, char* url, char* type)
{
    char* tempPath = getenv("TEMP");
    CURL* curl;
    FILE* fp;
    CURLcode res;

    char* outfilename = malloc(1024);
    strcpy(outfilename, tempPath);
    strcat(outfilename, "/");
    strcat(outfilename, app_id);
    if (strcmp(type, "hero") == 0) {
        strcat(outfilename, "_hero");
    }
    else if (strcmp(type, "logo") == 0) {
        strcat(outfilename, "_logo");
    }
    strcat(outfilename, ".png");

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

int createURIprotocol() {
    if (OS_Windows) {
        char cwd[MAX_PATH];
        //char buff[sizeof(cwd) + 1];
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

// Check if a string starts with a given substring
// https://stackoverflow.com/a/15515276/16642426
int startsWith(const char* a, const char* b)
{
    if (strncmp(a, b, strlen(b)) == 0) return 1;
    return 0;
}

// Copy a file to another directory
// https://stackoverflow.com/a/28797171/16642426
int copyfile(char* infilepath, char* infilename, char* outfileDir) {
    FILE* infile;
    FILE* outfile;
    char outfilename[MAX_PATH];

    infile = fopen(infilepath, "r");
    if (infile == NULL) {
        return 1;
    }
    strcpy(outfilename, outfileDir);
    strcat(outfilename, infilename);
    outfile = fopen(outfilename, "w");
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

        // If no valid URL was returned, return
        if (assetUrl == NULL)
        {
            return 1;
        }

        downloadAssetFile(app_id, assetUrl, type);
    }

    return 0;
}
