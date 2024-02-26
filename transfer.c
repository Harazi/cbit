#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"
#include "callbacks.h"
#include "do_commands.h"
#include "auth.h"

#define CMD "transfer"

void transfer_help(void)
{
	printf(
		"Usage: "PROGRAM_NAME" "CMD" { info | speedlimitsmode | togglespeedlimitsmode | downloadlimit | uploadlimit }\n"
		"       "PROGRAM_NAME" "CMD" { setdownloadlimit | setuploadlimit } LIMIT\n"
		"       "PROGRAM_NAME" "CMD" banpeers PEER...\n"
		"\n"
		"LIMIT := bytes/second\n"
		"PEER := IP:PORT\n"
	);
}

void do_transfer(int argc, char **argv)
{
	struct MemoryStruct response;
	char postField[BUFSIZ] = "";
	bool needFree = true;

	if (argc < 1 || !strcmp(*argv, "help")) {
		transfer_help();
		exit(EXIT_SUCCESS);
	}

	auth_login();

	if (!strcmp(*argv, "info"))
		response = GET("/transfer/info");
	else if (!strcmp(*argv, "speedlimitsmode")) {
		response = GET("/transfer/speedLimitsMode");
		if (response.size == 1) {
			bool enabled = response.memory[0] == '1';
			free(response.memory);
			response.memory = enabled ? "Enabled" : "Disabled";
			needFree = false;
		}
	}
	else if (!strcmp(*argv, "togglespeedlimitsmode"))
		response = POST("/transfer/toggleSpeedLimitsMode", "");
	else if (!strcmp(*argv, "downloadlimit"))
		response = GET("/transfer/downloadLimit");
	else if (!strcmp(*argv, "uploadlimit"))
		response = GET("/transfer/uploadLimit");
	else if (!strcmp(*argv, "setdownloadlimit")) {
		if (argc < 2) {
			fprintf(stderr, "Missing limit argument\n");
			exit(EXIT_FAILURE);
		}
		strcat(postField, "limit=");
		strcat(postField, argv[1]);
		response = POST("/transfer/setDownloadLimit", postField);
	} else if (!strcmp(*argv, "setuploadlimit")) {
		if (argc < 2) {
			fprintf(stderr, "Missing limit argument\n");
			exit(EXIT_FAILURE);
		}
		strcat(postField, "limit=");
		strcat(postField, argv[1]);
		response = POST("/transfer/setUploadLimit", postField);
	} else if (!strcmp(*argv, "banpeers")) {
		if (argc < 2) {
			fprintf(stderr, "Missing peers argument\n");
			exit(EXIT_FAILURE);
		}
		char peers[BUFSIZ] = "peers=";
		strcat(peers, *(++argv));
		argc--;
		while (--argc > 0) {
			strcat(peers, "|");
			strcat(peers, *(++argv));
		}
		response = POST("/transfer/banPeers", peers);
	} else {
		fprintf(stderr, "Command \"%s\" is unknown, try \""PROGRAM_NAME" "CMD" help\".\n", *argv);
		exit(EXIT_FAILURE);
	}

	if (response.size)
		printf("%s\n", response.memory);
	if (needFree)
		free(response.memory);
}
