#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>

#define max_line 10
#define max_length 8192

int main(int argc,char *argv[]) {
  long n_line = max_line;
  char *filename = NULL;
  FILE *input_stream = stdin;

  int opt;
  const char *optstring = "n:";
  while((opt = getopt(argc,argv,optstring)) != -1){
    switch(opt){
      case 'n':{
        char *endptr;
        errno = 0;
        long num = strtol(optarg,&endptr,10);
        if(errno != 0 || *endptr != '\0' || endptr == optarg || num < 0){
          fprintf(stderr,"pa2_tail: invalid number of lines: '%s'\n",optarg);
          return EXIT_FAILURE;
        }
        if(num == 0) return EXIT_SUCCESS;
        n_line = num;
        break;
      }
      case '?':
      default:
        return EXIT_FAILURE;
    }
  }

  if(optind < argc){
    filename = argv[optind];
    if(strcmp(filename,"-") != 0){
      struct stat file_stat;
      if(stat(filename,&file_stat) == 0){
        if(S_ISDIR(file_stat.st_mode)){
          fprintf(stderr,"pa2_tail: error reading '%s': Is a directory\n",filename);
          return EXIT_FAILURE;
        }
      }

      input_stream = fopen(filename,"r");
      if(input_stream == NULL){
        fprintf(stderr,"pa2_tail: cannot open '%s' for reading: ",filename);
        perror("");
        return EXIT_FAILURE;
      }
    }
  }

  char **ring_buffer = malloc(n_line * sizeof(char*));
  char *line_storage = malloc(n_line * max_length * sizeof(char));
  if(ring_buffer == NULL|| line_storage == NULL){
    perror("pa2_tail: memory allocation failed");
    if(input_stream != stdin) fclose(input_stream);
    free(ring_buffer);
    free(line_storage);
    return EXIT_FAILURE;
  }
  for(long i = 0;i < n_line;i++){
    ring_buffer[i] = line_storage + i * max_length;
    ring_buffer[i][0] = '\0';
  }
  long curr_idx = 0;
  long total_line_read = 0;
  char read_buffer[max_length];
  while(fgets(read_buffer,max_length,input_stream) != NULL){
    strncpy(ring_buffer[curr_idx],read_buffer,max_length - 1);
    ring_buffer[curr_idx][max_length - 1] = '\0';
    curr_idx = (curr_idx + 1) % n_line;
    total_line_read++;
  }

  if(ferror(input_stream)){
    fprintf(stderr,"pa2_tail: error reading %s: ",(input_stream == stdin) ? "standard input" : filename);
    perror("");
  }

  long start_idx;
  long line_to_print;
  if(total_line_read < n_line){
    start_idx = 0;
    line_to_print = total_line_read;
  }
  else{
    start_idx = curr_idx;
    line_to_print = n_line;
  }

  for(long i = 0;i < line_to_print;i++){
    long print_idx = (start_idx + i) % n_line;
    if(fputs(ring_buffer[print_idx],stdout) == EOF){
      perror("pa2_tail: error writing to stdout");
      break;
    }
  }
  free(ring_buffer);
  free(line_storage);
  if(input_stream != stdin){
    fclose(input_stream);
  }

  fflush(stdout);
  return EXIT_SUCCESS;
}
