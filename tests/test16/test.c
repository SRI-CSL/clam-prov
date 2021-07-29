// RUN: %clam-prov %s --add-metadata-config=%tests/test16/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test16/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK


/*
Output updated indirectly through an input
*/

void read(char *i){}
void write(char *o){}

char global_i, global_o;

int main(int argc, char *argv[]){
  char local_i, local_o;

  read(&local_i);

  global_i = local_i;
  local_o = global_i;
  global_o = local_o;

  write(&global_o);

  return 0;
}
