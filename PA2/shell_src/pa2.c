#include <editline/readline.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "helper.h"

void set_up_readline_on_newline() {
  rl_on_new_line();
  rl_replace_line("", 0);
  rl_redisplay();
}

void setup_pipe(Process* process, int pipefd[2]);
void setup_redirection(Process* process);
void dup2_stdio(Process* process);

// todo: implement this function
void sigint_handler(int signum) {
  write(STDOUT_FILENO,"\n",1);
  set_up_readline_on_newline();
}
// todo: implement this function
void child_handler(int signum) {
  (void)signum;
  Job *job = NULL;
  Process *process = NULL;

  while((process = wait_for_any_process(&job)) != NULL){
    if(job == NULL){
      continue;
    }
    if(job->state == JOB_DONE){
      if(job->id > 0){
        fprintf(stderr,"pa2: Job %ld, '%s' has ended\n",job->id,job->associated_command);
        set_up_readline_on_newline();
      }
      remove_job_from_table(job,NULL);
      free_job(job);
    }
    job = NULL;
  }
}

// todo: implement this function
status_t run_exit(Command* command, Jobs* jobs, status_t last_status) {
  if(!is_job_table_empty(jobs)){
    fprintf(stderr,"exit: There are still jobs active.\n");
    return 1;
  }
  if(command->num_args > 2){
    fprintf(stderr,"exit: too many arguments\n");
    return 1;
  }
  int exitcode = last_status;
  if(command->num_args == 2){
    char *endptr;
    const char *code_str = command->args[1];
    errno = 0;
    long num = strtol(coe_str,&endptr,10);
    if(errno != 0 || *endptr != '\0' || endptr == code_str){
      fprintf(stderr,"exit: %s: invalid integer\n",code_str);
      return 1;
    }
    if(num >= 255){
      exitcode = 255;
    }
    else{
      exitcode = (int)num;
    }
  }
  exit(exitcode);
  return 0;
}

// todo: implement this function
status_t run_cd(Command* command) {
  const char *target_path;
  if(command->num_args > 2){
    fprintf(stderr,"cd: too many arguments\n");
    return 1;
  }
  if(command->num_args == 1){
    target_path = getenv("HOME");
    if(target_path == NULL){
      fprintf(stderr,"cd: HOME environment variable not set\n");
      return 1;
    }
  }
  else{
    target_path = command->args[1];
  }
  if(chdir(target_path) != 0){
    fprintf(stderr,"cd: %s: ",target_path);
    perror("");
    return 1;
  }
  return 0;
}

// todo: implement this function
status_t run_pwd(Command* command) {
  bool flag = false;
  for(size_t i = 1;i < command->num_args;i++){
    if(strcmp(command->args[i],"-P") == 0){
      physical = true;
      break;
    }
  }
  if(!flag){
    const char *logical_path = getenv("PWD");
    if(logical_path != NULL){
      printf("%s\n",logical_path);
      return 0;
    }
  }
  char *cwd_buffer = getcwd(NULL,0);
  if(cwd_buffer == NULL){
    perror("pwd");
    return 1;
  }
  printf("%s\n",cwd_buffer);
  free(cwd_buffer);
  return 0;
}

// TODO: finish this function
status_t run_command(Command* command, Jobs* jobs, status_t last_status) {
  switch (command->type) {
    case COMMAND_BUILTIN:
      return run_builtin_command(command, jobs, last_status);
      break;
    case COMMAND_IMPLEMENTED:{
      char *bin_dir = get_bin_dir();
      size_t bin_len = strlen(bin_dir);
      size_t cmd_len = strlen(command->args[0]);
      char *exec_path = realloc(bin_dir,bin_len + cmd_len + 1);
      if(exec_path == NULL){
        perror("realloc failed");
        exit(EXIT_FAILURE);
      }
      strcat(exec_path,command->args[0]);
      execv(exec_path,command->args);
      perror(exec_path);
      free(exec_path);break;
    }
    default:
      execvp(command->args[0],command->args);
      if(errno == ENOENT){
        fprintf(stderr,"%s: command not found\n",command->args[0]);
      }
      else{
        perror(command->args[0]);
      }
      break;
  }
  return EXIT_FAILURE;
}

status_t run_single_builtin_command(Command* command,
                                    Jobs* jobs,
                                    status_t last_status) {
  int backup_fds[2] = {dup(STDIN_FILENO), dup(STDOUT_FILENO)};

  Job* job = default_job();
  set_foreground_job(job, jobs);

  Process* process = default_process();
  process->command = command;

  setup_redirection(process);
  dup2_stdio(process);
  int32_t status = run_builtin_command(process->command, jobs, last_status);

  // todo: restore fd
  dup2(backup_fds[0],STDIN_FILENO);
  dup2(backup_fds[1],STDIN_FILENO);
  // todo: end
  close(backup_fds[0]);
  close(backup_fds[1]);

  if (strncmp(command->args[0], "fg", 3) != 0) {
    remove_foreground_job(jobs, true);
  }

  return status;
}

