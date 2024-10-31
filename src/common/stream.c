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

#include <stdio.h>
#include <stdlib.h>

#include "cli/output.h"
#include "stream.h"
#include "tm-mem.h"

size_t stream_readline(FILE *stream, char **dst) {
  size_t len = 256;
  char  *buf = (char *)malloc(len * sizeof(char));
  size_t i   = 0;
  char   ch  = 0;

  mem_chkoom(buf);

  while (EOF != (ch = fgetc(stream)) && '\n' != ch) {
    if (i == len - 1) {
      len *= 2;
      buf = realloc(buf, len * sizeof(char));
      mem_chkoom(buf);
    }

    buf[i] = ch;
    i++;
  }

  buf[i] = 0;
  *dst   = buf;
  return i;
}
