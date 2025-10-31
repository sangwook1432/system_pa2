#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096

void output(FILE *stream, const char *filename){
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    while((bytes_read = fread(buffer,1,BUFFER_SIZE,stream)) > 0){
        if(fwrite(buffer,1,bytes_read,stdout) != bytes_read){
            perror("pa2_cat: fwrite error to stdout");
            break;
        }
    }

    if(ferror(stream)){
        fprintf(stderr,"pa2_cat: error reading %s: ",(stream == stdin) ? "standard input" : filename);
        perror("");
    }
}

int main(int argc, char *argv[]) {
    if(argc == 1){
        output(stdin,"stdin");
    }
    else{
        for(int i = 1;i < argc;i++){
            if(strcmp(argv[i],"-") == 0){
                output(stdin,"stdin");
                continue;
            }
            struct stat file_stat;
            if(stat(argv[i],&file_stat) == 0){
                if(S_ISDIR(file_stat.st_mode)){
                    fprintf(stderr, "pa2_cat: %s: Is a directory\n",argv[i]);
                    continue;
                }
            }
            FILE *fptr = fopen(argv[i],"r");
            if(fptr == NULL){
                fprintf(stderr,"pa2_cat: %s: ",argv[i]);
                perror("");
                continue;
            }
            output(fptr,argv[i]);
            fclose(fptr);
        }
    }
    fflush(stdout);
    return EXIT_SUCCESS;
}
