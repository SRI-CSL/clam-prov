// RUN: %clam-prov %s --add-metadata-config=%tests/test21/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test21/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK
// XFAIL: *

/*
Output dependent on one input and then update the output with a non-tagged input

In the output, the second `write` call is incorrectly dependent on the `read` call

*/

void read(char *i){}
void write(char *o){}

int main(int argc, char *argv[]){
  char i1, i2, o;

  read(&i1);

  o = i1;
  write(&o);

  // Ouptut being updated with i2 and i2 is not an input
  o = i2;
  write(&o);
  // This call-site is reported as being dependent on the 'read' call-site

  return 0;
}
