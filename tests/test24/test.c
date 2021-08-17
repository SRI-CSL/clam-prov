// RUN: %clam-prov %s --add-metadata-config=%tests/test24/AddMetadata.config --dependency-map-file=%T/DependencyMap.output
// RUN: %cmp %T/DependencyMap.output %tests/test24/DependencyMap.output.expected && echo "OK" > %T/result.txt || echo "FAIL" > %T/result.txt
// RUN: cat %T/result.txt | FileCheck %s
// CHECK: OK
// XFAIL: *

/*

Tag return value test.

The return value of mmap is the source. The source is written using write's operand.

*/

#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char* argv[]){
  void* addr;
  size_t length;
  int prot;
  int flags;
  int fd;
  off_t offset;

  length = 1;
  prot = PROT_READ;
  flags = MAP_PRIVATE | MAP_ANONYMOUS;
  fd = -1;
  offset = 0;

  // read source
  addr = mmap(NULL, length, prot, flags, fd, offset);

  // write sink 
  write(1, addr, length);

  munmap(addr, length);

}
