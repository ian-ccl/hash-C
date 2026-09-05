@import math
@import structs
@import templates
@import defer
@import os
@import cpp
@import types
@import lits

fn main(char[][] args) void {
    std:printf("math: %i\n", math:add(10, 20));
    std:printf("mul: %i\n", math:mul(6, 7));

    std:printf("structs: %i\n", structs:test());

    std:printf("templates: %i\n", templates:test());

    std:printf("defer: %i\n", _defer:test());

    std:printf("os: %i\n", os:test());

    std:printf("cpp: %i\n", cpp:test());

    std:printf("types: %i\n", types:test(7));

    std:printf("literals: %u\n", literals:test());
}