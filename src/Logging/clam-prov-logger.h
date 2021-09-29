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
#define CLAM_PROV_DIR_NAME ".clam-prov"
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

#define CLAM_PROV_SIZE_UNSIGNED_LONG sizeof(unsigned long)
#define CLAM_PROV_SIZE_INT sizeof(int)
#define CLAM_PROV_SIZE_LONG sizeof(long)
#define CLAM_PROV_SIZE_FUNCTION_NAME CLAM_PROV_FUNCTION_NAME_LENGTH
#define CLAM_PROV_SIZE_RECORD (CLAM_PROV_SIZE_UNSIGNED_LONG + CLAM_PROV_SIZE_INT + (2 * CLAM_PROV_SIZE_LONG) + CLAM_PROV_SIZE_FUNCTION_NAME)

// API
/*
  Copy the absolute path represented by '~/.clam-prov/audit.log' into `dst`. `dst` must be big enough to fit the path.
  Set `create` to '1' to create the directory `.clam-prov` if it doesn't exist.

  Returns 'dst' on success otherwise returns NULL.
*/
extern char* clam_prov_logger_get_home_file(char *dst, int create);
/*
  Copy the absolute path represented by '~/.clam-prov/audit.pipe' into `dst`. `dst` must be big enough to fit the path.
  Set `create` to '1' to create the directory `.clam-prov` if it doesn't exist.

  Returns 'dst' on success otherwise returns NULL.
*/
extern char* clam_prov_logger_get_home_pipe(char *dst, int create);
/*
  Insert a call-site record into the buffer.
  'control' - Unused
  Second argument - must be a 'long'. This is the call-site identifier
  Third argument - must be a 'long'. This is the return value of the call-site
  Fourth argument - must be a 'char*'. This is the name of the function at the call-site

  Checks if the buffer is full after each insert by calling 'clam_prov_logging_check_and_flush(0)'

  Returns 0 on failure, and 1 on success
*/
extern int clam_prov_logging_buffer(int control, ...);
/*
  Initialize logging.
  'control' - Unused
  'Second argument' - must be an 'int'. This is the maximum size of the buffer
  'Third argument' - must be an 'int'. This is the output mode. Values: '0' for file output, '1' for pipe output

  Returns 0 on failure, and 1 on success
*/
extern int clam_prov_logging_init(int control, ...);
/*
  Shutdown logging.
  'control' - Unused
  No other arguments used.

  Returns 0 on failure, and 1 on success
*/
extern int clam_prov_logging_shutdown(int control, ...);
/*
  Check if the buffer size is larger than the maximum allowed. Flush the buffer if the size is exceeded.
  `force` - Force flush the buffer even if the buffer size is not exceeded

  Returns 0 on failure, and 1 on success
*/
extern int clam_prov_logging_check_and_flush(int force);
