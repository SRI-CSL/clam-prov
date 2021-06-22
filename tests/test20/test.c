/*
Output should not be dependent on any input
*/
char i[2];
char o;

void read(char *i){}
void write(char *o){}

int main(int argc, char *argv[]){

  read(&i[0]);

  o = i[1];

  write(&o);

  return 0;
}
