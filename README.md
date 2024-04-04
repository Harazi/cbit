cbit
====
Command-line tool for headless qBittorrent.

Install
-------
The only dependency is [libcurl](https://github.com/curl/curl). cJSON is installed as a git submodule.

```sh
$ git clone --recursive https://github.com/Harazi/cbit
$ cd cbit
$ make
# make install
```

Usage
-----
Command structure is `EXECUTABLE OBJECT COMMAND`. The executable's and object's options are parsed separately,
executable's options must be placed before the object. Commands may have additional arguments, run `cbit OBJECT help`
to see available commands and options.
