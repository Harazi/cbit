#include <ctype.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"
#include "callbacks.h"
#include "do_commands.h"
#include "auth.h"
#include "cJSON/cJSON.h"

#define CMD "settings"

void settings_help(void)
{
	printf(
		"Usage: "PROGRAM_NAME" "CMD" KEY[=VALUE]...\n"
		"       "PROGRAM_NAME" "CMD" [ -s | --show-all ]\n"
	);
}

void do_settings(int argc, char **argv)
{
	struct MemoryStruct response; 
	char formattedOutput[BUFSIZ] = "";
	char numberToAscii[LARGEST_INT_LENGTH];

	if (argc < 1 || !strcmp(*argv, "help")) {
		settings_help();
		exit(EXIT_SUCCESS);
	}

	auth_login();

	if (!strcmp(*argv, "-s") || !strcmp(*argv, "--show-all")) {
		response = GET("/app/preferences");
		cJSON *json = cJSON_Parse(response.memory);
		if (json != NULL && cJSON_IsObject(json)) {

			cJSON *key;
			cJSON_ArrayForEach(key, json) {
				strcat(formattedOutput, key->string);
				strcat(formattedOutput, "\t");
				if (cJSON_IsString(key))
					strcat(formattedOutput, key->valuestring);
				if (cJSON_IsBool(key))
					strcat(formattedOutput, key->valueint ? "true" : "false");
				if (cJSON_IsNull(key))
					strcat(formattedOutput, "null");
				if (cJSON_IsNumber(key)) {
					snprintf(numberToAscii, LARGEST_INT_LENGTH, "%.f", key->valuedouble);
					strcat(formattedOutput, numberToAscii);
				}
				if (cJSON_IsObject(key))
					strcat(formattedOutput, "[Object]");
				if (cJSON_IsArray(key))
					strcat(formattedOutput, "[Array]");
				strcat(formattedOutput, "\n");
			}

			printf("%s", formattedOutput);
			cJSON_Delete(json);
			free(response.memory);
		}
		return;
	}

	bool postSettings = false;
	strcat(formattedOutput, "json={");
	for (int i = 0; i < argc; i++) {
		char *value = strchr(argv[i], '=');
		if (value == NULL)
			continue;
		*value = '\0';
		value++;
		postSettings = true;

		strcat(formattedOutput, "\"");
		strcat(formattedOutput, argv[i]);
		strcat(formattedOutput, "\":");

		if (isdigit(value[0]))
			strcat(formattedOutput, value);
		else if (!strcmp(value, "true") || !strcmp(value, "false") || !strcmp(value, "null"))
			strcat(formattedOutput, value);
		else {
			strcat(formattedOutput, "\"");
			strcat(formattedOutput, value);
			strcat(formattedOutput, "\"");
		}

		strcat(formattedOutput, ",");
	}
	if (postSettings) {
		// Overwrite comma
		formattedOutput[strlen(formattedOutput)-1] = '}';

		response = POST("/app/setPreferences", formattedOutput);
		if (response.size)
			printf("%s\n", response.memory);
		free(response.memory);
		response.size = 0;
		response.memory = NULL;
		*formattedOutput = '\0';
	}

	response = GET("/app/preferences");
	cJSON *json = cJSON_Parse(response.memory);
	if (json != NULL && cJSON_IsObject(json)) {
		for (int i = 0; i < argc; i++) {
			// Terminate on '='
			*(strchrnul(argv[i], '=')) = '\0';

			cJSON *key = cJSON_GetObjectItemCaseSensitive(json, argv[i]);
			if (key == NULL) {
				fprintf(stderr, "Unknown key %s\n", argv[i]);
				continue;
			}

			printf("%s=", argv[i]);
			if (cJSON_IsString(key))
				printf("%s\n", key->valuestring);
			if (cJSON_IsBool(key))
				printf("%s\n", key->valueint ? "true" : "false");
			if (cJSON_IsNull(key))
				printf("null\n");
			if (cJSON_IsNumber(key)) {
				printf("%d\n", key->valueint);
			}
			if (cJSON_IsObject(key))
				printf("[Object]");
			if (cJSON_IsArray(key))
				printf("[Array]");
		}
	}
	else
		fprintf(stderr, "/app/preferences: Unknown response\n%s\n", response.memory);
}
