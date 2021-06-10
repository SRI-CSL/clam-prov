void read(char *m){
  *m = 'i';
}

void write(char *m){

}

int main(int argc, char *argv[]){
  char i1[1], o1[1];
  read(&i1[0]);
  o1[0] = i1[0];
  write(&o1[0]);
  return 0;
}
