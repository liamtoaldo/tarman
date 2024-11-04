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

extern int tarman_plugin(const char *src, const char *dst, const char *cfg);

int main(int argc, char *argv[]) {
  if (4 != argc) {
    return EXIT_FAILURE;
  }

  // fclose(stdin);
  // fclose(stdout);
  // fclose(stderr);

  const char *src_file = argv[1];
  const char *dst_path = argv[2];
  const char *config   = argv[3];

  return tarman_plugin(src_file, dst_path, config);
}