// todo: implement this function
void setup_pipe(Process* process, int pipefd[2]) {
  SAFELY_RUN(pipe(pipefd));
  process->stdout_fd = pipefd[1];
  process->next_process->stdin_fd = pipefd[0];
}

// todo: implement this function
void setup_redirection(Process* process) {
  Command* command = process->command;
  char error_msg[1024];
  if (command->stdin != NULL) {
    int fd_in = open(command->stdin, O_RDONLY);
    
    if (fd_in == -1) {
      snprintf(error_msg, sizeof(error_msg), "pa2_shell: %s", command->stdin);
      perror(error_msg); 
      exit(EXIT_FAILURE);
    }
    process->stdin_fd = fd_in;
  }
  if (command->stdout != NULL) {
    int flags = O_WRONLY | O_CREAT; 
    if (command->append_stdout) {
      flags |= O_APPEND;
    } else {
      flags |= O_TRUNC;
    }
    int fd_out = open(command->stdout, flags, 0644); 
    if (fd_out == -1) {
      snprintf(error_msg, sizeof(error_msg), "pa2_shell: %s", command->stdout);
      perror(error_msg);
      exit(EXIT_FAILURE);
    }
    process->stdout_fd = fd_out;
  }
}

// todo: implement this function
void dup2_stdio(Process* process) {
  if (process->stdin_fd != -1) {
    SAFELY_RUN(dup2(process->stdin_fd,STDIN_FILENO));
  }

  if (process->stdout_fd != -1) {
    SAFELY_RUN(dup2(process->stdout_fd,STDOUT_FILENO));
  }
}

// todo: finish this function
status_t run_pipeline(Pipeline* pipeline,
                      Jobs* jobs,
                      char* cmd,
                      status_t last_status) {
  Job* job = default_job();
  job->associated_command = cmd;
  if (pipeline->is_in_background) {
    job->state = JOB_BACKGROUND;
  } else {
    set_foreground_job(job, jobs);
  }

  int32_t pipefd[2];

  Command* current_command = pipeline->first_command;
  Command* next_command = pipeline->first_command->next_command;

  while (current_command != NULL) {
    if (job->first_process == NULL) {
      job->first_process = job->last_process = default_process();
    } else {
      job->last_process = job->last_process->next_process;
    }

    Process* current_process = job->last_process;
    current_process->command = current_command;

    if (next_command != NULL) {
      current_process->next_process = default_process();
      setup_pipe(current_process, pipefd);
    }

    switch (current_process->pid = fork()) {
      case -1:
        perror("fork");
        exit(1);
      case 0:
        // TODO: set up signals
        signal(SIGINT,SIG_DFL);
        signal(SIGQUIT,SIG_DFL);
        signal(SIGTSTP,SIG_DFL);
        signal(SIGTTIN,SIG_DFL);
        signal(SIGTTOU,SIG_DFL);

        setup_redirection(current_process);
        dup2_stdio(current_process);
        exit(run_command(current_command, jobs, last_status));
    }

    if (job->pgid == -1) {
      job->pgid = current_process->pid;
      setpgid(current_process->pid, job->pgid);

      if (job->state == JOB_FOREGROUND)
        tcsetpgrp(STDIN_FILENO, job->pgid);
    } else {
      setpgid(current_process->pid, job->pgid);
    }

    current_process->is_running = true;

    if (job->first_process != current_process) {
      close(current_process->stdin_fd);
    }
    if (next_command != NULL) {
      close(pipefd[1]);
    }

    current_command = next_command;

    if (current_command != NULL) {
      next_command = current_command->next_command;
    }
  }

  int32_t status = handle_job(job, jobs);
  return status;
}

status_t interpret(Pipeline* pipeline,
                   Jobs* jobs,
                   char* cmd,
                   status_t last_status) {
  if (pipeline->num_commands == 0)
    return 0;

  status_t status;
  if (is_single_builtin_command(pipeline)) {
    status =
        run_single_builtin_command(pipeline->first_command, jobs, last_status);
  } else {
    status = run_pipeline(pipeline, jobs, cmd, last_status);
  }

  return status;
}

status_t evaluate(char* cmd, Jobs* jobs, status_t last_status) {
  Token tokens[MAX_TOKENS];
  Pipeline pipeline;
  lex(cmd, tokens);
  parse(tokens, &pipeline);
  interpret(&pipeline, jobs, cmd, last_status);
  free_tokens(tokens);
  return 0;
}

int main() {
  Jobs jobs;
  memset(&jobs, 0, sizeof(jobs));
  set_jobs_global(&jobs);

  status_t last_status = 0;

  // TODO: set signal handlers
  signal(SIGINT,sigint_handler);
  signal(SIGCHLD,child_handler);
  signal(SIGQUIT,SIG_IGN);
  signal(SIGTSTP,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGTTOU,SIG_IGN);
  // TODO: end

  while (1) {
    char* cmd;
    if ((cmd = readline("$ ")) == NULL)
      break;

    add_history(cmd);
    last_status = evaluate(cmd, &jobs, last_status);
  }

  return 0;
}