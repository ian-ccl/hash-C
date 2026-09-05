module api {
    fn math:add(int a, int b) int;
    fn math:mul(int a, int b) int;
}

fn math:add(int a, int b) int {
    return a + b;
}

fn math:mul(int a, int b) int {
    return a * b;
}