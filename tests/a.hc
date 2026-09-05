@import b
fn main(char[][] args) void {
    b:foo();
    defer std:printf("\n");
    const int x = 0;
    for (
        uint i = 1; 
        i < args.size; 
        i++
    ) {
        std:printf("%s ", args[i]);
    }
    
}