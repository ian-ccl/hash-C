module api {
    fn _defer:test() int;
}

fn _defer:test() int {
    int x = 0;

    defer {
        x = x + 10;
    }
    defer {
        x = x + 20;
    }

    x = x + 1;

    return x;
}