#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#define CMD "config"

struct CONFIG config;
CURL *curl;
char confFile[BUFSIZ] = "";

void create_config_file(const char *path)
{
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		perror(path);
		exit(errno);
	}

	fprintf(f,
		 "[Auth]\n"
		 "Username=admin\n"
		 "Password=adminadmin\n"
		 "[Server]\n"
		 "Url=http://localhost:8080\n");

	if (fclose(f) == EOF) {
		perror(path);
		exit(errno);
	}
}

void parse_config_file(const char *path)
{
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		perror(path);
		exit(errno);
	}

	enum SECTION { MAIN, AUTH, SERVER } section = MAIN;

	char *line = NULL;
	size_t lineLength = 0;
	while (getline(&line, &lineLength, f) != -1) {
		char *l = line;
		while (isblank(*l))
			l++;
		if (*l == '\n' || *l == '#')
			continue;

		if (*l == '[') {
			if (!strncmp(l, "[Auth]", 6))
				section = AUTH;
			if (!strncmp(l, "[Server]", 8))
				section = SERVER;

			continue;
		}

		if (section == AUTH) {
			if (!strncmp(l, "Username", 8)) {
				l += 8;
				while (isblank(*l))
					l++;
				if (*(l++) != '=')
					goto FAILED_PARSING;
				while (isblank(*l))
					l++;

				int i = 0;
				while (isgraph(l[i]))
					i++;
				config.auth.username = strndup(l, i);
			}
			if (!strncmp(l, "Password", 8)) {
				l += 8;
				while (isblank(*l))
					l++;
				if (*(l++) != '=')
					goto FAILED_PARSING;
				while (isblank(*l))
					l++;

				int i = 0;
				while (isgraph(l[i]))
					i++;
				config.auth.password = strndup(l, i);
			}
		}

		if (section == SERVER) {
			if (!strncmp(l, "Url", 3)) {
				l += 3;
				while (isblank(*l))
					l++;
				if (*(l++) != '=')
					goto FAILED_PARSING;
				while (isblank(*l))
					l++;

				int i = 0;
				while (isgraph(l[i]))
					i++;
				config.server.url = strndup(l, i);
			}
		}
	}
	if (ferror(f)) {
		perror(path);
		exit(errno);
	}

	free(line);
	if (fclose(f) == EOF) {
		perror(path);
		exit(errno);
	}
	return;

FAILED_PARSING:
	fprintf(stderr, "%s: Couldn't parse config file:\n\t%s", path, line);
	exit(EXIT_FAILURE);
}

void config_help(void)
{
	printf(
		"Usage: "PROGRAM_NAME" "CMD" KEY [VALUE]\n"
		"       "PROGRAM_NAME" "CMD" [-e|--edit]\n"
	);
}

void do_config(int argc, char **argv)
{
	if (argc < 1 || !strcmp(*argv, "help")) {
		config_help();
		exit(EXIT_SUCCESS);
	}
	else if (!strcmp(*argv, "-e") || !strcmp(*argv, "--edit")) {
		char *editor = getenv("EDITOR");
		if (editor == NULL)
			editor = getenv("VISUAL");
		if (editor == NULL)
			editor = "vim";

		execlp(editor, editor, confFile, (char *)NULL);
		perror(editor);
		exit(errno);
	}
	else if (argc == 1) {
		char section[BUFSIZ] = "[";
		char *key = strchr(argv[0], '.');
		if (key != NULL) {
			*key = '\0';
			key++;
		}
		strcat(section, argv[0]);
		strcat(section, "]\n");

		FILE *f = fopen(confFile, "r");
		if (f == NULL) {
			perror(confFile);
			exit(errno);
		}

		char *line = NULL;
		size_t lineLength = 0;
		bool inSection = false;
		while (getline(&line, &lineLength, f) != -1) {
			if (!inSection && !strcmp(line, section)) {
				inSection = true;
				if (key == NULL)
					printf("%s", line);
				continue;
			}
			if (inSection) {
				if (*line == '[')
					break;
				if (key == NULL) {
					printf("%s", line);
					continue;
				}
				if (!strncmp(line, key, strlen(key))) {
					char *ptr = line + strlen(key);
					while (isblank(*ptr))
						ptr++;
					if (*(ptr++) != '=')
						continue;
					while (isblank(*ptr))
						ptr++;
					printf("%s", ptr);
				}
			}
		}
		if (ferror(f)) {
			perror(confFile);
			exit(errno);
		}
		free(line);
		if (fclose(f) == EOF) {
			perror(confFile);
			exit(errno);
		}
	}
	else if (argc == 2) {
		char section[BUFSIZ] = "[";
		char *key = strchr(argv[0], '.');
		if (key == NULL) {
			config_help();
			exit(EXIT_FAILURE);
		}
		*key = '\0';
		key++;
		strcat(section, argv[0]);
		strcat(section, "]");

		FILE *f = fopen(confFile, "r+");
		if (f == NULL) {
			perror(confFile);
			exit(errno);
		}

		char *line = NULL;
		size_t lineLength = 0;
		bool inSection = false;
		bool storeInMem = false;
		unsigned long offset = 0;
		char mem[BUFSIZ] = "";
		char val[BUFSIZ] = "";
		while (getline(&line, &lineLength, f) != -1) {
			if (storeInMem) {
				strcat(mem, line);
				continue;
			}
			if (!inSection && !strcmp(line, section)) {
				inSection = true;
				continue;
			}
			if (inSection) {
				if (*line == '[') {
					offset = ftell(f) - strlen(line);
					storeInMem = true;
					strcat(mem, line);
					strcat(val, key);
					strcat(val, "=");
					strcat(val, argv[1]);
					continue;
				}
				if (!strncmp(line, key, strlen(key))) {
					char *ptr = line + strlen(key);
					while (isblank(*ptr))
						ptr++;
					if (*(ptr++) != '=')
						continue;

					offset = ftell(f) - (strlen(line) - (unsigned long) (ptr - line));
					storeInMem = true;
					strcat(val, argv[1]);
					continue;
				}
			}
		}
		if (ferror(f)) {
			perror(confFile);
			exit(errno);
		}
		free(line);

		if (!storeInMem) {
			//! Create the section
			fprintf(f, "%s\n%s=%s\n", section, key, argv[1]);
		} else {
			if (fseek(f, offset, SEEK_SET)) {
				perror(confFile);
				exit(errno);
			}
			fprintf(f, "%s\n", val);
			fprintf(f, "%s", mem);
			if (truncate(confFile, ftell(f))) {
				perror("truncate");
				exit(errno);
			}
		}

		if (fclose(f) == EOF) {
			perror(confFile);
			exit(errno);
		}
	}
	else {
		config_help();
		exit(EXIT_FAILURE);
	}
}
