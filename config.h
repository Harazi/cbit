#include <curl/curl.h>

#define PROGRAM_NAME "qbit-cli"
#define PROGRAM_VERSION "v0.1"
// https://stackoverflow.com/q/1848700
#define LARGEST_INT_LENGTH 18

struct AUTH {
	char *username;
	char *password;
};

struct SERVER {
	char *url;
};

struct CONFIG {
	struct AUTH auth;
	struct SERVER server;
};

extern struct CONFIG config;
extern CURL *curl;
extern char confFile[];


void create_config_file(const char *path);
void parse_config_file(const char *path);
