include_directories (${FCITX4_FCITX_INCLUDE_DIRS})
include_directories (${FCITX4_FCITX_CONFIG_INCLUDE_DIRS})
include_directories (${FCITX4_FCITX_UTILS_INCLUDE_DIRS})
include_directories (${DBUS_INCLUDE_DIRS})

fcitx_add_addon_full(dbus-commit-string SOURCES dbus-commit-string.cpp)
