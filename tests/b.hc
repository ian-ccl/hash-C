module api {
    fn b:foo() int;

}
@template hi T <typeonly>
struct hi:$T {
    $T i;
};
@templateend

@implement hi int
@implement_unique hi int
fn b:foo() int {
    std:printf("hola\n");
    return 1;
}

@c++
#include <iostream>
int holamundo() {
    std::cout << "hola mundo";
}
@c++end