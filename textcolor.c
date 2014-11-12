#include <stdio.h>
#include <stdarg.h>
#include "textcolor.h"

int print_color(char *text, int color) {
#if DEBUG
	return printf("%s", text);
#else
	return printf("\e[1;%dm%s\e[0m", color, text);
#endif
}

int vprint_color(char *text, int color, va_list vlist)
{
  char* msg;
#if DEBUG
  asprintf(&msg, "%s", text);
#else
  asprintf(&msg, "\e[1;%dm%s\e[0m", color, text);
#endif
  return vprintf(msg, vlist);
}

int print_spec(char *text, int type) {
	return print_color(text, type);
}

int print(int color, char *text, ...) {
  va_list args;
  va_start(args, text);

  int result = vprint_color(text, color, args);

  return result;
}

