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

#include <stdlib.h>
#include <string.h>

#include "cli/commands/commands.h"
#include "cli/commands/lookup.h"
#include "cli/output.h"
#include "cli/parser.h"

bool cli_parse(int         argc,
               char       *argv[],
               cli_info_t *cli_info,
               cli_exec_t *handler) {
  if (2 > argc) {
    return true;
  }

  // Descriptor of the command
  cli_cmd_desc_t cmd_desc;

  // If no matching command is found, an
  // error is thrown
  if (!cli_lkup_command(argv[1], &cmd_desc)) {
    cli_out_error("Unknown command '%s'", argv[1]);
    return false;
  }

  *handler = cmd_desc.exec_handler;

  for (int i = 2; i < argc; i++) {
    const char *argument = argv[i];
    const char *next     = NULL;
    if (argc - i != i) {
      next = argv[i + 1];
    }

    cli_cmd_desc_t opt_desc;

    // If no mathcing option was found
    // This argument is treated as the input file
    if (!cli_lkup_option(argument, &opt_desc)) {
      if (NULL != cli_info->input) {
        cli_out_error("Too many inputs");
        return false;
      }

      cli_info->input = argument;
      continue;
    }

    // Skip next CLI argument if the option required an arguments
    // of its own
    if (opt_desc.has_argument) {
      i++;
    }

    // If the handler is for an option
    if (NULL != opt_desc.handler && !opt_desc.handler(cli_info, next)) {
      return false;
    }
  }

  return true;
}
