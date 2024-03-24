#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <curl/curl.h>
#include "cJSON/cJSON.h"
#include "config.h"
#include "callbacks.h"
#include "do_commands.h"
#include "auth.h"

#define CMD "torrents"

char *supportedLinks[] = {
	"http://",
	"https://",
	"magnet:",
	"bc://bt/",
	NULL
};

struct option {
	char opt;
	char *description;
	char *key;
	char *placeholder;
	char *defaultValue;
	char *value;
};

struct option addOpts[] = {
	{ .opt = 'p', .key = "savepath",           .placeholder = "path",     .defaultValue = NULL,    .value = NULL, .description = "Download folder" },
	{ .opt = 'k', .key = "cookie",             .placeholder = "cookie",   .defaultValue = NULL,    .value = NULL, .description = "Cookie sent to download the .torrent file" },
	{ .opt = 'c', .key = "category",           .placeholder = "category", .defaultValue = NULL,    .value = NULL, .description = "Category for the torrent" },
	{ .opt = 't', .key = "tags",               .placeholder = "tags",     .defaultValue = NULL,    .value = NULL, .description = "Tags for the torrent, split by ','" },
	{ .opt = 'r', .key = "rename",             .placeholder = "rename",   .defaultValue = NULL,    .value = NULL, .description = "Rename torrent" },
	{ .opt = 'u', .key = "upLimit",            .placeholder = "limit",    .defaultValue = NULL,    .value = NULL, .description = "Set torrent upload speed limit. Unit in bytes/second" },
	{ .opt = 'd', .key = "dlLimit",            .placeholder = "limit",    .defaultValue = NULL,    .value = NULL, .description = "Set torrent download speed limit. Unit in bytes/second" },
	{ .opt = 'i', .key = "ratioLimit",         .placeholder = "ratio",    .defaultValue = NULL,    .value = NULL, .description = "Set torrent share ratio limit" },
	{ .opt = 'm', .key = "seedingTimeLimit",   .placeholder = "time",     .defaultValue = NULL,    .value = NULL, .description = "Set torrent seeding time limit. Unit in minutes" },
	{ .opt = 'H', .key = "skip_checking",      .placeholder = "",         .defaultValue = "false", .value = NULL, .description = "Skip hash checking" },
	{ .opt = 'P', .key = "paused",             .placeholder = "",         .defaultValue = "false", .value = NULL, .description = "Add torrents in the paused state" },
	{ .opt = 'R', .key = "root_folder",        .placeholder = "",         .defaultValue = "unset", .value = NULL, .description = "Create the root folder" },
	{ .opt = 'A', .key = "autoTTM",            .placeholder = "",         .defaultValue = NULL,    .value = NULL, .description = "Set Automatic Torrent Management" },
	{ .opt = 'S', .key = "sequentialDownload", .placeholder = "",         .defaultValue = "false", .value = NULL, .description = "Enable sequential download" },
	{ .opt = 'F', .key = "firstLastPiecePrio", .placeholder = "",         .defaultValue = "false", .value = NULL, .description = "Prioritize download first last piece" },
	{ .opt = '\0' }
};

static char *cmds[] = { "pause", "resume", "recheck", "reannounce", NULL };

void torrents_help(void)
{
	printf(
		"Usage: "PROGRAM_NAME" "CMD" list\n"
		"       "PROGRAM_NAME" "CMD" info HASH\n"
		"       "PROGRAM_NAME" "CMD" { pause | resume | recheck | reannounce } HASH...\n"
		"       "PROGRAM_NAME" "CMD" add [ADD_OPTIONS] { URL | FILE }...\n"
		"       "PROGRAM_NAME" "CMD" delete [DELETE_OPTIONS] HASH...\n"
		"       "PROGRAM_NAME" "CMD" rename NewName HASH\n"
		"       "PROGRAM_NAME" "CMD" show { webseeds | files | pieces | limits | trackers | category } HASH\n"
		"       "PROGRAM_NAME" "CMD" set { downloadlimit | uploadlimit | trackers | category } NewValue HASH\n"
		"       "PROGRAM_NAME" "CMD" toggle { sequential | firstandlastpieces | superseeding | forcestart | automanagement } HASH\n"
		"\n"
		"ADD_OPTIONS:\n"
	);

	for (int i = 0; addOpts[i].opt != '\0'; i++)
		printf("  -%c %-16s %s\n", addOpts[i].opt, addOpts[i].placeholder, addOpts[i].description);

	printf(
		"\n"
		"DELETE_OPTIONS:\n"
		"  -F %-16s Delete downloaded files\n",
		""
	);
}

