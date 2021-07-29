// RUN: %clam-prov %s --add-metadata-config=%tests/test11/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test11/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK

/*
The output depends on 4 global inputs (out of 8) inside a loop
*/

void read(char *i){}
void write(char *o){}

char i[8];

int main(int argc, char *argv[]){
  int counter;
  char o;

  counter = 0;

  read(&i[0]);
  read(&i[1]);
  read(&i[2]);
  read(&i[3]);
  read(&i[4]);
  read(&i[5]);
  read(&i[6]);
  read(&i[7]);

  for(counter = 0; counter < 4; counter++){
    if(counter == 0){
      o = i[0];
    }else if(counter == 1){
      o = i[2];
    }else if(counter == 2){
      o = i[4];
    }else{
      o = i[6];
    }
    write(&o);
  }

  return 0;
}
