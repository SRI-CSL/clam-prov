/*
The output depends on 4 inputs inside a loop
*/

void read(char *i){}
void write(char *o){}

int main(int argc, char *argv[]){
  int counter;
  char i1, i2, i3, i4, o;

  counter = 0;

  read(&i1);
  read(&i2);
  read(&i3);
  read(&i4);

  for(counter = 0; counter < 4; counter++){
    if(counter == 0){
      o = i1;
    }else if(counter == 1){
      o = i2;
    }else if(counter == 2){
      o = i3;
    }else{
      o = i4;
    }
    write(&o);
  }

  return 0;
}
