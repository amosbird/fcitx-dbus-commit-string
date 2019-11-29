# What's this?

An add-on for [fcitx](https://gitlab.com/fcitx/fcitx) to commit string via D-Bus.

# Basic Install

```
$ mkdir build
$ cd build
$ cmake ..
$ make
$ sudo make install
```

Restart fcitx to enable it.

# Dependency

- fcitx (core and dbus module)
- dbus

# Usage

```sh
> gdbus call --session --dest org.fcitx.Fcitx --object-path /CommitString --method org.fcitx.Fcitx.InputMethod.CommitString "Hello World"
Hello World
```