void do_torrents(int argc, char **argv)
{
	struct MemoryStruct response = { .size = 0, .memory = NULL};
	char postField[BUFSIZ] = "";
	char partialPath[BUFSIZ] = "/torrents/";

	if (argc < 1 || !strcmp(*argv, "help")) {
		torrents_help();
		exit(EXIT_SUCCESS);
	}

	auth_login();

	if (!strcmp(*argv, "list")) {
		response = GET("/torrents/info");
		goto PRINT_AND_CLEANUP;
	}

	if (argc < 2) {
		torrents_help();
		exit(EXIT_FAILURE);
	}

	// Simple commands that don't have any options or special handling
	for (int i = 0; cmds[i] != NULL; i++) {
		if (strcmp(*argv, cmds[i]))
			continue;

		strcat(postField, "hashes=");
		for (int i = 1; i < argc; i++) {
			strcat(postField, argv[i]);
			strcat(postField, "|");
		}
		postField[strlen(postField)-1] = '\0';

		strcat(partialPath, cmds[i]);
		response = POST(partialPath, postField);
		goto PRINT_AND_CLEANUP;
	}

	if (!strcmp(*argv, "info")) {
		strcat(postField, "hash=");
		strcat(postField, argv[1]);
		response = POST("/torrents/properties", postField);
		cJSON *json = cJSON_Parse(response.memory);
		if (cJSON_IsObject(json)) {
			char date[BUFSIZ];
			time_t time;
			int idx;
			cJSON *property;
			printf("%s\n", cJSON_GetObjectItemCaseSensitive(json, "name")->valuestring);
			printf("\tHash: %s\n", cJSON_GetObjectItemCaseSensitive(json, "hash")->valuestring);

			time = cJSON_GetObjectItemCaseSensitive(json, "addition_date")->valuedouble;
			strftime(date, sizeof(date), "%a, %d %b %Y %T %z", localtime(&time));
			printf("\tAdded on: %s\n", date);

			time = cJSON_GetObjectItemCaseSensitive(json, "completion_date")->valuedouble;
			if (time != -1) {
				strftime(date, sizeof(date), "%a, %d %b %Y %T %z", localtime(&time));
				printf("\tCompleted on: %s\n", date);
			}

			property = cJSON_GetObjectItemCaseSensitive(json, "total_size");
			idx = human_size(&property->valuedouble);
			printf("\tTotal Size: %.2f%s\n", property->valuedouble, sizeSuffixes[idx]);
			property = cJSON_GetObjectItemCaseSensitive(json, "total_downloaded");
			idx = human_size(&property->valuedouble);
			printf("\tDownloaded: %.2f%s\n", property->valuedouble, sizeSuffixes[idx]);
			property = cJSON_GetObjectItemCaseSensitive(json, "total_uploaded");
			idx = human_size(&property->valuedouble);
			printf("\tUploaded: %.2f%s\n", property->valuedouble, sizeSuffixes[idx]);

			property = cJSON_GetObjectItemCaseSensitive(json, "dl_speed");
			idx = human_size(&property->valuedouble);
			printf("\tDown speed: %.2f%s/s\n", property->valuedouble, sizeSuffixes[idx]);
			property = cJSON_GetObjectItemCaseSensitive(json, "up_speed");
			idx = human_size(&property->valuedouble);
			printf("\tUp speed: %.2f%s/s\n", property->valuedouble, sizeSuffixes[idx]);

			printf("\tRatio: %.2f\n", cJSON_GetObjectItemCaseSensitive(json, "share_ratio")->valuedouble); 
			printf("\tConnections: %d\n", cJSON_GetObjectItemCaseSensitive(json, "nb_connections")->valueint); 

			cJSON_free(json);
			free(response.memory);
			return;
		}
		else
			goto PRINT_AND_CLEANUP;
	}

	if (!strcmp(*argv, "add")) {
		curl_mime *mime = curl_mime_init(curl);
		int c;
		while ((c = getopt(argc, argv, ":p:k:c:t:r:u:d:i:m:HPRASF")) != -1) {
			if (c == ':') {
				fprintf(stderr, "Option -%c requires an operand\n", optopt);
				exit(EXIT_FAILURE);
			}
			if (c == '?') {
				fprintf(stderr, "Unrecognized option: '-%c'\n", optopt);
				torrents_help();
				exit(EXIT_FAILURE);
			}

			for (int i = 0; addOpts[i].opt != '\0'; i++) {
				if (addOpts[i].opt == c) {
					// Boolean options don't have a placeholder; they don't accept an argument
					curl_mimepart *tmp = curl_mime_addpart(mime);
					curl_mime_name(tmp, addOpts[i].key);
					curl_mime_data(tmp, addOpts[i].placeholder[0] == '\0' ? "true" : optarg, CURL_ZERO_TERMINATED);
					break;
				}
			}
		}
		for (; optind < argc; optind++) {
			for (int j = 0; supportedLinks[j] != NULL; j++) {
				if (!strncmp(argv[optind], supportedLinks[j], strlen(supportedLinks[j]))) {
					strcat(postField, argv[optind]);
					strcat(postField, "\n");
					goto CONTINUE_OUTER_LOOP;
				}
			}

			curl_mimepart *torrentFile = curl_mime_addpart(mime);
			curl_mime_name(torrentFile, "torrents");
			curl_mime_filedata(torrentFile, argv[optind]);

		CONTINUE_OUTER_LOOP:
			continue;
		}

		if (postField[0] != '\0') {
			curl_mimepart *urls = curl_mime_addpart(mime);
			curl_mime_name(urls, "urls");
			curl_mime_data(urls, postField, CURL_ZERO_TERMINATED);
		}

		response = POST_MIME("/torrents/add", mime);
		goto PRINT_AND_CLEANUP;
	}

	if (!strcmp(*argv, "delete")) {
		bool deleteFiles = false;
		strcat(postField, "hashes=");
		for (int i = 1; i < argc; i++) {
			if (!strcmp(argv[i], "-F")) {
				deleteFiles = true;
				continue;
			}

			strcat(postField, argv[i]);
			strcat(postField, "|");
		}
		postField[strlen(postField)-1] = '\0';
		if (deleteFiles)
			strcat(postField, "&deleteFiles=true");
		else
			strcat(postField, "&deleteFiles=true");

		response = POST("/torrents/delete", postField);
		goto PRINT_AND_CLEANUP;
	}

	if (!strcmp(*argv, "rename") && argc == 3) {
		strcat(postField, "hash=");
		strcat(postField, argv[2]);
		strcat(postField, "&name=");
		strcat(postField, argv[1]);

		response = POST("/torrents/rename", postField);
		goto PRINT_AND_CLEANUP;
	}

	if (!strcmp(*argv, "show") && argc == 3) {
		if (!strcmp(argv[1], "webseeds")) {
			strcat(postField, "hash=");
			strcat(postField, argv[2]);
			response = POST("/torrents/webseeds", postField);
			goto PRINT_AND_CLEANUP;
		}
		if (!strcmp(argv[1], "files")) {
			strcat(postField, "hash=");
			strcat(postField, argv[2]);
			response = POST("/torrents/files", postField);
			cJSON *json = cJSON_Parse(response.memory);
			if (cJSON_IsArray(json)) {
				printf("Index Size       Priority Progress File\n");
				int idx;
				char adjustBuf[LARGEST_INT_LENGTH];
				char adjustBuf2[LARGEST_INT_LENGTH];
				char *priorities[] = { "Skip", "Normal", "", "", "", "", "High", "Maximal" };
				cJSON *file;
				cJSON_ArrayForEach(file, json) {
					if (!cJSON_IsObject(file))
						continue;
					idx = human_size(&cJSON_GetObjectItemCaseSensitive(file, "size")->valuedouble);
					snprintf(
						adjustBuf, LARGEST_INT_LENGTH, "%.2f%s",
						cJSON_GetObjectItemCaseSensitive(file, "size")->valuedouble,
						sizeSuffixes[idx]
					);
					snprintf(
						adjustBuf2, LARGEST_INT_LENGTH, "%.1f%%",
						cJSON_GetObjectItemCaseSensitive(file, "priority")->valuedouble * 100
					);
					printf(
						"%-5d %-10s %-8s %-8s %s\n",
						cJSON_GetObjectItemCaseSensitive(file, "index")->valueint,
						adjustBuf,
						priorities[cJSON_GetObjectItemCaseSensitive(file, "priority")->valueint],
						adjustBuf2,
						cJSON_GetObjectItemCaseSensitive(file, "name")->valuestring
					);
				}
				cJSON_free(json);
				free(response.memory);
				return;
			}
			else
				goto PRINT_AND_CLEANUP;
		}
		if (!strcmp(argv[1], "pieces")) {
			strcat(postField, "hash=");
			strcat(postField, argv[2]);
			response = POST("/torrents/pieceStates", postField);
			goto PRINT_AND_CLEANUP;
		}
		if (!strcmp(argv[1], "limits")) {
			strcat(postField, "hashes=");
			strcat(postField, argv[2]);

			int idx;
			response = POST("/torrents/downloadLimit", postField);
			cJSON *json = cJSON_Parse(response.memory);
			if (json != NULL && cJSON_IsObject(json)) {
				cJSON *limit = cJSON_GetObjectItemCaseSensitive(json, argv[2]);
				if (cJSON_IsNumber(limit)) {
					idx = human_size(&limit->valuedouble);
					printf("Download: %.2f%s/s\n", limit->valuedouble, sizeSuffixes[idx]);
				}

				cJSON_free(json);
				free(response.memory);
				response.size = 0;
				response.memory = NULL;
			}

			response = POST("/torrents/uploadLimit", postField);
			json = cJSON_Parse(response.memory);
			if (json != NULL && cJSON_IsObject(json)) {
				cJSON *limit = cJSON_GetObjectItemCaseSensitive(json, argv[2]);
				if (cJSON_IsNumber(limit)) {
					idx = human_size(&limit->valuedouble);
					printf("Upload: %.2f%s/s\n", limit->valuedouble, sizeSuffixes[idx]);
				}

				cJSON_free(json);
				free(response.memory);
				response.size = 0;
				response.memory = NULL;
			}

			return;
		}
		if (!strcmp(argv[1], "trackers")) {
			strcat(postField, "hash=");
			strcat(postField, argv[2]);
			response = POST("/torrents/trackers", postField);
			cJSON *json = cJSON_Parse(response.memory);
			if (cJSON_IsArray(json)) {
				printf("Tier Peers Seeders Leechers Downloaded Status      Url\n");
				const char *status[] = { "Disabled", "No contact", "Working", "Updating", "Not working" };
				cJSON *tracker;
				cJSON_ArrayForEach(tracker, json) {
					printf(
						"%-4d %-5d %-7d %-8d %-10d %-11s %s\n",
						cJSON_GetObjectItemCaseSensitive(tracker, "tier")->valueint,
						cJSON_GetObjectItemCaseSensitive(tracker, "num_peers")->valueint,
						cJSON_GetObjectItemCaseSensitive(tracker, "num_seeds")->valueint,
						cJSON_GetObjectItemCaseSensitive(tracker, "num_leeches")->valueint,
						cJSON_GetObjectItemCaseSensitive(tracker, "num_downloaded")->valueint,
						status[cJSON_GetObjectItemCaseSensitive(tracker, "status")->valueint],
						cJSON_GetObjectItemCaseSensitive(tracker, "url")->valuestring
					);
				}
				cJSON_free(json);
				free(response.memory);
				return;
			}
			else
				goto PRINT_AND_CLEANUP;
		}
		if (!strcmp(argv[1], "category")) {
			response = GET("/torrents/info");
			cJSON *json = cJSON_Parse(response.memory);
			if (cJSON_IsArray(json)) {
				cJSON *torrent;
				cJSON_ArrayForEach(torrent, json) {
					if (!strcmp(argv[2], cJSON_GetObjectItemCaseSensitive(torrent, "hash")->valuestring)) {
						printf("%s\n", cJSON_GetObjectItemCaseSensitive(torrent, "category")->valuestring);
						break;
					}
				}
			}
			cJSON_free(json);
			free(response.memory);
			response.size = 0;
			response.memory = NULL;
			goto PRINT_AND_CLEANUP;
		}
	}

	if (!strcmp(*argv, "set") && argc > 3) {
		if (!strcmp(argv[1], "downloadlimit")) {
			strcat(postField, "hashes=");
			strcat(postField, argv[3]);
			strcat(postField, "&limit=");
			strcat(postField, argv[2]);
			response = POST("/torrents/setDownloadLimit", postField);
			goto PRINT_AND_CLEANUP;
		}
		if (!strcmp(argv[1], "uploadlimit")) {
			strcat(postField, "hashes=");
			strcat(postField, argv[3]);
			strcat(postField, "&limit=");
			strcat(postField, argv[2]);
			response = POST("/torrents/setUploadLimit", postField);
			goto PRINT_AND_CLEANUP;
		}
		if (!strcmp(argv[1], "trackers")) {
			bool removeAll = false;
			char *toRemove[256];
			char *toAdd[256];
			int removeCounter = 0;
			int addCounter = 0;
			for (int i = 2; i < argc-1; i++) {
				if (!strcmp(argv[i], "-")) {
					removeAll = true;
					continue;
				}

				if (argv[i][0] == '-') {
					toRemove[removeCounter++] = argv[i] + 1;
					continue;
				}

				if (argv[i][0] == '+')
					argv[i]++;
				toAdd[addCounter++] = argv[i];
			}

			if (removeAll) {
				removeCounter = 0;
				strcat(postField, "hash=");
				strcat(postField, argv[argc-1]);
				response = POST("/torrents/trackers", postField);
				cJSON *json = cJSON_Parse(response.memory);
				if (cJSON_IsArray(json)) {
					strcat(postField, "&urls=");
					cJSON *tracker;
					cJSON_ArrayForEach(tracker, json) {
						if (cJSON_IsObject(tracker)) {
							cJSON *url = cJSON_GetObjectItemCaseSensitive(tracker, "url");
							if (cJSON_IsString(url)) {
								if (url->valuestring[0] == '*')
									continue;

								strcat(postField, url->valuestring);
								strcat(postField, "|");
							}
						}
					}
					// URLs could be empty
					if (postField[strlen(postField)-1] == '|') {
						postField[strlen(postField)-1] = '\0';
						struct MemoryStruct tmp = POST("/torrents/removeTrackers", postField);
						free(tmp.memory);
					}
				}
				cJSON_free(json);
				free(response.memory);
				response.size = 0;
				response.memory = NULL;
				postField[0] = '\0';
			}

			strcat(postField, "hash=");
			strcat(postField, argv[argc-1]);
			strcat(postField, "&urls=");
			for (int i = 0; i < removeCounter; i++) {
				strcat(postField, toRemove[i]);
				strcat(postField, "|");
			}
			// URLs could be empty
			if (postField[strlen(postField)-1] == '|') {
				postField[strlen(postField)-1] = '\0';
				struct MemoryStruct tmp = POST("/torrents/removeTrackers", postField);
				free(tmp.memory);
				postField[0] = '\0';
			}

			if (postField[0] == '\0') {
				strcat(postField, "hash=");
				strcat(postField, argv[argc-1]);
				strcat(postField, "&urls=");
			}

			for (int i = 0; i < addCounter; i++) {
				strcat(postField, toAdd[i]);
				strcat(postField, "%0A");
			}
			// URLs could be empty
			size_t len = strlen(postField);
			if (postField[len-3] == '%' && postField[len-2] == '0' && postField[len-1] == 'A') {
				postField[strlen(postField)-3] = '\0';
				response = POST("/torrents/addTrackers", postField);
			}
			goto PRINT_AND_CLEANUP;
		}
		if (!strcmp(argv[1], "category")) {
			strcat(postField, "hashes=");
			strcat(postField, argv[3]);
			strcat(postField, "&category=");
			strcat(postField, argv[2]);
			response = POST("/torrents/setCategory", postField);
			goto PRINT_AND_CLEANUP;
		}
	}

	if (!strcmp(*argv, "toggle") && argc == 3) {
		if (!strcmp(argv[1], "sequential")) {
			strcat(postField, "hashes");
			strcat(postField, argv[2]);
			response = POST("/torrents/toggleSequentialDownload", postField);
			goto PRINT_AND_CLEANUP;
		}
		if (!strcmp(argv[1], "firstandlastpieces")) {
			strcat(postField, "hashes");
			strcat(postField, argv[2]);
			response = POST("/torrents/toggleFirstLastPiecePrio", postField);
			goto PRINT_AND_CLEANUP;
		}
		if (!strcmp(argv[1], "superseeding")) {
			bool wasEnabled = false;
			response = GET("/torrents/info");
			cJSON *json = cJSON_Parse(response.memory);
			if (cJSON_IsArray(json)) {
				cJSON *torrent;
				cJSON_ArrayForEach(torrent, json) {
					if (!strcmp(argv[2], cJSON_GetObjectItemCaseSensitive(torrent, "hash")->valuestring)) {
						wasEnabled = cJSON_GetObjectItemCaseSensitive(torrent, "super_seeding")->valueint;
						break;
					}
				}
			}
			cJSON_free(json);
			free(response.memory);
			response.size = 0;
			response.memory = NULL;

			strcat(postField, "hashes=");
			strcat(postField, argv[2]);
			strcat(postField, "&value=");
			strcat(postField, wasEnabled ? "false" : "true");
			response = POST("/torrents/setSuperSeeding", postField);
			puts(wasEnabled ? "Disabled" : "Enabled");
			goto PRINT_AND_CLEANUP;
		}
		if (!strcmp(argv[1], "forcestart")) {
			bool wasEnabled = false;
			response = GET("/torrents/info");
			cJSON *json = cJSON_Parse(response.memory);
			if (cJSON_IsArray(json)) {
				cJSON *torrent;
				cJSON_ArrayForEach(torrent, json) {
					if (!strcmp(argv[2], cJSON_GetObjectItemCaseSensitive(torrent, "hash")->valuestring)) {
						wasEnabled = cJSON_GetObjectItemCaseSensitive(torrent, "force_start")->valueint;
						break;
					}
				}
			}
			cJSON_free(json);
			free(response.memory);
			response.size = 0;
			response.memory = NULL;

			strcat(postField, "hashes=");
			strcat(postField, argv[2]);
			strcat(postField, "&value=");
			strcat(postField, wasEnabled ? "false" : "true");
			response = POST("/torrents/setForceStart", postField);
			puts(wasEnabled ? "Disabled" : "Enabled");
			goto PRINT_AND_CLEANUP;
		}
		if (!strcmp(argv[1], "automanagement")) {
			bool wasEnabled = false;
			response = GET("/torrents/info");
			cJSON *json = cJSON_Parse(response.memory);
			if (cJSON_IsArray(json)) {
				cJSON *torrent;
				cJSON_ArrayForEach(torrent, json) {
					if (!strcmp(argv[2], cJSON_GetObjectItemCaseSensitive(torrent, "hash")->valuestring)) {
						wasEnabled = cJSON_GetObjectItemCaseSensitive(torrent, "auto_tmm")->valueint;
						break;
					}
				}
			}
			cJSON_free(json);
			free(response.memory);
			response.size = 0;
			response.memory = NULL;

			strcat(postField, "hashes=");
			strcat(postField, argv[2]);
			strcat(postField, "&enable=");
			strcat(postField, wasEnabled ? "false" : "true");
			response = POST("/torrents/setAutoManagement", postField);
			puts(wasEnabled ? "Disabled" : "Enabled");
			goto PRINT_AND_CLEANUP;
		}
	}

	torrents_help();
	exit(EXIT_FAILURE);

PRINT_AND_CLEANUP:
	if (response.size)
		printf("%s\n", response.memory);
	free(response.memory);
}
