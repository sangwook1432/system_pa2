#include "parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Command* default_command() {
  Command* command = malloc(sizeof *command);
  *command = (Command){
      .args = {NULL},
      .stdin = NULL,
      .stdout = NULL,
      .num_args = 0,
      .append_stdout = false,
      .type = COMMAND_NULL,
      .next_command = NULL,
  };
  return command;
}

bool is_special_char(const char c) {
  return c == '<' || c == '>' || c == '|' || c == '\'' || c == '"' ||
         c == '\0' || isspace(c) || c == '&';
}

bool is_background(const Token token) {
  return token.type == TOKEN_BACKGROUND;
}

bool is_pipe(const Token token) {
  return token.type == TOKEN_PIPE;
}

bool is_redirection(const Token token) {
  return token.type == TOKEN_REDIRECT_IN || token.type == TOKEN_REDIRECT_OUT ||
         token.type == TOKEN_REDIRECT_APPEND;
}

bool is_builtin_command(const Token token) {
  const char* builtin_commands[] = {"cd", "exit", "pwd", "jobs",
                                    "fg", "bg",   NULL};

  for (int i = 0; builtin_commands[i] != NULL; i++) {
    if (strcmp(token.data, builtin_commands[i]) == 0) {
      return true;
    }
  }
  return false;
}

bool is_implemented_command(const Token token) {
  const char* implemented_commands[] = {
      "pa2_head", "pa2_tail", "pa2_cat", "pa2_cp", "pa2_mv", "pa2_rm", NULL};

  for (int i = 0; implemented_commands[i] != NULL; i++) {
    if (strcmp(token.data, implemented_commands[i]) == 0) {
      return true;
    }
  }
  return false;
}

bool is_single_builtin_command(Pipeline* pipeline) {
  return pipeline->num_commands == 1 &&
         pipeline->first_command->type == COMMAND_BUILTIN;
}

void lex(const char* cmd, Token* tokens) {
  int token_i = 0;

  for (const char* curr_char = cmd; *curr_char != '\0'; curr_char++) {
    if (isspace(*curr_char))
      continue;

    switch (*curr_char) {
      case '|':
        tokens[token_i++] = (Token){.type = TOKEN_PIPE, .data = "|"};
        break;
      case '<':
        tokens[token_i++] = (Token){.type = TOKEN_REDIRECT_IN, .data = "<"};
        break;
      case '>':
        if (*(curr_char + 1) == '>') {
          tokens[token_i++] =
              (Token){.type = TOKEN_REDIRECT_APPEND, .data = ">>"};
          curr_char++;
        } else {
          tokens[token_i++] = (Token){.type = TOKEN_REDIRECT_OUT, .data = ">"};
        }
        break;
      case '&':
        tokens[token_i++] = (Token){.type = TOKEN_BACKGROUND, .data = "&"};
        break;
      case '\'':
      case '\"': {
        // not required for PA2, it just makes checking commands much easier
        // since some commands almost require quotes (i.e awk)
        char quote = *curr_char;
        const char* start = ++curr_char;
        while (*curr_char != quote && *curr_char != '\0') {
          curr_char++;
        }
        tokens[token_i++] = (Token){
            .type = TOKEN_ARGUMENT,
            .data = strndup(start, curr_char - start),
        };
      } break;
      default: {
        const char* command_start = curr_char;

        TokenType type = (token_i == 0 || is_pipe(tokens[token_i - 1]))
                             ? TOKEN_COMMAND
                             : TOKEN_ARGUMENT;

        while (!is_special_char(*curr_char)) {
          curr_char++;
        }

        tokens[token_i++] =
            (Token){.type = type,
                    .data = strndup(command_start, curr_char - command_start)};
        curr_char--;
        break;
      }
    }
  }

  tokens[token_i] = (Token){.type = TOKEN_END, .data = NULL};
}

void free_tokens(Token tokens[]) {
  for (int i = 0; tokens[i].type != TOKEN_END; i++) {
    if (tokens[i].type == TOKEN_COMMAND || tokens[i].type == TOKEN_ARGUMENT) {
      free(tokens[i].data);
    }
  }
}

void free_pipeline(Pipeline* pipeline) {
  Command* current_command = pipeline->first_command;
  while (current_command != NULL) {
    Command* next_command = current_command->next_command;
    free(current_command);
    current_command = next_command;
  }
}

int parse(Token* tokens, Pipeline* pipeline) {
  pipeline->num_commands = 0;
  pipeline->is_in_background = false;
  pipeline->first_command = default_command();
  Command* current_command = pipeline->first_command;

  for (int i = 0; tokens[i].type != TOKEN_END; i++) {
    if (is_pipe(tokens[i]) || is_redirection(tokens[i]) ||
        is_background(tokens[i])) {
      switch (tokens[i].type) {
        case TOKEN_PIPE:
          Command* next_command = default_command();
          current_command->args[current_command->num_args] = NULL;
          current_command->next_command = next_command;
          current_command = next_command;
          break;
        case TOKEN_REDIRECT_IN:
          i++;
          current_command->stdin = tokens[i].data;
          break;
        case TOKEN_REDIRECT_APPEND:
          current_command->append_stdout = true;
          // fall through
        case TOKEN_REDIRECT_OUT:
          i++;
          current_command->stdout = tokens[i].data;
          break;
        case TOKEN_BACKGROUND:
          pipeline->is_in_background = true;
          break;
        default:
          fprintf(stderr, "Invalid token: %s\n", tokens[i].data);
          exit(1);
      }
    } else {
      if (current_command->num_args == 0) {
        pipeline->num_commands++;

        if (is_builtin_command(tokens[i])) {
          current_command->type = COMMAND_BUILTIN;
        } else if (is_implemented_command(tokens[i])) {
          current_command->type = COMMAND_IMPLEMENTED;
        } else {
          current_command->type = COMMAND_OTHER;
        }
      }

      current_command->args[current_command->num_args++] = tokens[i].data;
    }
  }
  return 0;
}
