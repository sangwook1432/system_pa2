#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(int argc,char *argv[]) {
  if(argc == 1){
    fprintf(stderr,"pa2_rm: missing operand\n");
    return EXIT_FAILURE;
  }
  int status = EXIT_SUCCESS;
  for(int i = 1;i < argc;i++){
    const char *pathname = argv[i];
    if(unlink(pathname) != 0){
      status = EXIT_FAILURE;
      fprintf(stderr,"pa2_rm: cannot remove '%s': ",pathname);
      if(errno == ENOENT){
        fprintf(stderr,"No such file or directory\n");
      }
      else if(errno == EISDIR){
        fprintf(stderr,"Is a directory\n");
      }
      else if(errno == EACCES || errno == EPERM){
        fprintf(stderr,"Permission denied\n");
      }
      else{
        perror("");
      }
    }
  }
  return status;
}