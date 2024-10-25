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

#include "cli/commands.h"
#include "cli/output.h"
#include "cli/parser.h"

cli_cmd_desc_t commands[] = {};

bool cli_parse(int         argc,
               char       *argv[],
               cli_info_t *cli_info,
               cli_exec_t *handler) {
  if (2 > argc) {
    return false;
  }

  for (int i = 1; i < argc; i++) {
    bool        found_matching_option = false;
    const char *argument              = argv[i];
    const char *next                  = NULL;
    if (argc - i != i) {
      next = argv[i + 1];
    }

    for (int j = 0;
         !found_matching_option && j < sizeof commands / sizeof(cli_cmd_desc_t);
         j++) {
      cli_cmd_desc_t opt_desc = commands[j];

      if (0 != strcmp(opt_desc.short_option, argument) &&
          0 != strcmp(opt_desc.full_option, argument)) {
        continue;
      }

      found_matching_option = true;
      if (opt_desc.has_argument) {
        i++; // Advance outer loop to avoid issues
      }

      // If the handler is for mutually exclusive option
      if (NULL != opt_desc.exec_handler) {
        if (NULL != *handler) {
          cli_out_error("More than one command specified");
          return false;
        }

        *handler = opt_desc.exec_handler;
      }

      // If the handler isfor a non-mutually exclusive option
      if (NULL != opt_desc.handler && !opt_desc.handler(cli_info, next)) {
        return false;
      }
    }

    // If no mathcing option was found
    // This argument is treated as the input file
    if (!found_matching_option) {
      if (NULL != cli_info->input) {
        cli_out_error("Too many inputs");
        return false;
      }

      cli_info->input = argument;
    }
  }

  return true;
}
