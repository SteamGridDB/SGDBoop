#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "curl-helper.h"

char *getAssetURL(char *grid_type, char *grid_id)
{
    char authHeader[] = "Authorization: Bearer a25e348c17fa6e8d16ca49ed59d2ac30";
    char url[] = "https://www.steamgriddb.com/api/sgdbop/";
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

char *downloadAssetFile(char *url, char *app_id, char* type)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char *outfilename = malloc(256);
    strcpy(outfilename, app_id);
    if (strcmp(type, "hero") == 0) {
        strcat(outfilename, "_hero");
    } else if (strcmp(type, "logo") == 0) {
        strcat(outfilename, "_logo");
    }
    strcat(outfilename, ".png");
    printf("%s\n", outfilename);

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outfilename,"wb");
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

int main()
{
    char type[] = "grid";
    char app_id[] = "195664";
    char *assetUrl = getAssetURL(type, app_id);

    // If no valid URL was returned, return
    if (assetUrl == NULL)
    {
        return 1;
    }

    printf("%s\n", assetUrl);
    downloadAssetFile(assetUrl, app_id, type);
    return 0;
}
