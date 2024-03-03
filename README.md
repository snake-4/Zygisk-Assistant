## ZygiskNext Assistant
This module makes ZygiskNext more stealthy but it might create problems with some Zygisk modules.

The module sets **DLCLOSE_MODULE_LIBRARY** and **FORCE_DENYLIST_UNMOUNT** Zygisk flags for all non-root application processes.

Please note that installing this module could lead to compatibility issues with other Zygisk modules. If you encounter any problems, please create an issue in this repository.
