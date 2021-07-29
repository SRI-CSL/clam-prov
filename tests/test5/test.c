// RUN: %clam-prov %s --add-metadata-config=%tests/test5/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test5/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK

/*
Simple test with one input and one output
*/

void read(char *input){
  *input = 'i';
}
void write(char *output){}

int main(int argc, char *argv[]){
  char input, output;

  read(&input);
  output = input;
  write(&output);

  return 0;
}
