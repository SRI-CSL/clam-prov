#include "clam-prov-logger.h"

// Per thread state
static __thread int clam_prov_thread_tid = -1;

// Global variable set by the user (from API)
static int clam_prov_max_records;
static int clam_prov_logger_output_mode = -1;
static char clam_prov_logger_output_path[CLAM_PROV_PATH_LENGTH];

/*
static int clam_prov_logger_profile_io = 1;
static double clam_prov_logger_profile_total_time_taken;
static double clam_prov_logger_profile_total_writes;
static double clam_prov_logger_profile_total_bytes;
*/

static int clam_prov_logger_output_fd;
static int clam_prov_logging_is_inited = 0;

// Records array
static clam_prov_record *clam_prov_records = NULL;
static int clam_prov_record_index = 0;

// Synchronization
static pthread_mutex_t clam_prov_lock;
static int clam_prov_lock_is_inited = 0;

// Protos
static int clam_prov_alloc_records();
static void clam_prov_free_records();
static void clam_prov_close_output();
static void clam_prov_do_cleanup();
static int clam_prov_open_output_file();
static int clam_prov_open_output_pipe();
static int clam_prov_open_output();
static char* copy_str_n(char *dst, char *src, int n, int best_effort);
static char* copy_function_name(char *dst, char *src);
static unsigned long get_current_milliseconds();
static char* create_home_path(char *dst, char *path_name, int create);

static char* create_home_path(char *dst, char *path_name, int create){
  uid_t uid;
  struct passwd *pw;
  char *home_dir;
  int dst_index;
  DIR *dir_check;

  if(dst == NULL){
    return NULL;
  }

  dst_index = 0;
  explicit_bzero((void *)dst, CLAM_PROV_PATH_LENGTH);

  uid = getuid();
  pw = getpwuid(uid);
  if(pw == NULL){
    return NULL;
  }
  home_dir = pw->pw_dir;

  dst_index += snprintf(dst + dst_index, (CLAM_PROV_PATH_LENGTH - dst_index), "%s/", home_dir);
  if(dst_index >= CLAM_PROV_PATH_LENGTH){
    return NULL;
  }

  dst_index += snprintf(dst + dst_index, (CLAM_PROV_PATH_LENGTH - dst_index), "%s/", CLAM_PROV_DIR_NAME);
  if(dst_index >= CLAM_PROV_PATH_LENGTH){
    return NULL;
  }

  dir_check = opendir(dst);
  if(dir_check != NULL){
    closedir(dir_check);
  }else if(errno == ENOENT){
    if(create == 1){
      if(mkdir(dst, CLAM_PROV_DIR_PERMISSIONS) != 0){
        return NULL;
      }
    }
  }else{
    return NULL;
  }

  dst_index += snprintf(dst + dst_index, (CLAM_PROV_PATH_LENGTH - dst_index), "%s", path_name);
  if(dst_index >= CLAM_PROV_PATH_LENGTH){
    return NULL;
  }

  return dst;
}

char* clam_prov_logger_get_home_file(char *dst, int create){
  return create_home_path(dst, CLAM_PROV_PATH_NAME_FILE, create);
}

char* clam_prov_logger_get_home_pipe(char *dst, int create){
  return create_home_path(dst, CLAM_PROV_PATH_NAME_PIPE, create);
}

static char* copy_str_n(char *dst, char *src, int n, int best_effort){
  if(src == NULL){
    return NULL;
  }
  int src_size = strlen(src);
  if(src_size == 0){
    return NULL;
  }else if(src_size >= n){
    if(best_effort == 1){
      src_size = n;
    }else{
      return NULL;
    }
  }
  explicit_bzero((void *)(dst), n);
  strncpy(dst, src, src_size);
  return dst;
}

static char* copy_function_name(char *dst, char *src){
  return copy_str_n(dst, src, CLAM_PROV_FUNCTION_NAME_LENGTH, 1);
}

static unsigned long get_current_milliseconds(){
  unsigned long millis;
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  millis = (spec.tv_sec * 1000) + (spec.tv_nsec / (1000 * 1000));
  return millis;
}

static int clam_prov_alloc_records(){
  if(clam_prov_max_records < 1){
    return 0; // Invalid max records
  }
  clam_prov_records = (clam_prov_record *)malloc(sizeof(clam_prov_record) * clam_prov_max_records);
  if(clam_prov_records == NULL){
    return 0; // Failed to allocate memory
  }
  return 1;
}

