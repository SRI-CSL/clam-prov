// RUN: %clam-prov %s --add-metadata-config=%tests/test4/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test4/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK

void read_one(char *m){
 *m = 'i';
}

void write_two(char *m, char *n){
}

int main(int argc, char *argv[]){
 char i1[1], i2[1];
 char o1[1], o2[1];

 read_one(&i1[0]); // call site '0' reads 'i1'
 read_one(&i2[0]); // call site '1' reads 'i2'

 o1[0] = i1[0];
 o2[0] = i2[0];

 write_two(&o1[0], &o2[0]); // call site '2' should be dependent on call site '0', and '1'
 // metadata on call site '2' -> 'clam-prov'={0, 1}
 return 0;
}
