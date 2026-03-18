# Mars - A Silly Game Framework

Mars is my framework for making silly games with C++. Though incomplete for now, it features:

1. An entity-component system
2. A nicely encapsulated renderer written in Vulkan
3. A `mars::Game` class for managing invariants and context
4. A `mars::Error` class template for error handling in a Zig-like style. Please do not use exceptions with Mars or you will make me very sad

## Renderer

The renderer, for now, is a two-phase system which first draws a 2D scene and then maps said scene to the faces of a cube.

## Future plans

- More rendering systems, with a flag passed to `mars::Game::init` controlling which one is initialized
- Haskell-like monad syntax for use with `mars::Error`
- A class template for optional values
- Custom linear algebra library to replace glm
- Proper hyprland support
- No use of C++ Standard Template Library
- Extensibility for Component Systems
- Proper documentation

## Why is it called Mars?

Mars is my favorite planet
