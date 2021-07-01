/*
The program reads two bytes 'input' from a function 'read',
  writes those two bytes to a new location 'output', and
  then writes the two bytes 'output' using the function 'write'.

The 'read', and the 'write' function both take the args -> a char pointer,
  and the number of bytes pointed by the char pointer.

The 'write' call should be tagged by two tags generated when read 'call'
*/

void read(char *input, int size);
void write(char *output, int size);

void read(char *input, int size){}
void write(char *output, int size){}

int main(int argc, char *argv[]){
  char input[2], output[2];

  read(&input[0], 2);
  output[0] = input[0];
  output[1] = input[1];
  write(&output[0], 2);

  return 0;
}
