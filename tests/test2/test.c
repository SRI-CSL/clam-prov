// RUN: %clam-prov %s --add-metadata-config=%tests/test2/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test2/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK

void read(char *m){
  *m = 'i';
}

void write(char *m){
}

int main(int argc, char *argv[]){
  char i[1], o[1];
  read(&i[0]);
  o[0] = i[0];
  write(&o[0]);
  return 0;
}
