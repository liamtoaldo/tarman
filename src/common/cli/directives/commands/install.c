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

#include "archive.h"
#include "cli/directives/commands.h"
#include "cli/directives/types.h"
#include "cli/output.h"
#include "os/fs.h"
#include "package.h"

static void safe_free(void *ptr) {
  if (NULL != ptr) {
    free(ptr);
  }
}

static void override_if_set(const char **dst, const char *src) {
  if (NULL != src) {
    *dst = src;
  }
}

static void override_recipe(rt_recipe_t *recipe, cli_info_t cli_info) {
  override_if_set(&recipe->pkg_name, cli_info.pkg_name);
  override_if_set(&recipe->recepie.pkg_info.application_name,
                  cli_info.app_name);
  override_if_set(&recipe->recepie.pkg_info.executable_path,
                  cli_info.exec_path);
  override_if_set(&recipe->recepie.pkg_info.working_directory,
                  cli_info.working_dir);
  override_if_set(&recipe->recepie.pkg_info.icon_path, cli_info.icon_path);

  // Add override for PATH and DESKTOP
}

int cli_cmd_install(cli_info_t info) {
  rt_recipe_t recipe = {0};
  int         ret    = EXIT_FAILURE;

  cli_out_progress("Initializing host file system");

  if (!os_fs_tm_init()) {
    cli_out_progress("Failed to inizialize host file system");
    goto cleanup;
  }

  cli_out_progress("Initiating installation process");

  // Check if -R is set and do stuff
  // Check if -U is set and do stuff

  override_recipe(&recipe, info);

  // Try to infer information such as
  // app name and exec path and working dir

  const char *archive_path = info.input;
  char       *pkg_path     = NULL;

  if (0 == os_fs_tm_dypkg(&pkg_path, recipe.pkg_name)) {
    cli_out_error("Unable to determine path for package");
    goto cleanup;
  }

  cli_out_progress("Creating package in '%s'", pkg_path);

  if (TM_FS_DIROP_STATUS_OK != os_fs_mkdir(pkg_path)) {
    cli_out_error("Unable to create directory in '%s'", pkg_path);
    goto cleanup;
  }

  cli_out_progress("Extracting archive '%s' to '%s'", archive_path, pkg_path);

  if (!archive_tar_extract(pkg_path, archive_path)) {
    cli_out_error("Unable to extract archive");
    goto cleanup;
  }

  // Create .trpkg file in package directory

  cli_out_success("Package '%s' installed successfully", recipe.pkg_name);
  ret = EXIT_SUCCESS;

cleanup:
  safe_free(pkg_path);
  return ret;
}
