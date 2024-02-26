#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "config.h"
#include "callbacks.h"
#include "do_commands.h"
#include "auth.h"

#define CMD "torrents"

void torrents_help(void)
{
	printf(
		"Usage: "PROGRAM_NAME" "CMD" info\n"
		"       "PROGRAM_NAME" "CMD" { properties | trackers | webseeds | files | piecestates | piecehashes } HASH\n"
		"       "PROGRAM_NAME" "CMD" { pause | resume | delete | recheck | reannounce } HASH\n"
		"       "PROGRAM_NAME" "CMD" add URI...\n"
		"       "PROGRAM_NAME" "CMD" { addtrackers | removetrackers } HASH TRACKER...\n"
	);
}

static inline struct MemoryStruct torrents_post_hash(int argc, char **argv, const char *path)
{
	char postField[BUFSIZ] = "hash=";
	if (argc < 2) {
		fprintf(stderr, "Missing hash argument\n");
		exit(EXIT_FAILURE);
	}
	strcat(postField, argv[1]);
	return POST(path, postField);
}

void do_torrents(int argc, char **argv)
{
	struct MemoryStruct response;
	char postField[BUFSIZ] = "";

	if (argc < 1 || !strcmp(*argv, "help")) {
		torrents_help();
		exit(EXIT_SUCCESS);
	}

	auth_login();

	if (!strcmp(*argv, "info"))
		response = GET("/torrents/info");
	else if (!strcmp(*argv, "properties"))
		response = torrents_post_hash(argc, argv, "/torrents/properties");
	else if (!strcmp(*argv, "trackers"))
		response = torrents_post_hash(argc, argv, "/torrents/trackers");
	else if (!strcmp(*argv, "webseeds"))
		response = torrents_post_hash(argc, argv, "/torrents/webseeds");
	else if (!strcmp(*argv, "files"))
		response = torrents_post_hash(argc, argv, "/torrents/files");
	else if (!strcmp(*argv, "piecestates"))
		response = torrents_post_hash(argc, argv, "/torrents/pieceStates");
	else if (!strcmp(*argv, "piecehashes"))
		response = torrents_post_hash(argc, argv, "/torrents/pieceHashes");
	else if (!strcmp(*argv, "pause"))
		response = torrents_post_hash(argc, argv, "/torrents/pause");
	else if (!strcmp(*argv, "resume"))
		response = torrents_post_hash(argc, argv, "/torrents/resume");
	else if (!strcmp(*argv, "delete"))
		response = torrents_post_hash(argc, argv, "/torrents/delete");
	else if (!strcmp(*argv, "recheck"))
		response = torrents_post_hash(argc, argv, "/torrents/recheck");
	else if (!strcmp(*argv, "reannounce"))
		response = torrents_post_hash(argc, argv, "/torrents/reannounce");
	else if (!strcmp(*argv, "add")) {
		if (argc < 2) {
			fprintf(stderr, "Missing URI argument\n");
			exit(EXIT_FAILURE);
		}
		curl_mime *mime = curl_mime_init(curl);
		curl_mimepart *part = curl_mime_addpart(mime);
		curl_mime_name(part, "urls");
		curl_mime_data(part, argv[1], CURL_ZERO_TERMINATED);

		POST_MIME("/torrents/add", mime);
	}
	else if (!strcmp(*argv, "addtrackers")) {
		if (argc < 2) {
			fprintf(stderr, "Missing hash and trackers arguments\n");
			exit(EXIT_FAILURE);
		}
		if (argc < 3) {
			fprintf(stderr, "Missing trackers argument\n");
			exit(EXIT_FAILURE);
		}
		strcat(postField, "hash=");
		strcat(postField, *(++argv));
		argc--;
		strcat(postField, "urls=");
		strcat(postField, *(++argv));
		argc--;
		while (--argc > 0) {
			strcat(postField, "%0A");
			strcat(postField, *(++argv));
		}
		response = POST("/torrents/addTrackers", postField);
	} else if (!strcmp(*argv, "removetrackers")) {
		if (argc < 2) {
			fprintf(stderr, "Missing hash and trackers arguments\n");
			exit(EXIT_FAILURE);
		}
		if (argc < 3) {
			fprintf(stderr, "Missing trackers argument\n");
			exit(EXIT_FAILURE);
		}
		strcat(postField, "hash=");
		strcat(postField, *(++argv));
		argc--;
		strcat(postField, "urls=");
		strcat(postField, *(++argv));
		argc--;
		while (--argc > 0) {
			strcat(postField, "|");
			strcat(postField, *(++argv));
		}
		response = POST("/torrents/removeTrackers", postField);
	}
	else {
		fprintf(stderr, "Command \"%s\" is unknown, try \""PROGRAM_NAME" "CMD" help\".\n", *argv);
		exit(EXIT_FAILURE);
	}

	if (response.size)
		printf("%s\n", response.memory);
	free(response.memory);
}
