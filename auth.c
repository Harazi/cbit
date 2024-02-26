#include <ctype.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "auth.h"
#include "callbacks.h"

void auth_login()
{
	struct MemoryStruct headers = { .size = 0, .memory = NULL };

	char data[BUFSIZ] = "username=";
	strcat(data, config.auth.username);
	strcat(data, "&password=");
	strcat(data, config.auth.password);

	char url[BUFSIZ] = "";
	strcat(url, config.server.url);
	strcat(url, "/api/v2/auth/login");

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dev_null);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "%s: curl: %s\n", config.server.url, curl_easy_strerror(res));
		exit(EXIT_FAILURE);
	}

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, NULL);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

	char *nl = strchr(headers.memory, '\r');
	if (nl == NULL) {
		fprintf(stderr, "%s: failed parsing response code\n", config.server.url);
		exit(EXIT_FAILURE);
	}

	*nl = '\0';
	char *ok = strstr(headers.memory, "200");
	if (ok == NULL) {
		fprintf(stderr, "%s: response code not ok", config.server.url);
		exit(EXIT_FAILURE);
	}

	*nl = '\r';
	char *firstCookie = strstr(headers.memory, "set-cookie:");
	if (firstCookie == NULL) {
		fprintf(stderr, "%s: failed loging-in\n", config.server.url);
		exit(EXIT_FAILURE);
	}

	firstCookie += 11; // skip "set-cookie:"
	while (isblank(*firstCookie))
		firstCookie++;
	char *firstCookieDelim = strchr(firstCookie, ';');
	if (firstCookieDelim == NULL) {
		fprintf(stderr, "%s: failed parsing cookies\n", config.server.url);
		fprintf(stderr, "%s\n", firstCookie);
		exit(EXIT_FAILURE);
	}

	free(headers.memory);
}
