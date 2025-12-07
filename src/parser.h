#ifndef PARSER_H
#define PARSER_H

void trim(char *s);
int split_pipeline(const char *input, char segments[][1024]);
char **tokenize(const char *cmd, int *count);

#endif
