// RUN: %clam-prov %s --add-metadata-config=%tests/test17/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test17/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK


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
