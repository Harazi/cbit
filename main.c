#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
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
	{ "torrents", do_torrents },
	{ "session", do_session },
	{ "settings", do_settings },
	{ 0, NULL }
};

int main(int argc, char *argv[])
{
	memset(&config, 0, sizeof(config));
	config.flags.color = isatty(STDOUT_FILENO);

	argc--;
	argv++;
	int parsedOptions = 0;
	for (; parsedOptions < argc; parsedOptions++) {
		if (!strcmp(argv[parsedOptions], "-v") || !strcmp(argv[parsedOptions], "--version")) {
			printf("%s %s\n", PROGRAM_NAME, PROGRAM_VERSION);
			return EXIT_SUCCESS;
		}
		if (!strcmp(argv[parsedOptions], "-h") || !strcmp(argv[parsedOptions], "--help")) {
			print_help(stdout);
			return EXIT_SUCCESS;
		}
		if (!strcmp(argv[parsedOptions], "-c") || !strcmp(argv[parsedOptions], "--config")) {
			if (parsedOptions+1 >= argc) {
				fprintf(stderr, "Incomplete option %s\n", argv[parsedOptions]);
				return EXIT_FAILURE;
			}
			strcat(confFile, argv[++parsedOptions]);
			continue;
		}
		if (!strcmp(argv[parsedOptions], "-C") || !strcmp(argv[parsedOptions], "--color")) {
			config.flags.color = true;
			continue;
		}

		break;
	}
	argc -= parsedOptions;
	argv += parsedOptions;

	if (*confFile == '\0') {
		char *confDir = getenv("XDG_CONFIG_DIR");
		if (confDir == NULL) {
			confDir = getenv("HOME");
			strcat(confFile, confDir);
			strcat(confFile, "/.config/cbit");
		} else {
			strcat(confFile, confDir);
			strcat(confFile, "/cbit");
		}
		if (mkdir(confFile, 0700)) {
			if (errno == EEXIST) {
				errno = 0;
			} else {
				perror(confFile);
				return errno;
			}
		}

		strcat(confFile, "/cbit.ini");
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
		char *vec[] = { "stats" };
		do_session(1, vec);
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
