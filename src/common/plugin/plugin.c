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

#include "os/fs.h"
#include "plugin/plugin.h"
#include "tm-mem.h"

#define LQUOTE 1
#define RQUOTE 1
#define SPACE  1

bool plugin_exists(const char *plugin) {
  const char *plugin_path = NULL;
  os_fs_tm_dyplugin(&plugin_path, plugin);

  fs_filetype_t      ftype;
  fs_fileop_status_t op_status = os_fs_file_gettype(&ftype, plugin_path);

  mem_safe_free(plugin_path);
  return TM_FS_FILETYPE_EXEC == ftype && TM_FS_FILEOP_STATUS_OK == op_status;
}

int plugin_run(const char *plugin, const char *dst, const char *src) {
  const char *plugin_path       = NULL;
  const char *plugconf_path     = NULL;
  size_t      plugin_path_len   = os_fs_tm_dyplugin(&plugin_path, plugin);
  size_t      plugconf_path_len = os_fs_tm_dyplugconf(&plugconf_path, plugin);

  size_t cmd_len = plugin_path_len + SPACE + LQUOTE + strlen(src) + RQUOTE +
                   SPACE + LQUOTE + strlen(dst) + RQUOTE + SPACE + LQUOTE +
                   plugconf_path_len + RQUOTE + 1;

  char *command = (char *)malloc(cmd_len * sizeof(char));
  mem_chkoom(command);

  sprintf(
      command, "%s \"%s\" \"%s\" \"%s\"", plugin_path, src, dst, plugconf_path);

  int ret = system(command);

  mem_safe_free(plugin_path);
  mem_safe_free(plugconf_path);
  mem_safe_free(command);
  return ret;
}
