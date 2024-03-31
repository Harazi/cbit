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
	cJSON *json;
	cJSON *obj;

	if (argc < 1 || !strcmp(*argv, "help")) {
		session_help();
		exit(EXIT_SUCCESS);
	}

	auth_login();

	if (!strcmp(*argv, "stats")) {

		response = GET("/transfer/info");
		json = cJSON_Parse(response.memory);
		if (json != NULL && cJSON_IsObject(json)) {

			int idx = -1;
			bool perSecond = false;
			cJSON_ArrayForEach(obj, json) {
				print_color(COLOR_BLUE);
				if (!strcmp(obj->string, "connection_status")) {
					printf("Connection Status");
					print_color(COLOR_RESET);
					printf(": ");
					if (!strcmp(obj->valuestring, "disconnected"))
						print_color(COLOR_RED);
					if (!strcmp(obj->valuestring, "firewalled"))
						print_color(COLOR_YELLOW);
					if (!strcmp(obj->valuestring, "connected"))
						print_color(COLOR_GREEN);
					printf("%s\n", obj->valuestring);
					print_color(COLOR_RESET);
					continue;
				}

				idx = human_size(&obj->valuedouble);

				if (!strcmp(obj->string, "dht_nodes")) {
					printf("DHT Nodes");
					idx = -1;
				}
				else if (!strcmp(obj->string, "dl_info_data"))
					printf("Downloaded Bytes");
				else if (!strcmp(obj->string, "dl_info_speed")) {
					printf("Download Speed");
					perSecond = true;
				}
				else if (!strcmp(obj->string, "dl_rate_limit")) {
					printf("Download Limit");
					perSecond = true;
				}
				else if (!strcmp(obj->string, "up_info_data"))
					printf("Uploaded Bytes");
				else if (!strcmp(obj->string, "up_info_speed")) {
					printf("Upload Speed");
					perSecond = true;
				}
				else if (!strcmp(obj->string, "up_rate_limit")) {
					printf("Upload Limit");
					perSecond = true;
				}
				else
					printf("%s", obj->string);

				print_color(COLOR_RESET);
				printf(": ");
				printf("%.*f%s%s\n", idx >= 0 ? 2 : 0, obj->valuedouble, idx >= 0 ? sizeSuffixes[idx] : "", idx >= 0 && perSecond ? "/s" : "");
				idx = -1;
				perSecond = false;
			}
		}

		cJSON_Delete(json);
		response.size = 0;
	}
	else if (!strcmp(*argv, "log")) {
		response = GET("/log/main");
		json = cJSON_Parse(response.memory);
		if (json != NULL && cJSON_IsArray(json)) {
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
