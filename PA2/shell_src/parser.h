#ifndef PARSING_H
#define PARSING_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define MAX_TOKENS 1000

typedef enum {
  TOKEN_COMMAND,
  TOKEN_ARGUMENT,
  TOKEN_PIPE,             // |
  TOKEN_REDIRECT_IN,      // <
  TOKEN_REDIRECT_OUT,     // >
  TOKEN_REDIRECT_APPEND,  // >>
  TOKEN_BACKGROUND,       // &
  TOKEN_END,
} TokenType;

typedef struct token {
  TokenType type;
  char* data;
} Token;

void free_tokens(Token tokens[]);

typedef enum {
  COMMAND_NULL,
  COMMAND_BUILTIN,      // cd, pwd, exit, fg, bg, jobs
  COMMAND_IMPLEMENTED,  // head, tail, cp, mv, rm
  COMMAND_OTHER
} CommandType;

typedef struct Command {
  char* args[MAX_TOKENS];
  char* stdin;
  char* stdout;
  size_t num_args;
  bool append_stdout;
  CommandType type; // COMMAND_NULL, COMMAND_BUILTIN, COMMAND_IMPLEMENTED, COMMAND_OTHER
  struct Command* next_command;
} Command;

Command* default_command();

typedef struct {
  bool is_in_background;
  Command* first_command;
  uint64_t num_commands;
} Pipeline;

void free_pipeline(Pipeline* pipeline);

bool is_background(const Token token);
bool is_special_char(const char c);
bool is_pipe(const Token token);
bool is_redirection(const Token token);
bool is_builtin_command(const Token token);
bool is_implemented_command(const Token token);
bool is_single_builtin_command(Pipeline* pipeline);

void lex(const char* cmd, Token* tokens);
int parse(Token* tokens, Pipeline* pipeline);

#endif
