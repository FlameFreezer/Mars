const std = @import("std");
const Mars = @import("Mars");

pub fn main() !void {
    var mars: Mars.State = undefined;
    try mars.init("Mars App");
    try mars.mainLoop();
    mars.cleanup();
}
