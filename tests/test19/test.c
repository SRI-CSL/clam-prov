/*
Output updated indirectly through a recursive function

The `write` call is not dependent on the `read` call
*/
char global_i, global_o;
int recursive_count = 0;

void read(char *i){}
void write(char *o){}

void recursive_function(){
  if(recursive_count >= 3){
    global_o = global_i;
    return;
  }
  recursive_count++;
  recursive_function();
}

int main(int argc, char *argv[]){

  read(&global_i);

  recursive_function();

  write(&global_o);

  return 0;
}
