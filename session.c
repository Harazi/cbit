#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "callbacks.h"
#include "do_commands.h"
#include "auth.h"
#include "cJSON/cJSON.h"

#define CMD "session"

void session_help(void)
{
	printf("Usage: "PROGRAM_NAME" "CMD" { stats | log }\n");
}

void do_session(int argc, char **argv)
{
	struct MemoryStruct response; 
	char formattedOutput[BUFSIZ] = "";

	if (argc < 1 || !strcmp(*argv, "help")) {
		session_help();
		exit(EXIT_SUCCESS);
	}

	auth_login();

	if (!strcmp(*argv, "stats")) {

		response = GET("/transfer/info");
		cJSON *json = cJSON_Parse(response.memory);
		if (json != NULL && cJSON_IsObject(json)) {

			cJSON *key;
			char n[12];
			cJSON_ArrayForEach(key, json) {
				if (!strcmp(key->string, "connection_status")) {
					strcat(formattedOutput, "Connection Status: ");
					strcat(formattedOutput, key->valuestring);
					strcat(formattedOutput, "\n");
					continue;
				}

				if (!strcmp(key->string, "dht_nodes"))
					strcat(formattedOutput, "DHT Nodes");
				else if (!strcmp(key->string, "dl_info_data"))
					strcat(formattedOutput, "Downloaded Bytes");
				else if (!strcmp(key->string, "dl_info_speed"))
					strcat(formattedOutput, "Download Speed");
				else if (!strcmp(key->string, "dl_rate_limit"))
					strcat(formattedOutput, "Download Limit");
				else if (!strcmp(key->string, "up_info_data"))
					strcat(formattedOutput, "Uploaded Bytes");
				else if (!strcmp(key->string, "up_info_speed"))
					strcat(formattedOutput, "Upload Speed");
				else if (!strcmp(key->string, "up_rate_limit"))
					strcat(formattedOutput, "Upload Limit");
				else
					strcat(formattedOutput, key->string);
				strcat(formattedOutput, ": ");
				snprintf(n, 7, "%d", key->valueint);
				strcat(formattedOutput, n);
				strcat(formattedOutput, "\n");
			}
		}

		cJSON_Delete(json);
		response.size = 0;

		printf("%s", formattedOutput);
	}
	else if (!strcmp(*argv, "log")) {
		response = GET("/log/main");

		cJSON *json = cJSON_Parse(response.memory);
		if (json != NULL && cJSON_IsArray(json)) {

			cJSON *obj;
			char n[12];
			struct MemoryStruct mem = { .size = 0, .memory = NULL };
			cJSON_ArrayForEach(obj, json) {
				cJSON *timestamp = cJSON_GetObjectItemCaseSensitive(obj, "timestamp");
				cJSON *message = cJSON_GetObjectItemCaseSensitive(obj, "message");
				if (cJSON_IsNumber(timestamp) && cJSON_IsString(message)) {
					snprintf(n, 11, "%d", timestamp->valueint);
					strcat(formattedOutput, n);
					strcat(formattedOutput, ": ");
					strcat(formattedOutput, message->valuestring);
					strcat(formattedOutput, "\n");
					size_t messageLen = strlen(formattedOutput);

					char *ptr = realloc(mem.memory, mem.size + messageLen + 1);
					if (!ptr) {
						/* out of memory! */
						fprintf(stderr, "not enough memory (realloc returned NULL)\n");
						exit(EXIT_FAILURE);
					}

					mem.memory = ptr;
					memcpy(&(mem.memory[mem.size]), formattedOutput, messageLen);
					formattedOutput[0] = '\0';
					mem.size += messageLen;
					mem.memory[mem.size] = 0;
				}
			}

			cJSON_Delete(json);
			response.size = 0;

			printf("%s", mem.memory);
			free(mem.memory);
		}
	}
	else {
		fprintf(stderr, "Command \"%s\" is unknown, try \""PROGRAM_NAME" "CMD" help\".\n", *argv);
		exit(EXIT_FAILURE);
	}

	if (response.size)
		printf("%s\n", response.memory);
	free(response.memory);
}
