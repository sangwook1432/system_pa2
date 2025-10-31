#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

const char *pa2_basename(const char *path){
  const char *base = strrchr(path,'/');
  return base ? base + 1: path;
}

int handle_rename_error(const char *source_path, const char *target_path) {
    const char *source_basename = pa2_basename(source_path);
    fprintf(stderr, "pa2_mv: ");
    if (errno == ENOENT) {
        fprintf(stderr, "cannot move '%s' to '%s': No such file or directory\n", source_basename, target_path);
    } 
    else if (errno == ENOTDIR) {
         fprintf(stderr, "cannot move '%s' to '%s': Not a directory\n", source_basename, target_path);
    } 
    else if (errno == EACCES) {
        fprintf(stderr, "cannot move '%s' to '%s': Permission denied\n", source_basename, target_path);
    } 
    else if (errno == EINVAL && source_path != NULL && target_path != NULL && strncmp(target_path, source_path, strlen(source_path)) == 0 && target_path[strlen(source_path)] == '/') {
        fprintf(stderr, "cannot move '%s' to a subdirectory of itself, '%s'\n", source_basename, target_path);
    } 
    else if (errno == EISDIR) {
        fprintf(stderr, "cannot move '%s' to '%s': Is a directory\n", source_basename, target_path);
    }  
    else if (errno == ENOTEMPTY || errno == EEXIST) {
        fprintf(stderr, "cannot move '%s' to '%s': Directory not empty\n", source_basename, target_path);
    }
    return EXIT_FAILURE;
}


int main(int argc,char *argv[]) {
  if(argc == 1){
    fprintf(stderr,"pa2_mv: missing file operand\n");
    return EXIT_FAILURE;
  }
  if(argc == 2){
    fprintf(stderr,"pa2_mv: missing destination file operand after '%s'\n",argv[1]);
    return EXIT_FAILURE;
  }

  int status = EXIT_SUCCESS;
  if(argc == 3){
    const char *source_path = argv[1];
    const char *dest_arg = argv[2];
    char target_path[PATH_MAX];
    struct stat source_stat,dest_stat;

    if(stat(source_path,&source_stat) != 0){
      fprintf(stderr,"pa2_mv: cannot stat '%s': ",source_path);
      perror("");
      return EXIT_FAILURE;
    }
    bool dest_exist = (stat(dest_arg,&dest_stat) == 0);
    bool dest_is_dir = false;
    if(dest_exist){
      dest_is_dir = S_ISDIR(dest_stat.st_mode);
    }
    if(dest_is_dir){
      const char *source_basename = pa2_basename(source_path);
      size_t dest_len = strlen(dest_arg);
      if(dest_len > 0 && dest_arg[dest_len-1] == '/'){
	      snprintf(target_path,PATH_MAX,"%s%s",dest_arg,source_basename);
      }
      else{
	      snprintf(target_path,PATH_MAX,"%s%s",dest_arg,source_basename);
      }
    }
    else{
      strncpy(target_path,dest_arg,PATH_MAX-1);
      target_path[PATH_MAX - 1] = '\0';
    }
    struct stat target_stat;
    bool target_exist_check = (stat(target_path,&target_stat) == 0);
    if(target_exist_check && source_stat.st_dev == target_stat.st_dev && source_stat.st_ino == target_stat.st_ino){
      fprintf(stderr,"pa2_mv: '%s' and '%s' are the same file\n",pa2_basename(source_path),pa2_basename(target_path));
      return EXIT_FAILURE;
    }
    if(rename(source_path,target_path) != 0){
      status = handle_rename_error(source_path,target_path);
    }
  }
  else{
    const char *directory_path = argv[argc-1];
    struct stat dir_stat;
    if(stat(directory_path,&dir_stat) != 0){
      fprintf(stderr,"pa2_mv: cannot stat '%s': ",directory_path);
      perror("");
      return EXIT_FAILURE;
    }
    else if(!S_ISDIR(dir_stat.st_mode)){
      fprintf(stderr,"pa2_mv: cannot stat '%s': Not a directory\n",directory_path);
      return EXIT_FAILURE;
    }
    for(int i = 1;i < argc-1;i++){
      const char *source_path = argv[i];
      char target_path[PATH_MAX];
      struct stat source_stat,target_stat;
      if(stat(source_path,&source_stat) != 0){
        fprintf(stderr,"pa2_mv: cannot stat '%s': ",source_path);
        perror("");
        status = EXIT_FAILURE;
        continue;
      }
      const char *source_basename = pa2_basename(source_path);
      size_t dir_len = strlen(directory_path);
      if(dir_len > 0 && directory_path[dir_len-1] == '/'){
	      snprintf(target_path,PATH_MAX,"%s%s",directory_path,source_basename);
      }
      else{
	      snprintf(target_path,PATH_MAX,"%s/%s",directory_path,source_basename);
      }
      bool target_exist_check = (stat(target_path,&target_stat) == 0);
      if(target_exist_check && source_stat.st_dev == target_stat.st_dev && source_stat.st_ino == target_stat.st_ino){
        fprintf(stderr,"pa2_mv: '%s' and '%s' are the same file\n",pa2_basename(source_path),pa2_basename(target_path));
        status = EXIT_FAILURE;
        continue;
      }
      if(rename(source_path,target_path) != 0){
        status = handle_rename_error(source_path,target_path);
      }
    }
  }
  return status;
}
