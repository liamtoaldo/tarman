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

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "os/fs.h"

#define TMLINUX_ASSERT(p, d) \
  if (NULL == p) {           \
    *d = NULL;               \
    return 0;                \
  }

#define TMLINUX_CLEANUP(p, d) \
  if (NULL == p) {            \
    *d = NULL;                \
    goto cleanup;             \
  }

typedef struct {
  char  *buf;
  size_t len;
} str_t;

static str_t tmLinuxHome    = {0};
static str_t tmLinuxRepos   = {0};
static str_t tmLinuxPkgs    = {0};
static str_t tmLinuxExtract = {0};

static const char *get_home_directory() {
  struct passwd *pw = getpwuid(getuid());
  return pw->pw_dir;
}

static void safe_free(void *ptr) {
  if (ptr) {
    free(ptr);
  }
}

static fs_dirop_status_t simplify(fs_dirop_status_t s) {
  if (TM_FS_DIROP_STATUS_EXIST == s) {
    return TM_FS_DIROP_STATUS_OK;
  }

  return s;
}

fs_dirop_status_t os_fs_mkdir(const char *path) {
  struct stat st = {0};

  if (-1 != stat(path, &st)) {
    return TM_FS_DIROP_STATUS_EXIST;
  }

  if (0 == mkdir(path, 0700)) {
    return TM_FS_DIROP_STATUS_OK;
  }

  switch (errno) {
  case EACCES:
    return TM_FS_DIROP_STATUS_PERM;

  case EEXIST:
    return TM_FS_DIROP_STATUS_EXIST;

  default:
    return TM_FS_DIROP_STATUS_ERR;
  }
}

fs_dirop_status_t os_fs_dir_rm(const char *path) {
  os_fs_dirstream_t dir;
  fs_dirop_status_t status = os_fs_dir_open(&dir, path);

  if (TM_FS_DIROP_STATUS_OK != status) {
    return status;
  }

  fs_dirop_status_t enum_status;
  fs_dirent_t       ent;
  while (TM_FS_DIROP_STATUS_END != (enum_status = os_fs_dir_next(dir, &ent))) {
    if (TM_FS_DIROP_STATUS_OK != enum_status) {
      return enum_status;
    }

    char *full_path;
    os_fs_path_dyconcat(&full_path, 2, path, ent.name);

    if (NULL == full_path) {
      return TM_FS_DIROP_STATUS_ERR;
    }

    switch (ent.file_type) {
    case TM_FS_FILETYPE_DIR:
      fs_dirop_status_t s = os_fs_dir_rm(full_path);
      if (TM_FS_DIROP_STATUS_OK != s) {
        safe_free(full_path);
        return s;
      }
      break;

    case TM_FS_FILETYPE_REGULAR:
    case TM_FS_FILETYPE_EXEC:
    case TM_FS_FILETYPE_UNKNOWN:
      if (0 != unlink(full_path)) {
        safe_free(full_path);
        return TM_FS_DIROP_STATUS_ERR;
      }
      break;
    }

    safe_free(full_path);
  }

  if (TM_FS_DIROP_STATUS_OK != os_fs_dir_close(dir)) {
    return TM_FS_DIROP_STATUS_ERR;
  }

  if (0 != rmdir(path)) {
    return TM_FS_DIROP_STATUS_ERR;
  }

  return TM_FS_DIROP_STATUS_OK;
}

fs_dirop_status_t os_fs_dir_count(size_t *count, const char *path) {
  size_t m_count = 0;

  os_fs_dirstream_t dir;
  fs_dirop_status_t status = os_fs_dir_open(&dir, path);

  if (TM_FS_DIROP_STATUS_OK != status) {
    return status;
  }

  fs_dirop_status_t enum_status;
  fs_dirent_t       ent;
  for (; TM_FS_DIROP_STATUS_END != (enum_status = os_fs_dir_next(dir, &ent));
       m_count++) {
    if (TM_FS_DIROP_STATUS_OK != enum_status) {
      return enum_status;
    }
  }

  *count = m_count;
  return TM_FS_DIROP_STATUS_OK;
}

fs_dirop_status_t os_fs_dir_open(os_fs_dirstream_t *stream, const char *path) {
  DIR *dir = opendir(path);

  if (NULL == dir) {
    *stream = NULL;
    switch (errno) {
    case ENOENT:
    case ENOTDIR:
      return TM_FS_DIROP_STATUS_NOEXIST;

    case EACCES:
      return TM_FS_DIROP_STATUS_PERM;

    defaut:
      return TM_FS_DIROP_STATUS_ERR;
    }
  }

  *stream = dir;
  return TM_FS_DIROP_STATUS_OK;
}

fs_dirop_status_t os_fs_dir_close(os_fs_dirstream_t stream) {
  DIR *dir = (DIR *)stream;

  if (0 == closedir(dir)) {
    return TM_FS_DIROP_STATUS_OK;
  }

  return TM_FS_DIROP_STATUS_ERR;
}

