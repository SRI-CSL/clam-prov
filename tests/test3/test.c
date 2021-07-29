// RUN: %clam-prov %s --add-metadata-config=%tests/test3/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test3/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK

void read(char *m){
  *m = 'i';
}

void write(char *m){

}

int main(int argc, char *argv[]){
  char i1[1], o1[1];
  read(&i1[0]);
  o1[0] = i1[0];
  write(&o1[0]);
  return 0;
}