static void clam_prov_free_records(){
  if(clam_prov_records != NULL){
    free(clam_prov_records);
  }
}

static void clam_prov_close_output(){
  if(clam_prov_logger_output_fd > -1){
    close(clam_prov_logger_output_fd);
  }
}

static void clam_prov_do_cleanup(){
  clam_prov_free_records();
  clam_prov_close_output();
}

static int clam_prov_open_output_file(){
  int create;
  char *full_path;

  create = 1;
  full_path = &clam_prov_logger_output_path[0];
  full_path = create_home_path(full_path, CLAM_PROV_PATH_NAME_FILE, create);
  if(full_path == NULL){
    return 0;
  }
  clam_prov_logger_output_fd = open(full_path, O_WRONLY|O_APPEND|O_CREAT, CLAM_PROV_PATH_PERMISSIONS);
  if(clam_prov_logger_output_fd < 0){
    return 0;
  }
  return 1;
}

static int clam_prov_open_output_pipe(){
  int create;
  char *full_path;

  create = 1;
  full_path = &clam_prov_logger_output_path[0];
  full_path = create_home_path(full_path, CLAM_PROV_PATH_NAME_PIPE, create);
  if(full_path == NULL){
    return 0;
  }
  if(mkfifo(full_path, CLAM_PROV_PATH_PERMISSIONS) != 0){
    if(errno != EEXIST){
      return 0;
    }
  }
  clam_prov_logger_output_fd = open(full_path, O_WRONLY);
  if(clam_prov_logger_output_fd < 0){
    return 0;
  }
  return 1;
}

static int clam_prov_open_output(){
  switch(clam_prov_logger_output_mode){
    case CLAM_PROV_OUTPUT_FILE: return clam_prov_open_output_file();
    case CLAM_PROV_OUTPUT_PIPE: return clam_prov_open_output_pipe();
    default: return 0;
  }
}

// User API

static char* copy_record_to_dst_buffer(char *dst, clam_prov_record *src){
  int offset = 0;
  memcpy((void*)(&dst[offset]), (void*)(&src->time), CLAM_PROV_SIZE_UNSIGNED_LONG);
  offset += CLAM_PROV_SIZE_UNSIGNED_LONG;
  memcpy((void*)(&dst[offset]), (void*)(&src->pid), CLAM_PROV_SIZE_INT);
  offset += CLAM_PROV_SIZE_INT;
  memcpy((void*)(&dst[offset]), (void*)(&src->call_site_id), CLAM_PROV_SIZE_LONG);
  offset += CLAM_PROV_SIZE_LONG;
  memcpy((void*)(&dst[offset]), (void*)(&src->exit), CLAM_PROV_SIZE_LONG);
  offset += CLAM_PROV_SIZE_LONG;
  memcpy((void*)(&dst[offset]), (void*)(&src->function_name[0]), CLAM_PROV_SIZE_FUNCTION_NAME);
  offset += CLAM_PROV_SIZE_FUNCTION_NAME;
  return &dst[offset];
}

static char* copy_records_to_dst_buffer(char *dst, int total_records){
  int current_record_index;

  current_record_index = 0;

  for(; current_record_index < total_records; current_record_index++){
    dst = copy_record_to_dst_buffer(dst, &clam_prov_records[current_record_index]);
  }
  return dst;
}

static int get_dst_buffer_size(int total_records){
  return CLAM_PROV_SIZE_RECORD * total_records;
}

static char* alloc_dst_buffer(int dst_buffer_size){
  return (char*)malloc(sizeof(char) * dst_buffer_size);
}

static void free_dst_buffer(char *dst){
  free(dst);
}

static int clam_prov_logging_check_and_flush_concrete(int force){
  int flush;
  flush = 0;

  if(clam_prov_logging_is_inited == 0){
    return 0; // Failed to init or no init
  }

  if(clam_prov_record_index == 0){
    return 1; // nothing to flush
  }

  if(force == 1){
    flush = 1;
  }else if(clam_prov_record_index >= clam_prov_max_records){
    flush = 1;
  }// TIME check, too TODO

  if(flush == 1){
    int total_records;
    int dst_buffer_size;
    char *dst;
    int written_bytes;

    total_records = clam_prov_record_index;
    dst_buffer_size = get_dst_buffer_size(total_records);
    dst = alloc_dst_buffer(dst_buffer_size);
    if(dst != NULL){
      copy_records_to_dst_buffer(dst, total_records);
      flock(clam_prov_logger_output_fd, LOCK_EX);
      written_bytes = write(clam_prov_logger_output_fd, (void*)(dst), dst_buffer_size);
      if(written_bytes > 0){
        fsync(clam_prov_logger_output_fd);
      }
      flock(clam_prov_logger_output_fd, LOCK_UN);
      free_dst_buffer(dst);
    }

    clam_prov_record_index = 0;
    if(dst_buffer_size != written_bytes){
      // something went wrong TODO
      return 0;
    }
  }
  return 1;
}

