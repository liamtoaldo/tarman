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

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cli/input.h"
#include "cli/output.h"

static void clear_input_stream() {
  char ch = 0;
  while (32 < (ch = getchar()) && !feof(stdin))
    ;
}

int cli_in_int(const char *msg, int range_min, int range_max) {
  bool use_range = true;

  if (range_min == range_max) {
    use_range = false;
    range_min = INT_MIN;
    range_max = INT_MAX;
  }

  while (true) {
    int input = 0;

    cli_out_newline();

    if (use_range) {
      cli_out_prompt("%s [%d, %d]:", msg, range_min, range_max);
    } else {
      cli_out_prompt("%s:", msg);
    }

    if (1 != scanf("%d", &input)) {
      clear_input_stream();
      cli_out_error("I/O Error: invalid input");
      continue;
    }

    if (input < range_min || input > range_max) {
      cli_out_error("Range Error: value '%d' is not valid for range [%d, %d]",
                    input,
                    range_min,
                    range_max);
      continue;
    }

    clear_input_stream();
    return input;
  }
}

bool cli_in_bool(const char *msg) {
  while (true) {
    char input = 0;

    cli_out_newline();
    cli_out_prompt("%s [Y/n]:", msg);

    if (1 != scanf("%c", &input)) {
      clear_input_stream();
      cli_out_error("I/O Error: invalid input");
      continue;
    }

    if ('\n' == input) {
      return true;
    }

    clear_input_stream();

    if ('Y' == toupper(input)) {
      return true;
    }

    if ('n' == tolower(input)) {
      return false;
    }

    cli_out_error("Range Error: '%c' is not a valid input for range [Y/n]");
  }
}

void cli_in_str(const char *msg, char *buf, size_t len) {
  cli_out_newline();
  cli_out_prompt("%s:", msg);

  size_t read = 0;
  for (; read < len; read++) {
    char c = getchar();

    if (EOF == c || '\n' == c) {
      return;
    }

    buf[read] = c;
  }

  clear_input_stream();
}

size_t cli_in_dystr(const char *msg, char **dst) {
  size_t len = 256;
  char  *buf = (char *)malloc(len * sizeof(char));
  size_t i   = 0;
  char   ch  = 0;

  if (NULL == buf) {
    cli_out_error("Unable to allocate memory, probably ran out of memory. "
                  "Exiting now");
    exit(EXIT_FAILURE);
  }

  cli_out_newline();
  cli_out_prompt("%s:", msg);

  while (EOF != (ch = getchar()) && '\n' != ch) {
    if (i == len - 1) {
      len *= 2;
      buf = realloc(buf, len * sizeof(char));

      if (NULL == buf) {
        cli_out_error("Unable to allocate memory, probably ran out of memory. "
                      "Exiting now");
        exit(EXIT_FAILURE);
      }
    }

    buf[i] = ch;
    i++;
  }

  buf[i] = 0;
  *dst   = buf;
  return i;
}
