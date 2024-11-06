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
#include <string.h>

#include "archive.h"
#include "cli/directives/commands.h"
#include "cli/directives/types.h"
#include "cli/input.h"
#include "cli/output.h"
#include "config.h"
#include "download.h"
#include "os/fs.h"
#include "package.h"
#include "tm-mem.h"

static bool override_if_src_set(const char **dst, const char *src, bool copy) {
  if (NULL != src && 0 != src[0]) {
    if (copy) {
      char *buf = (char *)malloc(strlen(src) + 1);
      mem_chkoom(buf);
      strcpy(buf, src);
      *dst = buf;
      return true;
    }

    *dst = src;
    return true;
  }

  return false;
}

static void override_if_dst_unset(const char **dst, const char *src) {
  if (NULL == *dst || 0 == *dst[0]) {
    const char *back = *dst;

    if (override_if_src_set(dst, src, false)) {
      mem_safe_free(back);
    }
  }
}

static void override_and_free(const char **dst, const char *src) {
  mem_safe_free(*dst);
  override_if_src_set(dst, src, true);
}

static void override_recipe(rt_recipe_t *recipe, cli_info_t cli_info) {
  override_if_src_set(&recipe->recepie.package_format, cli_info.pkg_fmt, true);
  override_if_src_set(&recipe->pkg_name, cli_info.pkg_name, true);
  override_if_src_set(
      &recipe->recepie.pkg_info.application_name, cli_info.app_name, true);
  override_if_src_set(
      &recipe->recepie.pkg_info.executable_path, cli_info.exec_path, true);
  override_if_src_set(
      &recipe->recepie.pkg_info.working_directory, cli_info.working_dir, true);
  override_if_src_set(
      &recipe->recepie.pkg_info.icon_path, cli_info.icon_path, true);

  recipe->recepie.add_to_path    = cli_info.add_path;
  recipe->recepie.add_to_desktop = cli_info.add_desktop;
  recipe->recepie.add_to_tarman  = cli_info.add_tarman;
}