int clam_prov_logging_check_and_flush(int force){
  int result;

  pthread_mutex_lock(&clam_prov_lock);
  result = clam_prov_logging_check_and_flush_concrete(force);
  pthread_mutex_unlock(&clam_prov_lock);

  return result;
}

static int clam_prov_logging_buffer_concrete(long call_site_id, long exit_value, char *function_name){
  if(clam_prov_logging_is_inited == 0){
    return 0; // Failed to init or no init
  }

  if(clam_prov_thread_tid == -1){
    clam_prov_thread_tid = (int)gettid();
  }

  struct clam_prov_record *clam_prov_record_instance;
  clam_prov_record_instance = &clam_prov_records[clam_prov_record_index];
  clam_prov_record_instance->time = get_current_milliseconds();
  clam_prov_record_instance->pid = clam_prov_thread_tid;
  clam_prov_record_instance->call_site_id = call_site_id;
  clam_prov_record_instance->exit = exit_value;

  if(copy_function_name(&clam_prov_record_instance->function_name[0], function_name) == NULL){
    return 0; // Failed to allocate memory for function name
  }

  clam_prov_record_index++;
  return 1;
}

int clam_prov_logging_buffer(int control, ...){
  int result;
  long call_site_id, exit_value;
  char *function_name;

  va_list args;
  va_start(args, control);

  call_site_id = va_arg(args, long);
  exit_value = va_arg(args, long);
  function_name = va_arg(args, char*);

  va_end(args);

  pthread_mutex_lock(&clam_prov_lock);
  result = clam_prov_logging_buffer_concrete(call_site_id, exit_value, function_name);
  pthread_mutex_unlock(&clam_prov_lock);

  clam_prov_logging_check_and_flush(0);

  return result;

}

static int clam_prov_logging_init_concrete(int max_records, int output_mode){
  int success;
  success = 0;

  if(clam_prov_logging_is_inited == 1){
    return 0; // Already initialized
  }

  clam_prov_max_records = max_records;
  if(clam_prov_alloc_records() == 1){
    clam_prov_logger_output_mode = output_mode;
    if(clam_prov_open_output() == 1){
      success = 1;
    }
  }

  if(success == 0){
    clam_prov_do_cleanup();
    clam_prov_logging_is_inited = 0;
    return 0;
  }else{
    clam_prov_logging_is_inited = 1;
    return 1;
  }
}

int clam_prov_logging_init(int control, ...){
  int result, max_records, output_mode;

  va_list args;
  va_start(args, control);

  max_records = va_arg(args, int);
  output_mode = va_arg(args, int);

  va_end(args);

  if (pthread_mutex_init(&clam_prov_lock, NULL) != 0) {
    return 0; // Failed to init mutex
  }

  clam_prov_lock_is_inited = 1;

  pthread_mutex_lock(&clam_prov_lock);
  result = clam_prov_logging_init_concrete(max_records, output_mode);
  pthread_mutex_unlock(&clam_prov_lock);

  return result;
}

static int clam_prov_logging_shutdown_concrete(){
  if(clam_prov_logging_is_inited == 0){
    return 0; // Not initialized
  }

  clam_prov_logging_check_and_flush(1);

  clam_prov_do_cleanup();

  clam_prov_logging_is_inited = 0;
  return 1;
}

int clam_prov_logging_shutdown(int control, ...){
  int result;

  pthread_mutex_lock(&clam_prov_lock);
  result = clam_prov_logging_shutdown_concrete();
  pthread_mutex_unlock(&clam_prov_lock);

  if (clam_prov_lock_is_inited == 1) {
    clam_prov_lock_is_inited = 0;
    pthread_mutex_destroy(&clam_prov_lock);
  }

  return result;
}