fs_dirop_status_t os_fs_dir_next(os_fs_dirstream_t stream, fs_dirent_t *ent) {
  DIR           *dir  = (DIR *)stream;
  struct dirent *next = readdir(dir);

  while (NULL != next &&
         (0 == strcmp(next->d_name, ".") || 0 == strcmp(next->d_name, ".."))) {
    next = readdir(dir);
  }

  if (NULL == next) {
    return TM_FS_DIROP_STATUS_END;
  }

  int         dird      = dirfd(dir);
  struct stat st        = {0};
  bool        stat_done = false;

  if (DT_UNKNOWN == next->d_type) {
    if (0 > fstatat(dird, next->d_name, &st, 0)) {
      return TM_FS_DIROP_STATUS_ERR;
    }

    stat_done = true;

    if (S_ISREG(st.st_mode)) {
      next->d_type = DT_REG;
    } else if (S_ISDIR(st.st_mode)) {
      next->d_type = DT_DIR;
    }
  }

  fs_dirent_t m_ent = (fs_dirent_t){.name = next->d_name};

  switch (next->d_type) {
  case DT_REG:
    if (!stat_done && 0 > fstatat(dird, next->d_name, &st, 0)) {
      return TM_FS_DIROP_STATUS_ERR;
    }
    if (S_IXGRP & st.st_mode) {
      m_ent.file_type = TM_FS_FILETYPE_EXEC;
    } else {
      m_ent.file_type = TM_FS_FILETYPE_REGULAR;
    }
    break;

  case DT_DIR:
    m_ent.file_type = TM_FS_FILETYPE_DIR;
    break;

  default:
    m_ent.file_type = TM_FS_FILETYPE_UNKNOWN;
    break;
  }

  *ent = m_ent;
  return TM_FS_DIROP_STATUS_OK;
}

fs_fileop_status_t os_fs_file_mv(const char *dst, const char *src) {
}

fs_fileop_status_t os_fs_file_cp(const char *dst, const char *src) {
}

fs_fileop_status_t os_fs_file_rm(const char *path) {
  if (0 == unlink(path)) {
    return TM_FS_FILEOP_STATUS_OK;
  }

  switch (errno) {
  case EACCES:
    return TM_FS_FILEOP_STATUS_PERM;

  case ENOENT:
    return TM_FS_FILEOP_STATUS_NOEXIST;

  default:
    return TM_FS_FILEOP_STATUS_ERR;
  }
}

size_t os_fs_path_vlen(size_t num_args, va_list args) {
  size_t len = 0;

  for (size_t i = 0; i < num_args; i++) {
    const char *path = va_arg(args, char *);
    size_t      plen = strlen(path);
    len += plen;
    if ('/' != path[plen - 1]) {
      len++; // Count the / separator
    }
  }

  return len - 1; // Remove last / separator
}

size_t os_fs_path_len(size_t num_args, ...) {
  va_list args;
  va_start(args, num_args);
  size_t ret = os_fs_path_vlen(num_args, args);
  va_end(args);
  return ret;
}

size_t os_fs_path_vconcat(char *dst, size_t num_args, va_list args) {
  size_t j = 0; // Index into dst

  for (size_t i = 0; i < num_args; i++) {
    const char *path = va_arg(args, char *);
    // Copy path to dst
    for (; *path; path++, j++) {
      dst[j] = *path;
    }
    if ('/' != path[j]) {
      dst[j] = '/'; // Add separator
    }
    j++;
  }

  dst[--j] = 0; // Add string terminator
  return j;
}

size_t os_fs_path_concat(char *dst, size_t num_args, ...) {
  va_list args;
  va_start(args, num_args);
  size_t ret = os_fs_path_vconcat(dst, num_args, args);
  va_end(args);
  return ret;
}

size_t os_fs_path_dyconcat(char **dst, size_t num_args, ...) {
  va_list args;
  va_start(args, num_args);
  size_t len = os_fs_path_vlen(num_args, args);
  va_end(args);

  size_t buf_sz = len + 1;
  char  *buf    = (char *)malloc(buf_sz * sizeof(char));

  if (NULL == buf) {
    *dst = NULL;
    return 0;
  }

  va_start(args, num_args);
  os_fs_path_vconcat(buf, num_args, args);
  va_end(args);

  *dst = buf;
  return len;
}

size_t os_fs_path_dyparent(char **dst, const char *path) {
  size_t last_sep_idx = 0;

  for (size_t i = 0; path[i]; i++) {
    if ('/' == path[i]) {
      last_sep_idx = i;
    }
  }

  // If the last separator is also the last char
  if (!path[last_sep_idx + 1]) {
    // Avoid situations like: path/to/something//
    //                                          ^
    for (size_t back = last_sep_idx - 1; back > 0 && '/' == path[back];
         back--) {
      last_sep_idx = back;
    }

    // Move back to previous /
    for (size_t back = last_sep_idx - 1; back > 0 && '/' != path[back];
         back--) {
      last_sep_idx = back;
    }

    // Point to last / instead of char
    last_sep_idx--;
  }

  // Avoid issues like path//to//////something/////////
  for (size_t back = last_sep_idx - 1; back > 0 && '/' == path[back]; back--) {
    last_sep_idx = back;
  }

  size_t buf_len = last_sep_idx + 1;
  char  *buf     = (char *)malloc(buf_len * sizeof(char));

  if (NULL == buf) {
    *dst = NULL;
    return 0;
  }

  strncpy(buf, path, last_sep_idx);
  buf[last_sep_idx] = 0; // Add string terminator

  *dst = buf;
  return buf_len;
}

