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

#include <stdbool.h>

// Contents of .tmpkg files
typedef struct {
  const char *url;
  const char *from_repoistory;
  const char *application_name;
  const char *executable_path;
  const char *working_directory;
  const char *icon_path;
} pkg_info_t;

// Contents of .tmrcp files
// In these files, the "FROM_REPOSITORY" property is not present
// The "from_repository" struct field, however, may be set at runtime
typedef struct {
  pkg_info_t pkg_info;
  bool       add_to_path;
  bool       add_to_desktop;
} recipe_t;

// "Runtime" recepie
typedef struct {
  recipe_t    recepie;
  const char *pkg_name;
} rt_recipe_t;

typedef struct {
  pkg_info_t  pkg_info;
  const char *pkg_name;
} pkg_t;
