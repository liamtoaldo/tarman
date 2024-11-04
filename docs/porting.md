# Porting
Tarman is designed to be easily portable to any Operating System that supports basic userspace functionality.

## Requirements
| Type      | Requirement                 | | Details                                                                                |
| --------- | ----------------------------- | -------------------------------------------------------------------------------------- |
| Mandatory | `libc` port                   | Must support all `libc` features used in the source code                               |
| Mandatory | Disk file system support      | Must implement all functions in [include/os/fs.h](../include/os/fs.h)                  |
| Mandatory | Spawning processes            | Must support the `system` interface from `stdlib.h`                                    |
| Mandatory | Environment variables         | Must implement all functions in [include/os/env.h]                                     |
| Optional  | Querying the console/terminal | Shall implement `os_console_get_sz` in [include/os/console.h](../include/os/console.h) |
| Optional  | Changing console text color   | Shall implement all functions in [include/os/console.h](../include/os/console.h)       |
| Optional  | Network support               | Shall provide a plugin `download-plugin` that can download files from a URL            |

## Providing concrete implementations
To port tarman to another OS, as detailed in the previous section, implementations must be provided for a range of tarman interface functions. Generally, implementations should be provided for all functions under the [include/os](../include/os/) directory (**sudirectories excluded**).

All symbols that rely on OS-specific implementations start with `os_` (e.g., `os_fs_path_len`). All functions that take a `va_list` as argument have the last word in their name start with a `v` (e.g., `os_fs_path_vlen`). All functions that perform dynamic memory allocations and return the allocated buffer to the caller use the first parameter (of type pointer-pointer, e.g., `void **`) to return the buffer pointer and have the last word in their name start with `dy` (e.g., `os_fs_path_dyconcat`).

All files under `src/os-common/$TARMAN_OSCOMMON` shall be structured as follows:
```c
// License here
// ...

#include <tm-os-defs.h>

// Other includes here
// ...

// Code here
```

### Common implementations
Some platforms share specifications that describe how to interact with them. For example, both macOS (Darwin) and Linux implement the POSIX specification. In these cases, new symbols can be declared and defined in a directory `include/os/$TARMAN_OSCOMMON/` (where `$TARMAN_OSCOMMON` is the spec name, e.g. `posix`). Implementations for the symbols declared in `include/os$TARMAN_OSCOMMON/*.h` shall be provided in `src/os-common/$TARMAN_OSCOMMON/*.c`.

**Example:** tarman implements POSIX support with symbols declared in [include/os/posix/](../include/os/posix/) that can then be called by OS-specific implementations. Implmeentations for all symbols declared by tarman's POSX headers are found in [src/os-common/posix/](../include/os/posix/).

### Naming
OS ports must choose a name that reflect the name of the platform they're targetting. This name shall be all lowercase and will be referred to as `$TARMAN_OS` from here on in thisdocument.

### Header files
OS ports must provide a header file named `tm-os-defs.h` under `include/os/$TARMAN_OS` if they wish to use common implementations such as POSIX. This header file is used to define macros/symbols that may be used by common implementations. For example, the `tm-os-defs.h` file for macOS is:
```c
#pragma once

#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif
```
### Source files
Implementations for OS-specific symbols shall be provided under `src/OS-specific/$TARMAN_OS` as C source files. If the implementation makes use of common code, these files shall include the necessary headers (e.g., implementations that rely on POSIX for FS operations shall include `os/posix/fs.h` alongside `os/fs.h`).

### Build files
A Makefile must be provided in `src/os-specific/$TARMAN_OS/Makefile`. This file is used to include additional sources such as OS-common implementations. Following is an example taken from [src/os-specific/linux/Makefile](../src/os-specific/linux/Makefile):
```Makefile
SRC+=$(call rwildcard, src/os-common/posix, *.c) # Adds POSIX sources
```

The `SRC` variable is provided by the root Makefile, just like the `rwildcard` (recursive wildcard) function.
