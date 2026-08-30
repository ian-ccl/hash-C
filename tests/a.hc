@import b
fn main(char[][] args) void {
    b:foo();
    for (
        uint i = 1; 
        i < args.size; 
        i++
    ) {
        std:printf("%s ", args[i]);
    }
    std:printf("\n");
}