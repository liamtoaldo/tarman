/*************************************************************************
| tarman                                                                 |
| Copyright (C) 2024 Alessandro Salerno                                  |
|                                                                        |
| This program is free software: you can redistribute it and/or modify   |
| it under the terms of the GNU General Public License as published by   |
| the Free Software Foundation, either version 3 of the License, or      |
| (at your option) any later version.                                    |
|                                                                        |
| This program is distributed in the hope that it will be useful,        |
| but WITHOUT ANY WARRANTY; without even the implied warranty of         |
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          |
| GNU General Public License for more details.                           |
|                                                                        |
| You should have received a copy of the GNU General Public License      |
| along with this program.  If not, see <https://www.gnu.org/licenses/>. |
*************************************************************************/

#include <stdarg.h>
#include <stdio.h>

#include "cli/output.h"

typedef enum {
  COLOR_Red     = 31,
  COLOR_Green   = 32,
  COLOR_Yellow  = 33,
  COLOR_Magenta = 35,
  COLOR_Cyan    = 36,
  COLOR_White   = 37
} color_t;

static void textcolor(bool bold, color_t fgcolor) {
  if (!bold) {
    printf("\e[0;%dm", fgcolor);
    return;
  }

  printf("\e[1;%dm", fgcolor);
}

static void reset_color() {
  printf("\e[m\n");
}

void prefix(color_t color) {
  textcolor(false, color);
  printf("=> ");
}

void cli_out_progress(const char *fmt, ...) {
  prefix(COLOR_Magenta);

  va_list args;
  va_start(args, fmt);
  textcolor(true, COLOR_White);
  vprintf(fmt, args);
  va_end(args);

  reset_color();
}

void cli_out_success(const char *fmt, ...) {
  prefix(COLOR_Green);

  va_list args;
  va_start(args, fmt);
  textcolor(true, COLOR_Green);
  vprintf(fmt, args);
  va_end(args);

  reset_color();
}

void cli_out_error(const char *fmt, ...) {
  prefix(COLOR_Red);

  va_list args;
  va_start(args, fmt);
  textcolor(true, COLOR_Red);
  printf("ERROR: ");
  vprintf(fmt, args);
  va_end(args);

  reset_color();
}

void cli_out_warning(const char *fmt, ...) {
  prefix(COLOR_Yellow);

  va_list args;
  va_start(args, fmt);
  textcolor(true, COLOR_Yellow);
  printf("WARNING: ");
  vprintf(fmt, args);
  va_end(args);

  reset_color();
}

void cli_out_prompt(const char *fmt, ...) {
  textcolor(true, COLOR_Cyan);
  printf(":: ");

  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);

  reset_color();
}
