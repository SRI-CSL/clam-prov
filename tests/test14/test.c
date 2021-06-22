/*
One output has no input and one output has one input
*/

void read(char *i){}
void write(char *o){}

int main(int argc, char *argv[]){
  char i[2];
  char o[2];

  read(&i[0]);

  o[0] = i[0];

  write(&o[0]);
  write(&o[1]);

  return 0;
}
