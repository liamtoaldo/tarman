# Contributing
This is an Open Source project, everybody is welcome to contribute with their knowledge and feedback. Before doing so, please read and make use of **ALL** documnetation on the matter (i.e., this file, the Code of Conduct, the Issue Template, the README, the LICENSE, and the Pull Request Template).

## Some ideas
This project is in very early development and I'm going to have very little time in the coming months, so here's a list of things that are missing or broken and that should be solved:
- Fix typos (especially `receipe`) wherever possible
- Refactor and cleanup (especially [src/common/cli/directives/commands/install.c](src/common/cli/directives/commands/install.c)
- Prefix `tarman/` to all `#include` statements in quotes, and subsequently rename `tm-mem.h` to `tarman/memory.h`
- Implement proper unit testing (see [src/common/cli/directives/test.c](src/common/cli/directives/test.c))
- Port to Microsoft Windows (create `Win32` common implementation in `src/os-common/win32` and Windows implementation in `src/os-specific/windows`)
- Change Plugin API's `cfg` field from pointing directly to a file to pointing to the configuration directory
- Add useful features to Plugin SDK (e.g., config loading and defaults using [src/common/config.c](src/common/config.c))
- Fix the probably million of bugs, crashes and undefined behaviour (e.g., `posix_fs_file_gettype` seems to never return `TM_FS_FILETYPE_REGULAR` wtf?)
- Add all commented commands in [src/common/cli/directives/lookup.c](src/common/cli/directives/lookup.c) (e.g., `update` should just download the package and reinstall it since we have no way to check the version, `sync`/`update-all` should do the same for repos and for packages (in this order), `remove-repo` should just remove the repo's directory, and so on)
- Remove code repetition (especially with dynamically allocated arrays that I basically reimplemented everywhere because it was faster)
- Give OS-specific and OS-common implementations the ability to detect if something wrong is happening (e.g., segmentation faults, termination, etc.) and gracefully terminate the execution. Again, remember that this MUST BE PORTABLE, so if it makes use of any OS-specific features, it should be encapsulated in an interface under [include/os](include/os)
- Add meta-recipes (recipes that allow people to download multiple packages at once)
- Update `.clang-format` to avoid atrocities such as `(0 == <\n>` in conditions   
- Make includes more efficient (remove duplicates, make it clearer when something is included implicitly, etc.)

## Repository management
It's recommended not to commit your changes directly to the main branch. Instead, create a branch with a descriptive name (such as `<my_name>-<my_change>` e.g, `a_salerno-new_parser`) and commit your changes there. Once your changes have been made **AND TESTED**, you can create a Pull Request to merge your code with the official main branch. When writing your Pull Request, please make use of the Pull Request Template.

## Code style
It'd be greatly appreciated if code written by contributors was formatted using `clang-format` with the `.clang-format` file included in this repository. Also, contributors are invited to take note of the following code style guidelines:
- Place declarations in the correct file and location (e.g., place CLI command function handler declarations in `include/cli/directives/commands.h`)
- Separate interfaces from implementations (i.e., do not write function implementations in header files)
- Use the `include` and `src` directories (and the respective subdirectories) **PROPERLY** (i.e., place header in `include`, common implementations in `src/common`, OS-dependent implementations in `src/os-specific/<os-name>`, etc.)
- Use [Yoda notation](https://en.wikipedia.org/wiki/Yoda_conditions) when writing conditions (e.g., `1 == x` instead of `x == 1`)
- When checking if pointers are `NULL` use explicit statements (i.e., `NULL == ptr` instead of `!ptr`)
- Avoid branchless code unless:
  A. It is intuitive and easy to read
  B. It performs better than other implementations (must be tested)
- Avoid magic numbers, declare constants or macros to disambiguate
- Use guards instead of endless `if-else` statements
- Remember to clean up memory after it's been used (`goto` is not evil, use `goto cleanup` in guards instead of `return`)
- The use of unconventional structures (such as `goto`) is permitted if:
  A. It is intiutive and easy to read (e.g., `goto cleanup` or `goto emit` in guards)
  B. It performs better than other implementations (must be tested)
- Use common sense

## Credits
Contributors are invited, but not required, to add their name and E-Mail address to the `AUTHORS` file. More information is available in that file.
