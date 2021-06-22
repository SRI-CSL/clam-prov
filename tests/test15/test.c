/*
Two inputs and output assigned by pointer manipulation
*/

void read(char *i){}
void write(char *o){}

int main(int argc, char *argv[]){
  char i[2];
  char o;
  char *i_ptr;

  read(&i[0]);
  read(&i[1]);

  i_ptr = &i[0];
  o = *(i_ptr + 1);

  write(&o);

  return 0;
}
