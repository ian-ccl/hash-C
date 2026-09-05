module api {
    fn literals:test() uint;
}

fn literals:test() uint {
    uint a = 0xff;
    uint b = 0b1010;
    uint c = 0o755;

    return a + b + c;
}