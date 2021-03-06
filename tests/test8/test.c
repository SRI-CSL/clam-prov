// RUN: %clam-prov %s --add-metadata-config=%tests/test8/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test8/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK

/*
Decide the value of the output based on two global variables
*/

void read(char *i){}
void write(char *o){}

int condition1 = 1;
int condition2 = 1;

int main(int argc, char *argv[]){
  char i1, i2, i3, i4, o;

  read(&i1);
  read(&i2);
  read(&i3);
  read(&i4);

  if(condition1 == 0 && condition2 == 0){
    o = i1;
  }else if(condition1 == 0 && condition2 == 1){
    o = i2;
  }else if(condition1 == 1 && condition2 == 0){
    o = i3;
  }else{
    o = i4;
  }

  write(&o);

  return 0;
}
