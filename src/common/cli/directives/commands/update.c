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

#include "archive.h"
#include "cli/directives/commands.h"
#include "cli/input.h"
#include "cli/output.h"
#include "config.h"
#include "download.h"
#include "os/fs.h"
#include "package.h"
#include "tm-mem.h"
#include "util/misc.h"

// TODO: this is taken from install.c
// This should be more generic and the repetition should
// be removed. Ideally, the same would apply to the entire code
// used to download and install packages since it will be
// useful in other parts (e.g., ujpdate all, sync etc)
static bool fetch_package(char      **archive_path,
                          const char *pkg_name,
                          const char *pkg_fmt,
                          const char *url) {
  util_misc_dytmpfile(archive_path, pkg_name, pkg_fmt);
  cli_out_progress("Downloading package from '%s' to '%s'", url, *archive_path);

  if (!download(*archive_path, url)) {
    cli_out_error("Unable to download package");
    return false;
  }

  return true;
}

int cli_cmd_update(cli_info_t info) {
  int         ret              = EXIT_FAILURE;
  const char *pkg_name         = info.input;
  char       *pkg_path         = NULL;
  char       *tmp_archive_path = NULL;
  const char *artifact_path    = NULL;
  recipe_t    recipe_artifact  = {0};

  if (NULL == pkg_name) {
    cli_out_error("You must specify a package name for it to be removed. Use "
                  "'tarman update <pkg name>'");
    return ret;
  }

  // TODO: Remove this once it's done
  if (!cli_in_bool("WARNINGS: This is a testing command, proceed?")) {
    return EXIT_SUCCESS;
  }

  cli_out_progress("Initializing host file system");

  if (!os_fs_tm_init()) {
    cli_out_progress("Failed to inizialize host file system");
    return ret;
  }

  os_fs_tm_dypkg(&pkg_path, pkg_name);

  os_fs_path_dyconcat((char **)&artifact_path, 2, pkg_path, "recipe.tarman");
  cli_out_progress("Using metadata (recipe artifact) file '%s'", artifact_path);

  if (TM_CFG_PARSE_STATUS_OK !=
      pkg_parse_tmrcp(&recipe_artifact, artifact_path)) {
    cli_out_error("Cannot update package '%s'. Missing or corrupt metadata "
                  "(recipe artifact) file",
                  pkg_name);
    goto cleanup;
  }

  // TODO: Check repository
  // TODO: Basically everything from here should use common functions
  //        by "common functions' I mean functions shared by install, updated
  //        and isimlar commands"

  if (NULL == recipe_artifact.pkg_info.url ||
      NULL == recipe_artifact.package_format) {
    cli_out_error(
        "Cannot update package '%s'. Some metadata properties are missing",
        pkg_name);
    goto cleanup;
  }

  if (!fetch_package(&tmp_archive_path,
                     pkg_name,
                     recipe_artifact.package_format,
                     recipe_artifact.pkg_info.url)) {
    goto cleanup;
  }

  cli_out_progress("Removing old package directory '%s'", pkg_path);

  if (TM_FS_DIROP_STATUS_OK != os_fs_dir_rm(pkg_path)) {
    cli_out_error("Unable to remove package directory '%s'", pkg_path);
    goto cleanup;
  }

  cli_out_progress("Creating new package directory '%s'", pkg_path);

  if (TM_FS_DIROP_STATUS_OK != os_fs_mkdir(pkg_path)) {
    cli_out_error("Unable to create directory, package has been removed but "
                  "not reinstalled, sorry");
    goto cleanup;
  }

  cli_out_progress(
      "Extracting archive '%s' to '%s'", tmp_archive_path, pkg_path);

  if (!archive_extract(
          pkg_path, tmp_archive_path, recipe_artifact.package_format)) {
    cli_out_error("Unable to extract archive, package has been removed but not "
                  "reinstalled, sorry");
    goto cleanup;
  }

  cli_out_progress("Removing cache '%s'", tmp_archive_path);

  if (TM_FS_FILEOP_STATUS_OK != os_fs_file_rm(tmp_archive_path)) {
    cli_out_warning("Unable to delete cache");
  }

  cli_out_success("Package '%s' updated successfully", tmp_archive_path);
  ret = EXIT_SUCCESS;

cleanup:
  // TODO: Make a free function for packages and recipes
  mem_safe_free(pkg_path);
  mem_safe_free(tmp_archive_path);
  mem_safe_free(artifact_path);
  pkg_free_rcp(recipe_artifact);
  return ret;
}
