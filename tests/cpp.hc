module api {
    fn cpp:test() int;
}

@c++
#include <cstdint>

extern "C" int cpp_helper(int x) {
    return x * 2;
}

struct CppOnly {
    int value;
};
@c++end

fn cpp:test() int {
    return cpp_helper(21);
}