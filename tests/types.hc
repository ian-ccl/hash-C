module api {
    fn types:test(int x) int;
}

typedef Number int;

fn types:test(int x) int {
    Number y = x;

    int* p = &y;

    return *p;
}