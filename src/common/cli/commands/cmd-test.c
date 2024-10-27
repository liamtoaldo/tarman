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
#include <stdlib.h>

#include "archive.h"
#include "cli/commands/commands.h"
#include "cli/output.h"

static bool test_tar() {
  cli_out_progress("Testing tar extraction...");
  bool r = archive_tar_extract("extract", "archive.tar");

  if (!r) {
    cli_out_error("Tar extraction failed");
  }

  return r;
}

int cli_cmd_test(cli_info_t info) {
  if (!test_tar()) {
    cli_out_error("Tests failed");
    return EXIT_FAILURE;
  }

  cli_out_success("All successful");
  return EXIT_SUCCESS;
}
