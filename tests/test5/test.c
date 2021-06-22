/*
Simple test with one input and one output
*/

void read(char *input){
  *input = 'i';
}
void write(char *output){}

int main(int argc, char *argv[]){
  char input, output;

  read(&input);
  output = input;
  write(&output);

  return 0;
}
