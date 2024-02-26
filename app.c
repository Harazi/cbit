#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "callbacks.h"
#include "do_commands.h"
#include "auth.h"

#define CMD "app"

void app_help(void)
{
	printf("Usage: "PROGRAM_NAME" "CMD" { version | webapiversion | buildinfo | defaultsavepath }\n");
}

void do_app(int argc, char **argv)
{
	struct MemoryStruct response; 
	char postField[BUFSIZ] = "";

	if (argc < 1 || !strcmp(*argv, "help")) {
		app_help();
		exit(EXIT_SUCCESS);
	}

	auth_login();

	if (!strcmp(*argv, "version"))
		response = GET("/app/version");
	else if (!strcmp(*argv, "webapiversion"))
		response = GET("/app/webapiVersion");
	else if (!strcmp(*argv, "buildinfo"))
		response = GET("/app/buildInfo");
	else if (!strcmp(*argv, "defaultsavepath"))
		response = GET("/app/defaultSavePath");
	else if (!strcmp(*argv, "shutdown"))
		response = POST("/app/shutdown", "");
	else if (!strcmp(*argv, "preferences"))
		response = GET("/app/preferences");
	else if (!strcmp(*argv, "setpreferences")) {
		if (argc < 2) {
			fprintf(stderr, "Missing preferences argument\n");
			exit(EXIT_FAILURE);
		}
		strcat(postField, "json=");
		strcat(postField, argv[1]);
		response = POST("/app/setPreferences", postField);
	}
	else {
		fprintf(stderr, "Command \"%s\" is unknown, try \""PROGRAM_NAME" "CMD" help\".\n", *argv);
		exit(EXIT_FAILURE);
	}

	if (response.size)
		printf("%s\n", response.memory);
	free(response.memory);
}
