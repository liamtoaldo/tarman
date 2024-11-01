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
#include "cli/input.h"
#include "cli/output.h"
#include "os/fs.h"
#include "package.h"
#include "tm-mem.h"

static void override_if_src_set(const char **dst, const char *src) {
  if (NULL != src && 0 != src[0]) {
    *dst = src;
  }
}

static void override_if_dst_unset(const char **dst, const char *src) {
  if (NULL == *dst || 0 == *dst[0]) {
    override_if_src_set(dst, src);
  }
}

static void override_recipe(rt_recipe_t *recipe, cli_info_t cli_info) {
  override_if_src_set(&recipe->pkg_name, cli_info.pkg_name);
  override_if_src_set(&recipe->recepie.pkg_info.application_name,
                      cli_info.app_name);
  override_if_src_set(&recipe->recepie.pkg_info.executable_path,
                      cli_info.exec_path);
  override_if_src_set(&recipe->recepie.pkg_info.working_directory,
                      cli_info.working_dir);
  override_if_src_set(&recipe->recepie.pkg_info.icon_path, cli_info.icon_path);

  // Add override for PATH and DESKTOP
}

static char *load_config_file(rt_recipe_t *recipe, char *pkg_path) {
  if (recipe->is_remote) {
    return NULL;
  }

  pkg_info_t tmpkg_file_data = {0};
  char      *tmpkg_file_path = NULL;

  os_fs_path_dyconcat(&tmpkg_file_path, 2, pkg_path, "info.tmpkg");

  pkg_parse_status_t status =
      pkg_parse_tmpkg(&tmpkg_file_data, tmpkg_file_path);

  switch (status) {
  case TM_PKG_PARSE_STATUS_NOFILE:
    cli_out_warning("Package configuration '%s' file does not exist",
                    tmpkg_file_path);
    return tmpkg_file_path;

  case TM_PKG_PARSE_STATUS_INVVAL:
  case TM_PKG_PARSE_STATUS_MALFORMED:
    cli_out_warning("Ignoring malformed package configuration file at '%s'",
                    tmpkg_file_path);
    return tmpkg_file_path;

  case TM_PKG_PARSE_STATUS_ERR:
  case TM_PKG_PARSE_STATUS_PERM:
    cli_out_error(
        "Unable to read contents of package configuration file at '%s'",
        tmpkg_file_path);
    return tmpkg_file_path;

  default:
    break;
  }

  cli_out_progress("Using package configuration file at '%s'", tmpkg_file_path);

  override_if_dst_unset(&recipe->recepie.pkg_info.url, tmpkg_file_data.url);
  override_if_dst_unset(&recipe->recepie.pkg_info.application_name,
                        tmpkg_file_data.application_name);
  override_if_dst_unset(&recipe->recepie.pkg_info.executable_path,
                        tmpkg_file_data.executable_path);
  override_if_dst_unset(&recipe->recepie.pkg_info.working_directory,
                        tmpkg_file_data.working_directory);
  override_if_dst_unset(&recipe->recepie.pkg_info.icon_path,
                        tmpkg_file_data.icon_path);

  return tmpkg_file_path;
}

static bool create_pkg_dir(char *pkg_path) {
  fs_dirop_status_t pkgdir_status = os_fs_mkdir(pkg_path);

  switch (pkgdir_status) {
  case TM_FS_DIROP_STATUS_EXIST:
    return cli_in_bool(
        "This pacakge is already installed, proceed with clean install?");

  case TM_FS_DIROP_STATUS_OK:
    return true;

  default:
    cli_out_error("Unable to create directory in '%s'", pkg_path);
    return false;
  }
}

int cli_cmd_install(cli_info_t info) {
  rt_recipe_t recipe = {0};
  int         ret    = EXIT_FAILURE;

  // Variables to be cleaned up
  // Declartion is here to avoid issues with goto
  char *pkg_path        = NULL;
  char *tmpkg_file_path = NULL;

  cli_out_progress("Initializing host file system");

  if (!os_fs_tm_init()) {
    cli_out_progress("Failed to inizialize host file system");
    goto cleanup;
  }

  cli_out_progress("Initiating installation process");

  // Check if -R is set and do stuff
  if (info.from_repo) {
    recipe.is_remote = true;
  }

  // Check if -U is set and do stuff

  override_recipe(&recipe, info);

  const char *archive_path = info.input;
  os_fs_tm_dypkg(&pkg_path, recipe.pkg_name);

  cli_out_progress("Creating package in '%s'", pkg_path);

  if (!create_pkg_dir(pkg_path)) {
    goto cleanup;
  }

  cli_out_progress("Extracting archive '%s' to '%s'", archive_path, pkg_path);

  if (!archive_tar_extract(pkg_path, archive_path)) {
    cli_out_error("Unable to extract archive");
    goto cleanup;
  }

  tmpkg_file_path = load_config_file(&recipe, pkg_path);

  // Infer additional information

  // Create new .trpkg file in package directory

  cli_out_success("Package '%s' installed successfully", recipe.pkg_name);
  ret = EXIT_SUCCESS;

cleanup:
  mem_safe_free(pkg_path);
  mem_safe_free(tmpkg_file_path);
  return ret;
}
