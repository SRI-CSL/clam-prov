// RUN: %clam-prov %s --add-metadata-config=%tests/test7/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test7/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK

/*
Decide the value of the output based on a global variable
*/

void read(char *i){}
void write(char *o){}

int condition = 1;

int main(int argc, char *argv[]){
  char i1, i2, o;

  read(&i1);
  read(&i2);

  o = (condition == 0) ? i1 : i2;

  write(&o);

  return 0;
}
