#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "callbacks.h"
#include "do_commands.h"
#include "auth.h"

#define CMD "sync"

void sync_help(void)
{
	printf(
		"Usage: "PROGRAM_NAME" "CMD" maindata\n"
		"       "PROGRAM_NAME" "CMD" torrentpeers HASH\n"
	);
}

void do_sync(int argc, char **argv)
{
	struct MemoryStruct response;
	char postField[BUFSIZ] = "";

	if (argc < 1 || !strcmp(*argv, "help")) {
		sync_help();
		exit(EXIT_SUCCESS);
	}

	auth_login();

	if (!strcmp(*argv, "maindata"))
		response = GET("/sync/maindata");
	else if (!strcmp(*argv, "torrentpeers")) {
		if (argc < 2) {
			fprintf(stderr, "Missing hash argument\n");
			exit(EXIT_FAILURE);
		}
		strcat(postField, "hash=");
		strcat(postField, argv[1]);
		response = POST("/sync/torrentPeers", postField);
	}
	else {
		fprintf(stderr, "Command \"%s\" is unknown, try \""PROGRAM_NAME" "CMD" help\".\n", *argv);
		exit(EXIT_FAILURE);
	}

	if (response.size)
		printf("%s\n", response.memory);
	free(response.memory);
}
