// RUN: %clam-prov %s --add-metadata-config=%tests/test21/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test21/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK
// XFAIL: *

/*
Output dependent on one input and then update the output with a non-tagged input

In the output, the second `write` call is correctly NOT dependent on the `read` call

*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void read(char *i){}
void write(char *o){}


int nd_int(){
  srand(time(NULL));
  return rand() ;
}


int main(int argc, char *argv[]){
  char i1, i2, o;

  // Avoid undefined behavior
  i2 = (char)((uint64_t)nd_int());
  read(&i1);

  o = i1;
  write(&o);

  // Ouptut being updated with i2 and i2 is not an input
  o = i2;
  write(&o);

  return 0;
}
