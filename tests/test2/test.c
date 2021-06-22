void read(char *m){
  *m = 'i';
}

void write(char *m){
}

int main(int argc, char *argv[]){
  char i[1], o[1];
  read(&i[0]);
  o[0] = i[0];
  write(&o[0]);
  return 0;
}
