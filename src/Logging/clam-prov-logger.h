#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <pwd.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>

// Constants
#define CLAM_PROV_OUTPUT_FILE 0
#define CLAM_PROV_OUTPUT_PIPE 1
#define CLAM_PROV_DIR_NAME ".clam_prov"
#define CLAM_PROV_PATH_NAME_FILE "audit.log"
#define CLAM_PROV_PATH_NAME_PIPE "audit.pipe"
#define CLAM_PROV_PATH_PERMISSIONS 0660
#define CLAM_PROV_DIR_PERMISSIONS 0700

// Data structures
#define CLAM_PROV_PATH_LENGTH 4096
#define CLAM_PROV_FUNCTION_NAME_LENGTH 256
typedef struct clam_prov_record{
  unsigned long time; // The time in millis when the call-site was executed
  int pid;            // The process which executed the call-site
  long call_site_id;  // The id of the call-site
  long exit;          // The return value of the call-site
  char function_name[CLAM_PROV_FUNCTION_NAME_LENGTH]; // The name of the function at the call-site
} clam_prov_record;

// API
extern char* clam_prov_logger_get_home_file(char *dst, int create);
extern char* clam_prov_logger_get_home_pipe(char *dst, int create);
extern int clam_prov_logging_buffer(int control, ...);
extern int clam_prov_logging_init(int control, ...);
extern int clam_prov_logging_shutdown(int control, ...);
extern int clam_prov_logging_check_and_flush(int force);
