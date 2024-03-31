#include <stdbool.h>
#include <curl/curl.h>

#define PROGRAM_NAME "cbit"
#define PROGRAM_VERSION "v0.1"
// https://stackoverflow.com/q/1848700
#define LARGEST_INT_LENGTH 18
#define DEFAULT_USERNAME "admin"
#define DEFAULT_PASSWORD "adminadmin"
#define DEFAULT_URL      "http://localhost:8080"

struct AUTH {
	char *username;
	char *password;
};

struct SERVER {
	char *url;
};

struct FLAGS {
	bool color;
};

struct CONFIG {
	struct AUTH auth;
	struct SERVER server;
	struct FLAGS flags;
};

extern struct CONFIG config;
extern CURL *curl;
extern char confFile[];
enum COLOR_NAME { COLOR_RESET = 0, COLOR_RED = 31, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE };
extern const char *sizeSuffixes[];

void create_config_file(const char *path);
void parse_config_file(const char *path);
void print_color(enum COLOR_NAME);
int human_size(double *bytes);
