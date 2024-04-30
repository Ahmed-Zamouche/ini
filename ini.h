#ifndef _INI_H_
#define _INI_H_
#include <stdio.h>

typedef void (*ini_handler)(char **section, char **key, char **value,
                            void *user_data);
int ini_parse_file(FILE *fp, ini_handler handler, void *user_data);
int ini_parse(const char *path, ini_handler handler, void *user_data);
#endif //_INI_H_
