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

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "package.h"
#include "stream.h"
#include "tm-mem.h"

// These types are defined it make the code
// easier to read. generic_<x>_t is a replacement
// for pkg/rcp-specific types
typedef int  generic_status_t;
typedef void generic_info_t;
typedef bool (*translator_t)(const char       *key,
                             const char       *value,
                             generic_info_t   *info,
                             generic_status_t *status);

typedef enum { PROP_MATCH_FALSE, PROP_MATCH_OK, PROP_MATCH_ERR } prop_match_t;

static prop_match_t set_property(const char  *prop,
                                 const char  *key,
                                 const char  *value,
                                 const char **target,
                                 size_t       num_args,
                                 ...) {
  if (0 != strcmp(prop, key)) {
    return PROP_MATCH_FALSE;
  }

  if (0 == num_args) {
    goto success;
  }

  va_list args;
  va_start(args, num_args);

  // Check the match range
  // This is if the propertly can only have
  // a select number of values (such as true or false)
  for (size_t i = 0; i < num_args; i++) {
    const char *range_elem = va_arg(args, char *);

    if (0 == strcmp(value, range_elem)) {
      va_end(args);
      goto success;
    }
  }

fail:
  va_end(args);
  return PROP_MATCH_ERR;

success:
  char *value_cpy = (char *)malloc((strlen(value) + 1) * sizeof(char));
  mem_chkoom(value_cpy);
  strcpy(value_cpy, value);
  *target = value_cpy;

  return PROP_MATCH_OK;
}

static prop_match_t eval_matches(size_t num_args, ...) {
  va_list args;
  va_start(args, num_args);

  prop_match_t ret = PROP_MATCH_FALSE;

  for (size_t i = 0; i < num_args; i++) {
    prop_match_t match = va_arg(args, prop_match_t);

    if (PROP_MATCH_ERR == match) {
      va_end(args);
      return PROP_MATCH_ERR;
    }

    if (PROP_MATCH_OK == match) {
      ret = PROP_MATCH_OK;
    }
  }

  va_end(args);
  return ret;
}

static bool pkg_translator(const char         *key,
                           const char         *value,
                           pkg_info_t         *pkg_info,
                           pkg_parse_status_t *status) {
  prop_match_t match = eval_matches(
      6,
      set_property("URL", key, value, &pkg_info->url, 0),
      set_property(
          "FROM_REPOSITORY", key, value, &pkg_info->from_repoistory, 0),
      set_property(
          "APPLICATION_NAME", key, value, &pkg_info->application_name, 0),
      set_property(
          "EXECUTABLE_PATH", key, value, &pkg_info->executable_path, 0),
      set_property(
          "WORKING_DIRECTORY", key, value, &pkg_info->working_directory, 0),
      set_property("ICON_PATH", key, value, &pkg_info->icon_path, 0));

  if (PROP_MATCH_ERR == match) {
    *status = TM_PKG_PARSE_STATUS_INVVAL;
    return false;
  }

  *status = TM_PKG_PARSE_STATUS_OK;
  return true;
}

static bool rcp_translator(const char         *key,
                           const char         *value,
                           recipe_t           *rcp,
                           rcp_parse_status_t *status) {
  pkg_parse_status_t pkg_status;

  if (!pkg_translator(key, value, &rcp->pkg_info, &pkg_status)) {
    *status = TM_RCP_PARSE_STATUS_INVVAL;
    return false;
  }

  const char *add_to_path    = NULL;
  const char *add_to_desktop = NULL;

  prop_match_t match = eval_matches(
      2,
      set_property("ADD_TO_PATH", key, value, &add_to_path, 2, "true", "false"),
      set_property(
          "ADD_TO_DESKTOP", key, value, &add_to_desktop, 2, "true", "false"));

  if (PROP_MATCH_ERR == match) {
    *status = TM_RCP_PARSE_STATUS_INVVAL;
    return false;
  }

  if (NULL != add_to_path && 0 == strcmp(add_to_path, "true")) {
    mem_safe_free(add_to_path);
    rcp->add_to_path = true;
  }

  if (NULL != add_to_desktop && 0 == strcmp(add_to_desktop, "true")) {
    mem_safe_free(add_to_desktop);
    rcp->add_to_desktop = true;
  }

  *status = TM_RCP_PARSE_STATUS_OK;
  return true;
}

static bool tokenize(char *line, const char **key, const char **value) {
  bool sep_found    = false;
  char first_val_ch = 0;

  for (size_t i = 0; line[i]; i++) {
    char ch = line[i];

    if (' ' == ch) {
      return false;
    }

    if ('=' == ch) {
      if (sep_found) {
        return false;
      }

      line[i]      = 0;
      *key         = line;
      *value       = &line[i + 1];
      first_val_ch = line[i + 1];
      sep_found    = true;
    }
  }

  return sep_found;
}

static bool generic_parse(FILE             *stream,
                          translator_t      translator,
                          generic_info_t   *info,
                          generic_status_t *status) {
  char *line_buffer;

  while (0 != stream_dyreadline(stream, &line_buffer)) {
    const char *key   = NULL;
    const char *value = NULL;

    if (!tokenize(line_buffer, &key, &value) ||
        !translator(key, value, info, status)) {
      mem_safe_free(line_buffer);
      return false;
    }

    mem_safe_free(line_buffer);
  }

  return true;
}

pkg_parse_status_t pkg_parse_ftmpkg(pkg_info_t *pkg_info, FILE *pkg_file) {
  if (NULL == pkg_file) {
    return TM_PKG_PARSE_STATUS_NOFILE;
  }

  pkg_parse_status_t status = TM_PKG_PARSE_STATUS_OK;
  generic_parse(pkg_file,
                (translator_t)pkg_translator,
                pkg_info,
                (generic_status_t *)&status);

  return status;
}

pkg_parse_status_t pkg_parse_tmpkg(pkg_info_t *pkg_info,
                                   const char *pkg_file_path) {
  return pkg_parse_ftmpkg(pkg_info, fopen(pkg_file_path, "r"));
}

rcp_parse_status_t pkg_parse_ftmrcp(recipe_t *rcp, FILE *rcp_file) {
  if (NULL == rcp_file) {
    return TM_RCP_PARSE_STATUS_NOFILE;
  }

  rcp_parse_status_t status = TM_RCP_PARSE_STATUS_OK;
  generic_parse(
      rcp_file, (translator_t)rcp_translator, rcp, (generic_status_t *)&status);

  return status;
}

rcp_parse_status_t pkg_parse_tmrcp(recipe_t *rcp, const char *rcp_file_path) {
  return pkg_parse_ftmrcp(rcp, fopen(rcp_file_path, "r"));
}
