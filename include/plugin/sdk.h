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

#ifdef TARMAN_PLUGIN_SDK_VERSION
#warning "Plugin redefines SDK version. This is not supported!"
#undef TARMAN_PLUGIN_SDK_VERSION
#endif

// Format: <Major>.<Minor>.<Revision>L
// Example: 1.0.1 -> 010001L
#define TARMAN_PLUGIN_SDK_VERSION 0100001L

// Structure passed to the `plugin_main` function
// This structure uses 16-byte alignment to avoid future
// strict-aliasing issues
typedef struct {
  const char *src;
  const char *dst;
  const char *cfg;
} __attribute__((aligned(16))) sdk_handover_t;

// Run a program on the user's computer
// Invokes tarman `os_exec` indirectly
// Returns the exit code of the program
int sdk_vexec(const char *executable, va_list args);

// (variadic) Run a program on the user's computer
// Wrapper for `sdk_vexec`
// Returns the exit code of the program
int sdk_exec(const char *executable, ...);

// Plugin entry point
// Returns the exit code of the plugin
int plugin_main(sdk_handover_t *handover);
