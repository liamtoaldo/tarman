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

#include <ctype.h>
#include <stdarg.h>
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

  os_fs_path_dyconcat(&tmpkg_file_path, 2, pkg_path, "package.tarman");

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

static bool gen_repos_list(char     ***repos_list,
                           size_t     *repos_count,
                           const char *pkg_name,
                           const char *repos_path) {
  os_fs_dirstream_t repos_stream;
  fs_dirop_status_t open_status = os_fs_dir_open(&repos_stream, repos_path);

  if (TM_FS_DIROP_STATUS_OK != open_status) {
    cli_out_error("Unable to open repositories directory");
    return false;
  }

  bool        ret = false;
  fs_dirent_t ent;
  size_t      repos_buf_sz = 16;
  char      **repos        = (char **)malloc(repos_buf_sz * sizeof(char *));
  size_t      i            = 0;
  mem_chkoom(repos);

  while (TM_FS_DIROP_STATUS_OK == os_fs_dir_next(repos_stream, &ent)) {
    if (TM_FS_FILETYPE_DIR != ent.file_type) {
      continue;
    }

    char *pkg_recipe = NULL;

    if (0 == os_fs_tm_dyrecipe(&pkg_recipe, ent.name, pkg_name)) {
      cli_out_error("Unable to determine recipe path for repository '%s'",
                    ent.name);
      goto cleanup;
    }

    fs_filetype_t rcp_file_type;

    if (TM_FS_FILEOP_STATUS_OK ==
        os_fs_file_gettype(&rcp_file_type, pkg_recipe)) {
      if (repos_buf_sz - 1 == i) {
        repos_buf_sz *= 2;
        repos = (char **)realloc(repos, repos_buf_sz);
        mem_chkoom(repos);
      }

      override_if_src_set((const char **)&repos[i], ent.name, true);
      i++;
    }

    mem_safe_free(pkg_recipe);
  }

  *repos_count = i;
  *repos_list  = repos;
  ret          = true;

cleanup:
  os_fs_dir_close(repos_stream);
  return ret;
}

static size_t user_choose(char      **options,
                          size_t      options_count,
                          bool        allow_custom,
                          const char *msg_fmt,
                          ...) {
  if (1 == options_count && !allow_custom) {
    return 1;
  }

  cli_out_newline();
  // printf("Multiple repositories found for package '%s', choose between:",
  //        recipe->pkg_name);
  va_list args;
  va_start(args, msg_fmt);
  vprintf(msg_fmt, args);
  va_end(args);
  putchar(':');
  cli_out_newline();

  size_t range_min = 1;

  if (allow_custom) {
    cli_out_space(8);
    printf("0. [Custom]");
    cli_out_newline();
    range_min = 0;
  }

  for (size_t i = 0; i < options_count; i++) {
    cli_out_space(8);
    printf("%ld. %s", i + 1, options[i]);
    cli_out_newline();
  }

  return cli_in_int(
      "Enter the desired option number", range_min, options_count);
}

static bool find_repository(rt_recipe_t *recipe) {
  const char *repos_path = NULL;

  if (0 == os_fs_tm_dyrepos((char **)&repos_path)) {
    cli_out_error("Unable to determine path to repositories directory");
    return false;
  }

  bool   ret         = false;
  char **repos       = NULL;
  size_t repos_count = 0;

  if (!gen_repos_list(&repos, &repos_count, recipe->pkg_name, repos_path)) {
    goto cleanup;
  }

  if (0 == repos_count) {
    cli_out_error("Package '%s' not found in local repositories",
                  recipe->pkg_name);
    goto cleanup;
  }

  unsigned long user_choice = user_choose(
      repos,
      repos_count,
      false,
      "Multiple repositories found for package '%s', choose between",
      recipe->pkg_name);

  recipe->recepie.pkg_info.from_repoistory = repos[user_choice - 1];

  if (load_recipe(recipe)) {
    ret = true;
  }

cleanup:
  for (size_t i = 0; i < repos_count; i++) {
    if (i != user_choice - 1) {
      mem_safe_free(repos[i]);
    }
  }

  mem_safe_free(repos);
  mem_safe_free(repos_path);

  return ret;
}

static bool fetch_package(const char **archive_path,
                          const char  *pkg_name,
                          const char  *pkg_fmt,
                          const char  *url) {
  if (!archive_dycreate(archive_path, pkg_name, pkg_fmt)) {
    cli_out_error("Unable to determine path to temporary archive");
    return false;
  }

  cli_out_progress("Downloading package from '%s' to '%s'", url, *archive_path);

  if (!download(*archive_path, url)) {
    cli_out_error("Unable to download package");
    return false;
  }

  return true;
}

