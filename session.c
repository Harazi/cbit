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
	printf(
		"Usage: "PROGRAM_NAME" "CMD" { stats | log | limits }\n"
		"       "PROGRAM_NAME" "CMD" set { downloadlimit | uploadlimit } LIMIT\n"
	);
}

void do_session(int argc, char **argv)
{
	struct MemoryStruct response; 
	char postField[BUFSIZ] = "";

	if (argc < 1 || !strcmp(*argv, "help")) {
		session_help();
		exit(EXIT_SUCCESS);
	}

	auth_login();

	if (!strcmp(*argv, "stats")) {

		response = GET("/transfer/info");
		cJSON *json = cJSON_Parse(response.memory);
		if (json != NULL && cJSON_IsObject(json)) {

			int idx = -1;
			bool perSecond = false;
			cJSON *key;
			cJSON_ArrayForEach(key, json) {
				print_color(COLOR_BLUE);
				if (!strcmp(key->string, "connection_status")) {
					printf("Connection Status");
					print_color(COLOR_RESET);
					printf(": ");
					if (!strcmp(key->valuestring, "disconnected"))
						print_color(COLOR_RED);
					if (!strcmp(key->valuestring, "firewalled"))
						print_color(COLOR_YELLOW);
					if (!strcmp(key->valuestring, "connected"))
						print_color(COLOR_GREEN);
					printf("%s\n", key->valuestring);
					print_color(COLOR_RESET);
					continue;
				}

				idx = human_size(&key->valuedouble);

				if (!strcmp(key->string, "dht_nodes")) {
					printf("DHT Nodes");
					idx = -1;
				}
				else if (!strcmp(key->string, "dl_info_data"))
					printf("Downloaded Bytes");
				else if (!strcmp(key->string, "dl_info_speed")) {
					printf("Download Speed");
					perSecond = true;
				}
				else if (!strcmp(key->string, "dl_rate_limit")) {
					printf("Download Limit");
					perSecond = true;
				}
				else if (!strcmp(key->string, "up_info_data"))
					printf("Uploaded Bytes");
				else if (!strcmp(key->string, "up_info_speed")) {
					printf("Upload Speed");
					perSecond = true;
				}
				else if (!strcmp(key->string, "up_rate_limit")) {
					printf("Upload Limit");
					perSecond = true;
				}
				else
					printf("%s", key->string);

				print_color(COLOR_RESET);
				printf(": ");
				printf("%.*f%s%s\n", idx >= 0 ? 2 : 0, key->valuedouble, idx >= 0 ? sizeSuffixes[idx] : "", idx >= 0 && perSecond ? "/s" : "");
				idx = -1;
				perSecond = false;
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
	else if (!strcmp(*argv, "limits")) {
		double speed;
		int idx;
		response = GET("/transfer/downloadLimit");
		if (response.size) {
			speed = atoll(response.memory);
			idx = human_size(&speed);
			printf("Download: %.2f%s/s\n", speed, sizeSuffixes[idx]);
			free(response.memory);
			response.size = 0;
			response.memory = NULL;
		}
		response = GET("/transfer/uploadLimit");
		if (response.size) {
			speed = atoll(response.memory);
			idx = human_size(&speed);
			printf("Upload: %.2f%s/s\n", speed, sizeSuffixes[idx]);
			free(response.memory);
			response.size = 0;
			response.memory = NULL;
		}
	}
	else if (!strcmp(*argv, "set")) {
		if (argc < 3) {
			fprintf(stderr, "Missing limit argument\n");
			exit(EXIT_FAILURE);
		}
		strcat(postField, "limit=");
		strcat(postField, argv[2]);
		if (!strcmp(argv[1], "downloadlimit"))
			response = POST("/transfer/setDownloadLimit", postField);
		else if (!strcmp(argv[1], "uploadlimit"))
			response = POST("/transfer/setUploadLimit", postField);
		else {
			session_help();
			exit(EXIT_FAILURE);
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
