module api {
    fn templates:test() int;
}

@template pair T U <typeonly>
cstruct Pair {
    $T first;
    $U second;
};
@templateend

@implement pair int uint

@template make_pair T U <typeonly>
fn make_$T($T a, $U b) $T {
    return a;
}
@templateend

@implement make_pair int uint

fn templates:test() int {
    Pair p;

    p.first = 10;
    p.second = 20;

    return p.first;
}