static bool infer_app_name(rt_recipe_t *recipe, const char *pkg_path) {
  cli_out_progress("Inferring application name");

  os_fs_dirstream_t stream;
  fs_dirop_status_t open_status = os_fs_dir_open(&stream, pkg_path);

  if (TM_FS_DIROP_STATUS_OK != open_status) {
    cli_out_error("Unable to visit package directory '%s'", pkg_path);
    return false;
  }

  fs_dirent_t ent;
  size_t      names_buf_sz = 16;
  char      **names        = (char **)malloc(names_buf_sz * sizeof(char *));
  size_t      count        = 1; // Count is one because there's a default value
  mem_chkoom(names);

  override_if_src_set((const char **)&names[0], recipe->pkg_name, true);
  names[0][0] = toupper(names[0][0]);

  while (TM_FS_DIROP_STATUS_OK == os_fs_dir_next(stream, &ent)) {
    if (TM_FS_FILETYPE_DIR == ent.file_type) {
      if (0 == strcmp(ent.name, names[0])) {
        continue;
      }

      if (names_buf_sz - 1 == count) {
        names_buf_sz *= 2;
        names = (char **)realloc(names, names_buf_sz);
        mem_chkoom(names);
      }

      override_if_src_set((const char **)&names[count], ent.name, true);
      count++;
    }
  }

  os_fs_dir_close(stream);

  unsigned long user_choice =
      user_choose(names, count, true, "Choose an application name");

  if (0 == user_choice) {
    cli_in_dystr("Enter working directory",
                 (char **)&recipe->recepie.pkg_info.application_name);
  } else {
    recipe->recepie.pkg_info.application_name = names[user_choice - 1];
  }

  for (size_t i = 0; i < count; i++) {
    if (i != user_choice - 1) {
      mem_safe_free(names[i]);
    }
  }

  mem_safe_free(names);
  return true;
}

static bool infer_exec(rt_recipe_t *recipe, const char *pkg_path) {
  return true;
}

static bool infer_working_dir(rt_recipe_t *recipe, const char *pkg_path) {
  cli_out_progress("Inferring working directory");

  if (NULL == recipe->recepie.pkg_info.executable_path) {
    return true;
  }

  char *parent = NULL;

  if (0 ==
      os_fs_path_dyparent(&parent, recipe->recepie.pkg_info.executable_path)) {
    return false;
  }

  recipe->recepie.pkg_info.working_directory = parent;
  return true;
}

static bool infer_additional_info(rt_recipe_t *recipe,
                                  cli_info_t   cli_info,
                                  const char  *pkg_path) {
  if (recipe->is_remote) {
    return true;
  }

  if (!recipe->recepie.add_to_path && !cli_info.add_path) {
    recipe->recepie.add_to_path =
        cli_in_bool("Do you want to add this package to PATH?");
  }

  if (!recipe->recepie.add_to_desktop && !cli_info.add_desktop) {
    recipe->recepie.add_to_desktop =
        cli_in_bool("Do you want to add this package as an app?");
  }

  if ((recipe->recepie.add_to_path || recipe->recepie.add_to_tarman ||
       recipe->recepie.add_to_desktop) &&
      NULL == recipe->recepie.pkg_info.executable_path &&
      !infer_exec(recipe, pkg_path)) {
    return false;
  }

  if (recipe->recepie.add_to_desktop &&
      NULL == recipe->recepie.pkg_info.application_name &&
      !infer_app_name(recipe, pkg_path)) {
    return false;
  }

  if (recipe->recepie.add_to_desktop &&
      NULL == recipe->recepie.pkg_info.working_directory &&
      !infer_working_dir(recipe, pkg_path)) {
    return false;
  }

  return true;
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

    if (!find_repository(&recipe)) {
      goto cleanup;
    }

    if (NULL == recipe.recepie.pkg_info.url) {
      cli_out_error("Package URL not found in recipe");
      goto cleanup;
    }

    if (!fetch_package(&archive_path,
                       recipe.pkg_name,
                       recipe.recepie.package_format,
                       recipe.recepie.pkg_info.url)) {
      goto cleanup;
    }
  }

  // Ask user to enter the package name
  while (NULL == recipe.pkg_name &&
         0 == cli_in_dystr("Enter package name", (char **)&recipe.pkg_name))
    ;

  // If -u is used to download from a URL
  if (info.from_url) {
    override_if_src_set(&recipe.recepie.pkg_info.url, info.input, true);

    if (!fetch_package(&archive_path,
                       recipe.pkg_name,
                       recipe.recepie.package_format,
                       recipe.recepie.pkg_info.url)) {
      goto cleanup;
    }
  }

  if (NULL == archive_path) {
    override_if_src_set(&archive_path, info.input, true);
  }

  if (0 == os_fs_tm_dypkg(&pkg_path, recipe.pkg_name)) {
    cli_out_error("Unable to determine path to directory for package '%s'",
                  recipe.pkg_name);
    goto cleanup;
  }

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

  if (!infer_additional_info(&recipe, info, pkg_path)) {
    goto cleanup;
  }

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
