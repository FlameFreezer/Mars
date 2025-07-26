const std = @import("std");
const Mars = @import("Mars");

pub fn main() !void {
    var mars: Mars.State = undefined;
    try Mars.init(&mars);
    try Mars.mainLoop(&mars);
    Mars.cleanup(&mars);
}
