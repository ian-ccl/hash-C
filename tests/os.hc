module api {
    fn os:test() int;
}

fn os:test() int {
    int x = 0;

    @os windows
        x = 10;

    @os linux
        x = 20;

    @os apple
        x = 30;

    @os other
        x = 40;

    @oselse
        x = 50;

    @osend

    return x;
}