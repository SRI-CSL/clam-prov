/*
A program with one input and one output.

The type of input and output is integer.
When the type is 'char' it works fine.

ERROR below on executing 'clam-pp':
=== Resolution of indirect calls ===
BRUNCH_STAT INDIRECT CALLS 0
BRUNCH_STAT RESOLVED CALLS 0
BRUNCH_STAT UNRESOLVED CALLS 0
BRUNCH_STAT IGNORED ASM CALLS 0
CLAM ERROR: All blocks and definitions must have a name at /home/docker/clean/clam-prov/clam/lib/Clam/CfgBuilder.cc:3530

*/
void read(int *input){
  *input = 100;
}
void write(int *output){}

int main(int argc, char *argv[]){
  int input, output;

  read(&input);
  output = input;
  write(&output);

  return 0;
}
