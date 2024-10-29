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
#include <string.h>

#include "cli/directives/commands.h"
#include "cli/directives/lookup.h"
#include "cli/directives/options.h"
#include "cli/directives/types.h"

static cli_drt_desc_t commands[] = {
    {NULL, TARMAN_CMD_HELP, NULL, false, cli_cmd_help, "Show this menu"},

    {NULL,
     TARMAN_CMD_INSTALL,
     NULL,
     false,
     cli_cmd_install,
     "Install a package"},

    {NULL,
     TARMAN_CMD_LIST,
     NULL,
     false,
     cli_cmd_list,
     "List all installed packages"},

    {NULL,
     TARMAN_CMD_REMOVE,
     NULL,
     false,
     cli_cmd_remove,
     "Remove an installed package"},

    {NULL,
     TARMAN_CMD_UPDATE,
     NULL,
     false,
     cli_cmd_update,
     "Update an installed package"},

    {NULL,
     TARMAN_CMD_UPDATE_ALL,
     NULL,
     false,
     cli_cmd_update_all,
     "Update all installed repos and pacakges"},

    {NULL,
     TARMAN_CMD_ADD_REPO,
     NULL,
     false,
     cli_cmd_add_repo,
     "Add a remote repository to the local database"},

    {NULL,
     TARMAN_CMD_REMOVE_REPO,
     NULL,
     false,
     cli_cmd_remove_repo,
     "Remove a local repository"},

    {NULL,
     TARMAN_CMD_LIST_REPOS,
     NULL,
     false,
     cli_cmd_list_repos,
     "List all local repositories"},

    {NULL, TARMAN_CMD_TEST, NULL, false, cli_cmd_test, "Test tarman"}};

static cli_drt_desc_t options[] = {
    {TARMAN_SOPT_FROM_URL,
     TARMAN_FOPT_FROM_URL,
     cli_opt_from_url,
     false,
     NULL,
     "Use URL as package input and perform download"},

    {TARMAN_SOPT_FROM_REPO,
     TARMAN_FOPT_FROM_REPO,
     cli_opt_from_repo,
     false,
     NULL,
     "Use simple package name as input and perform repo lookup"},

    {TARMAN_SOPT_PKG_NAME,
     TARMAN_FOPT_PKG_NAME,
     cli_opt_pkg_name,
     true,
     NULL,
     "Set or override package name"},

    {TARMAN_SOPT_APP_NAME,
     TARMAN_FOPT_APP_NAME,
     cli_opt_app_name,
     true,
     NULL,
     "Override application name during installation"},

    {TARMAN_SOPT_EXEC,
     TARMAN_FOPT_EXEC,
     cli_opt_exec,
     true,
     NULL,
     "Override executable path during installation"},

    {TARMAN_SOPT_WRK_DIR,
     TARMAN_FOPT_WRK_DIR,
     cli_opt_wrk_dir,
     true,
     NULL,
     "Specify a directory to use as working directory in the `.desktop` file "
     "during installation"},

    {TARMAN_SOPT_ICON,
     TARMAN_FOPT_ICON,
     cli_opt_icon,
     true,
     NULL,
     "Override icon path for desktop application"},

    {TARMAN_SOPT_ADD_PATH,
     TARMAN_FOPT_ADD_PATH,
     cli_opt_add_path,
     false,
     NULL,
     "Add package executable to PATH during installation"},

    {TARMAN_SOPT_ADD_DESKTOP,
     TARMAN_FOPT_ADD_DESKTOP,
     cli_opt_add_desktop,
     false,
     NULL,
     "Add package as `.desktop` application during installation"},
};

static bool find_desc(cli_drt_desc_t  descriptors[],
                      size_t          num_desc,
                      const char     *arg,
                      cli_drt_desc_t *dst) {
  bool found_matching_option = false;

  for (int i = 0; i < num_desc; i++) {
    cli_drt_desc_t opt_desc = descriptors[i];

    if ((NULL == opt_desc.short_option ||
         0 != strcmp(opt_desc.short_option, arg)) &&
        (NULL == opt_desc.full_option ||
         0 != strcmp(opt_desc.full_option, arg))) {
      continue;
    }

    *dst = opt_desc;
    return true;
  }

  return false;
}

bool cli_lkup_command(const char *command, cli_drt_desc_t *dst) {
  return find_desc(
      commands, sizeof commands / sizeof(cli_drt_desc_t), command, dst);
}

bool cli_lkup_option(const char *option, cli_drt_desc_t *dst) {
  return find_desc(
      options, sizeof options / sizeof(cli_drt_desc_t), option, dst);
}

cli_lkup_table_t cli_lkup_cmdtable() {
  return (cli_lkup_table_t){.table = commands,
                            .num_entries =
                                sizeof commands / sizeof(cli_drt_desc_t)};
}

cli_lkup_table_t cli_lkup_opttable() {
  return (cli_lkup_table_t){
      .table = options, .num_entries = sizeof options / sizeof(cli_drt_desc_t)};
}
