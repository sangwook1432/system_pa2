#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#define BUFFER_SIZE 4096
#define path_max 4096

void copyfile(const char *src,const char *dest){
  FILE *src_fptr; FILE *dest_fptr;
  char buffer[BUFFER_SIZE];
  size_t bytes_read;
  bool error = false;
  src_fptr = fopen(src,"r");
  if(src_fptr == NULL){
    fprintf(stderr,"pa2_cp: error opening source file '%s': ",src);
    perror("");
    return;
  }

  dest_fptr = fopen(dest,"w");
  if(dest_fptr == NULL){
    fprintf(stderr,"pa2_cp: cannot create regular file '%s': ",dest);
    perror("");
    fclose(src_fptr);
    return;
  }


  while((bytes_read = fread(buffer,1,BUFFER_SIZE,src_fptr)) > 0){
    if(fwrite(buffer,1,bytes_read,dest_fptr) != bytes_read){
      fprintf(stderr,"pa2_cp: error writing to destination file '%s': ",dest);
      perror("");
      error = true;
      break;
    }
  }
  if(!error && ferror(src_fptr)){
    fprintf(stderr,"pa2_cp: error reading source file '%s': ",src);
    perror("");
    error = true;
  }
  if(ferror(dest_fptr)){
    fprintf(stderr,"pa2_cp: error writing to destination file '%s': ",dest);
    perror("");
    error = true;
  }

  fclose(src_fptr);
  fclose(dest_fptr);
}

const char *pa2_basename(const char *path){
  const char *base = strrchr(path,'/');
  return base ? base +1 : path;
}

int main(int argc,char *argv[]) {
  if(argc == 1){
    fprintf(stderr,"pa2_cp: missing file operand\n");
    return EXIT_FAILURE;
  }
  if(argc == 2){
    fprintf(stderr,"pa2_cp: missing destination file operand after '%s'\n",argv[1]);
    return EXIT_FAILURE;
  }
  if(argc == 3){
    const char *sourcefile = argv[1];
    const char *dest = argv[2];
    char target[path_max];

    struct stat source_stat,dest_stat;
    if(stat(sourcefile,&source_stat) != 0){
      fprintf(stderr, "pa2_cp: cannot stat '%s': ",sourcefile);
      perror("");
      return EXIT_FAILURE;
    }

    bool dest_exist = (stat(dest,&dest_stat) == 0);
    bool dest_is_directory = false;
    if(dest_exist){
      dest_is_directory = S_ISDIR(dest_stat.st_mode);
    }
    if(dest_is_directory){
      const char *source_base = pa2_basename(sourcefile);
      int len = snprintf(target,path_max,"%s/%s",dest,source_base);
      if(len >= path_max){
        fprintf(stderr,"pa2_cp: target path is too long\n");
        return EXIT_FAILURE;
      }
    }
    else{
      strncpy(target,dest,path_max-1);
      target[path_max - 1] = '\0';
    }
    struct stat target_stat;
    bool target_existence = (stat(target,&target_stat) == 0);
    if(target_existence && source_stat.st_dev == target_stat.st_dev && source_stat.st_ino == target_stat.st_ino){
      fprintf(stderr,"pa2_cp: '%s' and '%s' are the same file\n",sourcefile,target);
      return EXIT_FAILURE;
    }
    copyfile(sourcefile,target);
  }
  else{
    const char *directory = argv[argc-1];
    struct stat dir_stat;
    if(stat(directory,&dir_stat) != 0){
      if(errno == ENOENT){
        char dir_with_slash[path_max];
        snprintf(dir_with_slash,path_max,"%s/",directory);
        fprintf(stderr,"pa2_cp: cannot create regular file '%s': Not a directory\n",dir_with_slash);
      }
      else{
        fprintf(stderr,"pa2_cp: cannot stat '%s': ",directory);
        perror("");
      }
      return EXIT_FAILURE;
    }
    else if(!S_ISDIR(dir_stat.st_mode)){
      fprintf(stderr,"pa2_cp: target '%s': Not a directory\n",directory);
      return EXIT_FAILURE;
    }
    for(int i = 1;i < argc-1;i++){
      const char *source_file = argv[i];
      char target_path[path_max];
      struct stat source_stat,target_stat;
      if(stat(source_file,&source_stat) != 0){
        fprintf(stderr,"pa2_cp: cannot stat '%s': ",source_file);
        perror("");
        continue;
      }
      const char *source_basename = pa2_basename(source_file);
      int len = snprintf(target_path,path_max,"%s/%s",directory,source_basename);
      if(len >= path_max){
        fprintf(stderr,"pa2_cp: target path for '%s' is too long\n",source_file);
        continue;
      }
      bool target_existence = (stat(target_path,&target_stat) == 0);
      if(target_existence && source_stat.st_dev == target_stat.st_dev && source_stat.st_ino == target_stat.st_ino){
        fprintf(stderr,"pa2_cp: '%s' and '%s' are the same file\n",source_file,target_path);
        continue;
      }
      copyfile(source_file,target_path);
    }
  }
  return EXIT_SUCCESS;
}