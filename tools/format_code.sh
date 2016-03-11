#!/bin/sh

find . -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' | \
        egrep -v '(^./third_party/|build/)' | \
        xargs clang-format-3.6 -i
