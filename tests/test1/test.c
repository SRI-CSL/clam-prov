#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

/*
  I) Program description, and output:

  The following program reads data from a file into input memory
  locations, copies the data read to output memory locations, and then
  writes the output memory locations to standard out.

  There are three input memory locations: 'input1', 'input2', 'input3'.
  There are two output memory locations: 'output1', 'output2'.

  The sources for output memory location 'output1' are: 'input1', and 'input2'.
  The ONLY source for output memory location 'output2' is: 'input3'.
*/

/*
  II) AddMetadata pass configuration, and output:

  The file addMetadata.config instructs the AddMetadata pass to add
  metadata 'input' to the second argument of all the `read` function
  calls, and to add the metadata 'output' to the second argument for
  all the `write` function calls.

  The file addMetadata.output shows the output of the AddMetadata pass
  on this program with the above mentioned configuration file. The
  output contains 3 entries for 3 read function calls where the second
  argument has the metadata 'input', and it also contains 2 entries
  for write fucntion calls where the second argument has the metadata
  'output'.
*/

int main(int argc, char *argv[]){
  int fd;
  ssize_t read_result;
  int write_result;

  // Input memory locations
  char input1;
  char input2;
  char input3;
  // Output memory locations
  char output1[2];
  char output2[1];

  // A. Populate input memory locations
  fd = open(argv[0], O_RDONLY);
  if(fd < 0){
    perror("Failed file open\n");
    return -1;
  }

  read_result = read(fd, &input1, 1);
  if(read_result < 0){ perror("Failed to read first input\n"); return -1; }

  read_result = read(fd, &input2, 1);
  if(read_result < 0){ perror("Failed to read second input\n"); return -1; }

  read_result = read(fd, &input3, 1);
  if(read_result < 0){ perror("Failed to read third input\n"); return -1; }

  // B. Copy input memory to output memory locations
  output1[0] = input1;
  output1[1] = input2;
  output2[0] = input3;

  // C. Write out output memory locations
  write_result = write(STDOUT_FILENO, &output1[0], 2);
  if(write_result < 0){ perror("Failed to write first output\n"); return -1; }

  write_result = write(STDOUT_FILENO, &output2[0], 1);
  if(write_result < 0){ perror("Failed to write second output\n"); return -1; }

  return 0;
}
