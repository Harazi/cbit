#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <curl/curl.h>
#include "config.h"
#include "do_commands.h"

void print_help(FILE *f)
{
	fprintf(f,
		"Usage: %s [ OPTIONS ] OBJECT { COMMAND | help }\n"
		"\n"
		"Objects:\n"
		"    config     Get or set %s configurations\n"
		"    app        Get qBittorrent app data\n"
		"    torrent    Operations related to torrents\n"
		"    setting    Get or set qBittorrent settings\n"
		"\n"
		"Options:\n"
		"    { -h | --help }             print help and exit\n"
		"    { -v | --version }          print version and exit\n"
		"    { -c | --config } <FILE>    specify config file\n",
		PROGRAM_NAME,
		PROGRAM_NAME);
}

struct {
	const char *cmd;
	void (*func)(int argc, char **argv);
} cmds[] = {
	{ "config", do_config },
	{ "app", do_app },
	{ "log", do_log },
	{ "sync", do_sync },
	{ "transfer", do_transfer },
	{ "torrents", do_torrents },
	{ "session", do_session },
	{ "settings", do_settings },
	{ 0, NULL }
};

int main(int argc, char *argv[])
{
	argc--;
	argv++;

	for (int n = 0; n < argc; n++) {
		if (!strcmp(argv[n], "-v") || !strcmp(argv[n], "--version")) {
			printf("%s %s\n", PROGRAM_NAME, PROGRAM_VERSION);
			return EXIT_SUCCESS;
		}
		if (!strcmp(argv[n], "-h") || !strcmp(argv[n], "--help")) {
			print_help(stdout);
			return EXIT_SUCCESS;
		}
		if (!strcmp(argv[n], "-c") || !strcmp(argv[n], "--config")) {
			if (n+1 >= argc) {
				fprintf(stderr, "Incomplete option %s\n", argv[n]);
				return EXIT_FAILURE;
			}
			strcat(confFile, argv[++n]);
			continue;
		}

		argc -= n;
		argv += n;
		break;
	}

	if (*confFile == '\0') {
		char *confDir = getenv("XDG_CONFIG_DIR");
		if (confDir == NULL) {
			confDir = getenv("HOME");
			strcat(confFile, confDir);
			strcat(confFile, "/.config/qbit-cli");
		} else {
			strcat(confFile, confDir);
			strcat(confFile, "/qbit-cli");
		}
		if (mkdir(confFile, 0700)) {
			if (errno == EEXIST) {
				errno = 0;
			} else {
				perror(confFile);
				return errno;
			}
		}

		strcat(confFile, "/qbit-cli.ini");
		FILE *f = fopen(confFile, "r");
		if (f == NULL) {
			if (errno == ENOENT) {
				errno = 0;
				create_config_file(confFile);
			} else {
				perror(confFile);
				return errno;
			}
		} else if (fclose(f) == EOF) {
			perror(confFile);
			exit(errno);
		}
	}

	parse_config_file(confFile);

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ""); // enable cookie engine

	if (argc == 0) {
		char *vec[] = { "info" };
		do_transfer(1, vec);
		goto CLEANUP;
	}

	if (!strcmp(*argv, "help")) {
		print_help(stdout);
		goto CLEANUP;
	}

	for (int i = 0; cmds[i].cmd != 0; i++) {
		if (!strcmp(*argv, cmds[i].cmd)) {
			cmds[i].func(--argc, ++argv);
			goto CLEANUP;
		}
	}

	fprintf(stderr, "Object \"%s\" is unknown, try \"%s help\".\n", *argv, PROGRAM_NAME);
	return EXIT_FAILURE;

CLEANUP:
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return EXIT_SUCCESS;
}
