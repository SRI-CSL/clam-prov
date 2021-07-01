#include <stdio.h>
#include <unistd.h>

// extern "C" {
// ssize_t write(int fd, const void *buf, size_t count);
ssize_t clam_prov_write(int fd, const void *buf, size_t count) {
  char *p = (char*) buf;
  while (count--) {
    char c = *(p++);
    if (write(fd, &c, 1) == -1) {
      return -1;
    }
  }
  return count;
}

// ssize_t read(int fd, const void *buf, size_t count);
ssize_t clam_prov_read(int fd, const void *buf, size_t count) {
  char *p = (char*) buf;
  while (count--) {
    if (read(fd, p++, 1) == -1) {
      return -1;
    }
  }
  return count;
}
//}

