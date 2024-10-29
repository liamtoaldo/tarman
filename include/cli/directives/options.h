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

#pragma once

#include "cli/directives/types.h"

#define TARMAN_SOPT_FROM_URL    "-U"
#define TARMAN_SOPT_FROM_REPO   "-R"
#define TARMAN_SOPT_PKG_NAME    "-N"
#define TARMAN_SOPT_APP_NAME    "-A"
#define TARMAN_SOPT_EXEC        "-X"
#define TARMAN_SOPT_WRK_DIR     "-W"
#define TARMAN_SOPT_ICON        "-I"
#define TARMAN_SOPT_ADD_PATH    "-P"
#define TARMAN_SOPT_ADD_DESKTOP "-D"

#define TARMAN_FOPT_FROM_URL    "--from-url"
#define TARMAN_FOPT_FROM_REPO   "--from-repo"
#define TARMAN_FOPT_PKG_NAME    "--pkg-name"
#define TARMAN_FOPT_APP_NAME    "--app-name"
#define TARMAN_FOPT_EXEC        "--exec"
#define TARMAN_FOPT_WRK_DIR     "--working-dir"
#define TARMAN_FOPT_ICON        "--icon"
#define TARMAN_FOPT_ADD_PATH    "--add-path"
#define TARMAN_FOPT_ADD_DESKTOP "--add-desktop"

bool cli_opt_from_url(cli_info_t *info, const char *next);
bool cli_opt_from_repo(cli_info_t *info, const char *next);
bool cli_opt_pkg_name(cli_info_t *info, const char *next);
bool cli_opt_app_name(cli_info_t *info, const char *next);
bool cli_opt_exec(cli_info_t *info, const char *next);
bool cli_opt_wrk_dir(cli_info_t *info, const char *next);
bool cli_opt_icon(cli_info_t *info, const char *next);
bool cli_opt_add_path(cli_info_t *info, const char *next);
bool cli_opt_add_desktop(cli_info_t *info, const char *next);
