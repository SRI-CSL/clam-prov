#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]){

  if(argc != 3){
    fprintf(stderr, "Missing arguments: <log file path> <number of records to read>\n");
    return 1;
  }

  char *log_path = argv[1];
  int record_count = atoi(argv[2]);
  int fd, i;

  fd = open(log_path, O_RDONLY);

  if(fd < 0){
    perror("Log file open failed!");
    return 1;
  }

  const int sizeof_unsigned_long = sizeof(unsigned long);
  const int sizeof_int = sizeof(int);
  const int sizeof_long = sizeof(long);
  const int sizeof_function_name = 256;
  const int data_size = sizeof_unsigned_long + sizeof_int + (2 * sizeof_long) + sizeof_function_name;

  for(i = 0; i < record_count; i++){
    int bytes_read;
    char data[data_size];

    bytes_read = read(fd, (void*)(&data[0]), data_size);

    if(bytes_read == 0){
      break;
    }

    if(bytes_read < 0){
      perror("Failed to read log file");
      break;
    }

    if(bytes_read != data_size){
      printf("Invalid number of bytes in log file. Must be a multiple of %d\n", data_size);
      break;
    }

    unsigned long time;
    int pid;
    long call_site_tag;
    long exit;
    char function_name[sizeof_function_name];

    int offset = 0;
    memcpy((void*)(&time), (void*)(&data[offset]), sizeof_unsigned_long);
    offset += sizeof_unsigned_long;
    memcpy((void*)(&pid), (void*)(&data[offset]), sizeof_int);
    offset += sizeof_int;
    memcpy((void*)(&call_site_tag), (void*)(&data[offset]), sizeof_long);
    offset += sizeof_long;
    memcpy((void*)(&exit), (void*)(&data[offset]), sizeof_long);
    offset += sizeof_long;
    memcpy((void*)(&function_name[0]), (void*)(&data[offset]), sizeof_function_name);
    offset += sizeof_function_name;

    printf("Record[time=%lu, pid=%d, call_site_tag=%ld, exit=%ld, function_name=%s]\n",
      time, pid, call_site_tag, exit, &function_name[0]
    );
  }

  close(fd);

  return 0;
}
