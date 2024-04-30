#include "ini.h"
#include "util.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum ini_parser_err {
  ini_parser_no_err = 0,

  // Keep last
  ini_parser_err_count,
};

static const int is_comment(const char *l) { return l[0] == ';'; }

static const int is_empty(const char *l) { return strlen(l) == 0; }

static int reallocate(char **ptr, const char *s, const char *e) {

  if (NULL == e) {
    e = s + (strlen(s) - 1);
  }

  int count = (e - s) + 1;

  if (count == 0) {
    free(*ptr);
    *ptr = NULL;
    return 0;
  }

  void *prev = *ptr;
  *ptr = realloc(*ptr, count + 1);

  if (*ptr == NULL) {
    free(prev);
    *ptr = NULL;
    return 0;
  }
  (*ptr)[count] = '\0';

  return count;
}

int parse_section(char **section, const char *s) {

  if ('[' != s[0]) {
    return 1;
  }

  char *e = strchr(s, ']');

  if (NULL == e) {
    return -1;
  }

  { // check syntax is correct. no more char after closing ']'
    const char *r = ltrim(e + 1);
    if (!is_empty(r)) {
      return -1;
    }
  }

  int count = reallocate(section, s + 1, e - 1);
  if (count == 0) {
    return -1;
  }

  strncpy(*section, s + 1, count);

  for (char *p = *section; *p; p++)
    *p = tolower(*p);

  return 0;
}

int parse_key_value(char **key, char **value, char *str) {

  char *sep = strchr(str, '=');

  if (NULL == sep) {
    return -1;
  }

  *sep = '\0';
  { // parse key
    const char *s = rtrim(str);
    int count = reallocate(key, s, NULL);
    if (0 == count) {
      return -1;
    }
    strncpy(*key, s, count);
  }
  { // parse value
    const char *s = ltrim(sep + 1);
    int count = reallocate(value, s, NULL);
    if (0 == count) {
      return -1;
    }
    strncpy(*value, s, count);
  }

  return 0;
}

int ini_parse_file(FILE *fp, ini_handler handler, void *user_data) {
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  char *section = NULL;
  char *key = NULL;
  char *value = NULL;

  int i = 0;

  while ((read = getline(&line, &len, fp)) != -1) {

    char *l = trim(line);
    i++;
    if (is_comment(l) || is_empty(l)) {
      continue;
    }
    { // parse the current line

      int result = parse_section(&section, l);
      if (result < 0) {
        fprintf(stderr, "error: ini parser at line %d near '%s'\n", i, l);
        return -1;

      } else if (result > 0) {
        if (parse_key_value(&key, &value, l) < 0) {
          fprintf(stderr, "error: ini parser at line %d near '%s'\n", i, l);
          return -1;
        }
        handler(&section, &key, &value, user_data);
      } else if (result == 0) {
        handler(&section, NULL, NULL, user_data);
      } else {
        assert(0);
        return -1;
      }
    }
    i++;
  }
  if (value)
    free(value);

  if (key)
    free(key);

  if (section)
    free(section);

  if (line)
    free(line);

  return 0;
}

int ini_parse(const char *path, ini_handler handler, void *user_data) {
  FILE *fp;

  fp = fopen(path, "r");
  if (fp == NULL) {
    fprintf(stderr, "error: ini parser. %s: %s\n", strerror(errno), path);
    return -1;
  }
  int result = ini_parse_file(fp, handler, user_data);

  fclose(fp);

  return result;
}
