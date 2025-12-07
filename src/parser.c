#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"

// Trim leading/trailing spaces
void trim(char *s) {
    int i = 0, j = strlen(s) - 1;
    while (isspace(s[i])) i++;
    while (j >= i && isspace(s[j])) j--;

    int len = j - i + 1;
    memmove(s, s + i, len);
    s[len] = '\0';
}

// Split pipeline "cmd1 | cmd2 | cmd3"
int split_pipeline(const char *input, char segments[][1024]) {
    char buf[1024];
    strcpy(buf, input);

    int count = 0;
    char *start = buf;
    char *p = buf;

    while (*p) {
        if (*p == '|') {
            *p = '\0';
            trim(start);
            strcpy(segments[count++], start);
            start = p + 1;
        }
        p++;
    }

    trim(start);
    strcpy(segments[count++], start);

    return count;
}

// Tokenizer with QUOTE SUPPORT
char **tokenize(const char *cmd, int *count) {
    char *copy = strdup(cmd);
    char **tokens = malloc(128 * sizeof(char *));
    *count = 0;

    int i = 0;
    while (copy[i]) {
        while (isspace(copy[i])) i++;
        if (!copy[i]) break;

        // Handle quoted token
        if (copy[i] == '"') {
            int start = ++i;
            while (copy[i] && copy[i] != '"') i++;

            int len = i - start;
            tokens[*count] = malloc(len + 1);
            strncpy(tokens[*count], &copy[start], len);
            tokens[*count][len] = '\0';
            (*count)++;

            if (copy[i] == '"') i++;
        }
        else {
            // Normal token
            int start = i;
            while (copy[i] && !isspace(copy[i])) i++;

            int len = i - start;
            tokens[*count] = malloc(len + 1);
            strncpy(tokens[*count], &copy[start], len);
            tokens[*count][len] = '\0';
            (*count)++;
        }
    }

    tokens[*count] = NULL;
    free(copy);
    return tokens;
}
