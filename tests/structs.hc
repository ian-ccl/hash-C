module api {
    cstruct Point {
        int x;
        int y;
    };

    fn structs:test() int;
}

cstruct Point { //copyable
    int x;
    int y;
};

struct Secret { //non copyable
    int value;
};

fn structs:test() int {
    Point a = make Point{1, 2};
    Point b = a;

    return b.x + b.y;
}