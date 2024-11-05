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

#ifdef TARMAN_PLUGIN_SDK_VERSION
#warning "Plugin redefines SDK version. This is not supported!"
#undef TARMAN_PLUGIN_SDK_VERSION
#endif

// Format: <Major>.<Minor>.<Revision>L
// Example: 1.0.0 -> 010000L
#define TARMAN_PLUGIN_SDK_VERSION 010000L

typedef struct {
  const char *src;
  const char *dst;
  const char *cfg;
} __attribute__((aligned(16))) sdk_handover_t;
