#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>

#define MAX_LINE 10
#define BUFFER_SIZE 4096

void head_function(FILE *stream,long n_line,const char *filename){
  char line_buffer[BUFFER_SIZE];
  long curr_line = 0;
  while(curr_line < n_line && fgets(line_buffer,BUFFER_SIZE,stream) != NULL){
    if(fputs(line_buffer,stdout) == EOF){
      perror("pa2_head: error writing to stdout");
      break;
    }
    curr_line++;
  }

  if(ferror(stream)){
    fprintf(stderr,"pa2_head: error reading %s: ",(stream == stdin) ? "standard input" : filename);
    perror("");
  }
}

int main(int argc,char *argv[]) {
  long n_line = MAX_LINE;
  char *filename = NULL;
  FILE *input_stream = stdin;

  int opt;
  const char *optstring = "n:";
  while((opt = getopt(argc,argv,optstring)) != -1){
    switch(opt){
      case 'n': {
        char *endptr;
        errno = 0;
        long num = strtol(optarg,&endptr,10);
        if(errno != 0 || *endptr != '\0' || endptr == optarg || num < 0){
          fprintf(stderr,"pa2_head: invalid number of lines: '%s'\n",optarg);
          return EXIT_FAILURE;
        }
        n_line = num;
        break;
      }
      case '?':
      default:
        fprintf(stderr,"Try 'pa2_head --help' for more information..\n");
        return EXIT_FAILURE;
    }
  }

  if(optind < argc){
    filename = argv[optind];
    if(strcmp(filename,"-") != 0){
      struct stat file_stat;
      if(stat(filename,&file_stat) == 0){
        if(S_ISDIR(file_stat.st_mode)){
          fprintf(stderr,"pa2_head: error reading '%s': Is a directory\n",filename);
          return EXIT_FAILURE;
        }
      }
      input_stream = fopen(filename,"r");
      if(input_stream == NULL){
        fprintf(stderr,"pa2_head: cannot open '%s' for reading: ",filename);
        perror("");
        return EXIT_FAILURE;
      }
    }
  }

  head_function(input_stream,n_line,(filename != NULL) ? filename : "standard input");
  if(input_stream != stdin){
    fclose(input_stream);
  }

  fflush(stdout);
  
  return EXIT_SUCCESS;
}
