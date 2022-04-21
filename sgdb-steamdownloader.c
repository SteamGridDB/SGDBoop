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

char *getAssetURL(char *grid_type, char *grid_id)
{
    char authHeader[] = "Authorization: Bearer a25e348c17fa6e8d16ca49ed59d2ac30";
    char *url = malloc(512);
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

return s.ptr;
    }
    return NULL;
}

char* downloadAssetFile(char* url, char* app_id, char* type)
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
    printf("%s\n", outfilename);

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
        wchar_t cwd[MAX_PATH];
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

// https://stackoverflow.com/a/15515276/16642426
int startsWith(const char* a, const char* b)
{
    if (strncmp(a, b, strlen(b)) == 0) return 1;
    return 0;
}

// https://stackoverflow.com/a/53360624/16642426
void replace_str(char* str, char* org, char* rep)
{
    char* ToRep = strstr(str, org);
    char* Rest = (char*)malloc(strlen(ToRep));
    strcpy(Rest, ((ToRep)+strlen(org)));

    strcpy(ToRep, rep);
    strcat(ToRep, Rest);

    free(Rest);
}

int main(int argc, char** argv)
{
    // char* type = malloc(128);
     //char* app_id = malloc(128);
     // No arguments, register program
    if (argc == 0 || (argc == 1 && !startsWith(argv[0], "sgdb://"))) {
        // Create the sgdb URI protocol
        if (createURIprotocol() == 1) {
            return 1;
        }
    }
    else {
        // If arguments were passed, run program normally

        // If the arguments aren't of the SGDB URI, return with an error
        if (startsWith(argv[1], "sgdb://")) {
            return 1;
        }

        // Get the params from the string
        char* app_id = strstr(argv[1], "sgdb:%5C%5C") + strlen("sgdb:%5C%5C");
        char* type = strstr(app_id, "%5C");
        type[0] = '\0';         // End app_id string
        type += 3;              // Move 3 places, the size of %5C
        char* grid_id = strstr(type, "%5C");
        grid_id[0] = '\0';
        grid_id += 3;

        // Get asset URL
        char* assetUrl = getAssetURL(type, grid_id);

        // If no valid URL was returned, return
        if (assetUrl == NULL)
        {
            return 1;
        }

        printf("%s\n", assetUrl);
        downloadAssetFile(assetUrl, app_id, type);
    }

    return 0;
}
