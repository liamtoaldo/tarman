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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"

bool archive_tar_extract(const char *dst, const char *src) {
  size_t dst_len = strlen(dst);
  size_t src_len = strlen(src);
  size_t cmd_len = dst_len + src_len + strlen("tar -xf -C ") + 1;

  char *command = (char *)malloc(cmd_len);
  if (NULL == command) {
    return false;
  }

  sprintf(command, "tar -xf %s -C %s", src, dst);
  int ecode = system(command);
  free(command);
  return !ecode;
}

bool archive_zip_extract(const char *dst, const char *src) {
  return false;
}
