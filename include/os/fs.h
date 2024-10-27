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

#include <stdarg.h>
#include <stdlib.h>

typedef enum {
  TM_OS_DIROP_STATUS_NOEXIST = 0,
  TM_OS_DIROP_STATUS_PERM    = 1,
  TM_OS_DIROP_STATUS_END     = 2,
  TM_OS_DIROP_STATUS_ERR     = 3,
  TM_OS_DIROP_STATUS_OK      = 4
} os_fs_dirop_status_t;

typedef enum {
  TM_OS_FILEOP_STATUS_NOEXIST = 0,
  TM_OS_FILEOP_STATUS_PERM    = 1,
  TM_OS_FILEOP_STATUS_ERR     = 2,
  TM_OS_FILEOP_STATUS_OK      = 3
} os_fs_fileop_status_t;

typedef enum {
  TM_OS_FS_FILETYPE_DIR,
  TM_OS_FS_FILETYPE_REGULAR,
  TM_OS_FS_FILETYPE_EXEC
} os_fs_filetype_t;

typedef void *os_fs_dirstream_t;

typedef struct {
  os_fs_filetype_t file_type;
  const char      *name;
} os_fs_dirent_t;

os_fs_dirop_status_t os_fs_mkdir(const char *path);
os_fs_dirop_status_t os_fs_dir_mv(const char *dst, const char *src);
os_fs_dirop_status_t os_fs_dir_cp(const char *dst, const char *src);
os_fs_dirop_status_t os_fs_dir_rm(const char *path);
os_fs_dirop_status_t os_fs_dir_count(size_t *count, const char *path);
os_fs_dirop_status_t os_fs_dir_open(os_fs_dirstream_t *stream);
os_fs_dirop_status_t os_fs_dir_close(os_fs_dirstream_t stream);
os_fs_dirop_status_t os_fs_dir_next(os_fs_dirstream_t stream,
                                    os_fs_dirent_t   *ent);

os_fs_fileop_status_t os_fs_file_mv(const char *dst, const char *src);
os_fs_fileop_status_t os_fs_file_cp(const char *dst, const char *src);
os_fs_fileop_status_t os_fs_file_rm(const char *path);

size_t os_fs_path_vlen(size_t num_args, va_list args);
size_t os_fs_path_len(size_t num_args, ...);
size_t os_fs_path_concat(const char *dst, size_t num_args, ...);
size_t os_fs_path_dyconcat(const char **dst, size_t num_args, ...);
size_t os_fs_path_dyparent(const char **dst, const char *path);

size_t os_fs_tm_dyhome(const char **dst);
size_t os_fs_tm_dyrepos(const char **dst);
size_t os_fs_fm_dyrepo(const char **dst, const char *repo_name);
size_t os_fs_fm_dyrepopkg(const char **dst,
                          const char  *repo_name,
                          const char  *pkg_name);
size_t os_fs_tm_dypkgs(const char **dst);
size_t os_fs_tm_dypkg(const char **dst, const char *pkg_name);
size_t os_fs_tm_dyrepocache(const char **dst);
size_t os_fs_fm_dyextract(const char **dst);