static char *load_config_file(rt_recipe_t *recipe, char *pkg_path) {
  if (recipe->is_remote) {
    return NULL;
  }

  pkg_info_t tmpkg_file_data = {0};
  char      *tmpkg_file_path = NULL;

  os_fs_path_dyconcat(&tmpkg_file_path, 2, pkg_path, "info.tmpkg");

  cfg_parse_status_t status =
      pkg_parse_tmpkg(&tmpkg_file_data, tmpkg_file_path);

  switch (status) {
  case TM_CFG_PARSE_STATUS_NOFILE:
    cli_out_warning("Package configuration file '%s' does not exist",
                    tmpkg_file_path);
    goto cleanup;

  case TM_CFG_PARSE_STATUS_INVVAL:
  case TM_CFG_PARSE_STATUS_MALFORMED:
    cli_out_warning("Ignoring malformed package configuration file at '%s'",
                    tmpkg_file_path);
    goto cleanup;

  case TM_CFG_PARSE_STATUS_ERR:
  case TM_CFG_PARSE_STATUS_PERM:
    cli_out_error(
        "Unable to read contents of package configuration file at '%s'",
        tmpkg_file_path);
    goto cleanup;

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

cleanup:
  mem_safe_free(tmpkg_file_path);
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

static void remove_pkg_cache(const char *archive_path) {
  cli_out_progress("Removing cache '%s'", archive_path);

  if (TM_FS_FILEOP_STATUS_OK != os_fs_file_rm(archive_path)) {
    cli_out_warning("Unable to delete cache");
  }
}

static bool load_recipe(rt_recipe_t *recipe) {
  bool     ret           = false;
  recipe_t rcp_file_data = {0};
  char    *rcp_file_path = NULL;

  os_fs_tm_dyrecipe(&rcp_file_path,
                    recipe->recepie.pkg_info.from_repoistory,
                    recipe->pkg_name);

  cfg_parse_status_t status = pkg_parse_tmrcp(&rcp_file_data, rcp_file_path);

  switch (status) {
  case TM_CFG_PARSE_STATUS_INVVAL:
  case TM_CFG_PARSE_STATUS_MALFORMED:
    cli_out_error(
        "Recipe file for package '%s' in repository '%s' is malformed",
        recipe->pkg_name,
        recipe->recepie.pkg_info.from_repoistory);
    goto cleanup;

  case TM_CFG_PARSE_STATUS_ERR:
  case TM_CFG_PARSE_STATUS_PERM:
    cli_out_error("Unable to read contents of recipe file '%s'", rcp_file_path);
    goto cleanup;

  default:
    break;
  }

  cli_out_progress("Using recipe file '%s'", rcp_file_path);

  override_if_dst_unset(&recipe->recepie.pkg_info.url,
                        rcp_file_data.pkg_info.url);
  override_if_dst_unset(&recipe->recepie.pkg_info.application_name,
                        rcp_file_data.pkg_info.application_name);
  override_if_dst_unset(&recipe->recepie.pkg_info.executable_path,
                        rcp_file_data.pkg_info.executable_path);
  override_if_dst_unset(&recipe->recepie.pkg_info.working_directory,
                        rcp_file_data.pkg_info.working_directory);
  override_if_dst_unset(&recipe->recepie.pkg_info.icon_path,
                        rcp_file_data.pkg_info.icon_path);
  override_if_dst_unset(&recipe->recepie.package_format,
                        rcp_file_data.package_format);

  ret = true;

cleanup:
  mem_safe_free(rcp_file_path);
  return ret;
}

static void find_repository(rt_recipe_t *recipe) {
  const char *repos_path = NULL;

  if (0 == os_fs_tm_dyrepos((char **)&repos_path)) {
    cli_out_error("Unable to determine path to repositories directory");
    goto cleanup;
  }

  os_fs_dirstream_t repos_stream;
  fs_dirop_status_t open_status = os_fs_dir_open(&repos_stream, repos_path);

  if (TM_FS_DIROP_STATUS_OK != open_status) {
    cli_out_error("Unable to open repositories directory");
    goto cleanup;
  }

  fs_dirent_t ent;

  size_t repos_buf_sz = 16;
  char **repos        = (char **)malloc(16 * sizeof(char *));
  size_t repos_i      = 0;
  mem_chkoom(repos);

  while (TM_FS_DIROP_STATUS_OK == os_fs_dir_next(repos_stream, &ent)) {
    if (TM_FS_FILETYPE_DIR != ent.file_type) {
      continue;
    }

    char *pkg_recipe = NULL;

    if (0 == os_fs_tm_dyrecipe(&pkg_recipe, ent.name, recipe->pkg_name)) {
      cli_out_error("Unable to determine recipe path for repository '%s'",
                    ent.name);
      os_fs_dir_close(repos_stream);
      goto cleanup;
    }

    fs_filetype_t rcp_file_type;

    if (TM_FS_FILEOP_STATUS_OK ==
        os_fs_file_gettype(&rcp_file_type, pkg_recipe)) {
      if (repos_buf_sz - 1 == repos_i) {
        repos_buf_sz *= 2;
        repos = (char **)realloc(repos, repos_buf_sz);
        mem_chkoom(repos);
      }

      override_if_src_set((const char **)&repos[repos_i], ent.name, true);
      repos_i++;
    }

    mem_safe_free(pkg_recipe);
  }

  os_fs_dir_close(repos_stream);

  if (0 == repos_i) {
    cli_out_error("Package '%s' not found in local repositories",
                  recipe->pkg_name);
    goto cleanup;
  }

  unsigned long user_choice = 0;

  if (1 == repos_i) {
    user_choice = 1;
    goto apply;
  }

  cli_out_newline();
  printf("Multiple repositories found for package '%s', choose between:",
         recipe->pkg_name);
  cli_out_newline();

  for (size_t i = 0; i < repos_i; i++) {
    cli_out_space(8);
    printf("%ld. %s", i + 1, repos[i]);
    cli_out_newline();
  }

  user_choice = cli_in_int("Enter repository identifier", 1, repos_i);

apply:
  recipe->recepie.pkg_info.from_repoistory = repos[user_choice - 1];
  if (!load_recipe(recipe)) {
    goto cleanup;
  }

cleanup:
  for (size_t i = 0; i < repos_i; i++) {
    if (i != user_choice - 1) {
      mem_safe_free(repos[i]);
    }
  }
  mem_safe_free(repos);
  mem_safe_free(repos_path);
}

int cli_cmd_install(cli_info_t info) {
  if (NULL == info.input) {
    cli_out_error("Must specify a package to install'");
    return EXIT_FAILURE;
  }

  rt_recipe_t recipe = {0};
  int         ret    = EXIT_FAILURE;

  // Variables to be cleaned up
  // Declartion is here to avoid issues with goto
  char       *pkg_path     = NULL;
  const char *archive_path = NULL;

  cli_out_progress("Initializing host file system");

  if (!os_fs_tm_init()) {
    cli_out_progress("Failed to inizialize host file system");
    goto cleanup;
  }

  cli_out_progress("Initiating installation process");

  override_recipe(&recipe, info);

  if (info.from_url && NULL == recipe.recepie.package_format) {
    cli_out_warning(
        "Package format not specified for remote download, using 'tar.gz'");
    override_if_src_set(&recipe.recepie.package_format, "tar.gz", true);
  }

  // Check if -r is set and use repositories
  if (info.from_repo) {
    recipe.is_remote = true;
    override_if_src_set(&recipe.pkg_name, info.input, true);
    find_repository(&recipe);

    if (NULL == recipe.recepie.pkg_info.url) {
      cli_out_error("Package URL not found in recipe");
      goto cleanup;
    }

    if (!archive_dycreate(
            &archive_path, recipe.pkg_name, recipe.recepie.package_format)) {
      cli_out_error("Unable to determine path to temporary archive");
      goto cleanup;
    }

    cli_out_progress("Downloading package from '%s' to '%s'",
                     recipe.recepie.pkg_info.url,
                     archive_path);

    if (!download(archive_path, recipe.recepie.pkg_info.url)) {
      cli_out_error("Unable to download package");
      goto cleanup;
    }
  }

  // Ask user to enter the package name
  while (NULL == recipe.pkg_name &&
         0 == cli_in_dystr("Enter package name", (char **)&recipe.pkg_name))
    ;

  if (info.from_url) {
    override_if_src_set(&recipe.recepie.pkg_info.url, info.input, true);

    if (!archive_dycreate(
            &archive_path, recipe.pkg_name, recipe.recepie.package_format)) {
      cli_out_error("Unable to determine path to temporary archive");
      goto cleanup;
    }

    cli_out_progress("Downloading package from '%s' to '%s'",
                     recipe.recepie.pkg_info.url,
                     archive_path);

    if (!download(archive_path, recipe.recepie.pkg_info.url)) {
      cli_out_error("Unable to download package");
      goto cleanup;
    }
  }

  if (NULL == archive_path) {
    // Contents of cli_info_t are not heap-allocated
    // However, archive_path and others may be overridden using heap-allocated
    // strings as such, they would have to be freed. If free is called on a
    // pointer outside the heap it leads to undefined behaviour. As such, all
    // non-heap things are copied
    override_if_src_set(&archive_path, info.input, true);
  }

  os_fs_tm_dypkg(&pkg_path, recipe.pkg_name);

  cli_out_progress("Creating package in '%s'", pkg_path);

  if (!create_pkg_dir(pkg_path)) {
    goto cleanup;
  }

  cli_out_progress("Extracting archive '%s' to '%s'", archive_path, pkg_path);

  if (!archive_extract(pkg_path, archive_path, NULL)) {
    cli_out_error("Unable to extract archive. You may be missing the plugin "
                  "for this archive type");
    goto cleanup;
  }

  load_config_file(&recipe, pkg_path);

  // Infer additional information

  // Create new .trpkg file in package directory

  if (info.from_url || info.from_repo && NULL != archive_path) {
    remove_pkg_cache(archive_path);
  }

  cli_out_success("Package '%s' installed successfully", recipe.pkg_name);
  ret = EXIT_SUCCESS;

cleanup:
  mem_safe_free(archive_path);
  mem_safe_free(pkg_path);
  mem_safe_free(recipe.pkg_name);
  mem_safe_free(recipe.recepie.package_format);
  mem_safe_free(recipe.recepie.pkg_info.url);
  mem_safe_free(recipe.recepie.pkg_info.from_repoistory);
  mem_safe_free(recipe.recepie.pkg_info.executable_path);
  mem_safe_free(recipe.recepie.pkg_info.application_name);
  mem_safe_free(recipe.recepie.pkg_info.working_directory);
  mem_safe_free(recipe.recepie.pkg_info.icon_path);
  return ret;
}
