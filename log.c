#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "callbacks.h"
#include "auth.h"

#define CMD "log"

void log_help(void)
{
	printf("Usage: "PROGRAM_NAME" "CMD" { main | peers }\n");
}

void do_log(int argc, char **argv)
{
	struct MemoryStruct response; 

	if (argc < 1 || !strcmp(*argv, "help")) {
		log_help();
		exit(EXIT_SUCCESS);
	}

	auth_login();

	if (!strcmp(*argv, "main"))
		response = GET("/log/main");
	else if (!strcmp(*argv, "peers"))
		response = GET("/log/peers");
	else {
		fprintf(stderr, "Command \"%s\" is unknown, try \""PROGRAM_NAME" "CMD" help\".\n", *argv);
		exit(EXIT_FAILURE);
	}

	if (response.size)
		printf("%s\n", response.memory);
	free(response.memory);
}
