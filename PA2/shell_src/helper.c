#include "helper.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* get_bin_dir() {
  // /proc/self/exe returns the path of the executable
  char* path = malloc(1024);
  ssize_t len = readlink("/proc/self/exe", path, 1024);

  if (len == -1) {
    perror("readlink");
    exit(EXIT_FAILURE);
  }
  path[len] = '\0';
  dirname(path);
  strcat(path, "/");
  return path;
}
