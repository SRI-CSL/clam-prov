// RUN: %clam-prov %s --add-metadata-config=%tests/test19/AddMetadata.config --dependency-map-file=%T/DependencyMap.output --enable-recursive=true
// RUN: %cmp %T/DependencyMap.output %tests/test19/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK

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
