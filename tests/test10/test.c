/*
The output depends on 4 global inputs inside a loop
*/

void read(char *i){}
void write(char *o){}

char i[4];

int main(int argc, char *argv[]){
  int counter;
  char o;

  counter = 0;

  read(&i[0]);
  read(&i[1]);
  read(&i[2]);
  read(&i[3]);

  for(counter = 0; counter < 4; counter++){
    if(counter == 0){
      o = i[0];
    }else if(counter == 1){
      o = i[1];
    }else if(counter == 2){
      o = i[2];
    }else{
      o = i[3];
    }
    write(&o);
  }

  return 0;
}
