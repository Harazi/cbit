#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "callbacks.h"
#include "config.h"

size_t dev_null(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	return size * nmemb;
}

size_t write_callback(char *contents, size_t size, size_t nitems, void *userdata)
{
	size_t realsize = size * nitems;
	struct MemoryStruct *mem = (struct MemoryStruct *)userdata;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (!ptr) {
		/* out of memory! */
		fprintf(stderr, "not enough memory (realloc returned NULL)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

struct MemoryStruct GET(const char *path)
{
	struct MemoryStruct response = { .size = 0, .memory = NULL };

	char urlFull[BUFSIZ] = "";
	strcat(urlFull, config.server.url);
	strcat(urlFull, "/api/v2");
	strcat(urlFull, path);

	curl_easy_setopt(curl, CURLOPT_URL, urlFull);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "%s: curl: %s\n", urlFull, curl_easy_strerror(res));
		exit(EXIT_FAILURE);
	}

	return response;
}

struct MemoryStruct POST(const char *path, const char *postField)
{
	struct MemoryStruct response = { .size = 0, .memory = NULL };

	char urlFull[BUFSIZ] = "";
	strcat(urlFull, config.server.url);
	strcat(urlFull, "/api/v2");
	strcat(urlFull, path);

	curl_easy_setopt(curl, CURLOPT_URL, urlFull);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postField);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "%s: curl: %s\n", urlFull, curl_easy_strerror(res));
		exit(EXIT_FAILURE);
	}

	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	return response;
}

struct MemoryStruct POST_MIME(const char *path, curl_mime *mime)
{
	struct MemoryStruct response = { .size = 0, .memory = NULL };

	char urlFull[BUFSIZ] = "";
	strcat(urlFull, config.server.url);
	strcat(urlFull, "/api/v2");
	strcat(urlFull, path);

	curl_easy_setopt(curl, CURLOPT_URL, urlFull);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "%s: curl: %s\n", urlFull, curl_easy_strerror(res));
		exit(EXIT_FAILURE);
	}

	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	return response;
}
