module api {
    fn b:foo() int;

}
@template hi T
struct hi:$T {
    $T i;
};
@templateend

@implement hi int
fn b:foo() int {
    std:printf("hola\n");
    return 1;
}