/*
Output updated indirectly through one function
*/
char global_i, global_o;

void read(char *i){}
void write(char *o){}

void middle_function(){
  global_o = global_i;
}

int main(int argc, char *argv[]){

  read(&global_i);

  middle_function();

  write(&global_o);

  return 0;
}
