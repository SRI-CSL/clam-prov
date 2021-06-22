/*
Decide the value of the output based on a global variable
*/

void read(char *i){}
void write(char *o){}

int condition = 1;

int main(int argc, char *argv[]){
  char i1, i2, o;

  read(&i1);
  read(&i2);

  o = (condition == 0) ? i1 : i2;

  write(&o);

  return 0;
}