size_t os_fs_tm_dyhome(char **dst) {
  char *tm_home = (char *)malloc((tmLinuxHome.len + 1) * sizeof(char));
  TMLINUX_ASSERT(tm_home, dst);
  strcpy(tm_home, tmLinuxHome.buf);
  *dst = tm_home;
  return tmLinuxHome.len;
}

size_t os_fs_tm_dyrepos(char **dst) {
  char *tm_repos = (char *)malloc((tmLinuxRepos.len + 1) * sizeof(char));
  TMLINUX_ASSERT(tm_repos, dst);
  strcpy(tm_repos, tmLinuxRepos.buf);
  *dst = tm_repos;
  return tmLinuxRepos.len;
}

size_t os_fs_tm_dypkgs(char **dst) {
  char *tm_pkgs = (char *)malloc((tmLinuxPkgs.len + 1) * sizeof(char));
  TMLINUX_ASSERT(tm_pkgs, dst);
  strcpy(tm_pkgs, tmLinuxPkgs.buf);
  *dst = tm_pkgs;
  return tmLinuxPkgs.len;
}

size_t os_fs_fm_dyextract(char **dst) {
  char *tm_extract = (char *)malloc((tmLinuxExtract.len + 1) * sizeof(char));
  TMLINUX_ASSERT(tm_extract, dst);
  strcpy(tm_extract, tmLinuxExtract.buf);
  *dst = tm_extract;
  return tmLinuxExtract.len;
}

size_t os_fs_tm_dyrepo(char **dst, const char *repo_name) {
  char  *tm_repo;
  size_t ret = os_fs_path_dyconcat(&tm_repo, 2, tmLinuxRepos.buf, repo_name);
  TMLINUX_ASSERT(tm_repo, dst);
  *dst = tm_repo;
  return ret;
}

size_t os_fs_tm_dypkg(char **dst, const char *pkg_name) {
  char  *tm_pkg;
  size_t ret = os_fs_path_dyconcat(&tm_pkg, 2, tmLinuxPkgs.buf, pkg_name);
  TMLINUX_ASSERT(tm_pkg, dst);
  *dst = tm_pkg;
  return ret;
}

size_t os_fs_tm_dycached(char **dst, const char *item_name) {
  char  *tm_cached;
  size_t ret =
      os_fs_path_dyconcat(&tm_cached, 2, tmLinuxExtract.buf, item_name);
  TMLINUX_ASSERT(tm_cached, dst);
  *dst = tm_cached;
  return ret;
}

size_t
os_fs_tm_dyrecepie(char **dst, const char *repo_name, const char *pkg_name) {
  size_t ret = 0;

  char *rec_name = (char *)malloc((strlen(pkg_name) + 1) * sizeof(char));
  TMLINUX_ASSERT(rec_name, dst);
  sprintf(rec_name, "%s.tarman", pkg_name);

  char *tm_recepie;
  ret = os_fs_path_dyconcat(
      &tm_recepie, 3, tmLinuxRepos.buf, repo_name, rec_name);
  TMLINUX_CLEANUP(tm_recepie, dst);

cleanup:
  safe_free(rec_name);
  *dst = tm_recepie;
  return ret;
}

bool os_fs_tm_dyinit() {
  const char *usr_home = get_home_directory();

  tmLinuxHome.len =
      os_fs_path_dyconcat(&tmLinuxHome.buf, 2, usr_home, "tarman");
  tmLinuxRepos.len =
      os_fs_path_dyconcat(&tmLinuxRepos.buf, 3, usr_home, "tarman", "repos");
  tmLinuxPkgs.len =
      os_fs_path_dyconcat(&tmLinuxPkgs.buf, 3, usr_home, "tarman", "pkgs");
  tmLinuxExtract.len =
      os_fs_path_dyconcat(&tmLinuxExtract.buf, 3, usr_home, "tarman", "tmp");

  if (NULL == tmLinuxHome.buf || NULL == tmLinuxRepos.buf ||
      NULL == tmLinuxPkgs.buf || NULL == tmLinuxExtract.buf) {
    return false;
  }

  if (TM_FS_DIROP_STATUS_OK != simplify(os_fs_mkdir(tmLinuxHome.buf)) ||
      TM_FS_DIROP_STATUS_OK != simplify(os_fs_mkdir(tmLinuxRepos.buf)) ||
      TM_FS_DIROP_STATUS_OK != simplify(os_fs_mkdir(tmLinuxPkgs.buf)) ||
      TM_FS_DIROP_STATUS_OK != simplify(os_fs_mkdir(tmLinuxExtract.buf))) {
    return false;
  }

  return true;
}
