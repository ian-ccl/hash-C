module api {
    cstruct Point {
        int x;
        int y;
    };

    fn structs:test() int;
}

cstruct Point {
    int x;
    int y;
};

struct Secret {
    int value;
};

fn structs:test() int {
    Point a = {1, 2};
    Point b = a;

    return b.x + b.y;
}