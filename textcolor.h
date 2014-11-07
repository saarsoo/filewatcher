#ifndef _TEXTCOLOR_H
#define _TEXTCOLOR_H

#define RED 31
#define YELLOW 33
#define BLUE 34
#define GREEN 32
#define GRAY 30
#define WHITE 0
#define PURPLE 35

#define ERROR RED
#define INFO YELLOW
#define OTHER GRAY
#define SUCCESS GREEN

int print_color(char *text, int color);

int print_spec(char *text, int type);

#endif
