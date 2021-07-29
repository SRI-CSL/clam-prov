// RUN: %clam-prov %s --add-metadata-config=%tests/test15/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test15/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK


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
