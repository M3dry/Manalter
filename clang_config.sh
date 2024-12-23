#!/usr/bin/env bash

included=$(g++ -v -c -xc++ /dev/null 2>&1 | sed -n '/#include <...> search starts here:/, /End of search list./p' | sed '1d;$d ; s/ */-I/' | paste -sd "," -)

printf "CompileFlags:\n\tAdd: [-isystem,%s]\n" "$included" > .clangd
