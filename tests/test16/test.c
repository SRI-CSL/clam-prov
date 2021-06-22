/*
Output updated indirectly through an input
*/

void read(char *i){}
void write(char *o){}

char global_i, global_o;

int main(int argc, char *argv[]){
  char local_i, local_o;

  read(&local_i);

  global_i = local_i;
  local_o = global_i;
  global_o = local_o;

  write(&global_o);

  return 0;
}
