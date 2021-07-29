// RUN: %clam-prov %s --add-metadata-config=%tests/test20/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test20/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK

/*
Output should not be dependent on any input
*/
char i[2];
char o;

void read(char *i){}
void write(char *o){}

int main(int argc, char *argv[]){

  read(&i[0]);

  o = i[1];

  write(&o);

  return 0;
}
