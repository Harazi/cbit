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
			cJSON_ArrayForEach(key, json) {
				if (!strcmp(key->string, "connection_status")) {
					printf("Connection Status: %s\n", key->valuestring);
					continue;
				}

				if (!strcmp(key->string, "dht_nodes"))
					printf("DHT Nodes: ");
				else if (!strcmp(key->string, "dl_info_data"))
					printf("Downloaded Bytes: ");
				else if (!strcmp(key->string, "dl_info_speed"))
					printf("Download Speed: ");
				else if (!strcmp(key->string, "dl_rate_limit"))
					printf("Download Limit: ");
				else if (!strcmp(key->string, "up_info_data"))
					printf("Uploaded Bytes: ");
				else if (!strcmp(key->string, "up_info_speed"))
					printf("Upload Speed: ");
				else if (!strcmp(key->string, "up_rate_limit"))
					printf("Upload Limit: ");
				else
					printf("%s: ", key->string);

				printf("%.f\n", key->valuedouble);
			}
		}

		cJSON_Delete(json);
		response.size = 0;
	}
	else if (!strcmp(*argv, "log")) {
		response = GET("/log/main");

		cJSON *json = cJSON_Parse(response.memory);
		if (json != NULL && cJSON_IsArray(json)) {

			cJSON *obj;
			cJSON_ArrayForEach(obj, json) {
				cJSON *timestamp = cJSON_GetObjectItemCaseSensitive(obj, "timestamp");
				cJSON *message = cJSON_GetObjectItemCaseSensitive(obj, "message");
				if (cJSON_IsNumber(timestamp) && cJSON_IsString(message))
					printf("%.f: %s\n", timestamp->valuedouble, message->valuestring);
			}

			cJSON_Delete(json);
			response.size = 0;
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
