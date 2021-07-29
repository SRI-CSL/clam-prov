// RUN: %clam-prov %s --add-metadata-config=%tests/test14/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test14/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK

/*
One output has no input and one output has one input
*/

void read(char *i){}
void write(char *o){}

int main(int argc, char *argv[]){
  char i[2];
  char o[2];

  read(&i[0]);

  o[0] = i[0];

  write(&o[0]);
  write(&o[1]);

  return 0;
}
