#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "callbacks.h"
#include "do_commands.h"
#include "auth.h"
#include "cJSON/cJSON.h"

#define CMD "app"

void app_help(void)
{
	printf("Usage: "PROGRAM_NAME" "CMD" { version | shutdown }\n");
}

void do_app(int argc, char **argv)
{
	struct MemoryStruct response; 
	char formattedOutput[BUFSIZ] = "";
	char numberToAscii[LARGEST_INT_LENGTH];

	if (argc < 1 || !strcmp(*argv, "help")) {
		app_help();
		exit(EXIT_SUCCESS);
	}

	auth_login();

	if (!strcmp(*argv, "shutdown"))
		response = POST("/app/shutdown", "");
	else if (!strcmp(*argv, "version")) {

		response = GET("/app/version");
		if (response.size) {
			strcat(formattedOutput, "qBittorrent version");
			// replace 'v' with a space
			*response.memory = ' ';
			strcat(formattedOutput, response.memory);
			strcat(formattedOutput, "\n");

			free(response.memory);
			response.memory = NULL;
			response.size = 0;
		}

		response = GET("/app/webapiVersion");
		if (response.size) {
			strcat(formattedOutput, "qBittorrent Web API version ");
			strcat(formattedOutput, response.memory);
			strcat(formattedOutput, "\n");

			free(response.memory);
			response.memory = NULL;
			response.size = 0;
		}

		response = GET("/app/buildInfo");
		if (response.size) {
			cJSON *json = cJSON_Parse(response.memory);
			if (json != NULL && cJSON_IsObject(json)) {

				cJSON *bitness = cJSON_GetObjectItemCaseSensitive(json, "bitness");
				if (cJSON_IsNumber(bitness)) {
					strcat(formattedOutput, "Bitness: ");
					snprintf(numberToAscii, LARGEST_INT_LENGTH, "%.f", bitness->valuedouble);
					strcat(formattedOutput, numberToAscii);
					strcat(formattedOutput, "\n");
				}

				strcat(formattedOutput, "Linked against:\n");

				cJSON *lib;
				cJSON_ArrayForEach(lib, json) {
					if (!strcmp(lib->string, "bitness"))
						continue;

					strcat(formattedOutput, "- ");
					strcat(formattedOutput, lib->string);
					strcat(formattedOutput, " ");
					strcat(formattedOutput, lib->valuestring);
					strcat(formattedOutput, "\n");
				}
			}

			cJSON_Delete(json);
			free(response.memory);
			response.memory = NULL;
			response.size = 0;
		}

		printf("%s", formattedOutput);
	}
	else {
		fprintf(stderr, "Command \"%s\" is unknown, try \""PROGRAM_NAME" "CMD" help\".\n", *argv);
		exit(EXIT_FAILURE);
	}

	if (response.size)
		printf("%s\n", response.memory);
	free(response.memory);
}
