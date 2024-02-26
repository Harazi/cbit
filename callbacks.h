#include <curl/curl.h>
#include <stddef.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

size_t dev_null(char *contents, size_t size, size_t nmemb, void *userdata);
size_t write_callback(char *contents, size_t size, size_t nitems, void *userdata);
struct MemoryStruct GET(const char *path);
struct MemoryStruct POST(const char *path, const char *postField);
struct MemoryStruct POST_MIME(const char *path, curl_mime *mime);
