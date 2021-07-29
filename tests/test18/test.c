// RUN: %clam-prov %s --add-metadata-config=%tests/test18/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test18/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK

/*
Output updated indirectly through 3 function
*/
char global_i, global_o;

void read(char *i){}
void write(char *o){}

void middle_function3(){
  global_o = global_i;
}

void middle_function2(){
  middle_function3();
}

void middle_function1(){
  middle_function2();
}

int main(int argc, char *argv[]){

  read(&global_i);

  middle_function1();

  write(&global_o);

  return 0;
}
