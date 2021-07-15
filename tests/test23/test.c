#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/*
  The following program reads only 4 bytes from it's own executables into an input buffer of size 6.

  The first two bytes in the input buffer are written to an output buffer for output 1.

  The second two bytes in the input buffer are written to an output buffer for output 2.

  The last two bytes in the input buffer (which were not read from the file) are written an output buffer for output 3.

  The output buffer for output 1 is written to a file.

  The output buffer for output 2 is written to another file.

  The output buffer for output 3 is written to another file.

  Result:
    Output 1 should be dependent on input
    Output 2 should be dependent on input
    Output 3 should NOT be dependent on input

*/

#define INPUT_BUFFER_SIZE 4

#define OUTPUT_1_FILE_PATH "output.1.file"
#define OUTPUT_2_FILE_PATH "output.2.file"
#define OUTPUT_3_FILE_PATH "output.3.file"

#define OUTPUT_1_BUFFER_SIZE 2
#define OUTPUT_2_BUFFER_SIZE 2
#define OUTPUT_3_BUFFER_SIZE 2

int main(int argc, char *argv[]){
  int result;

  int input_fd;
  int output_1_fd;
  int output_2_fd;
  int output_3_fd;

  // NOTE: Input has INPUT_BUFFER_SIZE + 2 (6) bytes
  char input_buffer[INPUT_BUFFER_SIZE + 2];
  char output_1_buffer[OUTPUT_1_BUFFER_SIZE];
  char output_2_buffer[OUTPUT_2_BUFFER_SIZE];
  char output_3_buffer[OUTPUT_3_BUFFER_SIZE];

  // Read the input
  input_fd = open(argv[0], O_RDONLY);
  if(input_fd < 0){
    perror("Failed to open input file!");
    return -1;
  }
  result = read(input_fd, &input_buffer[0], INPUT_BUFFER_SIZE);
  if(result < 0){
    perror("Failed to read input file!");
    return -1;
  }

  // Copy partial input to output buffer 1
  output_1_buffer[0] = input_buffer[0];
  output_1_buffer[1] = input_buffer[1];

  // Copy partial input to output buffer 2
  output_2_buffer[0] = input_buffer[2];
  output_2_buffer[1] = input_buffer[3];

  // Copy partial non-input to output buffer 3
  output_3_buffer[0] = input_buffer[4];
  output_3_buffer[1] = input_buffer[5];

  // Write output 1 which is dependent on input
  output_1_fd = open(OUTPUT_1_FILE_PATH, O_RDWR|O_CREAT, 0666);
  if(output_1_fd < 0){
    perror("Failed to open output 1 file!");
    return -1;
  }
  result = write(output_1_fd, &output_1_buffer[0], OUTPUT_1_BUFFER_SIZE);
  if(result < 0){
    perror("Failed to write output 1 file!");
    return -1;
  }

  // Write output 2 which is dependent on input
  output_2_fd = open(OUTPUT_2_FILE_PATH, O_RDWR|O_CREAT, 0666);
  if(output_2_fd < 0){
    perror("Failed to open output 2 file!");
    return -1;
  }
  result = write(output_2_fd, &output_2_buffer[0], OUTPUT_2_BUFFER_SIZE);
  if(result < 0){
    perror("Failed to write output 2 file!");
    return -1;
  }

  // Write output 3 which is NOT dependent on input
  output_3_fd = open(OUTPUT_3_FILE_PATH, O_RDWR|O_CREAT, 0666);
  if(output_3_fd < 0){
    perror("Failed to open output 3 file!");
    return -1;
  }
  result = write(output_3_fd, &output_3_buffer[0], OUTPUT_3_BUFFER_SIZE);
  if(result < 0){
    perror("Failed to write output 3 file!");
    return -1;
  }

  close(input_fd);
  close(output_1_fd);
  close(output_2_fd);
  close(output_3_fd);

  return 0;
}
